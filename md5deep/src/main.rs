use clap::Parser;
use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::Path;

mod hash;
use hash::compute_hash_stdin;
mod process;
use process::{ProcessState, process, process_with_matching};
mod r#match;
use r#match::{KnownHash, KnownHashes};

#[derive(Debug, Clone, clap::ValueEnum)]
enum HashAlgorithm {
    Md5,
    Sha1,
    Sha256,
    Xxhash,
    Blake3,
}

#[derive(Parser)]
#[command(name = "md5deep")]
#[command(version = env!("CARGO_PKG_VERSION"))]
#[command(about = "A tool for computing cryptographic hashes of files")]
#[command(author = "Jesse Kornblum <jessekornblum@gmail.com>")]
struct Args {
    /// Files or directories to process
    files: Vec<String>,

    /// Recursively process directories
    #[arg(short = 'r', long = "recursive")]
    recursive: bool,

    /// Select hash algorithm
    #[arg(short = 'c', long = "algorithm", default_value = "md5")]
    algorithm: HashAlgorithm,

    /// Output in JSON format
    #[arg(short = 'j', long = "json")]
    json: bool,

    /// Only process files on the same filesystem
    #[arg(short = 'x', long = "one-filesystem")]
    one_filesystem: bool,

    /// Read file arguments from file, one per line
    #[arg(short = 'f', long = "filelist")]
    filelist: Option<String>,

    /// Read known hashes from file for matching
    #[arg(short = 'm', long = "matching")]
    matching: Option<String>,
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

fn output_matching_result(
    file_path: &Path,
    hash: &str,
    algorithm: &HashAlgorithm,
    known_hash: &KnownHash,
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
        if let Some(known_filename) = &known_hash.filename {
            println!(
                "{}  {}  MATCH: {}",
                hash,
                file_path.display(),
                known_filename
            );
        } else {
            println!("{}  {}  MATCH", hash, file_path.display());
        }
    }
}

fn main() {
    let args = Args::parse();

    let mut file_args = args.files;

    // If -f/--filelist was provided, read arguments from the file
    if let Some(filename) = args.filelist {
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

    // Load known hashes if matching mode is enabled
    let known_hashes = if let Some(matching_file) = &args.matching {
        match KnownHashes::load_from_file(matching_file) {
            Ok(hashes) => Some(hashes),
            Err(e) => {
                eprintln!(
                    "{}: Error loading known hashes from '{}': {}",
                    env!("CARGO_PKG_NAME"),
                    matching_file,
                    e
                );
                return;
            }
        }
    } else {
        None
    };

    let mut results = Vec::new();

    // If no file arguments provided, process stdin
    if file_args.is_empty() {
        match compute_hash_stdin(&args.algorithm) {
            Ok(hash) => {
                if let Some(ref known_hashes) = known_hashes {
                    // Check for matches in known hashes
                    if let Some(known_hash) =
                        known_hashes.find_match(&hash, args.algorithm.as_str())
                    {
                        output_matching_result(
                            &Path::new("-"),
                            &hash,
                            &args.algorithm,
                            known_hash,
                            args.json,
                            &mut results,
                        );
                    }
                } else if args.json {
                    let mut algorithm_hash = std::collections::HashMap::new();
                    algorithm_hash.insert(args.algorithm.as_str().to_string(), hash.to_string());

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
            if let Some(ref known_hashes) = known_hashes {
                // In matching mode, we need to compute hash and check for matches
                process_with_matching(
                    &arg,
                    &mut ProcessState {
                        recursive_mode: args.recursive,
                        algorithm: &args.algorithm,
                        json_mode: args.json,
                        single_filesystem: args.one_filesystem,
                        results: &mut results,
                    },
                    known_hashes,
                );
            } else {
                // Normal processing without matching
                process(
                    &arg,
                    &mut ProcessState {
                        recursive_mode: args.recursive,
                        algorithm: &args.algorithm,
                        json_mode: args.json,
                        single_filesystem: args.one_filesystem,
                        results: &mut results,
                    },
                );
            }
        }
    }

    // Output JSON array if in JSON mode
    if args.json {
        println!("{}", serde_json::to_string(&results).unwrap());
    }
}
