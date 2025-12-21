#include <gtest/gtest.h>

#include "abnet/abnet.hpp"

TEST(System, GetHostname) {
  abnet::error_code ec;

  char hostname[128]; // Buffer for hostname
  int ret = abnet::socket_ops::gethostname(hostname, sizeof(hostname), ec);

  // gethostname should return 0 on success
  ASSERT_EQ(ret, 0) << "gethostname failed";

  // Hostname should not be empty
  ASSERT_GT(strlen(hostname), 0u) << "hostname is empty";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}