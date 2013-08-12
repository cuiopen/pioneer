/*! \file
 * \brief Thread pool core.
 *
 * This file contains the threadpool's core class: pool<Task, SchedulingPolicy>.
 *
 * Thread pools are a mechanism for asynchronous and parallel processing
 * within the same process. The pool class provides a convenient way
 * for dispatching asynchronous tasks as functions objects. The scheduling
 * of these tasks can be easily controlled by using customized schedulers.
 *
 * Copyright (c) 2005-2007 Philipp Henkel
 *
 * Use, modification, and distribution are  subject to the
 * boostplus Software License, Version 1.0. (See accompanying  file
 * LICENSE_1_0.txt or copy at http://www.boostplus.org/LICENSE_1_0.txt)
 *
 * http://threadpool.sourceforge.net
 *
 */

#ifndef THREADPOOL_POOL_HPP_INCLUDED
#define THREADPOOL_POOL_HPP_INCLUDED

#include <memory>
#include <functional>

#include "./task_adaptors.hpp"
#include "./scheduling_policies.hpp"
#include "./size_policies.hpp"
#include "./shutdown_policies.hpp"
#include "./detail/pool_core.hpp"

namespace boostplus {
  namespace threadpool {

    /*! \brief Thread pool.
     *
     * Thread pools are a mechanism for asynchronous and parallel processing
     * within the same process. The pool class provides a convenient way
     * for dispatching asynchronous tasks as functions objects. The scheduling
     * of these tasks can be easily controlled by using customized schedulers.
     * A task must not throw an exception.
     *
     * A pool is DefaultConstructible, CopyConstructible and Assignable.
     * It has reference semantics; all copies of the same pool are equivalent and interchangeable.
     * All operations on a pool except assignment are strongly thread safe or sequentially consistent;
     * that is, the behavior of concurrent calls is as if the calls have been issued sequentially in an unspecified order.
     *
     * \param Task A function object which implements the operator 'void operator() (void) const'. The operator () is called by the pool to execute the task. Exceptions are ignored.
     * \param SchedulingPolicy A task container which determines how tasks are scheduled. It is guaranteed that this container is accessed only by one thread at a time. The scheduler shall not throw exceptions.
     *
     * \remarks The pool class is thread-safe.
     *
     * \see Tasks: task_func, prio_task_func
     * \see Scheduling policies: fifo_scheduler, lifo_scheduler, prio_scheduler
     */
    template
    <
        typename Task = task_func,
        template<typename > class SchedulingPolicy = fifo_scheduler,
        template<typename > class SizePolicy = static_size,
        template<typename > class SizePolicyController = resize_controller,
        template<typename > class ShutdownPolicy = wait_for_all_tasks
    >
    class thread_pool {
    public:

      typedef detail::pool_core<Task, SchedulingPolicy, SizePolicy, SizePolicyController, ShutdownPolicy> pool_core_type;

    public:

      typedef Task task_type;
      typedef SchedulingPolicy<task_type> scheduler_type;
      typedef SizePolicy<pool_core_type> size_policy_type;
      typedef SizePolicyController<pool_core_type> size_controller_type;

    public:

      /*! Constructor.
       * \param initial_threads The pool is immediately resized to set the specified number of threads. The pool's actual number threads depends on the SizePolicy.
       */
      thread_pool(size_t initial_threads = 1) :
          _core(new pool_core_type),
          _shutdown_controller(static_cast<void*>(0), std::bind(&pool_core_type::shutdown, _core))
      {
        size_policy_type::init(*_core, initial_threads);
      }

      /*! Gets the size controller which manages the number of threads in the pool.
       * \return The size controller.
       * \see SizePolicy
       */
      size_controller_type size_controller() {
        return _core->size_controller();
      }

      /*! Gets the number of threads in the pool.
       * \return The number of threads.
       */
      size_t size() const {
        return _core->size();
      }

      /*! Schedules a task for asynchronous execution. The task will be executed once only.
       * \param task The task function object. It should not throw execeptions.
       * \return true, if the task could be scheduled and false otherwise.
       */
      bool schedule(const task_type& task) {
        return _core->schedule(task);
      }

      bool schedule(task_type&& task) {
        return _core->schedule(task);
      }

      /*! Returns the number of tasks which are currently executed.
       * \return The number of active tasks.
       */
      size_t active() const {
        return _core->active();
      }

      /*! Returns the number of tasks which are ready for execution.
       * \return The number of pending tasks.
       */
      size_t pending_tasks() const {
        return _core->pending_tasks();
      }

      /*! Removes all pending tasks from the pool's scheduler.
       */
      void clear() {
        _core->clear();
      }

      /*! Indicates that there are no tasks pending.
       * \return true if there are no tasks ready for execution.
       * \remarks This function is more efficient that the check 'pending() == 0'.
       */
      bool empty() const {
        return _core->empty();
      }

      /*! The current thread of execution is blocked until the sum of all active
       *  and pending tasks is equal or less than a given threshold.
       * \param task_threshold The maximum number of tasks in pool and scheduler.
       */
      void wait(size_t task_threshold = 0) const {
        _core->wait(task_threshold);
      }

      /*! The current thread of execution is blocked until the timestamp is met
       * or the sum of all active and pending tasks is equal or less
       * than a given threshold.
       * \param timestamp The time when function returns at the latest.
       * \param task_threshold The maximum number of tasks in pool and scheduler.
       * \return true if the task sum is equal or less than the threshold, false otherwise.
       */
      template<typename Duration>
      bool wait(const Duration& timestamp, size_t task_threshold = 0) const {
        return _core->wait(timestamp, task_threshold);
      }

    private:

      std::shared_ptr<pool_core_type> _core; // pimpl idiom
      std::shared_ptr<void> _shutdown_controller; // If the last pool holding a pointer to the core is deleted the controller shuts the pool down.
    };

    /*! \brief Fifo pool.
     *
     * The pool's tasks are fifo scheduled task_func functors.
     *
     */
    typedef thread_pool<task_func, fifo_scheduler, static_size, resize_controller,
        wait_for_all_tasks> fifo_pool;

    /*! \brief Lifo pool.
     *
     * The pool's tasks are lifo scheduled task_func functors.
     *
     */
    typedef thread_pool<task_func, lifo_scheduler, static_size, resize_controller,
        wait_for_all_tasks> lifo_pool;

    /*! \brief Pool for prioritized task.
     *
     * The pool's tasks are prioritized prio_task_func functors.
     *
     */
    typedef thread_pool<prio_task_func, prio_scheduler, static_size, resize_controller,
        wait_for_all_tasks> prio_pool;

  }
}

#endif // THREADPOOL_POOL_HPP_INCLUDED
