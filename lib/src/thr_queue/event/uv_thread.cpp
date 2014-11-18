#include <atomic>
#include <cassert>
#include <deque>
#include <memory>
#include "thr_queue/thread_api.h"
#include "uv_thread.h"

namespace game_engine {
namespace thr_queue {
namespace event {

static void run_loop();
struct uv_thread {
public:
  uv_thread()
      : should_stop(false)
      , global_async(std::unique_ptr<uv_async_t>(new uv_async_t))
      , thr(run_loop) {}

  ~uv_thread() {
    // we have to make sure that we call uv_async_send with an async handler that's
    // already initialized.
    should_stop = true;
    boost::unique_lock<boost::mutex> lock(init_start_requests_mt, boost::defer_lock);
    while (lock.lock(), !async_constructed) {
      lock.unlock();
    }
    assert (async_constructed);
    uv_async_send(global_async.get());
    lock.unlock();
    thr.join();
  }

  std::atomic<bool> should_stop;
  bool async_constructed = false;
  std::unique_ptr<uv_async_t> global_async = nullptr;
  boost::thread thr;
  std::deque<std::function<void()>> init_start_requests;
  boost::mutex init_start_requests_mt;
};

static uv_thread thr;

void uv_thr_init(uv_async_t *async, uv_async_cb f_ptr) {
  assert(thr.global_async);
  assert(!thr.should_stop);

  boost::mutex mt;
  boost::condition_variable cv;
  boost::lock(thr.init_start_requests_mt, mt);
  boost::unique_lock<boost::mutex> mt_lock(mt, boost::adopt_lock);

  {
    boost::lock_guard<boost::mutex> req_lock(thr.init_start_requests_mt,
                                         boost::adopt_lock);

    thr.init_start_requests.emplace_back([=, &mt, &cv] {
      uv_async_init(uv_default_loop(), async, f_ptr);
      // we make sure that the requesting thread has started waiting.
      boost::unique_lock<boost::mutex> mt_lock(mt);
      cv.notify_all();
    });
  }

  uv_async_send(thr.global_async.get());

  cv.wait(mt_lock);
}

void process_init_start_requests_or_stop(uv_async_t *) {
  boost::unique_lock<boost::mutex> lock(thr.init_start_requests_mt,
                                    boost::defer_lock);

  while (lock.lock(), thr.init_start_requests.size()) {
    auto q = std::move(thr.init_start_requests);
    lock.unlock();
    for (auto &f : q) {
      f();
    }
  }

  if (thr.should_stop) {
    uv_stop(uv_default_loop());
    return;
  }
}

void run_loop() {
  assert(thr.global_async);
  boost::unique_lock<boost::mutex> lock(thr.init_start_requests_mt);
  uv_async_init(uv_default_loop(),
                thr.global_async.get(),
                process_init_start_requests_or_stop);
  thr.async_constructed = true;
  lock.unlock();

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
}
}
}
