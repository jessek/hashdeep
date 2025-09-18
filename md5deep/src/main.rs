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
    for arg in &args[1..] {
        if arg == "-v" || arg == "--version" {
            println!("{}", env!("CARGO_PKG_VERSION"));
            return;
        }
        if arg == "-h" || arg == "--help" {
            println!("Usage: {} [options] [files...]\n", args[0]);
            println!("Options:");
            println!("  -r, --recursive         Recursively process directories");
            println!(
                "  -c, --algorithm <alg>   Select hash algorithm: md5, sha1, sha256, xxhash, blake3"
            );
            println!("  -j, --json              Output in JSON format");
            println!("  -x, --one-filesystem    Only process files on the same filesystem");
            println!("  -f, --filelist <file>   Read file arguments from <file>, one per line");
            println!("  -v, --version           Show version information");
            println!("  -h, --help              Show this help message");
            return;
        }
    }

    let mut file_args = Vec::new();
    let mut file_arg_from_file: Option<String> = None;
    let mut i = 0;
    while i < args.len() {
        let arg = &args[i];
        if (arg == "-f" || arg == "--filelist") && i + 1 < args.len() {
            file_arg_from_file = Some(args[i + 1].clone());
            i += 2;
            continue;
        }
        if i == 0 {
            i += 1;
            continue;
        }
        // Skip flag arguments
        if arg == "-r"
            || arg == "--recursive"
            || arg == "-j"
            || arg == "--json"
            || arg == "-x"
            || arg == "--one-filesystem"
        {
            i += 1;
            continue;
        }
        // Skip algorithm arguments (they follow -c or --algorithm)
        if i > 1 && (args[i - 1] == "-c" || args[i - 1] == "--algorithm") {
            i += 1;
            continue;
        }
        file_args.push(arg.clone());
        i += 1;
    }

    // If -f/--filelist was provided, read arguments from the file
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

    // Parse flags
    let recursive_mode =
        args.contains(&"-r".to_string()) || args.contains(&"--recursive".to_string());
    let json_mode = args.contains(&"-j".to_string()) || args.contains(&"--json".to_string());
    let single_filesystem =
        args.contains(&"-x".to_string()) || args.contains(&"--one-filesystem".to_string());
    let mut algorithm = HashAlgorithm::Md5;
    let mut i = 0;
    while i < args.len() {
        if args[i] == "-c" || args[i] == "--algorithm" {
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
                i += 1;
            }
        }
        i += 1;
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
