





























#ifndef BASE_HISTOGRAM_H_
#define BASE_HISTOGRAM_H_

#include <map>
#include <string>
#include <vector>

#include "base/lock.h"
#include "base/time.h"





#define HISTOGRAM_TIMES(name, sample) do { \
    static Histogram counter((name), base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromSeconds(10), 50); \
    counter.AddTime(sample); \
  } while (0)

#define HISTOGRAM_COUNTS(name, sample) do { \
    static Histogram counter((name), 1, 1000000, 50); \
    counter.Add(sample); \
  } while (0)

#define HISTOGRAM_COUNTS_100(name, sample) do { \
    static Histogram counter((name), 1, 100, 50); \
    counter.Add(sample); \
  } while (0)

#define HISTOGRAM_COUNTS_10000(name, sample) do { \
    static Histogram counter((name), 1, 10000, 50); \
    counter.Add(sample); \
  } while (0)

#define HISTOGRAM_PERCENTAGE(name, under_one_hundred) do { \
    static LinearHistogram counter((name), 1, 100, 101); \
    counter.Add(under_one_hundred); \
  } while (0)



#define HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) do { \
    static Histogram counter((name), min, max, bucket_count); \
    if ((sample) < (max)) counter.AddTime(sample); \
  } while (0)















#define ASSET_HISTOGRAM_COUNTS(name, sample) do { \
    static ThreadSafeHistogram counter((name), 1, 1000000, 50); \
    if (0 == sample) break; \
    if (sample >= 0) \
      counter.Add(sample); \
    else\
      counter.Remove(-sample); \
  } while (0)



#ifndef NDEBUG

#define DHISTOGRAM_TIMES(name, sample) HISTOGRAM_TIMES(name, sample)
#define DHISTOGRAM_COUNTS(name, sample) HISTOGRAM_COUNTS(name, sample)
#define DASSET_HISTOGRAM_COUNTS(name, sample) ASSET_HISTOGRAM_COUNTS(name, \
                                                                     sample)
#define DHISTOGRAM_PERCENTAGE(name, under_one_hundred) HISTOGRAM_PERCENTAGE(\
    name, under_one_hundred)
#define DHISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) \
    HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count)

#else  

#define DHISTOGRAM_TIMES(name, sample) do {} while (0)
#define DHISTOGRAM_COUNTS(name, sample) do {} while (0)
#define DASSET_HISTOGRAM_COUNTS(name, sample) do {} while (0)
#define DHISTOGRAM_PERCENTAGE(name, under_one_hundred) do {} while (0)
#define DHISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) \
    do {} while (0)

#endif  







static const int kUmaTargetedHistogramFlag = 0x1;




static const int kRendererHistogramFlag = 1 << 4;

#define UMA_HISTOGRAM_TIMES(name, sample) do { \
    static Histogram counter((name), base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromSeconds(10), 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.AddTime(sample); \
  } while (0)

#define UMA_HISTOGRAM_MEDIUM_TIMES(name, sample) do { \
    static Histogram counter((name), base::TimeDelta::FromMilliseconds(10), \
                             base::TimeDelta::FromMinutes(3), 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.AddTime(sample); \
  } while (0)


#define UMA_HISTOGRAM_LONG_TIMES(name, sample) do { \
    static Histogram counter((name), base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromHours(1), 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.AddTime(sample); \
  } while (0)

#define UMA_HISTOGRAM_CLIPPED_TIMES(name, sample, min, max, bucket_count) do { \
    static Histogram counter((name), min, max, bucket_count); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    if ((sample) < (max)) counter.AddTime(sample); \
  } while (0)

