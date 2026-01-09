use std::path::PathBuf;

fn main() {
    // Allow disabling link behavior for tooling environments.
    if std::env::var_os("CARGO_FEATURE_LINK_NATIVE").is_none() {
        return;
    }

    // Repo layout: services/ffi -> repo_root/build/debug/lib
    let manifest_dir =
        PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let repo_root = manifest_dir
        .parent()
        .and_then(|p| p.parent())
        .expect("repo root");

    let build_type = std::env::var("KADEDB_CMAKE_PRESET").unwrap_or_else(|_| "debug".to_string());
    let lib_dir = repo_root.join("build").join(build_type).join("lib");

    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=kadedb_c");

    // Prefer rpath for dev/CI so executables can find libkadedb_c.so without LD_LIBRARY_PATH.
    #[cfg(target_os = "linux")]
    {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir.display());
    }
    #[cfg(target_os = "macos")]
    {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir.display());
    }
}
