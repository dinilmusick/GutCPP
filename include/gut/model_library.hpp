#pragma once
// Native C++17 ModelLibrary counterpart to pinned GutTS models/model-library.ts.
// Host runtime performs real model loading and unloading; no inference is
// fabricated by the foundation. Synchronous operations are explicit.
#include "gut/model_registry.hpp"
#include "gut/model_residency.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace gut {
template <class T> class model_runtime {
public:
  virtual ~model_runtime()=default;
  virtual T load(const model_metadata& metadata)=0;
  virtual void unload(T& model)=0;
};

template <class T> class model_library {
  model_metadata metadata_;
  std::shared_ptr<model_runtime<T>> runtime_;
  model_residency<T> residency_;
public:
  explicit model_library(model_metadata metadata,std::shared_ptr<model_runtime<T>> runtime)
      :metadata_(std::move(metadata)),runtime_(std::move(runtime)){
    if(!runtime_)throw std::invalid_argument("ModelLibrary requires a real runtime");
    if(metadata_.id.empty())throw std::invalid_argument("ModelLibrary requires a model identifier");
  }
  model_library(const model_library&)=delete;
  model_library& operator=(const model_library&)=delete;
  const model_metadata& metadata()const noexcept{return metadata_;}
  bool is_loaded()const{return residency_.is_loaded();}
  std::size_t active_users()const{return residency_.active_users();}
  std::exception_ptr last_cleanup_error()const{return residency_.last_cleanup_error();}
  template <class Operation>
  auto with_model(Operation&& operation)->std::invoke_result_t<Operation,T&>{
    return residency_.run(
      [this]{return runtime_->load(metadata_);},
      [this](T& model){runtime_->unload(model);},
      std::forward<Operation>(operation));
  }
};
} // namespace gut
