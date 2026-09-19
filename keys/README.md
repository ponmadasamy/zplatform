# MCUboot development keys

Private signing keys must **never** be committed to this repository.

## Generate a development RSA-2048 key

```bash
# From the ZPlatform repository root (west workspace activated)
imgtool keygen -k keys/root-rsa-2048.pem -t rsa-2048
imgtool getpub -k keys/root-rsa-2048.pem > keys/root-rsa-2048.pem.pub
```

`imgtool` is provided by MCUboot (`bootloader/mcuboot/scripts` or `pip install imgtool`).

Point MCUboot at the key via:

```text
CONFIG_BOOT_SIGNATURE_KEY_FILE="keys/root-rsa-2048.pem"
```

relative to the ZPlatform module root (or an absolute path).

## Production

Use a hardware security module or offline key ceremony. Do not reuse
development keys in production images.
