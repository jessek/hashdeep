use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();

    // Check for -v flag
    if args.len() > 1 && args[1] == "-v" {
        println!("{}", env!("CARGO_PKG_VERSION"));
        return;
    }

    // Print each command line argument on its own line
    for (i, arg) in args.iter().enumerate() {
        if i == 0 {
            // Skip the program name
            continue;
        }
        println!("{}", arg);
    }
}
