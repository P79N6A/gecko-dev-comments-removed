






































#ifndef BASE_METRICS_HISTOGRAM_H_
#define BASE_METRICS_HISTOGRAM_H_
#pragma once

#include <map>
#include <string>
#include <vector>

#if defined(CHROMIUM_MOZILLA_BUILD)
#define BASE_API
#else
#include "base/base_api.h"
#endif
#include "testing/gtest/include/gtest/gtest_prod.h"
#include "base/time.h"
#include "base/lock.h"

class Pickle;

namespace base {




#define HISTOGRAM_TIMES(name, sample) HISTOGRAM_CUSTOM_TIMES( \
    name, sample, base::TimeDelta::FromMilliseconds(1), \
    base::TimeDelta::FromSeconds(10), 50)

#define HISTOGRAM_COUNTS(name, sample) HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 1000000, 50)

#define HISTOGRAM_COUNTS_100(name, sample) HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 100, 50)

#define HISTOGRAM_COUNTS_10000(name, sample) HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 10000, 50)

#define HISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryGet(name, min, max, bucket_count, \
                                            base::Histogram::kNoFlags); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)

#define HISTOGRAM_PERCENTAGE(name, under_one_hundred) \
    HISTOGRAM_ENUMERATION(name, under_one_hundred, 101)



#define HISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryTimeGet(name, min, max, bucket_count, \
                                                base::Histogram::kNoFlags); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->AddTime(sample); \
  } while (0)


#define HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryTimeGet(name, min, max, bucket_count, \
                                                base::Histogram::kNoFlags); \
    DCHECK_EQ(name, counter->histogram_name()); \
    if ((sample) < (max)) counter->AddTime(sample); \
  } while (0)




#define HISTOGRAM_ENUMERATION(name, sample, boundary_value) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::LinearHistogram::FactoryGet(name, 1, boundary_value, \
          boundary_value + 1, base::Histogram::kNoFlags); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)

#define HISTOGRAM_CUSTOM_ENUMERATION(name, sample, custom_ranges) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::CustomHistogram::FactoryGet(name, custom_ranges, \
                                                  base::Histogram::kNoFlags); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)




#ifndef NDEBUG

#define DHISTOGRAM_TIMES(name, sample) HISTOGRAM_TIMES(name, sample)
#define DHISTOGRAM_COUNTS(name, sample) HISTOGRAM_COUNTS(name, sample)
#define DHISTOGRAM_PERCENTAGE(name, under_one_hundred) HISTOGRAM_PERCENTAGE(\
    name, under_one_hundred)
#define DHISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count) \
    HISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count)
#define DHISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) \
    HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count)
#define DHISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count) \
    HISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count)
#define DHISTOGRAM_ENUMERATION(name, sample, boundary_value) \
    HISTOGRAM_ENUMERATION(name, sample, boundary_value)
#define DHISTOGRAM_CUSTOM_ENUMERATION(name, sample, custom_ranges) \
    HISTOGRAM_CUSTOM_ENUMERATION(name, sample, custom_ranges)

#else  

#define DHISTOGRAM_TIMES(name, sample) do {} while (0)
#define DHISTOGRAM_COUNTS(name, sample) do {} while (0)
#define DHISTOGRAM_PERCENTAGE(name, under_one_hundred) do {} while (0)
#define DHISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count) \
    do {} while (0)
#define DHISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) \
    do {} while (0)
#define DHISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count) \
    do {} while (0)
#define DHISTOGRAM_ENUMERATION(name, sample, boundary_value) do {} while (0)
#define DHISTOGRAM_CUSTOM_ENUMERATION(name, sample, custom_ranges) \
    do {} while (0)

#endif  







#define UMA_HISTOGRAM_TIMES(name, sample) UMA_HISTOGRAM_CUSTOM_TIMES( \
    name, sample, base::TimeDelta::FromMilliseconds(1), \
    base::TimeDelta::FromSeconds(10), 50)

#define UMA_HISTOGRAM_MEDIUM_TIMES(name, sample) UMA_HISTOGRAM_CUSTOM_TIMES( \
    name, sample, base::TimeDelta::FromMilliseconds(10), \
    base::TimeDelta::FromMinutes(3), 50)


