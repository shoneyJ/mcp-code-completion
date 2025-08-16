#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

class Process {
public:
    pid_t pid{-1};

    bool spawn(const std::vector<std::string>& args) {
        pid = fork();
        if (pid == 0) {
            // Child process
            std::vector<char*> cargs;
            for (auto& arg : args) cargs.push_back(const_cast<char*>(arg.c_str()));
            cargs.push_back(nullptr);
            execvp(cargs[0], cargs.data());
            _exit(127); // exec failed
        }
        return pid > 0;
    }

    void terminate() {
        if (pid > 0) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

int main() {
    // Arguments for llama-server
    std::vector<std::string> args = {
        "./llama-server",
        "-m", std::string(std::getenv("HOME")) + "/models/qwen/Qwen_Qwen2.5-Coder-1.5B-Instruct-GGUF_qwen2.5-coder-1.5b-instruct-q4_k_m.gguf",
        "-ngl", "99",
        "-c", "2048",
        "--host", "0.0.0.0",
        "--port", "8080"
    };

    Process llama;
    if (!llama.spawn(args)) {
        std::cerr << "Failed to spawn llama-server!\n";
        return 1;
    }

    std::cout << "llama-server started successfully (pid=" << llama.pid << ").\n";

    // Idle timeout management
    auto last_activity = std::chrono::steady_clock::now();
    constexpr auto idle_limit = std::chrono::minutes(5);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::chrono::steady_clock::now();

        if (now - last_activity > idle_limit) {
            std::cout << "Idle timeout reached. Terminating llama-server.\n";
            llama.terminate();
            break;
        }
    }

    std::cout << "Server stopped.\n";
    return 0;
}