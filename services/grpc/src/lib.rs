use std::pin::Pin;

use kadedb_services_auth::{authorize_bearer_header, AuthConfig, AuthError, Permission};
use tokio_stream::wrappers::ReceiverStream;
use tokio_stream::wrappers::TcpListenerStream;
use tonic::{transport::Server, Request, Response, Status};

pub mod kadedb {
    tonic::include_proto!("kadedb");
}

fn map_auth_error(err: AuthError) -> Status {
    match err {
        AuthError::Forbidden => Status::permission_denied("forbidden"),
        _ => Status::unauthenticated("unauthenticated"),
    }
}

use kadedb::query_service_server::{QueryService, QueryServiceServer};
use kadedb::{QueryRequest, QueryRow};

#[derive(Default)]
pub struct QueryServiceImpl;

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

pub async fn serve(addr: std::net::SocketAddr, auth_cfg: AuthConfig) {
    let listener = tokio::net::TcpListener::bind(addr).await.expect("bind");
    serve_with_listener(listener, auth_cfg).await;
}

pub async fn serve_with_listener(listener: tokio::net::TcpListener, auth_cfg: AuthConfig) {
    let interceptor = move |req: Request<()>| -> Result<Request<()>, Status> {
        if !auth_cfg.enabled {
            return Ok(req);
        }

        let header = req
            .metadata()
            .get("authorization")
            .and_then(|v| v.to_str().ok());

        authorize_bearer_header(&auth_cfg, header, Permission::Read)
            .map(|_| req)
            .map_err(map_auth_error)
    };

    let svc = QueryServiceServer::with_interceptor(QueryServiceImpl, interceptor);

    Server::builder()
        .add_service(svc)
        .serve_with_incoming(TcpListenerStream::new(listener))
        .await
        .expect("serve");
}
