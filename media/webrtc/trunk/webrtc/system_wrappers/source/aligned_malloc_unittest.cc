









#include "system_wrappers/interface/aligned_malloc.h"

#if _WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#include "typedefs.h"  

#include "gtest/gtest.h"


bool CorrectUsage(size_t size, size_t alignment) {
  webrtc::Allocator<char>::scoped_ptr_aligned scoped(
      webrtc::AlignedMalloc<char>(size, alignment));
  if (scoped.get() == NULL) {
    return false;
  }
  const uintptr_t scoped_address = reinterpret_cast<uintptr_t> (scoped.get());
  return 0u == scoped_address % alignment;
}

TEST(AlignedMalloc, GetRightAlign) {
  const size_t size = 100;
  const size_t alignment = 32;
  const size_t left_missalignment = 8;
  webrtc::Allocator<char>::scoped_ptr_aligned scoped(
      webrtc::AlignedMalloc<char>(size, alignment));
  EXPECT_TRUE(scoped.get() != NULL);
  const uintptr_t aligned_address = reinterpret_cast<uintptr_t> (scoped.get());
  const uintptr_t missaligned_address = aligned_address - left_missalignment;
  const char* missaligned_ptr = reinterpret_cast<const char*>(
      missaligned_address);
  const char* realigned_ptr = webrtc::GetRightAlign(
      missaligned_ptr, alignment);
  EXPECT_EQ(scoped.get(), realigned_ptr);
}

TEST(AlignedMalloc, IncorrectSize) {
  const size_t incorrect_size = 0;
  const size_t alignment = 64;
  EXPECT_FALSE(CorrectUsage(incorrect_size, alignment));
}

TEST(AlignedMalloc, IncorrectAlignment) {
  const size_t size = 100;
  const size_t incorrect_alignment = 63;
  EXPECT_FALSE(CorrectUsage(size, incorrect_alignment));
}

TEST(AlignedMalloc, AlignTo2Bytes) {
  size_t size = 100;
  size_t alignment = 2;
  EXPECT_TRUE(CorrectUsage(size, alignment));
}

TEST(AlignedMalloc, AlignTo32Bytes) {
  size_t size = 100;
  size_t alignment = 32;
  EXPECT_TRUE(CorrectUsage(size, alignment));
}

TEST(AlignedMalloc, AlignTo128Bytes) {
  size_t size = 100;
  size_t alignment = 128;
  EXPECT_TRUE(CorrectUsage(size, alignment));
}
