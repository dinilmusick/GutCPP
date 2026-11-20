#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace gut {
struct Error {
  std::string code;
  std::string message;
};
template <typename T> class Result {
 public:
  static Result success(T value) { return Result(std::in_place_index<0>, std::move(value)); }
  static Result failure(std::string code, std::string message) {
    return Result(std::in_place_index<1>, Error{std::move(code), std::move(message)});
  }
  bool ok() const noexcept { return std::holds_alternative<T>(data_); }
  explicit operator bool() const noexcept { return ok(); }
  const T& value() const { return std::get<T>(data_); }
  T& value() { return std::get<T>(data_); }
  const Error& error() const { return std::get<Error>(data_); }
 private:
  template <typename... Args> Result(std::in_place_index_t<0> tag, Args&&... args)
      : data_(tag, std::forward<Args>(args)...) {}
  template <typename... Args> Result(std::in_place_index_t<1> tag, Args&&... args)
      : data_(tag, std::forward<Args>(args)...) {}
  std::variant<T, Error> data_;
};
}  // namespace gut
