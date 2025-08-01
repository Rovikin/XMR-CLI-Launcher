#!/data/data/com.termux/files/usr/bin/bash

set -e

echo -e "\033[1;32m🔄 Updating packages...\033[0m"
pkg update -y && pkg upgrade -y

DEPS=(git clang python openssl tor monero monero-wallet-cli)

echo -e "\033[1;34m📦 Checking dependencies...\033[0m"
for pkg in "${DEPS[@]}"; do
    if ! command -v "$pkg" >/dev/null 2>&1; then
        echo -e "\033[1;33mInstalling $pkg...\033[0m"
        pkg install -y "$pkg"
    else
        echo -e "\033[1;32m$pkg already installed.\033[0m"
    fi
done

REPO_URL="https://github.com/Rovikin/XMR-CLI-Launcher.git"
REPO_DIR="$HOME/XMR-CLI-Launcher"

if [ ! -d "$REPO_DIR" ]; then
    echo -e "\033[1;34m📥 Cloning repository...\033[0m"
    git clone "$REPO_URL" "$REPO_DIR"
else
    echo -e "\033[1;32mRepository already cloned.\033[0m"
fi

echo -e "\033[1;34m📁 Creating ~/vault and ~/xmr...\033[0m"
mkdir -p ~/vault ~/xmr
cp "$REPO_DIR/lock.py" "$REPO_DIR/unlock.py" ~/vault/

echo -e "\033[1;35m🚀 Launching Tor and monero-wallet-cli for initial setup...\033[0m"
echo -e "\n\033[1;33m📘 IMPORTANT SETUP INSTRUCTIONS:\033[0m"
echo -e "1. Choose \033[1;36mCreate wallet from scratch\033[0m"
echo -e "2. Enter wallet name: \033[1;32mwallet\033[0m \033[1;31m(REQUIRED)\033[0m"
echo -e "3. Enter password twice"
echo -e "4. Save your seed phrase"
echo -e "5. Just press Enter for restore height prompts"
echo -e "6. Type \033[1;36mq\033[0m or \033[1;36mexit\033[0m to quit after setup\n"

tor & disown
echo -e "\033[0;36m⏳ Waiting 15 seconds for Tor bootstrap...\033[0m"
sleep 15

cd ~/xmr
monero-wallet-cli --daemon-address un4yrhwq4d53caoiaadeiur5e5wgkgp74zw3p3twqh3nxh6ztz347dad.onion:18081 \
                  --proxy 127.0.0.1:9050 \
                  --trusted-daemon

echo -e "\033[1;34m🛠️ Compiling start.cpp...\033[0m"
cd "$REPO_DIR"
g++ start.cpp -o start

echo -e "\033[1;34m🔐 Encrypting wallet...\033[0m"
python ~/vault/lock.py

echo -e "\033[1;34m🔗 Setting up 'start' alias...\033[0m"
ALIAS_CMD="alias start='clear && python ~/vault/unlock.py && cd ~/xmr && ~/XMR-CLI-Launcher/start && python ~/vault/lock.py && cd ~/'"
if ! grep -Fxq "$ALIAS_CMD" ~/.bashrc; then
    echo "$ALIAS_CMD" >> ~/.bashrc
    echo -e "\033[1;32mAlias 'start' added to ~/.bashrc\033[0m"
else
    echo -e "\033[1;33mAlias 'start' already exists.\033[0m"
fi

echo -e "\033[1;36m✅ Done. Please restart Termux or run: source ~/.bashrc\033[0m"
echo -e "\033[1;32mThen just type: start\033[0m"
