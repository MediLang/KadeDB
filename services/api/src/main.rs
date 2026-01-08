use axum::{
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use serde::{Deserialize, Serialize};

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()),
        )
        .init();

    let app = router();

    let listener = tokio::net::TcpListener::bind("0.0.0.0:8080")
        .await
        .expect("bind 0.0.0.0:8080");

    tracing::info!("listening on {}", listener.local_addr().unwrap());
    axum::serve(listener, app).await.expect("serve");
}

fn router() -> Router {
    Router::new()
        .route("/health", get(health))
        .route("/query", post(query))
        .route("/tables", post(create_table))
}

#[derive(Debug, Serialize)]
struct HealthResponse {
    status: &'static str,
}

async fn health() -> Json<HealthResponse> {
    Json(HealthResponse { status: "ok" })
}

#[derive(Debug, Deserialize)]
struct QueryRequest {
    query: String,
}

#[derive(Debug, Serialize)]
struct QueryResponse {
    ok: bool,
    echoed_query: String,
}

async fn query(Json(req): Json<QueryRequest>) -> (StatusCode, Json<QueryResponse>) {
    (
        StatusCode::OK,
        Json(QueryResponse {
            ok: true,
            echoed_query: req.query,
        }),
    )
}

#[derive(Debug, Deserialize)]
struct CreateTableRequest {
    name: String,
    columns: Vec<ColumnDef>,
}

#[derive(Debug, Deserialize)]
struct ColumnDef {
    name: String,
    column_type: String,
    nullable: Option<bool>,
}

#[derive(Debug, Serialize)]
struct CreateTableResponse {
    ok: bool,
    table: String,
    column_count: usize,
    columns: Vec<ColumnSummary>,
}

#[derive(Debug, Serialize)]
struct ColumnSummary {
    name: String,
    column_type: String,
    nullable: bool,
}

async fn create_table(
    Json(req): Json<CreateTableRequest>,
) -> (StatusCode, Json<CreateTableResponse>) {
    let table = req.name;
    let columns: Vec<ColumnSummary> = req
        .columns
        .into_iter()
        .map(|c| ColumnSummary {
            name: c.name,
            column_type: c.column_type,
            nullable: c.nullable.unwrap_or(true),
        })
        .collect();
    let column_count = columns.len();

    (
        StatusCode::OK,
        Json(CreateTableResponse {
            ok: true,
            table,
            column_count,
            columns,
        }),
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use axum::body::Body;
    use axum::http::{Request, StatusCode};
    use tower::ServiceExt;

    #[tokio::test]
    async fn health_returns_ok() {
        let app = router();

        let res = app
            .oneshot(
                Request::builder()
                    .uri("/health")
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(res.status(), StatusCode::OK);
    }

    #[tokio::test]
    async fn query_echoes_query() {
        let app = router();
        let body = serde_json::json!({"query": "SELECT 1"}).to_string();

        let res = app
            .oneshot(
                Request::builder()
                    .method("POST")
                    .uri("/query")
                    .header("content-type", "application/json")
                    .body(Body::from(body))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(res.status(), StatusCode::OK);
    }
}
