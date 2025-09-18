use crate::hash::compute_hash;
use crate::{HashAlgorithm, HashResult};
use std::fs;
use std::path::Path;
use walkdir::WalkDir;

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
