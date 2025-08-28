#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <fstream>
class Process
{
public:
    pid_t pid{-1};

    bool spawn(const std::vector<std::string> &args)
    {
        pid = fork();
        if (pid == 0)
        {
            // Child process
            std::vector<char *> cargs;
            cargs.reserve(args.size() + 1);
            for (auto &arg : args)
                cargs.push_back(const_cast<char *>(arg.c_str()));
            cargs.push_back(nullptr);
            execvp(cargs[0], cargs.data());
            _exit(127); // exec failed
        }
        return pid > 0;
    }

    void terminate()
    {
        if (pid > 0)
        {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

// Convert hex string to integer port
inline int hex_to_port(const std::string &hex)
{
    return std::stoi(hex, nullptr, 16);
}
// Check /proc/net/tcp for active ESTABLISHED connections on given port
bool has_active_connections(int port)
{
    static constexpr char ESTABLISHED[] = "01"; // TCP_ESTABLISHED
    std::ifstream tcp("/proc/net/tcp");
    if (!tcp.is_open())
        return false;

    std::string line;
    std::getline(tcp, line); // skip header

    while (std::getline(tcp, line))
    {
        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
            continue;

        std::istringstream iss(line);
        std::string sl, local_address, rem_address, st;
        iss >> sl >> local_address >> rem_address >> st;

        // local_address looks like "0100007F:1F90" (127.0.0.1:8080 in hex)

        std::string hex_port = local_address.substr(colon_pos + 1);
        int local_port = hex_to_port(hex_port);

        if (local_port == port && st == ESTABLISHED)
        { // "01" = ESTABLISHED
            return true;
        }
    }
    return false;
}
int main()
{
    // Arguments for llama-server
    int port = 8080;
    std::vector<std::string> args = {
        "../extern/llama.cpp/build/bin/llama-server",
        "-m", std::string(std::getenv("HOME")) + "/models/qwen/Qwen_Qwen2.5-Coder-1.5B-Instruct-GGUF_qwen2.5-coder-1.5b-instruct-q4_k_m.gguf",
        "-ngl", "99",
        "-c", "2048",
        "--host", "0.0.0.0",
        "--port", std::to_string(port)};

    Process llama;
    if (!llama.spawn(args))
    {
        std::cerr << "Failed to spawn llama-server!\n";
        return 1;
    }

    std::cout << "llama-server started successfully (pid=" << llama.pid << ").\n";

    // Wait briefly to check if process died immediately
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int status;
    pid_t result = waitpid(llama.pid, &status, WNOHANG);
    if (result == llama.pid)
    {
        std::cerr << "llama-server exited early with status " << status << "\n";
        return 1;
    }

    // Idle timeout management
    auto last_activity = std::chrono::steady_clock::now();
    constexpr auto idle_limit = std::chrono::minutes(5);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (has_active_connections(port))
        {
            // Convert steady_clock to system_clock for human-readable time
            auto now_system = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now_system);

            std::cout << "last active updated to: " << std::ctime(&now_time);
        }

        if (std::chrono::steady_clock::now() - last_activity > idle_limit)
        {
            std::cout << "Idle timeout reached. Terminating llama-server.\n";
            llama.terminate();
            break;
        }
    }

    std::cout << "Server stopped.\n";
    return 0;
}