#define UMA_HISTOGRAM_LONG_TIMES(name, sample) UMA_HISTOGRAM_CUSTOM_TIMES( \
    name, sample, base::TimeDelta::FromMilliseconds(1), \
    base::TimeDelta::FromHours(1), 50)

#define UMA_HISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryTimeGet(name, min, max, bucket_count, \
            base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->AddTime(sample); \
  } while (0)


#define UMA_HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryTimeGet(name, min, max, bucket_count, \
           base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    if ((sample) < (max)) counter->AddTime(sample); \
  } while (0)

#define UMA_HISTOGRAM_COUNTS(name, sample) UMA_HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 1000000, 50)

#define UMA_HISTOGRAM_COUNTS_100(name, sample) UMA_HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 100, 50)

#define UMA_HISTOGRAM_COUNTS_10000(name, sample) UMA_HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 10000, 50)

#define UMA_HISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::Histogram::FactoryGet(name, min, max, bucket_count, \
          base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_MEMORY_KB(name, sample) UMA_HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1000, 500000, 50)

#define UMA_HISTOGRAM_MEMORY_MB(name, sample) UMA_HISTOGRAM_CUSTOM_COUNTS( \
    name, sample, 1, 1000, 50)

#define UMA_HISTOGRAM_PERCENTAGE(name, under_one_hundred) \
    UMA_HISTOGRAM_ENUMERATION(name, under_one_hundred, 101)

#define UMA_HISTOGRAM_BOOLEAN(name, sample) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::BooleanHistogram::FactoryGet(name, \
          base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->AddBoolean(sample); \
  } while (0)

#define UMA_HISTOGRAM_ENUMERATION(name, sample, boundary_value) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::LinearHistogram::FactoryGet(name, 1, boundary_value, \
          boundary_value + 1, base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_CUSTOM_ENUMERATION(name, sample, custom_ranges) do { \
    static base::Histogram* counter(NULL); \
    if (!counter) \
      counter = base::CustomHistogram::FactoryGet(name, custom_ranges, \
          base::Histogram::kUmaTargetedHistogramFlag); \
    DCHECK_EQ(name, counter->histogram_name()); \
    counter->Add(sample); \
  } while (0)



class BooleanHistogram;
class CustomHistogram;
class Histogram;
class LinearHistogram;

class BASE_API Histogram {
 public:
  typedef int Sample;  
  typedef int Count;  
  static const Sample kSampleType_MAX = INT_MAX;
  
  static const size_t kBucketCount_MAX;

  typedef std::vector<Count> Counts;
  typedef std::vector<Sample> Ranges;

  
  
  enum ClassType {
    HISTOGRAM,
    LINEAR_HISTOGRAM,
    BOOLEAN_HISTOGRAM,
    CUSTOM_HISTOGRAM,
    NOT_VALID_IN_RENDERER
  };

  enum BucketLayout {
    EXPONENTIAL,
    LINEAR,
    CUSTOM
  };

  enum Flags {
    kNoFlags = 0,
    kUmaTargetedHistogramFlag = 0x1,  

    
    
    
    
    
    kIPCSerializationSourceFlag = 0x10,

    kHexRangePrintingFlag = 0x8000,  
  };

  enum Inconsistencies {
    NO_INCONSISTENCIES = 0x0,
    RANGE_CHECKSUM_ERROR = 0x1,
    BUCKET_ORDER_ERROR = 0x2,
    COUNT_HIGH_ERROR = 0x4,
    COUNT_LOW_ERROR = 0x8,

    NEVER_EXCEEDED_VALUE = 0x10
  };

  struct DescriptionPair {
    Sample sample;
    const char* description;  
  };

  
  

  class BASE_API SampleSet {
   public:
    explicit SampleSet();
    ~SampleSet();

    
    void Resize(const Histogram& histogram);
    void CheckSize(const Histogram& histogram) const;

    
    void Accumulate(Sample value, Count count, size_t index);

    
    Count counts(size_t i) const { return counts_[i]; }
    Count TotalCount() const;
    int64 sum() const { return sum_; }
    int64 redundant_count() const { return redundant_count_; }

    
    void Add(const SampleSet& other);
    void Subtract(const SampleSet& other);

