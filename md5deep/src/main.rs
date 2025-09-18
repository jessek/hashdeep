use blake3::Hasher;
use serde::{Deserialize, Serialize};
use sha1::Digest as Sha1Digest;
use std::env;
use std::fs;
use std::path::Path;
use walkdir::WalkDir;
use xxhash_rust::xxh3::xxh3_64;

#[derive(Debug, Clone)]
enum HashAlgorithm {
    Md5,
    Sha1,
    Sha256,
    Xxhash,
    Blake3,
}

impl HashAlgorithm {
    fn as_str(&self) -> &'static str {
        match self {
            HashAlgorithm::Md5 => "md5",
            HashAlgorithm::Sha1 => "sha1",
            HashAlgorithm::Sha256 => "sha256",
            HashAlgorithm::Xxhash => "xxhash",
            HashAlgorithm::Blake3 => "blake3",
        }
    }
}

#[derive(Serialize, Deserialize)]
struct HashResult {
    filename: String,
    size: u64,
    #[serde(flatten)]
    algorithm_hash: std::collections::HashMap<String, String>,
}

fn compute_hash(file_path: &Path, algorithm: &HashAlgorithm) -> Result<String, std::io::Error> {
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

fn output_hash_result(
    file_path: &Path,
    hash: &str,
    algorithm: &HashAlgorithm,
    json_mode: bool,
    results: &mut Vec<HashResult>,
) {
    if json_mode {
        let metadata = fs::metadata(file_path).unwrap();
        let mut algorithm_hash = std::collections::HashMap::new();
        algorithm_hash.insert(algorithm.as_str().to_string(), hash.to_string());

        let result = HashResult {
            filename: file_path.display().to_string(),
            size: metadata.len(),
            algorithm_hash,
        };
        results.push(result);
    } else {
        println!("{}  {}", hash, file_path.display());
    }
}

fn process(
    arg: &String,
    recursive_mode: bool,
    algorithm: &HashAlgorithm,
    json_mode: bool,
    single_filesystem: bool,
    results: &mut Vec<HashResult>,
) {
    let path = Path::new(arg);

    // Get metadata to determine file type
    let metadata = match fs::metadata(path) {
        Ok(meta) => meta,
        Err(e) => {
            eprintln!("{}: {}", arg, e);
            return;
        }
    };

    if metadata.is_file() {
        // It's a regular file
        match compute_hash(path, algorithm) {
            Ok(hash) => {
                output_hash_result(path, &hash, algorithm, json_mode, results);
            }
            Err(e) => {
                eprintln!(
                    "{}: Error computing hash for '{}': {}",
                    env!("CARGO_PKG_NAME"),
                    arg,
                    e
                );
            }
        }
    } else if metadata.is_dir() {
        // It's a directory
        if recursive_mode {
            // Walk the directory tree recursively
            let walker = if single_filesystem {
                WalkDir::new(path).same_file_system(true)
            } else {
                WalkDir::new(path)
            };

            for entry in walker {
                match entry {
                    Ok(entry) => {
                        if entry.file_type().is_file() {
                            match compute_hash(entry.path(), algorithm) {
                                Ok(hash) => {
                                    output_hash_result(
                                        entry.path(),
                                        &hash,
                                        algorithm,
                                        json_mode,
                                        results,
                                    );
                                }
                                Err(e) => {
                                    eprintln!(
                                        "{}: Error computing hash for '{}': {}",
                                        env!("CARGO_PKG_NAME"),
                                        entry.path().display(),
                                        e
                                    );
                                }
                            }
                        }
                    }
                    Err(e) => {
                        eprintln!("Error accessing entry: {}", e);
                    }
                }
            }
        } else {
            eprintln!("{}: Is a directory", arg);
        }
    } else {
        // It's neither a file nor a directory (e.g., symlink, device, etc.)
        println!("Special file: {}", arg);
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut recursive_mode = false;
    let mut json_mode = false;
    let mut single_filesystem = false;
    let mut algorithm = HashAlgorithm::Md5; // Default to MD5
    let mut i = 1;

    // Parse command line arguments
    while i < args.len() {
        match args[i].as_str() {
            "-v" => {
                println!("{}", env!("CARGO_PKG_VERSION"));
                return;
            }
            "-r" => {
                recursive_mode = true;
            }
            "-j" => {
                json_mode = true;
            }
            "-x" => {
                single_filesystem = true;
            }
            "-c" => {
                if i + 1 < args.len() {
                    match args[i + 1].as_str() {
                        "md5" => algorithm = HashAlgorithm::Md5,
                        "sha1" => algorithm = HashAlgorithm::Sha1,
                        "sha256" => algorithm = HashAlgorithm::Sha256,
                        "xxhash" => algorithm = HashAlgorithm::Xxhash,
                        "blake3" => algorithm = HashAlgorithm::Blake3,
                        _ => {
                            eprintln!("{}: Invalid algorithm '{}'. Valid options are: md5, sha1, sha256, xxhash, blake3", env!("CARGO_PKG_NAME"), args[i + 1]);
                            return;
                        }
                    }
                    i += 1; // Skip the algorithm argument
                } else {
                    eprintln!(
                        "{}: -c flag requires an algorithm name",
                        env!("CARGO_PKG_NAME")
                    );
                    return;
                }
            }
            _ => {
                // This is a regular argument, not a flag
            }
        }
        i += 1;
    }

    // Process each non-flag argument
    let mut results = Vec::new();
    for (i, arg) in args.iter().enumerate() {
        if i == 0 {
            // Skip the program name
            continue;
        }
        // Skip flag arguments
        if arg == "-r" || arg == "-c" || arg == "-j" || arg == "-x" {
            continue;
        }
        // Skip algorithm arguments (they follow -c)
        if i > 1 && args[i - 1] == "-c" {
            continue;
        }
        process(
            arg,
            recursive_mode,
            &algorithm,
            json_mode,
            single_filesystem,
            &mut results,
        );
    }

    // Output JSON array if in JSON mode
    if json_mode {
        println!("{}", serde_json::to_string(&results).unwrap());
    }
}