#define UMA_HISTOGRAM_COUNTS(name, sample) do { \
    static Histogram counter((name), 1, 1000000, 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_COUNTS_100(name, sample) do { \
    static Histogram counter((name), 1, 100, 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_COUNTS_10000(name, sample) do { \
    static Histogram counter((name), 1, 10000, 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_MEMORY_KB(name, sample) do { \
    static Histogram counter((name), 1000, 500000, 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_MEMORY_MB(name, sample) do { \
    static Histogram counter((name), 1, 1000, 50); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(sample); \
  } while (0)

#define UMA_HISTOGRAM_PERCENTAGE(name, under_one_hundred) do { \
    static LinearHistogram counter((name), 1, 100, 101); \
    counter.SetFlags(kUmaTargetedHistogramFlag); \
    counter.Add(under_one_hundred); \
  } while (0)



class Pickle;

class Histogram {
 public:
  typedef int Sample;  
  typedef int Count;  
  static const Sample kSampleType_MAX = INT_MAX;

  typedef std::vector<Count> Counts;
  typedef std::vector<Sample> Ranges;

  static const int kHexRangePrintingFlag;

  enum BucketLayout {
    EXPONENTIAL,
    LINEAR
  };

  
  

  class SampleSet {
   public:
    explicit SampleSet();
    
    void Resize(const Histogram& histogram);
    void CheckSize(const Histogram& histogram) const;

    
    void Accumulate(Sample value, Count count, size_t index);

    
    Count counts(size_t i) const { return counts_[i]; }
    Count TotalCount() const;
    int64 sum() const { return sum_; }
    int64 square_sum() const { return square_sum_; }

    
    void Add(const SampleSet& other);
    void Subtract(const SampleSet& other);

    bool Serialize(Pickle* pickle) const;
    bool Deserialize(void** iter, const Pickle& pickle);

   protected:
    
    
    Counts counts_;

    
    
    int64 sum_;         
    int64 square_sum_;  
  };
  

  Histogram(const char* name, Sample minimum,
            Sample maximum, size_t bucket_count);
  Histogram(const char* name, base::TimeDelta minimum,
            base::TimeDelta maximum, size_t bucket_count);
  virtual ~Histogram();

  void Add(int value);
  
  void AddTime(base::TimeDelta time) {
    Add(static_cast<int>(time.InMilliseconds()));
  }

  void AddSampleSet(const SampleSet& sample);

  
  void WriteHTMLGraph(std::string* output) const;
  void WriteAscii(bool graph_it, const std::string& newline,
                  std::string* output) const;

  
  
  
  void SetFlags(int flags) { flags_ |= flags; }
  void ClearFlags(int flags) { flags_ &= ~flags; }
  int flags() const { return flags_; }

  virtual BucketLayout histogram_type() const { return EXPONENTIAL; }

  
  
  
  
  

  
  
  static std::string SerializeHistogramInfo(const Histogram& histogram,
                                            const SampleSet& snapshot);
  
  
  
  static void DeserializeHistogramList(
      const std::vector<std::string>& histograms);
  static bool DeserializeHistogramInfo(const std::string& state);


  
  
  
  const std::string histogram_name() const { return histogram_name_; }
  Sample declared_min() const { return declared_min_; }
  Sample declared_max() const { return declared_max_; }
  virtual Sample ranges(size_t i) const { return ranges_[i];}
  virtual size_t bucket_count() const { return bucket_count_; }
  
  
  virtual void SnapshotSample(SampleSet* sample) const;

 protected:
  
  virtual bool PrintEmptyBucket(size_t index) const { return true; }

  
  
  
  
  virtual void InitializeBucketRange();
  
  virtual size_t BucketIndex(Sample value) const;
  
  virtual double GetBucketSize(Count current, size_t i) const;

  
  
  
  virtual const std::string GetAsciiBucketRange(size_t it) const;

  
  
  
  
  virtual void Accumulate(Sample value, Count count, size_t index);

  
  
  
  void SetBucketRange(size_t i, Sample value);

  
  
  bool ValidateBucketRanges() const;

 private:
  
  void Initialize();

  
  

  
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

  
  

  
  
  
  
  
  const std::string histogram_name_;
  Sample declared_min_;  
  Sample declared_max_;  
  size_t bucket_count_;  

  
  int flags_;

  
  
  
  
  Ranges ranges_;

  
  
  SampleSet sample_;

  
  bool registered_;

  DISALLOW_COPY_AND_ASSIGN(Histogram);
};





class LinearHistogram : public Histogram {
 public:
  struct DescriptionPair {
    Sample sample;
    const char* description;  
  };
  LinearHistogram(const char* name, Sample minimum,
                  Sample maximum, size_t bucket_count);

  LinearHistogram(const char* name, base::TimeDelta minimum,
                  base::TimeDelta maximum, size_t bucket_count);
  ~LinearHistogram() {}

  
  
  void SetRangeDescriptions(const DescriptionPair descriptions[]);

  virtual BucketLayout histogram_type() const { return LINEAR; }

 protected:
  
  virtual void InitializeBucketRange();
  
  virtual size_t BucketIndex(Sample value) const;
  virtual double GetBucketSize(Count current, size_t i) const;

  
  
  virtual const std::string GetAsciiBucketRange(size_t i) const;

  
  
  virtual bool PrintEmptyBucket(size_t index) const;

 private:
  
  
  
  typedef std::map<Sample, std::string> BucketDescriptionMap;
  BucketDescriptionMap bucket_description_;

  DISALLOW_COPY_AND_ASSIGN(LinearHistogram);
};




class BooleanHistogram : public LinearHistogram {
 public:
  explicit BooleanHistogram(const char* name)
    : LinearHistogram(name, 0, 2, 3) {
  }

  void AddBoolean(bool value) { Add(value ? 1 : 0); }

 private:
  DISALLOW_COPY_AND_ASSIGN(BooleanHistogram);
};





class ThreadSafeHistogram : public Histogram {
 public:
  ThreadSafeHistogram(const char* name, Sample minimum,
                      Sample maximum, size_t bucket_count);

  
  void Remove(int value);

 protected:
  
  virtual void Accumulate(Sample value, Count count, size_t index);

  virtual void SnapshotSample(SampleSet* sample) const;

 private:
  mutable Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeHistogram);
};






class StatisticsRecorder {
 public:
  typedef std::vector<Histogram*> Histograms;

  StatisticsRecorder();

  ~StatisticsRecorder();

  
  static bool WasStarted();

  
  
  static bool Register(Histogram* histogram);
  
  static void UnRegister(Histogram* histogram);

  
  
  
  static void WriteHTMLGraph(const std::string& query, std::string* output);
  static void WriteGraph(const std::string& query, std::string* output);

  
  static void GetHistograms(Histograms* output);

  
  static Histogram* GetHistogram(const std::string& query);

  static void set_dump_on_exit(bool enable) { dump_on_exit_ = enable; }

  
  
  
  
  static void GetSnapshot(const std::string& query, Histograms* snapshot);


 private:
  
  typedef std::map<std::string, Histogram*> HistogramMap;

  static HistogramMap* histograms_;

  
  static Lock* lock_;

  
  static bool dump_on_exit_;

  DISALLOW_COPY_AND_ASSIGN(StatisticsRecorder);
};

#endif  
