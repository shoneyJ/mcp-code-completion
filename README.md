# MCP (Ollama) Server

This is a Rust-based HTTP server designed to provide code and command completion services using an AI model.

## Dependencies

- `anyhow`
- `axum`
- `reqwest`
- `serde`
- `serde_json`
- `tokio`
- `tracing`

You can install the required dependencies by running:

```sh
cargo build
```

## Running the Server

To run the server, execute the following command in your terminal:

```sh
cargo run --release
```

By default, the server will bind to `127.0.0.1:3000`. You can specify a different address by setting the `MCP_BIND` environment variable.

## Environment Variables

- `OLLAMA_URL`: The URL of the Ollama API endpoint. Default is `http://127.0.0.1:11434/api/generate`.
- `OLLAMA_MODEL`: The name of the AI model to be used for code completion. Required.

## API Endpoint

The server provides a single POST endpoint at `/mcp` which handles JSON-RPC requests for code and command completion.

### Example Request:

```json
{
  "jsonrpc": "2.0",
  "method": "callTool",
  "params": {
    "name": "code-completion",
    "arguments": {
      "prefix": "fn main() {"
    }
  },
  "id": 1
}
```

### Example Response:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "completion": "    println!('Hello, world!');\n}"
  }
}
```

## Error Handling

The server returns JSON-RPC formatted errors in case of issues, such as unsupported methods or internal errors.

- `unsupported_method`: The method called is not supported.
- `unknown_tool`: The tool specified does not exist.
- `internal_error`: An unexpected error occurred during processing.

For more information on the code and command completion functionality, refer to the source code in `src/main.rs`.

## Contributing

Feel free to contribute by submitting issues or pull requests. Ensure your code adheres to Rust's style guidelines and includes tests where applicable.

```

### Next Step:
The README file has been created. You can now save this content as a `README.md` file in the root of your project directory.

If you have any further questions or need additional assistance, feel free to ask!


```
