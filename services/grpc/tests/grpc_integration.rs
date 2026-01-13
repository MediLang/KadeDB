use kadedb_services_auth::AuthConfig;
use kadedb_services_grpc::{
    kadedb::query_service_client::QueryServiceClient, kadedb::QueryRequest,
};

#[tokio::test]
async fn grpc_query_streams_rows() {
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0")
        .await
        .expect("bind");
    let addr = listener.local_addr().expect("local_addr");

    let server = tokio::spawn(async move {
        kadedb_services_grpc::serve_with_listener(
            listener,
            AuthConfig {
                enabled: false,
                jwt_secret: None,
            },
        )
        .await;
    });

    let endpoint = format!("http://{addr}");
    let mut client = QueryServiceClient::connect(endpoint)
        .await
        .expect("connect");

    let mut stream = client
        .query(QueryRequest {
            query: "SELECT 1".to_string(),
        })
        .await
        .expect("query")
        .into_inner();

    let mut count = 0usize;
    while let Some(_row) = stream.message().await.expect("message") {
        count += 1;
    }

    assert_eq!(count, 3);

    server.abort();
}
