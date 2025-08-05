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
#include <cstdlib>
#include <dirent.h>

namespace cfg {
    constexpr const char* DAEMON = "un4yrhwq4d53caoiaadeiur5e5wgkgp74zw3p3twqh3nxh6ztz347dad.onion";
    constexpr const char* PROXY = "127.0.0.1";
    constexpr const char* TOR = "/data/data/com.termux/files/usr/bin/tor";
    constexpr int PORT = 18081, SOCKS = 9050, TIMEOUT = 30;
    constexpr const char* LOGFILE = "/data/data/com.termux/files/home/monero-wallet-cli.log"; // explicit log file
}

pid_t tor_pid = 0;

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

std::string detect_wallet_name(const std::string& basedir) {
    DIR* dir = opendir(basedir.c_str());
    if (!dir) return "";

    std::string found;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string fname = ent->d_name;
        if (fname.size() > 5 && fname.substr(fname.size() - 5) == ".keys") {
            found = fname.substr(0, fname.size() - 5);
            break;
        }
    }
    closedir(dir);
    return found;
}

void delete_wallet_log() {
    unlink(cfg::LOGFILE); // Hapus log setelah wallet tutup
}

void start_wallet() {
    const char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Environment variable HOME not set.\n");
        _exit(1);
    }

    std::string basedir = std::string(home) + "/xmr";
    std::string wallet_name = detect_wallet_name(basedir);
    if (wallet_name.empty()) {
        fprintf(stderr, "❌ Wallet .keys file not found in %s\n", basedir.c_str());
        _exit(1);
    }

    std::string wallet_path = basedir + "/" + wallet_name;
    chmod(wallet_path.c_str(), 0600);

    pid_t pid = fork();
    if (pid == 0) {
        chdir(basedir.c_str()); // supaya log gak nyasar

        std::vector<std::string> args = {
            "monero-wallet-cli",
            "--wallet-file", wallet_path,
            "--daemon-address", std::string(cfg::DAEMON) + ":" + std::to_string(cfg::PORT),
            "--proxy", std::string(cfg::PROXY) + ":" + std::to_string(cfg::SOCKS),
            "--trusted-daemon",
            "--log-level", "0",
            "--log-file", cfg::LOGFILE
        };

        std::vector<char*> argv;
        for (auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);

        execvp("monero-wallet-cli", argv.data());
        _exit(1); // kalau exec gagal
    }

    // Tunggu wallet selesai
    waitpid(pid, nullptr, 0);

    // Hapus log setelah keluar
    delete_wallet_log();
}

void cleanup(int) {
    if (tor_pid > 0) {
        kill(tor_pid, SIGTERM);
        waitpid(tor_pid, nullptr, 0);
    }

    delete_wallet_log(); // pastikan log dihapus saat Ctrl+C
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

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
