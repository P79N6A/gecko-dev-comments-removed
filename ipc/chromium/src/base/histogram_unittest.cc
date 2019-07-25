





#include "base/metrics/histogram.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

class HistogramTest : public testing::Test {
};


TEST(HistogramTest, StartupShutdownTest) {
  
  Histogram* histogram(Histogram::FactoryGet(
      "TestHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), histogram);
  Histogram* histogram1(Histogram::FactoryGet(
      "Test1Histogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), histogram1);
  EXPECT_NE(histogram, histogram1);


  Histogram* linear_histogram(LinearHistogram::FactoryGet(
      "TestLinearHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), linear_histogram);
  Histogram* linear_histogram1(LinearHistogram::FactoryGet(
      "Test1LinearHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), linear_histogram1);
  EXPECT_NE(linear_histogram, linear_histogram1);

  std::vector<int> custom_ranges;
  custom_ranges.push_back(1);
  custom_ranges.push_back(5);
  custom_ranges.push_back(10);
  custom_ranges.push_back(20);
  custom_ranges.push_back(30);
  Histogram* custom_histogram(CustomHistogram::FactoryGet(
      "TestCustomHistogram", custom_ranges, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), custom_histogram);
  Histogram* custom_histogram1(CustomHistogram::FactoryGet(
      "Test1CustomHistogram", custom_ranges, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), custom_histogram1);

  
  HISTOGRAM_TIMES("Test2Histogram", TimeDelta::FromDays(1));
  HISTOGRAM_COUNTS("Test3Histogram", 30);

  DHISTOGRAM_TIMES("Test4Histogram", TimeDelta::FromDays(1));
  DHISTOGRAM_COUNTS("Test5Histogram", 30);

  HISTOGRAM_ENUMERATION("Test6Histogram", 129, 130);

  
  Histogram::SampleSet sample1;
  Histogram::SampleSet sample2;

  
  sample1 = sample2;
  Histogram::SampleSet sample3(sample1);

  
  StatisticsRecorder recorder;
}


TEST(HistogramTest, RecordedStartupTest) {
  
  StatisticsRecorder recorder;  

  StatisticsRecorder::Histograms histograms;
  EXPECT_EQ(0U, histograms.size());
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(0U, histograms.size());

  
  Histogram* histogram(Histogram::FactoryGet(
      "TestHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), histogram);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(1U, histograms.size());
  Histogram* histogram1(Histogram::FactoryGet(
      "Test1Histogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), histogram1);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(2U, histograms.size());

  Histogram* linear_histogram(LinearHistogram::FactoryGet(
      "TestLinearHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), linear_histogram);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(3U, histograms.size());

  Histogram* linear_histogram1(LinearHistogram::FactoryGet(
      "Test1LinearHistogram", 1, 1000, 10, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), linear_histogram1);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(4U, histograms.size());

  std::vector<int> custom_ranges;
  custom_ranges.push_back(1);
  custom_ranges.push_back(5);
  custom_ranges.push_back(10);
  custom_ranges.push_back(20);
  custom_ranges.push_back(30);
  Histogram* custom_histogram(CustomHistogram::FactoryGet(
      "TestCustomHistogram", custom_ranges, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), custom_histogram);
  Histogram* custom_histogram1(CustomHistogram::FactoryGet(
      "TestCustomHistogram", custom_ranges, Histogram::kNoFlags));
  EXPECT_NE(reinterpret_cast<Histogram*>(NULL), custom_histogram1);

  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(5U, histograms.size());

  
  HISTOGRAM_TIMES("Test2Histogram", TimeDelta::FromDays(1));
  HISTOGRAM_COUNTS("Test3Histogram", 30);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(7U, histograms.size());

  HISTOGRAM_ENUMERATION("TestEnumerationHistogram", 20, 200);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
  EXPECT_EQ(8U, histograms.size());

  DHISTOGRAM_TIMES("Test4Histogram", TimeDelta::FromDays(1));
  DHISTOGRAM_COUNTS("Test5Histogram", 30);
  histograms.clear();
  StatisticsRecorder::GetHistograms(&histograms);  
#ifndef NDEBUG
  EXPECT_EQ(10U, histograms.size());
#else
  EXPECT_EQ(8U, histograms.size());
#endif
}

TEST(HistogramTest, RangeTest) {
  StatisticsRecorder recorder;
  StatisticsRecorder::Histograms histograms;

  recorder.GetHistograms(&histograms);
  EXPECT_EQ(0U, histograms.size());

  Histogram* histogram(Histogram::FactoryGet(
      "Histogram", 1, 64, 8, Histogram::kNoFlags));  
  
  EXPECT_EQ(0, histogram->ranges(0));
  int power_of_2 = 1;
  for (int i = 1; i < 8; i++) {
    EXPECT_EQ(power_of_2, histogram->ranges(i));
    power_of_2 *= 2;
  }
  EXPECT_EQ(INT_MAX, histogram->ranges(8));

  Histogram* short_histogram(Histogram::FactoryGet(
      "Histogram Shortened", 1, 7, 8, Histogram::kNoFlags));
  
  
  for (int i = 0; i < 8; i++)
    EXPECT_EQ(i, short_histogram->ranges(i));
  EXPECT_EQ(INT_MAX, short_histogram->ranges(8));

  Histogram* linear_histogram(LinearHistogram::FactoryGet(
      "Linear", 1, 7, 8, Histogram::kNoFlags));
  
  for (int i = 0; i < 8; i++)
    EXPECT_EQ(i, linear_histogram->ranges(i));
  EXPECT_EQ(INT_MAX, linear_histogram->ranges(8));

  Histogram* linear_broad_histogram(LinearHistogram::FactoryGet(
      "Linear widened", 2, 14, 8, Histogram::kNoFlags));
  
  for (int i = 0; i < 8; i++)
    EXPECT_EQ(2 * i, linear_broad_histogram->ranges(i));
  EXPECT_EQ(INT_MAX, linear_broad_histogram->ranges(8));

  Histogram* transitioning_histogram(Histogram::FactoryGet(
      "LinearAndExponential", 1, 32, 15, Histogram::kNoFlags));
  
  EXPECT_EQ(0, transitioning_histogram->ranges(0));
  EXPECT_EQ(1, transitioning_histogram->ranges(1));
  EXPECT_EQ(2, transitioning_histogram->ranges(2));
  EXPECT_EQ(3, transitioning_histogram->ranges(3));
  EXPECT_EQ(4, transitioning_histogram->ranges(4));
  EXPECT_EQ(5, transitioning_histogram->ranges(5));
  EXPECT_EQ(6, transitioning_histogram->ranges(6));
  EXPECT_EQ(7, transitioning_histogram->ranges(7));
  EXPECT_EQ(9, transitioning_histogram->ranges(8));
  EXPECT_EQ(11, transitioning_histogram->ranges(9));
  EXPECT_EQ(14, transitioning_histogram->ranges(10));
  EXPECT_EQ(17, transitioning_histogram->ranges(11));
  EXPECT_EQ(21, transitioning_histogram->ranges(12));
  EXPECT_EQ(26, transitioning_histogram->ranges(13));
  EXPECT_EQ(32, transitioning_histogram->ranges(14));
  EXPECT_EQ(INT_MAX, transitioning_histogram->ranges(15));

  std::vector<int> custom_ranges;
  custom_ranges.push_back(0);
  custom_ranges.push_back(9);
  custom_ranges.push_back(10);
  custom_ranges.push_back(11);
  custom_ranges.push_back(300);
  Histogram* test_custom_histogram(CustomHistogram::FactoryGet(
      "TestCustomRangeHistogram", custom_ranges, Histogram::kNoFlags));

  EXPECT_EQ(custom_ranges[0], test_custom_histogram->ranges(0));
  EXPECT_EQ(custom_ranges[1], test_custom_histogram->ranges(1));
  EXPECT_EQ(custom_ranges[2], test_custom_histogram->ranges(2));
  EXPECT_EQ(custom_ranges[3], test_custom_histogram->ranges(3));
  EXPECT_EQ(custom_ranges[4], test_custom_histogram->ranges(4));

  recorder.GetHistograms(&histograms);
  EXPECT_EQ(6U, histograms.size());
}

TEST(HistogramTest, CustomRangeTest) {
  StatisticsRecorder recorder;
  StatisticsRecorder::Histograms histograms;

  
  std::vector<int> custom_ranges;
  
  custom_ranges.push_back(9);
  custom_ranges.push_back(10);
  custom_ranges.push_back(11);
  Histogram* test_custom_histogram(CustomHistogram::FactoryGet(
      "TestCustomRangeHistogram", custom_ranges, Histogram::kNoFlags));

  EXPECT_EQ(0, test_custom_histogram->ranges(0));  
  EXPECT_EQ(custom_ranges[0], test_custom_histogram->ranges(1));
  EXPECT_EQ(custom_ranges[1], test_custom_histogram->ranges(2));
  EXPECT_EQ(custom_ranges[2], test_custom_histogram->ranges(3));

  
  const int kSmall = 7;
  const int kMid = 8;
  const int kBig = 9;
  custom_ranges.clear();
  custom_ranges.push_back(kBig);
  custom_ranges.push_back(kMid);
  custom_ranges.push_back(kSmall);
  custom_ranges.push_back(kSmall);
  custom_ranges.push_back(kMid);
  custom_ranges.push_back(0);  
  custom_ranges.push_back(kBig);

  Histogram* unsorted_histogram(CustomHistogram::FactoryGet(
      "TestCustomUnsortedDupedHistogram", custom_ranges, Histogram::kNoFlags));
  EXPECT_EQ(0, unsorted_histogram->ranges(0));
  EXPECT_EQ(kSmall, unsorted_histogram->ranges(1));
  EXPECT_EQ(kMid, unsorted_histogram->ranges(2));
  EXPECT_EQ(kBig, unsorted_histogram->ranges(3));
}



TEST(HistogramTest, BoundsTest) {
  const size_t kBucketCount = 50;
  Histogram* histogram(Histogram::FactoryGet(
      "Bounded", 10, 100, kBucketCount, Histogram::kNoFlags));

  
  histogram->Add(5);
  histogram->Add(-50);

  histogram->Add(100);
  histogram->Add(10000);

  
  Histogram::SampleSet sample;
  histogram->SnapshotSample(&sample);
  EXPECT_EQ(2, sample.counts(0));
  EXPECT_EQ(0, sample.counts(1));
  size_t array_size = histogram->bucket_count();
  EXPECT_EQ(kBucketCount, array_size);
  EXPECT_EQ(0, sample.counts(array_size - 2));
  EXPECT_EQ(2, sample.counts(array_size - 1));
}


TEST(HistogramTest, BucketPlacementTest) {
  Histogram* histogram(Histogram::FactoryGet(
      "Histogram", 1, 64, 8, Histogram::kNoFlags));  

  
  EXPECT_EQ(0, histogram->ranges(0));
  int power_of_2 = 1;
  for (int i = 1; i < 8; i++) {
    EXPECT_EQ(power_of_2, histogram->ranges(i));
    power_of_2 *= 2;
  }
  EXPECT_EQ(INT_MAX, histogram->ranges(8));

  
  histogram->Add(0);
  power_of_2 = 1;
  for (int i = 1; i < 8; i++) {
    for (int j = 0; j <= i; j++)
      histogram->Add(power_of_2);
    power_of_2 *= 2;
  }
  

  
  Histogram::SampleSet sample;
  histogram->SnapshotSample(&sample);
  EXPECT_EQ(INT_MAX, histogram->ranges(8));
  for (int i = 0; i < 8; i++)
    EXPECT_EQ(i + 1, sample.counts(i));
}

}  





TEST(HistogramTest, CorruptSampleCounts) {
  Histogram* histogram(Histogram::FactoryGet(
      "Histogram", 1, 64, 8, Histogram::kNoFlags));  

  EXPECT_EQ(0, histogram->sample_.redundant_count());
  histogram->Add(20);  
  histogram->Add(40);
  EXPECT_EQ(2, histogram->sample_.redundant_count());

  Histogram::SampleSet snapshot;
  histogram->SnapshotSample(&snapshot);
  EXPECT_EQ(Histogram::NO_INCONSISTENCIES, 0);
  EXPECT_EQ(0, histogram->FindCorruption(snapshot));  
  EXPECT_EQ(2, snapshot.redundant_count());

  snapshot.counts_[3] += 100;  
  EXPECT_EQ(Histogram::COUNT_LOW_ERROR, histogram->FindCorruption(snapshot));
  snapshot.counts_[2] -= 200;
  EXPECT_EQ(Histogram::COUNT_HIGH_ERROR, histogram->FindCorruption(snapshot));

  
  snapshot.counts_[1] += 100;
  EXPECT_EQ(0, histogram->FindCorruption(snapshot));
}

TEST(HistogramTest, CorruptBucketBounds) {
  Histogram* histogram(Histogram::FactoryGet(
      "Histogram", 1, 64, 8, Histogram::kNoFlags));  

  Histogram::SampleSet snapshot;
  histogram->SnapshotSample(&snapshot);
  EXPECT_EQ(Histogram::NO_INCONSISTENCIES, 0);
  EXPECT_EQ(0, histogram->FindCorruption(snapshot));  

  std::swap(histogram->ranges_[1], histogram->ranges_[2]);
  EXPECT_EQ(Histogram::BUCKET_ORDER_ERROR | Histogram::RANGE_CHECKSUM_ERROR,
            histogram->FindCorruption(snapshot));

  std::swap(histogram->ranges_[1], histogram->ranges_[2]);
  EXPECT_EQ(0, histogram->FindCorruption(snapshot));

  ++histogram->ranges_[3];
  EXPECT_EQ(Histogram::RANGE_CHECKSUM_ERROR,
            histogram->FindCorruption(snapshot));

  
  --histogram->ranges_[4];
  EXPECT_EQ(Histogram::RANGE_CHECKSUM_ERROR,
            histogram->FindCorruption(snapshot));

  
  --histogram->ranges_[3];
  ++histogram->ranges_[4];
}



TEST(HistogramTest, Crc32TableTest) {
  for (int i = 0; i < 256; ++i) {
    uint32 checksum = i;
    for (int j = 0; j < 8; ++j) {
      const uint32 kReversedPolynomial = 0xedb88320L;
      if (checksum & 1)
        checksum = kReversedPolynomial ^ (checksum >> 1);
      else
        checksum >>= 1;
    }
    EXPECT_EQ(Histogram::kCrcTable[i], checksum);
  }
}

}  
