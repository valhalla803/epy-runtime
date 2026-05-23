import os
import sys
import zlib
import marshal
import struct
import py_compile
import tempfile
import hashlib
import secrets

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes

MAGIC = b"EPYB"

RUNTIME_SECRET_1 = bytes([
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x10,
    0x21,0x32,0x43,0x54,0x65,0x76,0x87,0x98,
    0xa9,0xba,0xcb,0xdc,0xed,0xfe,0x0f,0x1a
])

RUNTIME_SECRET_2 = bytes([
    0x91,0x82,0x73,0x64,0x55,0x46,0x37,0x28,
    0x19,0x0a,0xfb,0xec,0xdd,0xce,0xbf,0xa0,
    0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89,
    0x9a,0xab,0xbc,0xcd,0xde,0xef,0xf1,0x02
])

PRIVATE_KEY = bytes([
    0x9d,0x61,0xb1,0x9d,
    0xef,0xfd,0x5a,0x60,
    0xba,0x84,0x4a,0xf4,
    0x92,0xec,0x2c,0xc4,
    0x44,0x49,0xc5,0x69,
    0x7b,0x32,0x69,0x19,
    0x70,0x3b,0xac,0x03,
    0x1c,0xae,0x7f,0x60
])

def derive_master_key(bundle_salt, build_seed):
    return hashlib.sha256(
        RUNTIME_SECRET_1 +
        RUNTIME_SECRET_2 +
        bundle_salt +
        build_seed
    ).digest()

def derive_module_key(master_key, module_hash, module_name):
    info = module_hash + module_name.encode()
    hkdf = HKDF(
        algorithm=hashes.SHA256(),
        length=32,
        salt=None,
        info=info
    )
    return hkdf.derive(master_key)

def build_module(filepath, bundle_salt, build_seed):
    module_name = os.path.splitext(
        os.path.basename(filepath)
    )[0]

    with open(filepath, "rb") as f:
        source = f.read()

    module_hash = hashlib.sha256(source).digest()

    with tempfile.NamedTemporaryFile(suffix=".pyc") as temp:
        py_compile.compile(
            filepath,
            cfile=temp.name
        )
        with open(temp.name, "rb") as f:
            pyc = f.read()

    raw = pyc[16:]
    compressed = zlib.compress(raw)

    master_key = derive_master_key(
        bundle_salt,
        build_seed
    )

    module_key = derive_module_key(
        master_key,
        module_hash,
        module_name
    )

    nonce = secrets.token_bytes(12)

    aes = AESGCM(module_key)

    encrypted = aes.encrypt(
        nonce,
        compressed,
        None
    )

    ciphertext = encrypted[:-16]
    tag = encrypted[-16:]
 
    header = struct.pack(
        "<4sHHII12s16s",
        b"EPY\x00",
        sys.version_info.major,
        sys.version_info.minor,
        len(raw),
        len(ciphertext),
        nonce,
        tag
    )

    return {
        "name": module_name,
        "data": header + ciphertext,
        "hash": module_hash
    }

def main():

    if len(sys.argv) < 3:
        print(
            "Usage: builder.py output.epyb files..."
        )
        return

    output = sys.argv[1]
    files = sys.argv[2:]

    bundle_salt = secrets.token_bytes(32)
    build_seed = secrets.token_bytes(32)

    modules = []

    for file in files:
        modules.append(
            build_module(
                file,
                bundle_salt,
                build_seed
            )
        )

    bundle = bytearray()

    bundle += MAGIC

    bundle += struct.pack(
        "<I",
        len(modules)
    )

    bundle += bundle_salt
    bundle += build_seed
    
    bundle += b"\x00" * 64

    table_offset = len(bundle)

    current_offset = (
        table_offset +
        (104 * len(modules))
    )

    table_entries = []
    payloads = []

    for module in modules:
        module_hash = module["hash"]

        entry = struct.pack(
            "<64sII32s",
            module["name"].encode(),
            current_offset,
            len(module["data"]),
            module_hash
        )

        table_entries.append(entry)
        payloads.append(module["data"])
        current_offset += len(module["data"])

    for entry in table_entries:
        bundle += entry

    for payload in payloads:
        bundle += payload

    private_key = Ed25519PrivateKey.from_private_bytes(
        PRIVATE_KEY
    )

    signature = private_key.sign(
        bytes(bundle)
    )

    bundle += signature

    with open(output, "wb") as f:
        f.write(bundle)

    print(f"[+] Built: {output}")

if __name__ == "__main__":
    main()