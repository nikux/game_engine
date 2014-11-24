#include <cassert>
#include "thr_queue/coroutine.h"
#include "global_thr_pool_impl.h"

namespace game_engine {
namespace thr_queue {

coroutine::coroutine(coroutine &&other) : coroutine() {
  swap(*this, other);
}

coroutine &coroutine::operator=(coroutine &&rhs) {
  using std::swap;
  coroutine temp; // make sure we don't allocate a stack.

  swap(temp, rhs);
  swap(temp, *this);

  return *this;
}

coroutine::~coroutine() {
  if (typ == coroutine_type::master) {
    assert(!stack &&
           "master coroutines don't have allocated stacks since the OS "
           "takes care of that");
  } else {
    assert(stack &&
           "if this is not a master_coroutine then it should have a stack");
  }
}

coroutine_type coroutine::type() const { return typ; }

void coroutine::switch_to_from(coroutine &other) {
  auto func_ptr = reinterpret_cast<intptr_t>(function.get());
  boost::context::jump_fcontext(&other.ctx, ctx, func_ptr);
}

coroutine::coroutine() :
  typ(coroutine_type::master)
{}

size_t coroutine::default_stacksize() {
  return 4096 // 4 KB
       * 16;  // 64 KB in total.
}

void coroutine::finish_coroutine() { global_thr_pool.yield(); }

void swap(coroutine &lhs, coroutine &rhs) {
  using std::swap;
  swap(lhs.ctx, rhs.ctx);
  swap(lhs.stack, rhs.stack);
  swap(lhs.function, rhs.function);
  swap(lhs.typ, rhs.typ);
}

void set_cor_type(coroutine_type cor_typ) {
	global_thr_pool.yield([ cor_typ ] {
	  running_coroutine_or_yielded_from->typ = cor_typ;
	  global_thr_pool.schedule(std::move(*running_coroutine_or_yielded_from), true);
	});
}
}
}
