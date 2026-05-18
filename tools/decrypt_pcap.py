#!/usr/bin/env python3
"""
Decrypt an ESP32 Marauder encrypted PCAP (.epcap) file.

File format produced by the device:
  [4 bytes]  Magic: b'MEPC'
  [16 bytes] IV (nonce, plaintext)
  [N bytes]  AES-128-CTR encrypted PCAP data (global header + packet records)

Key derivation:
  key = SHA-256(PIN + "AA:BB:CC:DD:EE:FF")[:16]
  where "AA:BB:CC:DD:EE:FF" is the device's WiFi station MAC address.

Requirements:
  pip install pycryptodome

Usage:
  python decrypt_pcap.py <file.epcap> --pin <6-digit-pin> --mac <AA:BB:CC:DD:EE:FF>
  python decrypt_pcap.py <file.epcap> --pin 123456 --mac AA:BB:CC:DD:EE:FF
"""

import argparse
import hashlib
import struct
import sys
from pathlib import Path


MAGIC = b'MEPC'
HEADER_SIZE = 4 + 16  # magic + IV


def derive_key(pin: str, mac: str) -> bytes:
    """Derive AES-128 key: first 16 bytes of SHA-256(pin + mac_string)."""
    material = (pin + mac).encode('utf-8')
    digest = hashlib.sha256(material).digest()
    return digest[:16]


def decrypt(data: bytes, key: bytes, iv: bytes) -> bytes:
    try:
        from Crypto.Cipher import AES
    except ImportError:
        sys.exit("pycryptodome not found. Install with: pip install pycryptodome")

    cipher = AES.new(key, AES.MODE_CTR, nonce=b'', initial_value=iv)
    return cipher.decrypt(data)


def main():
    parser = argparse.ArgumentParser(
        description="Decrypt an ESP32 Marauder .epcap file to a standard .pcap"
    )
    parser.add_argument("input", help="Path to the .epcap file")
    parser.add_argument("--pin", required=True, help="6-digit PIN used on the device")
    parser.add_argument(
        "--mac",
        required=True,
        help='Device WiFi MAC address, e.g. AA:BB:CC:DD:EE:FF',
    )
    parser.add_argument("--output", help="Output .pcap path (default: <input>_dec.pcap)")
    args = parser.parse_args()

    in_path = Path(args.input)
    if not in_path.exists():
        sys.exit(f"File not found: {in_path}")

    raw = in_path.read_bytes()

    if len(raw) < HEADER_SIZE:
        sys.exit("File is too short to be a valid .epcap")

    magic = raw[:4]
    if magic != MAGIC:
        sys.exit(f"Bad magic: expected {MAGIC!r}, got {magic!r}")

    iv = raw[4:20]
    encrypted = raw[20:]

    key = derive_key(args.pin, args.mac.upper())
    plaintext = decrypt(encrypted, key, iv)

    out_path = Path(args.output) if args.output else in_path.with_name(
        in_path.stem + "_dec.pcap"
    )
    out_path.write_bytes(plaintext)
    print(f"Decrypted {len(plaintext)} bytes -> {out_path}")

    # Quick sanity check: PCAP magic number
    if len(plaintext) >= 4:
        pcap_magic = struct.unpack_from("<I", plaintext)[0]
        if pcap_magic == 0xa1b2c3d4:
            print("PCAP magic verified — decryption successful.")
        else:
            print(
                f"Warning: unexpected PCAP magic 0x{pcap_magic:08x}. "
                "Check PIN and MAC address."
            )


if __name__ == "__main__":
    main()
