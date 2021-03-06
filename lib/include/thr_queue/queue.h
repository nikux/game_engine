#pragma once

namespace game_engine {
namespace thr_queue {
class coroutine;
class queue;
}
}

#include "thr_queue/event/future.h"
#include "thr_queue/functor.h"

#include "thr_queue/thread_api.h"
#include <deque>
#include <functional>
#include <vector>

namespace game_engine {
namespace thr_queue {
enum class queue_type
{
  serial,
  parallel
};

/** \brief Tag struct for the work-stealing constructor of queue. */
struct steal_work_t
{
  constexpr steal_work_t( ){};
};
constexpr steal_work_t steal_work;

/** \brief A queue for organizing work.
 * There are two types of queues, parallel and serial.
 * Each works differently:
 * * A parallel queue is used for storing units of work with no chronological
 *   relation between themselves, and therefore can be run in parallel.
 *   It can be used for submiting batches of number crunching work, for
 *   example.
 * * A serial queue executes elements in a way such that the chronological
 *   relation between them is respected. That means that they are generally
 *   executed one after the other in one thread.
 */
class queue
{
private:
  /** A subclass of the functor template that also moves the result of the
   * stored function to the promise type when it's ran.
   */
  template < typename F >
  struct work : functor
  {
    using result_type = typename std::result_of< F( ) >::type;
    event::promise< result_type > prom;
    F func;
    work( F f, event::promise< result_type > p );

    void operator( )( ) final override;
  };

public:
  using callback_t = std::function< void( queue& ) >;

public:
  /** \brief Constructs a queue of a certain type. */
  queue( queue_type ty );

  /** \brief Constructs a queue of a certain type with a callback that will be
   * called when work is added.
   */
  queue( queue_type ty, callback_t cb );

  /** \brief Constructs a queue by stealing the work from another one.
   * It copies the type and moves the work queue, but everything else
   * is initialized to the default values.
   */
  queue( steal_work_t, queue& other );

  queue& operator=( queue&& rhs );
  queue( queue&& rhs );

  ~queue( ){};

  /** \brief Adds a function to be executed to the queue. */
  template < typename F >
  event::future< typename queue::work< F >::result_type > submit_work( F func );

  /** \brief Appends a queue to this one.
   * Depending on what type of queue is appended and what type this queue is
   * it's execution will be different.
   * A parallel queue appended to another parallel queue: the queues' work
   * will be merged.
   * A parallel queue appended to a serial queue: the queue will be added as
   * a whole work unit to this one. However, the appended queue's work will
   * be executed in parallel and the serial queue will wait for it to finish
   * completely.
   * A serial queue appended to a parallel queue: the serial queue will be
   * considered a whole unit of work to be executed in parallel with the
   * queue's work.
   * A serial queue appended to a serial queue: the queues will be merged.
   * */
  void append_queue( queue q );

  /** \brief Run one unit of work from this queue.
   * Returns true if we did work.
   * */
  bool run_once( );

  /** \brief Run everything from this queue in the caller's thread.
   * It's important to note that parallel queues will be run serially.
   * If that's not what you want, you run it there using the run_in_pool method.
   */
  void run_until_empty( );

  queue_type type( ) const;

private:
  /** \brief A utility function that is only an implementation detail.
   * It has to be added here as a friend.
   */
  friend std::vector< coroutine > queue_to_vec_cor( queue q, event::condition_variable* cv, event::mutex* mt,
                                                    size_t min );

  friend void swap( queue& lhs, queue& rhs );
  boost::recursive_mutex queue_mut;
  std::deque< std::unique_ptr< functor > > work_queue;
  queue_type typ;
  callback_t cb_added;
};

void swap( queue& lhs, queue& rhs );
}
}

#include "thr_queue/queue.inl"
