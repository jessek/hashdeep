use blake3::Hasher;
use sha1::Digest as Sha1Digest;
use std::fs;
use std::io::{self, Read};
use std::path::Path;
use xxhash_rust::xxh3::xxh3_64;

use crate::HashAlgorithm;

pub fn compute_hash(file_path: &Path, algorithm: &HashAlgorithm) -> Result<String, std::io::Error> {
    let contents = fs::read(file_path)?;

    let hash = match algorithm {
        HashAlgorithm::Md5 => {
            let digest = md5::compute(&contents);
            format!("{:x}", digest)
        }
        HashAlgorithm::Sha1 => {
            let mut hasher = sha1::Sha1::new();
            hasher.update(&contents);
            format!("{:x}", hasher.finalize())
        }
        HashAlgorithm::Sha256 => {
            let mut hasher = sha2::Sha256::new();
            hasher.update(&contents);
            format!("{:x}", hasher.finalize())
        }
        HashAlgorithm::Xxhash => {
            let hash = xxh3_64(&contents);
            format!("{:x}", hash)
        }
        HashAlgorithm::Blake3 => {
            let mut hasher = Hasher::new();
            hasher.update(&contents);
            let hash = hasher.finalize();
            format!("{}", hash.to_hex())
        }
    };

    Ok(hash)
}

pub fn compute_hash_stdin(algorithm: &HashAlgorithm) -> Result<String, std::io::Error> {
    let mut contents = Vec::new();
    io::stdin().read_to_end(&mut contents)?;

    let hash = match algorithm {
        HashAlgorithm::Md5 => {
            let digest = md5::compute(&contents);
            format!("{:x}", digest)
        }
        HashAlgorithm::Sha1 => {
            let mut hasher = sha1::Sha1::new();
            hasher.update(&contents);
            format!("{:x}", hasher.finalize())
        }
        HashAlgorithm::Sha256 => {
            let mut hasher = sha2::Sha256::new();
            hasher.update(&contents);
            format!("{:x}", hasher.finalize())
        }
        HashAlgorithm::Xxhash => {
            let hash = xxh3_64(&contents);
            format!("{:x}", hash)
        }
        HashAlgorithm::Blake3 => {
            let mut hasher = Hasher::new();
            hasher.update(&contents);
            let hash = hasher.finalize();
            format!("{}", hash.to_hex())
        }
    };

    Ok(hash)
}
