use crate::HashResult;
use std::fs;

#[derive(Debug, Clone)]
pub struct KnownHash {
    pub hash: String,
    pub filename: Option<String>,
}

#[derive(Debug)]
pub struct KnownHashes {
    pub hashes: std::collections::HashMap<String, Vec<KnownHash>>, // algorithm -> list of hashes
}

impl KnownHashes {
    pub fn new() -> Self {
        Self {
            hashes: std::collections::HashMap::new(),
        }
    }

    pub fn load_from_file(filename: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let content = fs::read_to_string(filename)?;

        // Try to parse as JSON first
        if let Ok(json_hashes) = serde_json::from_str::<Vec<HashResult>>(&content) {
            return Self::from_json_hashes(json_hashes);
        }

        // If JSON parsing fails, try plain text format
        Self::from_plain_text(&content)
    }

    fn from_json_hashes(json_hashes: Vec<HashResult>) -> Result<Self, Box<dyn std::error::Error>> {
        let mut known_hashes = Self::new();

        for hash_result in json_hashes {
            for (algorithm, hash) in hash_result.algorithm_hash {
                let known_hash = KnownHash {
                    hash,
                    filename: hash_result.filename.clone(),
                };
                known_hashes
                    .hashes
                    .entry(algorithm)
                    .or_insert_with(Vec::new)
                    .push(known_hash);
            }
        }

        Ok(known_hashes)
    }

    fn from_plain_text(content: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let mut known_hashes = Self::new();

        for line in content.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            // Parse line format: "hash  filename" or just "hash"
            let parts: Vec<&str> = line.splitn(2, "  ").collect();
            let hash = parts[0].to_string();
            let filename = if parts.len() > 1 && !parts[1].is_empty() {
                Some(parts[1].to_string())
            } else {
                None
            };

            // We need to determine the algorithm based on hash length
            // This is a simple heuristic - in practice, you might want to be more sophisticated
            let algorithm = match hash.len() {
                16 => "xxhash".to_string(),
                32 => "md5".to_string(),
                40 => "sha1".to_string(),
                // @nocommit - Resolve conflict between sha256 and blake3
                64 => "sha256".to_string(), // Both sha256 and blake3 are 64 chars, default to sha256
                _ => "md5".to_string(),     // Default to md5 for unknown lengths
            };

            let known_hash = KnownHash { hash, filename };
            known_hashes
                .hashes
                .entry(algorithm)
                .or_insert_with(Vec::new)
                .push(known_hash);
        }

        Ok(known_hashes)
    }

    pub fn find_match(&self, hash: &str, algorithm: &str) -> Option<&KnownHash> {
        if let Some(hashes) = self.hashes.get(algorithm) {
            hashes.iter().find(|h| h.hash == hash)
        } else {
            None
        }
    }
}
