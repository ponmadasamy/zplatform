# MCUboot development keys

Private signing keys must **never** be committed to this repository.

STM3210E-EVAL uses **ECDSA-P256** (TinyCrypt) so MCUboot fits in the
64 KiB boot partition. RSA-2048 + mbedTLS is too large for that slot.

## Generate a development ECDSA-P256 key

```bash
# From the ZPlatform repository root (west workspace activated)
imgtool keygen -k keys/root-ec-p256.pem -t ecdsa-p256
imgtool getpub -k keys/root-ec-p256.pem > keys/root-ec-p256.pem.pub
```

`imgtool` is provided by MCUboot (`bootloader/mcuboot/scripts` or `pip install imgtool`).

Point MCUboot at the key via `CONFIG_BOOT_SIGNATURE_KEY_FILE` in
`boards/st/stm3210e_eval/support/mcuboot.conf` (absolute path is
recommended on Windows when the west workspace and ZPlatform live on
different drives).

## Optional RSA-2048 (larger boards)

```bash
imgtool keygen -k keys/root-rsa-2048.pem -t rsa-2048
imgtool getpub -k keys/root-rsa-2048.pem > keys/root-rsa-2048.pem.pub
```

Requires mbedTLS + `tf-psa-crypto` in the west allowlist and a larger
boot partition.

## Production

Use a hardware security module or offline key ceremony. Do not reuse
development keys in production images.
