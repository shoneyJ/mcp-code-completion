use anyhow::Result;
use axum::{
    Router,
    extract::Json,
    http::StatusCode,
    response::{IntoResponse, Response},
    routing::post,
};
use reqwest::Client;
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use std::{env, net::SocketAddr, time::Duration};
use tokio::time::timeout;
use tracing::{error, info};

#[derive(Debug, Deserialize)]
struct JsonRpcRequest {
    method: String,
    params: Option<Value>,
    id: Option<Value>,
}

#[derive(Debug, Serialize)]
struct JsonRpcResponse {
    jsonrpc: &'static str,
    id: Value,
    result: Option<Value>,
    error: Option<Value>,
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt::init();

    let bind = env::var("MCP_BIND").unwrap_or_else(|_| "127.0.0.1:3000".into());
    let addr: SocketAddr = bind.parse()?;
    info!("Starting MCP (Ollama) server on http://{}", addr);

    let app = Router::new().route("/mcp", post(handle_mcp));

    axum::serve(tokio::net::TcpListener::bind(addr).await.unwrap(), app)
        .await
        .unwrap();
    Ok(())
}

async fn handle_mcp(Json(payload): Json<JsonRpcRequest>) -> Response {
    if payload.method != "callTool" {
        let resp = make_error(
            payload.id.clone().unwrap_or(Value::Null),
            "unsupported_method",
            "Only callTool is supported",
        );
        return (StatusCode::BAD_REQUEST, Json(resp)).into_response();
    }

    let params = payload.params.unwrap_or_default();
    let name = params
        .get("name")
        .and_then(|v| v.as_str())
        .unwrap_or_default();
    let args = params.get("arguments").cloned().unwrap_or_default();
    let id = payload.id.unwrap_or(Value::Null);

    match name {
        "code-completion-ollama" => {
            let prefix = args
                .get("prefix")
                .and_then(|v| v.as_str())
                .unwrap_or("")
                .to_string();
            match ollama_completion(prefix.clone()).await {
                Ok(completion) => {
                    let resp = JsonRpcResponse {
                        jsonrpc: "2.0",
                        id,
                        result: Some(json!({ "completion": completion })),
                        error: None,
                    };
                    (StatusCode::OK, Json(resp)).into_response()
                }
                Err(e) => {
                    error!("completion failed: {:?}", e);
                    let resp = make_error(Value::Null, "internal_error", &format!("{}", e));
                    (StatusCode::INTERNAL_SERVER_ERROR, Json(resp)).into_response()
                }
            }
        }
        "code-completion-llamacpp" => {
            let prefix = args
                .get("prefix")
                .and_then(|v| v.as_str())
                .unwrap_or("")
                .to_string();
            match llamacpp_completion(prefix.clone()).await {
                Ok(completion) => {
                    let resp = JsonRpcResponse {
                        jsonrpc: "2.0",
                        id,
                        result: Some(json!({ "completion": completion })),
                        error: None,
                    };
                    (StatusCode::OK, Json(resp)).into_response()
                }
                Err(e) => {
                    error!("completion failed: {:?}", e);
                    let resp = make_error(Value::Null, "internal_error", &format!("{}", e));
                    (StatusCode::INTERNAL_SERVER_ERROR, Json(resp)).into_response()
                }
            }
        }
        other => {
            let resp = make_error(id, "unknown_tool", &format!("Unknown tool: {}", other));
            (StatusCode::BAD_REQUEST, Json(resp)).into_response()
        }
    }
}

fn make_error(id: Value, code: &str, message: &str) -> JsonRpcResponse {
    JsonRpcResponse {
        jsonrpc: "2.0",
        id,
        result: None,
        error: Some(json!({ "code": code, "message": message })),
    }
}

