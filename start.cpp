#include <unistd.h>
#include <csignal>
#include <cstring>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

namespace cfg {
constexpr const char* WALLET_DIR = "/data/data/com.termux/files/home/xmr";
constexpr const char* DAEMON = "un4yrhwq4d53caoiaadeiur5e5wgkgp74zw3p3twqh3nxh6ztz347dad.onion";
constexpr const char* PROXY = "127.0.0.1";
constexpr const char* TOR = "/data/data/com.termux/files/usr/bin/tor";
constexpr int PORT = 18081, SOCKS = 9050, TIMEOUT = 30;
}

pid_t tor_pid = 0;
std::string wallet_name;

bool is_tor_up() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return false;
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(cfg::SOCKS);
    inet_pton(AF_INET, cfg::PROXY, &a.sin_addr);
    if (connect(s, (sockaddr*)&a, sizeof(a)) != 0) return close(s), false;

    const uint8_t req[] = {0x05, 0x01, 0x00};
    uint8_t res[2];
    if (write(s, req, 3) != 3 || read(s, res, 2) != 2 || res[0] != 0x05 || res[1] != 0x00)
        return close(s), false;

    close(s);
    return true;
}

pid_t start_tor() {
    if (access(cfg::TOR, X_OK) != 0) return -1;
    pid_t pid = fork();
    if (pid != 0) return pid;

    int null = open("/dev/null", O_WRONLY);
    if (null >= 0) {
        dup2(null, STDOUT_FILENO);
        dup2(null, STDERR_FILENO);
        close(null);
    }
    execl(cfg::TOR, cfg::TOR, "--quiet", nullptr);
    _exit(1);
}

bool wait_tor(int t) {
    while (t-- > 0) {
        if (is_tor_up()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return false;
}

bool detect_wallet() {
    DIR* dir = opendir(cfg::WALLET_DIR);
    if (!dir) return false;

    dirent* ent;
    while ((ent = readdir(dir))) {
        std::string name = ent->d_name;
        if (name.size() > 6 && name.substr(name.size() - 5) == ".keys") {
            wallet_name = name.substr(0, name.size() - 5);  // Strip ".keys"
            break;
        }
    }
    closedir(dir);
    return !wallet_name.empty();
}

void start_wallet() {
    std::string wallet_path = std::string(cfg::WALLET_DIR) + "/" + wallet_name;
    chmod(wallet_path.c_str(), 0600);
    std::vector<std::string> a = {
        "monero-wallet-cli",
        "--wallet-file", wallet_path,
        "--daemon-address", std::string(cfg::DAEMON) + ":" + std::to_string(cfg::PORT),
        "--proxy", std::string(cfg::PROXY) + ":" + std::to_string(cfg::SOCKS),
        "--trusted-daemon",
        "--log-file", "/dev/null"
    };
    std::vector<char*> argv;
    for (auto& s : a) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);
    execvp("monero-wallet-cli", argv.data());
    _exit(1);
}

void cleanup(int) {
    if (tor_pid > 0) {
        kill(tor_pid, SIGTERM);
        waitpid(tor_pid, nullptr, 0);
    }
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    if (!detect_wallet()) {
        write(STDERR_FILENO, "Wallet not found in ~/xmr\n", 27);
        return 1;
    }

    if (!is_tor_up()) {
        tor_pid = start_tor();
        if (tor_pid <= 0 || !wait_tor(cfg::TIMEOUT)) {
            if (tor_pid > 0) kill(tor_pid, SIGKILL);
            return 1;
        }
    }

    start_wallet();
    return 0;
}
