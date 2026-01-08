use std::pin::Pin;

use tokio_stream::wrappers::ReceiverStream;
use tonic::{transport::Server, Request, Response, Status};

pub mod kadedb {
    tonic::include_proto!("kadedb");
}

use kadedb::query_service_server::{QueryService, QueryServiceServer};
use kadedb::{QueryRequest, QueryRow};

#[derive(Default)]
struct QueryServiceImpl;

#[tonic::async_trait]
impl QueryService for QueryServiceImpl {
    type QueryStream = Pin<Box<dyn tokio_stream::Stream<Item = Result<QueryRow, Status>> + Send>>;

    async fn query(
        &self,
        request: Request<QueryRequest>,
    ) -> Result<Response<Self::QueryStream>, Status> {
        let query = request.into_inner().query;

        let (tx, rx) = tokio::sync::mpsc::channel(8);

        tokio::spawn(async move {
            let rows = [
                serde_json::json!({"echo": query, "row": 1}).to_string(),
                serde_json::json!({"echo": query, "row": 2}).to_string(),
                serde_json::json!({"echo": query, "row": 3}).to_string(),
            ];

            for json in rows {
                if tx.send(Ok(QueryRow { json })).await.is_err() {
                    break;
                }
            }
        });

        Ok(Response::new(
            Box::pin(ReceiverStream::new(rx)) as Self::QueryStream
        ))
    }
}

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()),
        )
        .init();

    let addr = "0.0.0.0:50051".parse().expect("valid addr");
    let svc = QueryServiceServer::new(QueryServiceImpl);

    tracing::info!("gRPC listening on {addr}");

    Server::builder()
        .add_service(svc)
        .serve(addr)
        .await
        .expect("serve");
}
