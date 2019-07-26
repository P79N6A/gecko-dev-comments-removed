









#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/desktop_capture/differ.h"
#include "webrtc/modules/desktop_capture/differ_block.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {


const int kScreenWidth= 96;
const int kScreenHeight = 96;



const int kPartialScreenWidth = 70;
const int kPartialScreenHeight = 70;

class DifferTest : public testing::Test {
 public:
  DifferTest() {
  }

 protected:
  void InitDiffer(int width, int height) {
    width_ = width;
    height_ = height;
    bytes_per_pixel_ = kBytesPerPixel;
    stride_ = (kBytesPerPixel * width);
    buffer_size_ = width_ * height_ * bytes_per_pixel_;

    differ_.reset(new Differ(width_, height_, bytes_per_pixel_, stride_));

    prev_.reset(new uint8_t[buffer_size_]);
    memset(prev_.get(), 0, buffer_size_);

    curr_.reset(new uint8_t[buffer_size_]);
    memset(curr_.get(), 0, buffer_size_);
  }

  void ClearBuffer(uint8_t* buffer) {
    memset(buffer, 0, buffer_size_);
  }

  
  void MarkDirtyBlocks(const void* prev_buffer, const void* curr_buffer) {
    differ_->MarkDirtyBlocks(prev_buffer, curr_buffer);
  }

  void MergeBlocks(DesktopRegion* dirty) {
    differ_->MergeBlocks(dirty);
  }

  
  int RegionRectCount(const DesktopRegion& region) {
    int count = 0;
    for (DesktopRegion::Iterator iter(region);
         !iter.IsAtEnd(); iter.Advance()) {
      ++count;
    }
    return count;
  }

  
  
  DiffInfo DiffBlock(int block_x, int block_y) {
    
    int block_offset = ((block_y * stride_) + (block_x * bytes_per_pixel_))
                        * kBlockSize;
    return BlockDifference(prev_.get() + block_offset,
                           curr_.get() + block_offset,
                           stride_);
  }

  
  
  void WriteBlockPixel(uint8_t* buffer, int block_x, int block_y,
                       int pixel_x, int pixel_y, uint32_t value) {
    WritePixel(buffer, (block_x * kBlockSize) + pixel_x,
               (block_y * kBlockSize) + pixel_y, value);
  }

  
  
  
  
  
  
  
  
  void WritePixel(uint8_t* buffer, int x, int y, uint32_t value) {
    uint8_t* pixel = reinterpret_cast<uint8_t*>(&value);
    buffer += (y * stride_) + (x * bytes_per_pixel_);
    for (int b = bytes_per_pixel_ - 1; b >= 0; b--) {
      *buffer++ = pixel[b];
    }
  }

  
  
  

  
  void ClearDiffInfo() {
    memset(differ_->diff_info_.get(), 0, differ_->diff_info_size_);
  }

  
  DiffInfo GetDiffInfo(int x, int y) {
    DiffInfo* diff_info = differ_->diff_info_.get();
    return diff_info[(y * GetDiffInfoWidth()) + x];
  }

  
  int GetDiffInfoWidth() {
    return differ_->diff_info_width_;
  }

  
  int GetDiffInfoHeight() {
    return differ_->diff_info_height_;
  }

  
  int GetDiffInfoSize() {
    return differ_->diff_info_size_;
  }

  void SetDiffInfo(int x, int y, const DiffInfo& value) {
    DiffInfo* diff_info = differ_->diff_info_.get();
    diff_info[(y * GetDiffInfoWidth()) + x] = value;
  }

  
  void MarkBlocks(int x_origin, int y_origin, int width, int height) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        SetDiffInfo(x_origin + x, y_origin + y, 1);
      }
    }
  }

  
  
  
  bool CheckDirtyRegionContainsRect(const DesktopRegion& region,
                                    int x, int y,
                                    int width, int height) {
    DesktopRect r =
      DesktopRect::MakeXYWH(x * kBlockSize, y * kBlockSize,
                                    width * kBlockSize, height * kBlockSize);
    for (DesktopRegion::Iterator i(region); !i.IsAtEnd(); i.Advance()) {
      if (i.rect().equals(r))
        return true;
    }
    return false;
  }

  
  
  
  bool MarkBlocksAndCheckMerge(int x_origin, int y_origin,
                               int width, int height) {
    ClearDiffInfo();
    MarkBlocks(x_origin, y_origin, width, height);

    DesktopRegion dirty;
    MergeBlocks(&dirty);


    DesktopRect expected_rect = DesktopRect::MakeXYWH(
        x_origin * kBlockSize, y_origin * kBlockSize,
        width * kBlockSize, height * kBlockSize);

    
    
    DesktopRegion::Iterator it(dirty);
    return !it.IsAtEnd() && expected_rect.equals(it.rect()) &&
        (it.Advance(), it.IsAtEnd());
  }

  
  scoped_ptr<Differ> differ_;

  
  int width_;
  int height_;
  int bytes_per_pixel_;
  int stride_;

  
  int buffer_size_;

  
  scoped_array<uint8_t> prev_;
  scoped_array<uint8_t> curr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DifferTest);
};

