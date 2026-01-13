use clap::{Parser, Subcommand};
use kadedb_services_grpc::kadedb::query_service_client::QueryServiceClient;
use kadedb_services_grpc::kadedb::QueryRequest;

#[derive(Parser)]
#[command(name = "kadedb-services-examples")]
struct Cli {
    #[command(subcommand)]
    cmd: Command,
}

#[derive(Subcommand)]
enum Command {
    RestHealth {
        #[arg(long, default_value = "http://127.0.0.1:8080")]
        base_url: String,
    },
    RestQuery {
        #[arg(long, default_value = "http://127.0.0.1:8080")]
        base_url: String,
        #[arg(long)]
        token: Option<String>,
        #[arg(long, default_value = "SELECT 1")]
        query: String,
    },
    GrpcQuery {
        #[arg(long, default_value = "http://127.0.0.1:50051")]
        endpoint: String,
        #[arg(long)]
        token: Option<String>,
        #[arg(long, default_value = "SELECT 1")]
        query: String,
    },
}

#[tokio::main]
async fn main() {
    let cli = Cli::parse();

    match cli.cmd {
        Command::RestHealth { base_url } => {
            let url = format!("{base_url}/health");
            let res = reqwest::get(url).await.expect("http get");
            let status = res.status();
            let body = res.text().await.unwrap_or_default();
            println!("{status}\n{body}");
        }
        Command::RestQuery {
            base_url,
            token,
            query,
        } => {
            let url = format!("{base_url}/query");
            let client = reqwest::Client::new();
            let mut req = client.post(url).json(&serde_json::json!({"query": query}));
            if let Some(token) = token {
                req = req.header("authorization", format!("Bearer {token}"));
            }
            let res = req.send().await.expect("http post");
            let status = res.status();
            let body = res.text().await.unwrap_or_default();
            println!("{status}\n{body}");
        }
        Command::GrpcQuery {
            endpoint,
            token,
            query,
        } => {
            let mut client = QueryServiceClient::connect(endpoint)
                .await
                .expect("connect");

            let mut req = tonic::Request::new(QueryRequest { query });
            if let Some(token) = token {
                req.metadata_mut().insert(
                    "authorization",
                    format!("Bearer {token}").parse().expect("metadata"),
                );
            }

            let mut stream = client.query(req).await.expect("query").into_inner();
            while let Some(row) = stream.message().await.expect("message") {
                println!("{}", row.json);
            }
        }
    }
}
