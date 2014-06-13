#include "../global_thr_pool_impl.h"
#include "thr_queue/coroutine.h"
#include "thr_queue/event/cond_var.h"

namespace game_engine {
namespace thr_queue {

static std::mutex cond_var_global_mt;
static std::deque<coroutine> cor_queue;

namespace event {
void condition_variable::wait() {
  global_thr_pool.yield([&] {
    // even though this code will be run by another coroutine, it'll still
    // be run by the same thread as this coroutine, so no problems should
    // arise when unlocking the mutex.
    // also, running_coroutine_or_yielded from still points to this coroutine.
    std::lock_guard<std::mutex> cond_var_lock(mt);
    waiting_cors.emplace_back(std::move(*running_coroutine_or_yielded_from));
  });
}

void condition_variable::notify() {
  std::unique_lock<std::mutex> mt_lock(mt);
  auto wc = std::move(waiting_cors);
  mt_lock.unlock();

  for (auto &cor : wc) {
    global_thr_pool.schedule(std::move(cor), true);
  }
}
}
}
}
