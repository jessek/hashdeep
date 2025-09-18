use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::Path;

mod hash;
use hash::compute_hash_stdin;
mod process;
use process::{ProcessState, process};

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
