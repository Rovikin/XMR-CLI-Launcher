#include <unistd.h>
#include <csignal>
#include <cstring>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <cstdlib>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

namespace cfg {
    constexpr const char* LOGFILE = "/data/data/com.termux/files/home/monero-wallet-cli.log";
}

void delete_wallet_log() {
    unlink(cfg::LOGFILE); // Bersihin log setelah selesai
}

std::string detect_wallet_name(const std::string& basedir) {
    DIR* dir = opendir(basedir.c_str());
    if (!dir) return "";

    std::string found;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string fname = ent->d_name;
        if (fname.size() > 5 && fname.substr(fname.size() - 5) == ".keys") {
            found = fname.substr(0, fname.size() - 5); // hapus ".keys"
            break;
        }
    }
    closedir(dir);
    return found;
}

void start_wallet_offline() {
    const char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "HOME env not set\n");
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
        chdir(basedir.c_str());

        std::vector<std::string> args = {
            "monero-wallet-cli",
            "--wallet-file", wallet_path,
            "--offline",
            "--log-level", "0",
            "--log-file", cfg::LOGFILE
        };

        std::vector<char*> argv;
        for (auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);

        execvp("monero-wallet-cli", argv.data());
        _exit(1);
    }

    // Tunggu sampai proses selesai
    waitpid(pid, nullptr, 0);
    delete_wallet_log();
}

void cleanup(int) {
    delete_wallet_log(); // jaga-jaga kalau user pencet Ctrl+C
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    start_wallet_offline();
    return 0;
}
