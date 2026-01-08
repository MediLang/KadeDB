fn main() {
    let protoc = protoc_bin_vendored::protoc_bin_path().expect("vendored protoc");
    std::env::set_var("PROTOC", protoc);

    tonic_build::configure()
        .build_server(true)
        .compile_protos(&["../proto/kadedb.proto"], &["../proto"])
        .expect("compile proto");
}
