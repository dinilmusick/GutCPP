#pragma once
// Thread-safe, synchronous C++17 counterpart to GutTS ModelResidency.
// The loader/unloader are injected by the caller, never simulated. A model is
// shared while active leases exist; the final release unloads it exactly once.
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace gut {
template<class T> class model_residency {
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::shared_ptr<T> model_;
  std::size_t users_=0;
  bool loading_=false;
  bool unloading_=false;
  std::exception_ptr last_cleanup_error_;

  template<class Unloader> void release(Unloader& unload,bool preserve_primary) {
    std::shared_ptr<T> model_to_unload;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if(users_==0)std::terminate();
      --users_;
      if(users_==0){unloading_=true;model_to_unload=std::move(model_);}
    }
    if(model_to_unload) {
      std::exception_ptr cleanup_error;
      try{std::invoke(unload,*model_to_unload);}catch(...){cleanup_error=std::current_exception();}
      {
        std::lock_guard<std::mutex> lock(mutex_);
        last_cleanup_error_=cleanup_error;
        unloading_=false;
      }
      cv_.notify_all();
      if(cleanup_error&&!preserve_primary)std::rethrow_exception(cleanup_error);
    }
  }
 public:
  model_residency()=default;
  model_residency(const model_residency&)=delete;
  model_residency& operator=(const model_residency&)=delete;
  bool is_loaded() const {std::lock_guard<std::mutex> lock(mutex_);return static_cast<bool>(model_);}
  std::size_t active_users() const {std::lock_guard<std::mutex> lock(mutex_);return users_;}
  std::exception_ptr last_cleanup_error() const {std::lock_guard<std::mutex> lock(mutex_);return last_cleanup_error_;}

  template<class Loader,class Unloader,class Operation>
  auto run(Loader&& load,Unloader&& unload,Operation&& operation)
      -> std::invoke_result_t<Operation,T&> {
    using result_type=std::invoke_result_t<Operation,T&>;
    static_assert(!std::is_reference<result_type>::value,
      "A model lease cannot return a reference to a model that is unloaded upon release");
    std::shared_ptr<T> resource;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock,[&]{return !loading_&&!unloading_;});
      if(model_) {resource=model_;++users_;}
      else {loading_=true;}
    }
    if(!resource) {
      std::shared_ptr<T> created;
      try{created=std::make_shared<T>(std::invoke(std::forward<Loader>(load)));}
      catch(...) {
        {std::lock_guard<std::mutex> lock(mutex_);loading_=false;}
        cv_.notify_all();throw;
      }
      {
        std::lock_guard<std::mutex> lock(mutex_);
        model_=std::move(created);resource=model_;++users_;loading_=false;
      }
      cv_.notify_all();
    }
    try {
      if constexpr(std::is_void<result_type>::value) {
        std::invoke(std::forward<Operation>(operation),*resource);
        release(unload,false);
      }else {
        result_type value=std::invoke(std::forward<Operation>(operation),*resource);
        release(unload,false);
        return value;
      }
    }catch(...) {
      // If the operation itself failed, don't hide that exception with a
      // secondary cleanup failure; preserve that failure for inspection.
      // release is called once; cleanup failures from release(false) are
      // propagated as-is and must not cause a second release.
      const auto original=std::current_exception();
      bool already_released;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        already_released = (users_==0);
      }
      if(!already_released)release(unload,true);
      std::rethrow_exception(original);
    }
  }
};
} // namespace gut
