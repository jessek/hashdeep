use crate::hash::compute_hash;
use crate::{HashAlgorithm, HashResult};
use std::fs;
use std::path::Path;
use walkdir::WalkDir;

// Import from match module
use crate::r#match::KnownHashes;

pub struct ProcessState<'a> {
    pub recursive_mode: bool,
    pub algorithm: &'a HashAlgorithm,
    pub json_mode: bool,
    pub single_filesystem: bool,
    pub results: &'a mut Vec<HashResult>,
}

pub fn process(arg: &String, state: &mut ProcessState) {
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
                crate::output_hash_result(
                    path,
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
                        let entry_path = entry.path();
                        if entry_path.is_file() {
                            match compute_hash(entry_path, state.algorithm) {
                                Ok(hash) => {
                                    crate::output_hash_result(
                                        entry_path,
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
                                        entry_path.display(),
                                        e
                                    );
                                }
                            }
                        }
                    }
                    Err(e) => {
                        eprintln!("{}: Error reading directory entry: {}", arg, e);
                    }
                }
            }
        } else {
            eprintln!("{}: is a directory (use -r to process recursively)", arg);
        }
    } else {
        eprintln!("{}: is not a regular file or directory", arg);
    }
}

pub fn process_with_matching(path: &str, state: &mut ProcessState, known_hashes: &KnownHashes) {
    let path = Path::new(path);

    if path.is_file() {
        // Compute hash for the file
        match compute_hash(path, state.algorithm) {
            Ok(hash) => {
                // Check for matches in known hashes
                if let Some(known_hash) = known_hashes.find_match(&hash, state.algorithm.as_str()) {
                    crate::output_matching_result(
                        path,
                        &hash,
                        state.algorithm,
                        known_hash,
                        state.json_mode,
                        state.results,
                    );
                }
            }
            Err(e) => {
                eprintln!(
                    "{}: Error computing hash for '{}': {}",
                    env!("CARGO_PKG_NAME"),
                    path.display(),
                    e
                );
            }
        }
    } else if path.is_dir() && state.recursive_mode {
        // Process directory recursively
        let walker = walkdir::WalkDir::new(path)
            .follow_links(false)
            .same_file_system(state.single_filesystem);

        for entry in walker {
            match entry {
                Ok(entry) => {
                    if entry.file_type().is_file() {
                        let file_path = entry.path();
                        match compute_hash(file_path, state.algorithm) {
                            Ok(hash) => {
                                // Check for matches in known hashes
                                if let Some(known_hash) =
                                    known_hashes.find_match(&hash, state.algorithm.as_str())
                                {
                                    crate::output_matching_result(
                                        file_path,
                                        &hash,
                                        state.algorithm,
                                        known_hash,
                                        state.json_mode,
                                        state.results,
                                    );
                                }
                            }
                            Err(e) => {
                                eprintln!(
                                    "{}: Error computing hash for '{}': {}",
                                    env!("CARGO_PKG_NAME"),
                                    file_path.display(),
                                    e
                                );
                            }
                        }
                    }
                }
                Err(e) => {
                    eprintln!(
                        "{}: Error accessing '{}': {}",
                        env!("CARGO_PKG_NAME"),
                        e.path()
                            .map(|p| p.display().to_string())
                            .unwrap_or_else(|| "unknown".to_string()),
                        e
                    );
                }
            }
        }
    } else if !path.exists() {
        eprintln!(
            "{}: '{}': No such file or directory",
            env!("CARGO_PKG_NAME"),
            path.display()
        );
    } else {
        eprintln!(
            "{}: '{}': Is a directory (use -r for recursive processing)",
            env!("CARGO_PKG_NAME"),
            path.display()
        );
    }
}