TEST_F(DifferTest, Setup) {
  InitDiffer(kScreenWidth, kScreenHeight);
  
  
  
  
  
  
  
  
  
  
  EXPECT_EQ(4, GetDiffInfoWidth());
  EXPECT_EQ(4, GetDiffInfoHeight());
  EXPECT_EQ(16, GetDiffInfoSize());
}

TEST_F(DifferTest, MarkDirtyBlocks_All) {
  InitDiffer(kScreenWidth, kScreenHeight);
  ClearDiffInfo();

  
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
      WriteBlockPixel(curr_.get(), x, y, 10, 10, 0xff00ff);
    }
  }

  MarkDirtyBlocks(prev_.get(), curr_.get());

  
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
      EXPECT_EQ(1, GetDiffInfo(x, y))
          << "when x = " << x << ", and y = " << y;
    }
  }
}

TEST_F(DifferTest, MarkDirtyBlocks_Sampling) {
  InitDiffer(kScreenWidth, kScreenHeight);
  ClearDiffInfo();

  
  WriteBlockPixel(curr_.get(), 1, 0, 10, 10, 0xff00ff);
  WriteBlockPixel(curr_.get(), 2, 1, 10, 10, 0xff00ff);
  WriteBlockPixel(curr_.get(), 0, 2, 10, 10, 0xff00ff);

  MarkDirtyBlocks(prev_.get(), curr_.get());

  
  EXPECT_EQ(0, GetDiffInfo(0, 0));
  EXPECT_EQ(0, GetDiffInfo(0, 1));
  EXPECT_EQ(1, GetDiffInfo(0, 2));
  EXPECT_EQ(1, GetDiffInfo(1, 0));
  EXPECT_EQ(0, GetDiffInfo(1, 1));
  EXPECT_EQ(0, GetDiffInfo(1, 2));
  EXPECT_EQ(0, GetDiffInfo(2, 0));
  EXPECT_EQ(1, GetDiffInfo(2, 1));
  EXPECT_EQ(0, GetDiffInfo(2, 2));
}

TEST_F(DifferTest, DiffBlock) {
  InitDiffer(kScreenWidth, kScreenHeight);

  
  EXPECT_EQ(0, DiffBlock(0, 0));
  EXPECT_EQ(0, DiffBlock(1, 1));

  
  
  int max = kBlockSize - 1;
  WriteBlockPixel(curr_.get(), 1, 1, 0, 0, 0xffffff);
  WriteBlockPixel(curr_.get(), 1, 1, 0, max, 0xffffff);
  WriteBlockPixel(curr_.get(), 1, 1, max, 0, 0xffffff);
  WriteBlockPixel(curr_.get(), 1, 1, max, max, 0xffffff);
  EXPECT_EQ(0, DiffBlock(0, 0));
  EXPECT_EQ(0, DiffBlock(0, 1));
  EXPECT_EQ(0, DiffBlock(0, 2));
  EXPECT_EQ(0, DiffBlock(1, 0));
  EXPECT_EQ(1, DiffBlock(1, 1));  
  EXPECT_EQ(0, DiffBlock(1, 2));
  EXPECT_EQ(0, DiffBlock(2, 0));
  EXPECT_EQ(0, DiffBlock(2, 1));
  EXPECT_EQ(0, DiffBlock(2, 2));
}

TEST_F(DifferTest, Partial_Setup) {
  InitDiffer(kPartialScreenWidth, kPartialScreenHeight);
  
  
  
  
  
  
  
  
  
  
  
  EXPECT_EQ(4, GetDiffInfoWidth());
  EXPECT_EQ(4, GetDiffInfoHeight());
  EXPECT_EQ(16, GetDiffInfoSize());
}

TEST_F(DifferTest, Partial_FirstPixel) {
  InitDiffer(kPartialScreenWidth, kPartialScreenHeight);
  ClearDiffInfo();

  
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
      WriteBlockPixel(curr_.get(), x, y, 0, 0, 0xff00ff);
    }
  }

  MarkDirtyBlocks(prev_.get(), curr_.get());

  
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
      EXPECT_EQ(1, GetDiffInfo(x, y))
          << "when x = " << x << ", and y = " << y;
    }
  }
}

