import os
import tarfile
import base64
import shutil
from pathlib import Path

from rich.console import Console
from rich.prompt import Prompt
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.backends import default_backend

console = Console()

# --- Konfigurasi Path ---
HOME_DIR = Path.home()
VAULT_DIR = HOME_DIR / "vault"
XMR_DIR = HOME_DIR / "xmr"
ENCRYPTED_FILE = VAULT_DIR / "xmr.enc"

# --- Fungsi Derivasi Kunci ---
def derive_key(password: str, salt: bytes) -> bytes:
    """Derivasi kunci Fernet dari password dan salt."""
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend()
    )
    return base64.urlsafe_b64encode(kdf.derive(password.encode()))

# --- Fungsi Enkripsi ---
def encrypt_directory():
    if not XMR_DIR.is_dir():
        console.print(f"[red]Error:[/red] Direktori '{XMR_DIR}' tidak ditemukan. Tidak ada yang bisa dienkripsi.")
        return False

    password = Prompt.ask(
        "password:",
        password=True,
        console=console
    )
    confirm_password = Prompt.ask(
        "confirm password:",
        password=True,
        console=console
    )

    if password != confirm_password:
        console.print("[red]Password tidak cocok. Enkripsi dibatalkan.[/red]")
        return False

    salt = os.urandom(16)
    key = derive_key(password, salt)
    fernet = Fernet(key)

    VAULT_DIR.mkdir(parents=True, exist_ok=True)

    temp_tar_path = VAULT_DIR / "xmr_temp.tar"

    try:
        # Arsipkan direktori ~/xmr/
        with tarfile.open(temp_tar_path, "w") as tar:
            tar.add(XMR_DIR, arcname=XMR_DIR.name)

        # Baca data dari arsip tar
        with open(temp_tar_path, "rb") as f:
            tar_data = f.read()

        # Enkripsi data arsip
        encrypted_data = fernet.encrypt(tar_data)

        # Simpan salt dan data terenkripsi ke file target
        with open(ENCRYPTED_FILE, "wb") as f:
            f.write(salt + encrypted_data)

        # Hapus direktori asli secara otomatis
        shutil.rmtree(XMR_DIR)

        return True

    except Exception as e:
        console.print(f"[bold red]Error saat enkripsi: {e}[/bold red]")
        return False
    finally:
        if temp_tar_path.exists():
            os.remove(temp_tar_path)

# --- Main Logic untuk lock.py ---
def main():
    VAULT_DIR.mkdir(parents=True, exist_ok=True)
    encrypt_directory()

if __name__ == "__main__":
    main()
