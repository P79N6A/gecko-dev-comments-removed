






























#include "breakpad_googletest_includes.h"
#include "common/linux/safe_readlink.h"

using google_breakpad::SafeReadLink;

TEST(SafeReadLinkTest, ZeroBufferSize) {
  char buffer[1];
  EXPECT_FALSE(SafeReadLink("/proc/self/exe", buffer, 0));
}

TEST(SafeReadLinkTest, BufferSizeTooSmall) {
  char buffer[1];
  EXPECT_FALSE(SafeReadLink("/proc/self/exe", buffer, 1));
}

TEST(SafeReadLinkTest, BoundaryBufferSize) {
  char buffer[PATH_MAX];
  EXPECT_TRUE(SafeReadLink("/proc/self/exe", buffer, sizeof(buffer)));
  size_t path_length = strlen(buffer);
  EXPECT_LT(0U, path_length);
  EXPECT_GT(sizeof(buffer), path_length);

  
  char buffer2[PATH_MAX];
  EXPECT_TRUE(SafeReadLink("/proc/self/exe", buffer2, path_length + 1));
  EXPECT_EQ(path_length, strlen(buffer2));
  EXPECT_EQ(0, strncmp(buffer, buffer2, PATH_MAX));

  
  EXPECT_FALSE(SafeReadLink("/proc/self/exe", buffer, path_length));
}

TEST(SafeReadLinkTest, NonexistentPath) {
  char buffer[PATH_MAX];
  EXPECT_FALSE(SafeReadLink("nonexistent_path", buffer, sizeof(buffer)));
}

TEST(SafeReadLinkTest, NonSymbolicLinkPath) {
  char actual_path[PATH_MAX];
  EXPECT_TRUE(SafeReadLink("/proc/self/exe", actual_path, sizeof(actual_path)));

  char buffer[PATH_MAX];
  EXPECT_FALSE(SafeReadLink(actual_path, buffer, sizeof(buffer)));
}

TEST(SafeReadLinkTest, DeduceBufferSizeFromCharArray) {
  char buffer[PATH_MAX];
  char* buffer_pointer = buffer;
  EXPECT_TRUE(SafeReadLink("/proc/self/exe", buffer_pointer, sizeof(buffer)));
  size_t path_length = strlen(buffer);

  
  
  char buffer2[PATH_MAX];
  EXPECT_TRUE(SafeReadLink("/proc/self/exe", buffer2));
  EXPECT_EQ(path_length, strlen(buffer2));
  EXPECT_EQ(0, strncmp(buffer, buffer2, PATH_MAX));
}
