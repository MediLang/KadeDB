use kadedb_services_auth::AuthConfig;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()),
        )
        .init();

    let auth_cfg = AuthConfig::from_env();

    let listener = tokio::net::TcpListener::bind("0.0.0.0:8080")
        .await
        .expect("bind 0.0.0.0:8080");

    tracing::info!("listening on {}", listener.local_addr().unwrap());
    kadedb_services_api::serve(listener, auth_cfg).await;
}
