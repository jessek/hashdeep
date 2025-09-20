# Hashdeep version 5

This is Hashdeep, a set of cross-platform tools to compute hashes, or message digests, for any number of files while optionally recursively walking through a directory structure.  It can also take a list of known hashes and display the filenames of input files whose hashes either do or do not match any of the known hashes. This version supports MD5, SHA-1, SHA-256, xxh3_64, and BLAKE3 hashes.

This version is written in Rust, while earlier versions were written in C/C++. Please note that many command line flags have changed!

## Installing from source

```shell
cargo build -r
```

## Installing binaries

