#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace kadedb {

// Error/status codes for storage operations
enum class StatusCode {
  Ok = 0,
  NotFound,
  AlreadyExists,
  InvalidArgument,
  FailedPrecondition,
  Internal
};

class Status {
public:
  Status() : code_(StatusCode::Ok) {}
  explicit Status(StatusCode c, std::string msg = {})
      : code_(c), message_(std::move(msg)) {}

  static Status OK() { return Status(StatusCode::Ok); }
  static Status NotFound(std::string msg = {}) {
    return Status(StatusCode::NotFound, std::move(msg));
  }
  static Status AlreadyExists(std::string msg = {}) {
    return Status(StatusCode::AlreadyExists, std::move(msg));
  }
  static Status InvalidArgument(std::string msg = {}) {
    return Status(StatusCode::InvalidArgument, std::move(msg));
  }
  static Status FailedPrecondition(std::string msg = {}) {
    return Status(StatusCode::FailedPrecondition, std::move(msg));
  }
  static Status Internal(std::string msg = {}) {
    return Status(StatusCode::Internal, std::move(msg));
  }

  bool ok() const { return code_ == StatusCode::Ok; }
  StatusCode code() const { return code_; }
  const std::string &message() const { return message_; }

private:
  StatusCode code_;
  std::string message_;
};

// Simple result wrapper for APIs that need to return a value or an error
// Usage:
//   Result<int> r = Result<int>::ok(42);
//   Result<std::string> e = Result<std::string>::err(Status::NotFound("x"));

template <typename T> class Result {
public:
  static Result<T> ok(T value) {
    return Result<T>(Status::OK(), std::move(value));
  }
  static Result<T> err(Status s) { return Result<T>(std::move(s)); }

  bool hasValue() const { return status_.ok(); }
  const Status &status() const { return status_; }

  // Accessors (caller must ensure hasValue())
  const T &value() const { return *value_; }
  T &value() { return *value_; }

  // Move-out accessor: safely moves the contained value out of the Result,
  // leaving the optional disengaged. Caller must ensure hasValue().
  T takeValue() {
    if (!status_.ok() || !value_.has_value())
      throw std::runtime_error("Result::takeValue(): no value present");
    T tmp = std::move(*value_);
    value_.reset();
    return tmp;
  }

private:
  explicit Result(Status s) : status_(std::move(s)) {}
  Result(Status s, T v) : status_(std::move(s)), value_(std::move(v)) {}

  Status status_;
  std::optional<T> value_;
};

} // namespace kadedb
