use jsonwebtoken::{Algorithm, DecodingKey, Validation};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Role {
    Read,
    Write,
    Admin,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Permission {
    Read,
    Write,
}

#[derive(Debug, thiserror::Error)]
pub enum AuthError {
    #[error("missing authorization")]
    MissingAuthorization,

    #[error("invalid authorization scheme")]
    InvalidAuthorizationScheme,

    #[error("jwt error")]
    Jwt(#[from] jsonwebtoken::errors::Error),

    #[error("missing role claim")]
    MissingRole,

    #[error("unknown role")]
    UnknownRole,

    #[error("forbidden")]
    Forbidden,
}

#[derive(Debug, Clone)]
pub struct AuthConfig {
    pub enabled: bool,
    pub jwt_secret: Option<String>,
}

impl AuthConfig {
    pub fn from_env() -> Self {
        let enabled = std::env::var("KADEDB_AUTH_ENABLED")
            .ok()
            .as_deref()
            .map(|v| v == "1" || v.eq_ignore_ascii_case("true"))
            .unwrap_or(false);

        let jwt_secret = std::env::var("KADEDB_JWT_SECRET").ok();

        Self {
            enabled,
            jwt_secret,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Claims {
    pub sub: Option<String>,
    pub role: Option<String>,
    pub exp: Option<u64>,
    pub iat: Option<u64>,
}

fn role_from_claims(claims: &Claims) -> Result<Role, AuthError> {
    let role = claims.role.as_deref().ok_or(AuthError::MissingRole)?;
    match role {
        "read" => Ok(Role::Read),
        "write" => Ok(Role::Write),
        "admin" => Ok(Role::Admin),
        _ => Err(AuthError::UnknownRole),
    }
}

fn role_allows(role: Role, permission: Permission) -> bool {
    match (role, permission) {
        (Role::Admin, _) => true,
        (Role::Write, Permission::Read) => true,
        (Role::Write, Permission::Write) => true,
        (Role::Read, Permission::Read) => true,
        (Role::Read, Permission::Write) => false,
    }
}

pub fn authorize_bearer_header(
    cfg: &AuthConfig,
    authorization_header: Option<&str>,
    required: Permission,
) -> Result<Option<Role>, AuthError> {
    if !cfg.enabled {
        return Ok(None);
    }

    let secret = cfg
        .jwt_secret
        .as_deref()
        .ok_or(AuthError::MissingAuthorization)?;

    let header = authorization_header.ok_or(AuthError::MissingAuthorization)?;
    let token = header
        .strip_prefix("Bearer ")
        .ok_or(AuthError::InvalidAuthorizationScheme)?;

    let mut validation = Validation::new(Algorithm::HS256);
    validation.validate_exp = false;

    let data = jsonwebtoken::decode::<Claims>(
        token,
        &DecodingKey::from_secret(secret.as_bytes()),
        &validation,
    )?;

    let role = role_from_claims(&data.claims)?;
    if !role_allows(role, required) {
        return Err(AuthError::Forbidden);
    }

    Ok(Some(role))
}
