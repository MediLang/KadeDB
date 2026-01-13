use kadedb_services_auth::AuthConfig;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()),
        )
        .init();

    let addr = "0.0.0.0:50051".parse().expect("valid addr");
    let auth_cfg = AuthConfig::from_env();

    tracing::info!("gRPC listening on {addr}");

    kadedb_services_grpc::serve(addr, auth_cfg).await;
}
