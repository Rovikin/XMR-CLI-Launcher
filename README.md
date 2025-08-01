# XMR-CLI-Launcher

A minimalistic, privacy-focused Monero wallet setup for Android Termux.  
Runs through Tor. Encrypts wallet data automatically. No root required.

---

## ⚙️ Features

- Monero CLI wallet over Tor .onion node
- Auto-encryption and decryption of wallet files using Python (Fernet + PBKDF2)
- Auto compile and launch with hardened start script
- Works entirely inside Termux — no root, no proot
- Designed for anonymous, mobile-first Monero operations

---

## 📦 Requirements

- Android phone with Termux
- Internet connection
- No root access needed
- Git, curl, clang/g++, tor, python (auto-installed by installer)

---

## 🧪 Installation

Paste this command into Termux:

```bash
curl -sLO https://raw.githubusercontent.com/Rovikin/XMR-CLI-Launcher/main/install.sh
bash install.sh
```

---

## 🔐 How It Works

- `install.sh` sets up the entire environment, checks and installs all dependencies
- User generates a Monero wallet through CLI connected to a `.onion` remote node
- Upon exit, wallet data is encrypted and stored inside `~/vault/xmr.enc`
- Subsequent launches use an `alias start` that unlocks, runs, and re-locks automatically

---

## 🛡️ Security Notes

- Wallet files are encrypted with AES256 using PBKDF2-derived key (via `cryptography.fernet`)
- Salt is randomized per encryption and stored alongside ciphertext
- No third-party server interaction. Fully local, fully offline-ready.

---

## 🧠 Developer Notes

- C++ launcher (`start.cpp`) checks for Tor connection and launches `monero-wallet-cli`
- Python scripts (`lock.py`, `unlock.py`) handle encryption and decryption of wallet directory
- SSH-based Git push/pull enabled via `gh auth login`

---

## ✅ License

MIT — use, remix, fork freely. Privacy is a right.

---

Made with ❤️ for cypherpunks, minimalists, and Monero believers.