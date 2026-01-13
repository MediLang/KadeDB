use kadedb_services_api as api;
use kadedb_services_auth::AuthConfig;

#[tokio::test]
async fn health_endpoint_works_over_http() {
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0")
        .await
        .expect("bind");
    let addr = listener.local_addr().expect("local_addr");

    let server = tokio::spawn(async move {
        api::serve(
            listener,
            AuthConfig {
                enabled: false,
                jwt_secret: None,
            },
        )
        .await;
    });

    let url = format!("http://{addr}/health");
    let res = reqwest::get(url).await.expect("http get");
    assert!(res.status().is_success());

    server.abort();
}

#[tokio::test]
async fn query_endpoint_requires_auth_when_enabled() {
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0")
        .await
        .expect("bind");
    let addr = listener.local_addr().expect("local_addr");

    let server = tokio::spawn(async move {
        api::serve(
            listener,
            AuthConfig {
                enabled: true,
                jwt_secret: Some("secret".to_string()),
            },
        )
        .await;
    });

    let client = reqwest::Client::new();
    let url = format!("http://{addr}/query");
    let res = client
        .post(url)
        .json(&serde_json::json!({"query": "SELECT 1"}))
        .send()
        .await
        .expect("http post");

    assert_eq!(res.status(), reqwest::StatusCode::UNAUTHORIZED);

    server.abort();
}
