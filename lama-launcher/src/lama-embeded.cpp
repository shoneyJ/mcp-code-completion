#include <cstdint>
#include <iostream>
#include <llama.h>
#include <string>
#include <vector>
void run_lama_embeded() {

  // Set model parameters
  llama_model_params params = llama_model_params();
  params.n_gpu_layers = 99;

  // Load model

  llama_model *model = llama_model_load_from_file(
      "../models/qwen/Qwen2.5-Coder-7B-Instruct-Q4_K_M.gguf", params);

  if (!model) {
    std::cerr << "Failed to load model\n";
  }

  llama_context_params ctx_params = llama_context_params();
  ctx_params.n_threads = 4;
  ctx_params.n_ctx = 512;

  llama_context *ctx = llama_init_from_model(model, ctx_params);
  if (!ctx) {
    std::cerr << "Failed to initialize context\n";
    llama_model_free(model);
  }
  // Prompt to send to the model
  std::string prompt = "Act as a terminal assistant. User described: 'record "
                       "screen and convert mp4 to gif using GPU'";
  // Tokenize the prompt
  std::vector<llama_token> tokens(256);
  int32_t n_tokens =
      llama_tokenize(llama_get_vocab(ctx), prompt.c_str(), -1,
                     tokens.data(), // <--- pointer to vector storage
                     tokens.size(), true, false);

  // Evaluate the prompt
  llama_eval(model, tokens.data(), tokens.size(), 0, params.n_ctx, params);

  // Generate output tokens
  std::vector<llama_token> output_tokens;
  llama_token token;
  while ((token = llama_sample_token(model, params)) != LLAMA_TOKEN_EOS) {
    output_tokens.push_back(token);
  }

  // Convert tokens to string
  std::string output = llama_token_to_piece(output_tokens);

  std::cout << "Model output:\n" << output << std::endl;

  // Free model
  llama_free(ctx);
  llama_model_free(model);
}
