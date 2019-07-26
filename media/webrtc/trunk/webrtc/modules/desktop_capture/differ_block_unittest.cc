









#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/desktop_capture/differ_block.h"
#include "webrtc/system_wrappers/interface/ref_count.h"

namespace webrtc {



static const int kTimesToRun = 900;

static void GenerateData(uint8_t* data, int size) {
  for (int i = 0; i < size; ++i) {
    data[i] = i;
  }
}


static const int kSizeOfBlock = kBlockSize * kBlockSize * kBytesPerPixel;
uint8_t block_buffer[kSizeOfBlock * 2 + 16];

void PrepareBuffers(uint8_t* &block1, uint8_t* &block2) {
  block1 = reinterpret_cast<uint8_t*>
      ((reinterpret_cast<uintptr_t>(&block_buffer[0]) + 15) & ~15);
  GenerateData(block1, kSizeOfBlock);
  block2 = block1 + kSizeOfBlock;
  memcpy(block2, block1, kSizeOfBlock);
}

TEST(BlockDifferenceTestSame, BlockDifference) {
  uint8_t* block1;
  uint8_t* block2;
  PrepareBuffers(block1, block2);

  
  for (int i = 0; i < kTimesToRun; ++i) {
    int result = BlockDifference(block1, block2, kBlockSize * kBytesPerPixel);
    EXPECT_EQ(0, result);
  }
}

TEST(BlockDifferenceTestLast, BlockDifference) {
  uint8_t* block1;
  uint8_t* block2;
  PrepareBuffers(block1, block2);
  block2[kSizeOfBlock-2] += 1;

  for (int i = 0; i < kTimesToRun; ++i) {
    int result = BlockDifference(block1, block2, kBlockSize * kBytesPerPixel);
    EXPECT_EQ(1, result);
  }
}

TEST(BlockDifferenceTestMid, BlockDifference) {
  uint8_t* block1;
  uint8_t* block2;
  PrepareBuffers(block1, block2);
  block2[kSizeOfBlock/2+1] += 1;

  for (int i = 0; i < kTimesToRun; ++i) {
    int result = BlockDifference(block1, block2, kBlockSize * kBytesPerPixel);
    EXPECT_EQ(1, result);
  }
}

TEST(BlockDifferenceTestFirst, BlockDifference) {
  uint8_t* block1;
  uint8_t* block2;
  PrepareBuffers(block1, block2);
  block2[0] += 1;

  for (int i = 0; i < kTimesToRun; ++i) {
    int result = BlockDifference(block1, block2, kBlockSize * kBytesPerPixel);
    EXPECT_EQ(1, result);
  }
}

}  
