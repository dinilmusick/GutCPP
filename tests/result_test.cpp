#include "gut/result.hpp"
#include <cassert>
#include <string>
#include <memory>
int main() {
  auto success = gut::Result<int>::success(42);
  assert(success.ok() && success.value() == 42);
  success.value() = 43;
  assert(success.value() == 43);
  auto failure = gut::Result<int>::failure("INVALID", "bad input");
  assert(!failure.ok());
  assert(failure.error().code == "INVALID");
  assert(failure.error().message == "bad input");
  auto move_only = gut::Result<std::unique_ptr<int>>::success(std::make_unique<int>(7));
  assert(*move_only.value() == 7);
}
