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
from cryptography.exceptions import InvalidTag

console = Console()

HOME_DIR = Path.home()
VAULT_DIR = HOME_DIR / "vault"
XMR_DIR = HOME_DIR / "xmr"
ENCRYPTED_FILE = VAULT_DIR / "xmr.enc"

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

def decrypt_file():
    if not ENCRYPTED_FILE.is_file():
        console.print(f"[red]Error:[/red] File enkripsi '{ENCRYPTED_FILE}' tidak ditemukan.")
        return False

    password = Prompt.ask(
        "password:",
        password=True,
        console=console
    )

    try:
        with open(ENCRYPTED_FILE, "rb") as f:
            full_encrypted_data = f.read()

        if len(full_encrypted_data) < 16:
            console.print("[red]File enkripsi korup atau terlalu pendek.[/red]")
            return False

        salt = full_encrypted_data[:16]
        encrypted_data = full_encrypted_data[16:]

        key = derive_key(password, salt)
        fernet = Fernet(key)

        decrypted_data = fernet.decrypt(encrypted_data)

        if XMR_DIR.is_dir():
            shutil.rmtree(XMR_DIR)

        temp_decrypted_tar_path = VAULT_DIR / "xmr_decrypted_temp.tar"
        with open(temp_decrypted_tar_path, "wb") as f:
            f.write(decrypted_data)

        with tarfile.open(temp_decrypted_tar_path, "r") as tar:
            tar.extractall(path=HOME_DIR, filter='data')

        return True

    except InvalidTag:
        console.print("[bold red]Password salah atau file enkripsi korup. Dekripsi gagal.[/bold red]")
        return False
    except Exception as e:
        console.print(f"[bold red]Terjadi error saat dekripsi: {e}[/bold red]")
        return False
    finally:
        if 'temp_decrypted_tar_path' in locals() and temp_decrypted_tar_path.exists():
            os.remove(temp_decrypted_tar_path)

def main():
    decrypt_file()

if __name__ == "__main__":
    main()
