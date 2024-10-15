#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <condition_variable>
#include <thread>
namespace limb {
namespace tp {

/**
 * @brief The Worker class owns task queue and executing thread.
 * In thread it tries to pop task from queue. If queue is empty then it tries
 * to steal task from the sibling worker. If steal was unsuccessful then spins
 * with one millisecond delay.
 */
template <typename Task, template <typename> class Queue> class Worker {
public:
  /**
   * @brief Worker Constructor.
   * @param queue_size Length of undelaying task queue.
   */
  explicit Worker(std::shared_ptr<Queue<Task>> qtptr);

  /**
   * @brief Move ctor implementation.
   */
  Worker(Worker &&rhs) noexcept;

  /**
   * @brief Move assignment implementaion.
   */
  Worker &operator=(Worker &&rhs) noexcept;

  /**
   * @brief start Create the executing thread and start tasks execution.
   * @param id Worker ID.
   * @param cv Varible that indicated that a new element is pushed on a queue.
   */
  void start(size_t id, std::condition_variable &cv);

  /**
   * @brief instructs worker that he should terminane on next iteration.
   */
  void stop();

  /**
   * @brief Waits until the executing thread became finished.
   */
  void join();

  /**
   * @brief getWorkerIdForCurrentThread Return worker ID associated with
   * current thread if exists.
   * @return Worker ID.
   */
  static size_t getWorkerIdForCurrentThread();

private:
  /**
   * @brief threadFunc Executing thread function.
   * @param id Worker ID to be associated with this thread.
   * @param cv Varible that indicated that a new element is pushed on a queue.
   */
  void threadFunc(size_t id, std::condition_variable &cv);

  std::shared_ptr<Queue<Task>> m_queue;
  std::mutex available_handler;

  std::atomic<bool> m_running_flag;
  std::thread m_thread;
};

/// Implementation

namespace detail {
inline size_t *thread_id() {
  static thread_local size_t tss_id = -1u;
  return &tss_id;
}
} // namespace detail

template <typename Task, template <typename> class Queue>
inline Worker<Task, Queue>::Worker(std::shared_ptr<Queue<Task>> qtptr) : m_queue(qtptr), m_running_flag(true) {}

template <typename Task, template <typename> class Queue> inline Worker<Task, Queue>::Worker(Worker &&rhs) noexcept {
  *this = rhs;
}

template <typename Task, template <typename> class Queue>
inline Worker<Task, Queue> &Worker<Task, Queue>::operator=(Worker &&rhs) noexcept {
  if (this != &rhs) {
    m_queue = std::move(rhs.m_queue);
    m_running_flag = rhs.m_running_flag.load();
    m_thread = std::move(rhs.m_thread);
  }
  return *this;
}

template <typename Task, template <typename> class Queue> inline void Worker<Task, Queue>::stop() {
  m_running_flag.store(false, std::memory_order_relaxed);
}

template <typename Task, template <typename> class Queue> inline void Worker<Task, Queue>::join() { m_thread.join(); }

template <typename Task, template <typename> class Queue>
inline void Worker<Task, Queue>::start(size_t id, std::condition_variable &cv) {
  m_thread = std::thread(&Worker<Task, Queue>::threadFunc, this, id, std::ref(cv));
}

template <typename Task, template <typename> class Queue>
inline size_t Worker<Task, Queue>::getWorkerIdForCurrentThread() {
  return *detail::thread_id();
}

template <typename Task, template <typename> class Queue>
inline void Worker<Task, Queue>::threadFunc(size_t id, std::condition_variable &cv) {
  *detail::thread_id() = id;

  Task handler;

  while (m_running_flag.load(std::memory_order_relaxed)) {
    std::unique_lock lk(available_handler);
    cv.wait(lk, [this, &handler] {
      return this->m_queue->pop(handler) || !m_running_flag.load(std::memory_order_relaxed);
    });
    try {
      handler();
    } catch (...) {
      // suppress all exceptions
    }
  }
}

} // namespace tp
} // namespace limb