    bool Serialize(Pickle* pickle) const;
    bool Deserialize(void** iter, const Pickle& pickle);

   protected:
    
    
    Counts counts_;

    
    
    int64 sum_;         

   private:
    
    FRIEND_TEST(HistogramTest, CorruptSampleCounts);

    
    
    
    
    
    
    
    int64 redundant_count_;
  };

  
  
  
  static Histogram* FactoryGet(const std::string& name,
                               Sample minimum,
                               Sample maximum,
                               size_t bucket_count,
                               Flags flags);
  static Histogram* FactoryTimeGet(const std::string& name,
                                   base::TimeDelta minimum,
                                   base::TimeDelta maximum,
                                   size_t bucket_count,
                                   Flags flags);

  void Add(int value);

  
  virtual void AddBoolean(bool value);

  
  void AddTime(TimeDelta time) {
    Add(static_cast<int>(time.InMilliseconds()));
  }

  void AddSampleSet(const SampleSet& sample);

  
  virtual void SetRangeDescriptions(const DescriptionPair descriptions[]);

  
  void WriteHTMLGraph(std::string* output) const;
  void WriteAscii(bool graph_it, const std::string& newline,
                  std::string* output) const;

  
  
  
  void SetFlags(Flags flags) { flags_ = static_cast<Flags> (flags_ | flags); }
  void ClearFlags(Flags flags) { flags_ = static_cast<Flags>(flags_ & ~flags); }
  int flags() const { return flags_; }

  
  
  
  
  

  
  
  static std::string SerializeHistogramInfo(const Histogram& histogram,
                                            const SampleSet& snapshot);
  
  
  
  static bool DeserializeHistogramInfo(const std::string& histogram_info);

  
  
  
  
  
  virtual Inconsistencies FindCorruption(const SampleSet& snapshot) const;

  
  
  
  virtual ClassType histogram_type() const;
  const std::string& histogram_name() const { return histogram_name_; }
  Sample declared_min() const { return declared_min_; }
  Sample declared_max() const { return declared_max_; }
  virtual Sample ranges(size_t i) const;
  uint32 range_checksum() const { return range_checksum_; }
  virtual size_t bucket_count() const;
  
  
  virtual void SnapshotSample(SampleSet* sample) const;

  virtual bool HasConstructorArguments(Sample minimum, Sample maximum,
                                       size_t bucket_count);

  virtual bool HasConstructorTimeDeltaArguments(TimeDelta minimum,
                                                TimeDelta maximum,
                                                size_t bucket_count);
  
  bool HasValidRangeChecksum() const;

 protected:
  Histogram(const std::string& name, Sample minimum,
            Sample maximum, size_t bucket_count);
  Histogram(const std::string& name, TimeDelta minimum,
            TimeDelta maximum, size_t bucket_count);

  virtual ~Histogram();

  
  void InitializeBucketRange();

  
  virtual bool PrintEmptyBucket(size_t index) const;

  
  
  
  
  virtual size_t BucketIndex(Sample value) const;
  
  virtual double GetBucketSize(Count current, size_t i) const;

  
  void ResetRangeChecksum();

  
  
  
  virtual const std::string GetAsciiBucketRange(size_t it) const;

  
  
  
  
  virtual void Accumulate(Sample value, Count count, size_t index);

  
  
  
  void SetBucketRange(size_t i, Sample value);

  
  
  bool ValidateBucketRanges() const;

  virtual uint32 CalculateRangeChecksum() const;

 private:
  
  FRIEND_TEST(HistogramTest, CorruptBucketBounds);
  FRIEND_TEST(HistogramTest, CorruptSampleCounts);
  FRIEND_TEST(HistogramTest, Crc32SampleHash);
  FRIEND_TEST(HistogramTest, Crc32TableTest);

  friend class StatisticsRecorder;  

  
  void Initialize();

  
  static uint32 Crc32(uint32 sum, Sample range);

  
  

  
  double GetPeakBucketSize(const SampleSet& snapshot) const;

  
  void WriteAsciiHeader(const SampleSet& snapshot,
                        Count sample_count, std::string* output) const;

  
  
