use blake3::Hasher;
use serde::{Deserialize, Serialize};
use sha1::Digest as Sha1Digest;
use std::env;
use std::fs;
use std::io::{self, Read};
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

#[derive(Serialize, Deserialize, Debug)]
struct HashResult {
    filename: String,
    size: u64,
    #[serde(flatten)]
    algorithm_hash: std::collections::HashMap<String, String>,
}

#[derive(Debug)]
struct ProcessState<'a> {
    recursive_mode: bool,
    algorithm: &'a HashAlgorithm,
    json_mode: bool,
    single_filesystem: bool,
    results: &'a mut Vec<HashResult>,
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

fn compute_hash_stdin(algorithm: &HashAlgorithm) -> Result<String, std::io::Error> {
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

fn process(arg: &String, state: &mut ProcessState) {
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
        match compute_hash(path, state.algorithm) {
            Ok(hash) => {
                output_hash_result(path, &hash, state.algorithm, state.json_mode, state.results);
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
        if state.recursive_mode {
            // Walk the directory tree recursively
            let walker = if state.single_filesystem {
                WalkDir::new(path).same_file_system(true)
            } else {
                WalkDir::new(path)
            };

            for entry in walker {
                match entry {
                    Ok(entry) => {
                        if entry.file_type().is_file() {
                            match compute_hash(entry.path(), state.algorithm) {
                                Ok(hash) => {
                                    output_hash_result(
                                        entry.path(),
                                        &hash,
                                        state.algorithm,
                                        state.json_mode,
                                        state.results,
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
                            eprintln!(
                                "{}: Invalid algorithm '{}'. Valid options are: md5, sha1, sha256, xxhash, blake3",
                                env!("CARGO_PKG_NAME"),
                                args[i + 1]
                            );
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

    // Collect non-flag arguments
    let mut file_args = Vec::new();
    let mut file_arg_from_file: Option<String> = None;
    let mut i = 0;
    while i < args.len() {
        let arg = &args[i];
        if arg == "-f" && i + 1 < args.len() {
            file_arg_from_file = Some(args[i + 1].clone());
            i += 2;
            continue;
        }
        if i == 0 {
            i += 1;
            continue;
        }
        // Skip flag arguments
        if arg == "-r" || arg == "-c" || arg == "-j" || arg == "-x" {
            i += 1;
            continue;
        }
        // Skip algorithm arguments (they follow -c)
        if i > 1 && args[i - 1] == "-c" {
            i += 1;
            continue;
        }
        file_args.push(arg.clone());
        i += 1;
    }

    // If -f was provided, read arguments from the file
    if let Some(filename) = file_arg_from_file {
        match std::fs::read_to_string(&filename) {
            Ok(contents) => {
                for line in contents.lines() {
                    let trimmed = line.trim();
                    if !trimmed.is_empty() {
                        file_args.push(trimmed.to_string());
                    }
                }
            }
            Err(e) => {
                eprintln!(
                    "{}: Error reading argument file '{}': {}",
                    env!("CARGO_PKG_NAME"),
                    filename,
                    e
                );
                return;
            }
        }
    }

    let mut results = Vec::new();

    // If no file arguments provided, process stdin
    if file_args.is_empty() {
        match compute_hash_stdin(&algorithm) {
            Ok(hash) => {
                if json_mode {
                    let mut algorithm_hash = std::collections::HashMap::new();
                    algorithm_hash.insert(algorithm.as_str().to_string(), hash.to_string());

                    let result = HashResult {
                        filename: "<stdin>".to_string(),
                        size: 0, // We don't know the size of stdin
                        algorithm_hash,
                    };
                    results.push(result);
                } else {
                    println!("{}  -", hash);
                }
            }
            Err(e) => {
                eprintln!(
                    "{}: Error computing hash for stdin: {}",
                    env!("CARGO_PKG_NAME"),
                    e
                );
            }
        }
    } else {
        // Process each file argument
        for arg in file_args {
            process(
                &arg,
                &mut ProcessState {
                    recursive_mode,
                    algorithm: &algorithm,
                    json_mode,
                    single_filesystem,
                    results: &mut results,
                },
            );
        }
    }

    // Output JSON array if in JSON mode
    if json_mode {
        println!("{}", serde_json::to_string(&results).unwrap());
    }
}