/// Call Ollama /api/generate to get a short completion.
/// Config via env:
///   OLLAMA_URL (default http://127.0.0.1:11434/api/generate)
///   OLLAMA_MODEL (required)
async fn ollama_completion(prefix: String) -> Result<String> {
    let ollama_url =
        env::var("OLLAMA_URL").unwrap_or_else(|_| "http://127.0.0.1:11434/api/generate".into());
    let model = env::var("OLLAMA_MODEL").unwrap_or_else(|_| {
        // sensible default; user should set their preferred model (e.g., "llama3.2")
        "qwen2.5-coder:7b".into()
    });

    // Build a concise prompt that asks for continuation only.
    // Tweak the prompt/template to improve results for shell commands vs code.
    let prompt = format!(
        "You are a concise code/command-completion assistant. The user typed: \"{}\".\nReturn only the completion/continuation (no explanation).",
        prefix.replace('"', "\\\"")
    );

    let client = Client::builder().timeout(Duration::from_secs(10)).build()?;

    let body = json!({
        "model": model,
        "prompt": prompt,
        "stream": false,
        // you can add "options": { "temperature": 0.2, "max_tokens": 64 } per Ollama docs
    });

    // Some Ollama installs expect POST with body as application/json.
    // Wrap call in timeout to avoid hanging requests.
    let resp = timeout(
        Duration::from_secs(8),
        client.post(&ollama_url).json(&body).send(),
    )
    .await??;
    let j: Value = resp.error_for_status()?.json().await?;

    // Ollama /api/generate typically returns { "response": "...", "done": true, ... }
    // or other shapes; try to extract the likely fields.
    if let Some(s) = j.get("response").and_then(|v| v.as_str()) {
        return Ok(s.trim().to_string());
    }
    // some versions might return "text" or "result"
    if let Some(s) = j.get("text").and_then(|v| v.as_str()) {
        return Ok(s.trim().to_string());
    }
    // fallback: if a top-level "choices" (OpenAI-like) exists
    if let Some(choice) = j.get("choices").and_then(|c| c.get(0)) {
        if let Some(txt) = choice.get("text").and_then(|t| t.as_str()) {
            return Ok(txt.trim().to_string());
        }
        if let Some(msg) = choice
            .get("message")
            .and_then(|m| m.get("content"))
            .and_then(|c| c.as_str())
        {
            return Ok(msg.trim().to_string());
        }
    }

    // final fallback: stringify the whole body (not ideal)
    Ok(j.to_string())
}

async fn llamacpp_completion(prefix: String) -> Result<String> {
    let llamacpp_url =
        env::var("LLAMACPP_URL").unwrap_or_else(|_| "http://127.0.0.1:8080/completion".into());

    let prompt = format!(
        "You are a concise code/command-completion assistant. The user typed: \"{}\".\nReturn only the completion/continuation (no explanation).",
        prefix.replace('"', "\\\"")
    );

    let client = Client::builder().timeout(Duration::from_secs(10)).build()?;

    let body = json!({

        "prompt": prompt,


    });

    let resp = timeout(
        Duration::from_secs(8),
        client.post(&llamacpp_url).json(&body).send(),
    )
    .await??;
    let j: Value = resp.error_for_status()?.json().await?;

    // or other shapes; try to extract the likely fields.
    if let Some(s) = j.get("response").and_then(|v| v.as_str()) {
        return Ok(s.trim().to_string());
    }
    // some versions might return "text" or "result"
    if let Some(s) = j.get("text").and_then(|v| v.as_str()) {
        return Ok(s.trim().to_string());
    }
    // fallback: if a top-level "choices" (OpenAI-like) exists
    if let Some(choice) = j.get("choices").and_then(|c| c.get(0)) {
        if let Some(txt) = choice.get("text").and_then(|t| t.as_str()) {
            return Ok(txt.trim().to_string());
        }
        if let Some(msg) = choice
            .get("message")
            .and_then(|m| m.get("content"))
            .and_then(|c| c.as_str())
        {
            return Ok(msg.trim().to_string());
        }
    }

    // final fallback: stringify the whole body (not ideal)
    Ok(j.to_string())
}