  void WriteAsciiBucketContext(const int64 past, const Count current,
                               const int64 remaining, const size_t i,
                               std::string* output) const;

  
  
  void WriteAsciiBucketValue(Count current, double scaled_sum,
                             std::string* output) const;

  
  void WriteAsciiBucketGraph(double current_size, double max_size,
                             std::string* output) const;

  
  
  static const uint32 kCrcTable[256];
  
  

  
  
  const std::string histogram_name_;
  Sample declared_min_;  
  Sample declared_max_;  
  size_t bucket_count_;  

  
  Flags flags_;

  
  
  
  
  Ranges ranges_;

  
  
  
  uint32 range_checksum_;

  
  
  SampleSet sample_;

  DISALLOW_COPY_AND_ASSIGN(Histogram);
};





class BASE_API LinearHistogram : public Histogram {
 public:
  virtual ~LinearHistogram();

  

  static Histogram* FactoryGet(const std::string& name,
                               Sample minimum,
                               Sample maximum,
                               size_t bucket_count,
                               Flags flags);
  static Histogram* FactoryTimeGet(const std::string& name,
                                   TimeDelta minimum,
                                   TimeDelta maximum,
                                   size_t bucket_count,
                                   Flags flags);

  
  virtual ClassType histogram_type() const;

  
  
  virtual void SetRangeDescriptions(const DescriptionPair descriptions[]);

 protected:
  LinearHistogram(const std::string& name, Sample minimum,
                  Sample maximum, size_t bucket_count);

  LinearHistogram(const std::string& name, TimeDelta minimum,
                  TimeDelta maximum, size_t bucket_count);

  
  void InitializeBucketRange();
  virtual double GetBucketSize(Count current, size_t i) const;

  
  
  virtual const std::string GetAsciiBucketRange(size_t i) const;

  
  
  virtual bool PrintEmptyBucket(size_t index) const;

 private:
  
  
  
  typedef std::map<Sample, std::string> BucketDescriptionMap;
  BucketDescriptionMap bucket_description_;

  DISALLOW_COPY_AND_ASSIGN(LinearHistogram);
};




class BASE_API BooleanHistogram : public LinearHistogram {
 public:
  static Histogram* FactoryGet(const std::string& name, Flags flags);

  virtual ClassType histogram_type() const;

  virtual void AddBoolean(bool value);

 private:
  explicit BooleanHistogram(const std::string& name);

  DISALLOW_COPY_AND_ASSIGN(BooleanHistogram);
};




class BASE_API CustomHistogram : public Histogram {
 public:

  static Histogram* FactoryGet(const std::string& name,
                               const std::vector<Sample>& custom_ranges,
                               Flags flags);

  
  virtual ClassType histogram_type() const;

 protected:
  CustomHistogram(const std::string& name,
                  const std::vector<Sample>& custom_ranges);

  
  void InitializedCustomBucketRange(const std::vector<Sample>& custom_ranges);
  virtual double GetBucketSize(Count current, size_t i) const;

  DISALLOW_COPY_AND_ASSIGN(CustomHistogram);
};






class BASE_API StatisticsRecorder {
 public:
  typedef std::vector<Histogram*> Histograms;

  StatisticsRecorder();

  ~StatisticsRecorder();

  
  static bool IsActive();

  
  
  
  
  static Histogram* RegisterOrDeleteDuplicate(Histogram* histogram);

  
  
  
  static void WriteHTMLGraph(const std::string& query, std::string* output);
  static void WriteGraph(const std::string& query, std::string* output);

  
  static void GetHistograms(Histograms* output);

  
  
  
  static bool FindHistogram(const std::string& query, Histogram** histogram);

  static bool dump_on_exit() { return dump_on_exit_; }

  static void set_dump_on_exit(bool enable) { dump_on_exit_ = enable; }

  
  
  
  
  static void GetSnapshot(const std::string& query, Histograms* snapshot);


 private:
  
  typedef std::map<std::string, Histogram*> HistogramMap;

  static HistogramMap* histograms_;

  
  static Lock* lock_;

  
  static bool dump_on_exit_;

  DISALLOW_COPY_AND_ASSIGN(StatisticsRecorder);
};

}  

#endif  
