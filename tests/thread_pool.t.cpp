#include <gtest/gtest.h>

#include <thread-pool/thread-pool.hpp>

#include <functional>
#include <future>
#include <memory>
#include <thread>

namespace TestLinkage {
size_t getWorkerIdForCurrentThread() { return *limb::tp::detail::thread_id(); }

size_t getWorkerIdForCurrentThread2() {
  return limb::tp::Worker<std::function<void()>, limb::tp::MPMCBoundedQueue>::getWorkerIdForCurrentThread();
}
} // namespace TestLinkage

TEST(ThreadPool, postJob) {
  limb::tp::ThreadPool pool;

  std::packaged_task<int()> t([]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 42;
  });

  std::future<int> r = t.get_future();

  pool.post(t);

  ASSERT_EQ(42, r.get());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}