TEST_F(DifferTest, Partial_BorderPixel) {
  InitDiffer(kPartialScreenWidth, kPartialScreenHeight);
  ClearDiffInfo();

  
  for (int y = 0; y < height_; y++) {
    WritePixel(curr_.get(), width_ - 1, y, 0xff00ff);
  }
  for (int x = 0; x < width_; x++) {
    WritePixel(curr_.get(), x, height_ - 1, 0xff00ff);
  }

  MarkDirtyBlocks(prev_.get(), curr_.get());

  
  int x_last = GetDiffInfoWidth() - 2;
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    EXPECT_EQ(1, GetDiffInfo(x_last, y))
        << "when x = " << x_last << ", and y = " << y;
  }
  int y_last = GetDiffInfoHeight() - 2;
  for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
    EXPECT_EQ(1, GetDiffInfo(x, y_last))
        << "when x = " << x << ", and y = " << y_last;
  }
  
  for (int y = 0; y < GetDiffInfoHeight() - 2; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 2; x++) {
      EXPECT_EQ(0, GetDiffInfo(x, y)) << "when x = " << x << ", and y = " << y;
    }
  }
}

TEST_F(DifferTest, MergeBlocks_Empty) {
  InitDiffer(kScreenWidth, kScreenHeight);

  
  
  
  
  
  
  
  
  
  
  ClearDiffInfo();

  DesktopRegion dirty;
  MergeBlocks(&dirty);

  EXPECT_TRUE(dirty.is_empty());
}

TEST_F(DifferTest, MergeBlocks_SingleBlock) {
  InitDiffer(kScreenWidth, kScreenHeight);
  
  
  for (int y = 0; y < GetDiffInfoHeight() - 1; y++) {
    for (int x = 0; x < GetDiffInfoWidth() - 1; x++) {
      ASSERT_TRUE(MarkBlocksAndCheckMerge(x, y, 1, 1)) << "x: " << x
                                                       << "y: " << y;
    }
  }
}

TEST_F(DifferTest, MergeBlocks_BlockRow) {
  InitDiffer(kScreenWidth, kScreenHeight);

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 0, 2, 1));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 1, 3, 1));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(1, 2, 2, 1));
}

TEST_F(DifferTest, MergeBlocks_BlockColumn) {
  InitDiffer(kScreenWidth, kScreenHeight);

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 0, 1, 2));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(1, 1, 1, 2));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(2, 0, 1, 3));
}

TEST_F(DifferTest, MergeBlocks_BlockRect) {
  InitDiffer(kScreenWidth, kScreenHeight);

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 0, 2, 2));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(1, 1, 2, 2));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(1, 0, 2, 3));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 1, 3, 2));

  
  
  
  
  
  
  
  
  
  ASSERT_TRUE(MarkBlocksAndCheckMerge(0, 0, 3, 3));
}




TEST_F(DifferTest, MergeBlocks_MultiRect) {
  InitDiffer(kScreenWidth, kScreenHeight);
  DesktopRegion dirty;

  
  
  
  
  
  
  
  
  
  ClearDiffInfo();
  MarkBlocks(1, 0, 1, 1);
  MarkBlocks(0, 1, 1, 1);
  MarkBlocks(2, 2, 1, 1);

  dirty.Clear();
  MergeBlocks(&dirty);

  ASSERT_EQ(3, RegionRectCount(dirty));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 1, 0, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 1, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 2, 2, 1, 1));

  
  
  
  
  
  
  
  
  
  ClearDiffInfo();
  MarkBlocks(2, 0, 1, 1);
  MarkBlocks(0, 1, 3, 2);

  dirty.Clear();
  MergeBlocks(&dirty);

  ASSERT_EQ(2, RegionRectCount(dirty));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 2, 0, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 1, 3, 2));

  
  
  
  
  
  
  
  
  
  ClearDiffInfo();
  MarkBlocks(0, 1, 1, 1);
  MarkBlocks(2, 1, 1, 1);
  MarkBlocks(0, 2, 3, 1);

  dirty.Clear();
  MergeBlocks(&dirty);

  ASSERT_EQ(3, RegionRectCount(dirty));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 1, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 2, 1, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 2, 3, 1));

  
  
  
  
  
  
  
  
  
  ClearDiffInfo();
  MarkBlocks(0, 0, 3, 1);
  MarkBlocks(0, 1, 1, 1);
  MarkBlocks(2, 1, 1, 1);
  MarkBlocks(0, 2, 3, 1);

  dirty.Clear();
  MergeBlocks(&dirty);

  ASSERT_EQ(4, RegionRectCount(dirty));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 0, 3, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 1, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 2, 1, 1, 1));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 2, 3, 1));

  
  
  
  
  
  
  
  
  
  ClearDiffInfo();
  MarkBlocks(0, 0, 2, 2);
  MarkBlocks(1, 2, 1, 1);

  dirty.Clear();
  MergeBlocks(&dirty);

  ASSERT_EQ(2, RegionRectCount(dirty));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 0, 0, 2, 2));
  ASSERT_TRUE(CheckDirtyRegionContainsRect(dirty, 1, 2, 1, 1));
}

}  
