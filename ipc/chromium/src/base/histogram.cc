








#include "base/histogram.h"

#include <math.h>
#include <string>

#include "base/logging.h"
#include "base/pickle.h"
#include "base/string_util.h"

using base::TimeDelta;

typedef Histogram::Count Count;


const int Histogram::kHexRangePrintingFlag = 0x8000;

Histogram::Histogram(const char* name, Sample minimum,
                     Sample maximum, size_t bucket_count)
  : histogram_name_(name),
    declared_min_(minimum),
    declared_max_(maximum),
    bucket_count_(bucket_count),
    flags_(0),
    ranges_(bucket_count + 1, 0),
    sample_(),
    registered_(false) {
  Initialize();
}

Histogram::Histogram(const char* name, TimeDelta minimum,
                     TimeDelta maximum, size_t bucket_count)
  : histogram_name_(name),
    declared_min_(static_cast<int> (minimum.InMilliseconds())),
    declared_max_(static_cast<int> (maximum.InMilliseconds())),
    bucket_count_(bucket_count),
    flags_(0),
    ranges_(bucket_count + 1, 0),
    sample_(),
    registered_(false) {
  Initialize();
}

Histogram::~Histogram() {
  if (registered_)
    StatisticsRecorder::UnRegister(this);
  
  DCHECK(ValidateBucketRanges());
}

void Histogram::Add(int value) {
  if (!registered_)
    registered_ = StatisticsRecorder::Register(this);
  if (value >= kSampleType_MAX)
    value = kSampleType_MAX - 1;
  if (value < 0)
    value = 0;
  size_t index = BucketIndex(value);
  DCHECK(value >= ranges(index));
  DCHECK(value < ranges(index + 1));
  Accumulate(value, 1, index);
}

void Histogram::AddSampleSet(const SampleSet& sample) {
  sample_.Add(sample);
}


void Histogram::WriteHTMLGraph(std::string* output) const {
  
  output->append("<PRE>");
  WriteAscii(true, "<br>", output);
  output->append("</PRE>");
}

void Histogram::WriteAscii(bool graph_it, const std::string& newline,
                           std::string* output) const {
  
  
  SampleSet snapshot;
  SnapshotSample(&snapshot);
  Count sample_count = snapshot.TotalCount();

  WriteAsciiHeader(snapshot, sample_count, output);
  output->append(newline);

  
  double max_size = 0;
  if (graph_it)
    max_size = GetPeakBucketSize(snapshot);

  
  
  size_t largest_non_empty_bucket = bucket_count() - 1;
  while (0 == snapshot.counts(largest_non_empty_bucket)) {
    if (0 == largest_non_empty_bucket)
      break;  
    --largest_non_empty_bucket;
  }

  
  size_t print_width = 1;
  for (size_t i = 0; i < bucket_count(); ++i) {
    if (snapshot.counts(i)) {
      size_t width = GetAsciiBucketRange(i).size() + 1;
      if (width > print_width)
        print_width = width;
    }
  }

  int64 remaining = sample_count;
  int64 past = 0;
  
  for (size_t i = 0; i < bucket_count(); ++i) {
    Count current = snapshot.counts(i);
    if (!current && !PrintEmptyBucket(i))
      continue;
    remaining -= current;
    StringAppendF(output, "%#*s ", print_width, GetAsciiBucketRange(i).c_str());
    if (0 == current && i < bucket_count() - 1 && 0 == snapshot.counts(i + 1)) {
      while (i < bucket_count() - 1 && 0 == snapshot.counts(i + 1))
        ++i;
      output->append("... ");
      output->append(newline);
      continue;  
    }
    double current_size = GetBucketSize(current, i);
    if (graph_it)
      WriteAsciiBucketGraph(current_size, max_size, output);
    WriteAsciiBucketContext(past, current, remaining, i, output);
    output->append(newline);
    past += current;
  }
  DCHECK(past == sample_count);
}

bool Histogram::ValidateBucketRanges() const {
  
  DCHECK(ranges_.size() == bucket_count_ + 1);
  DCHECK(0 == ranges_[0]);
  DCHECK(declared_min() == ranges_[1]);
  DCHECK(declared_max() == ranges_[bucket_count_ - 1]);
  DCHECK(kSampleType_MAX == ranges_[bucket_count_]);
  return true;
}

void Histogram::Initialize() {
  sample_.Resize(*this);
  if (declared_min_ <= 0)
    declared_min_ = 1;
  if (declared_max_ >= kSampleType_MAX)
    declared_max_ = kSampleType_MAX - 1;
  DCHECK(declared_min_ > 0);  
  DCHECK(declared_min_ <= declared_max_);
  DCHECK(1 < bucket_count_);
  size_t maximal_bucket_count = declared_max_ - declared_min_ + 2;
  DCHECK(bucket_count_ <= maximal_bucket_count);
  DCHECK(0 == ranges_[0]);
  ranges_[bucket_count_] = kSampleType_MAX;
  InitializeBucketRange();
  DCHECK(ValidateBucketRanges());
  registered_ = StatisticsRecorder::Register(this);
}









void Histogram::InitializeBucketRange() {
  double log_max = log(static_cast<double>(declared_max()));
  double log_ratio;
  double log_next;
  size_t bucket_index = 1;
  Sample current = declared_min();
  SetBucketRange(bucket_index, current);
  while (bucket_count() > ++bucket_index) {
    double log_current;
    log_current = log(static_cast<double>(current));
    
    log_ratio = (log_max - log_current) / (bucket_count() - bucket_index);
    
    log_next = log_current + log_ratio;
    int next;
    next = static_cast<int>(floor(exp(log_next) + 0.5));
    if (next > current)
      current = next;
    else
      ++current;  
    SetBucketRange(bucket_index, current);
  }

  DCHECK(bucket_count() == bucket_index);
}

size_t Histogram::BucketIndex(Sample value) const {
  
  
  DCHECK(ranges(0) <= value);
  DCHECK(ranges(bucket_count()) > value);
  size_t under = 0;
  size_t over = bucket_count();
  size_t mid;

  do {
    DCHECK(over >= under);
    mid = (over + under)/2;
    if (mid == under)
      break;
    if (ranges(mid) <= value)
      under = mid;
    else
      over = mid;
  } while (true);

  DCHECK(ranges(mid) <= value && ranges(mid+1) > value);
  return mid;
}






double Histogram::GetBucketSize(Count current, size_t i) const {
  DCHECK(ranges(i + 1) > ranges(i));
  static const double kTransitionWidth = 5;
  double denominator = ranges(i + 1) - ranges(i);
  if (denominator > kTransitionWidth)
    denominator = kTransitionWidth;  
  return current/denominator;
}









void Histogram::Accumulate(Sample value, Count count, size_t index) {
  
  sample_.Accumulate(value, count, index);
}



void Histogram::SnapshotSample(SampleSet* sample) const {
  
  *sample = sample_;
}




void Histogram::SetBucketRange(size_t i, Sample value) {
  DCHECK(bucket_count_ > i);
  ranges_[i] = value;
}




double Histogram::GetPeakBucketSize(const SampleSet& snapshot) const {
  double max = 0;
  for (size_t i = 0; i < bucket_count() ; ++i) {
    double current_size = GetBucketSize(snapshot.counts(i), i);
    if (current_size > max)
      max = current_size;
  }
  return max;
}

void Histogram::WriteAsciiHeader(const SampleSet& snapshot,
                                 Count sample_count,
                                 std::string* output) const {
  StringAppendF(output,
                "Histogram: %s recorded %ld samples",
                histogram_name().c_str(),
                sample_count);
  if (0 == sample_count) {
    DCHECK(0 == snapshot.sum());
  } else {
    double average = static_cast<float>(snapshot.sum()) / sample_count;
    double variance = static_cast<float>(snapshot.square_sum())/sample_count
                      - average * average;
    double standard_deviation = sqrt(variance);

    StringAppendF(output,
                  ", average = %.1f, standard deviation = %.1f",
                  average, standard_deviation);
  }
  if (flags_ & ~kHexRangePrintingFlag )
    StringAppendF(output, " (flags = 0x%x)", flags_ & ~kHexRangePrintingFlag);
}

void Histogram::WriteAsciiBucketContext(const int64 past,
                                        const Count current,
                                        const int64 remaining,
                                        const size_t i,
                                        std::string* output) const {
  double scaled_sum = (past + current + remaining) / 100.0;
  WriteAsciiBucketValue(current, scaled_sum, output);
  if (0 < i) {
    double percentage = past / scaled_sum;
    StringAppendF(output, " {%3.1f%%}", percentage);
  }
}

const std::string Histogram::GetAsciiBucketRange(size_t i) const {
  std::string result;
  if (kHexRangePrintingFlag & flags_)
    StringAppendF(&result, "%#x", ranges(i));
  else
    StringAppendF(&result, "%d", ranges(i));
  return result;
}

void Histogram::WriteAsciiBucketValue(Count current, double scaled_sum,
                                      std::string* output) const {
  StringAppendF(output, " (%d = %3.1f%%)", current, current/scaled_sum);
}

void Histogram::WriteAsciiBucketGraph(double current_size, double max_size,
                                      std::string* output) const {
  const int k_line_length = 72;  
  int x_count = static_cast<int>(k_line_length * (current_size / max_size)
                                 + 0.5);
  int x_remainder = k_line_length - x_count;

  while (0 < x_count--)
    output->append("-");
  output->append("O");
  while (0 < x_remainder--)
    output->append(" ");
}


std::string Histogram::SerializeHistogramInfo(const Histogram& histogram,
                                              const SampleSet& snapshot) {
  Pickle pickle;

  pickle.WriteString(histogram.histogram_name());
  pickle.WriteInt(histogram.declared_min());
  pickle.WriteInt(histogram.declared_max());
  pickle.WriteSize(histogram.bucket_count());
  pickle.WriteInt(histogram.histogram_type());
  pickle.WriteInt(histogram.flags());

  snapshot.Serialize(&pickle);
  return std::string(static_cast<const char*>(pickle.data()), pickle.size());
}


void Histogram::DeserializeHistogramList(
    const std::vector<std::string>& histograms) {
  for (std::vector<std::string>::const_iterator it = histograms.begin();
       it < histograms.end();
       ++it) {
    DeserializeHistogramInfo(*it);
  }
}


bool Histogram::DeserializeHistogramInfo(const std::string& histogram_info) {
  if (histogram_info.empty()) {
      return false;
  }

  Pickle pickle(histogram_info.data(),
                static_cast<int>(histogram_info.size()));
  void* iter = NULL;
  size_t bucket_count;
  int declared_min;
  int declared_max;
  int histogram_type;
  int flags;
  std::string histogram_name;
  SampleSet sample;

  if (!pickle.ReadString(&iter, &histogram_name) ||
      !pickle.ReadInt(&iter, &declared_min) ||
      !pickle.ReadInt(&iter, &declared_max) ||
      !pickle.ReadSize(&iter, &bucket_count) ||
      !pickle.ReadInt(&iter, &histogram_type) ||
      !pickle.ReadInt(&iter, &flags) ||
      !sample.Histogram::SampleSet::Deserialize(&iter, pickle)) {
    LOG(ERROR) << "Picke error decoding Histogram: " << histogram_name;
    return false;
  }

  Histogram* render_histogram =
      StatisticsRecorder::GetHistogram(histogram_name);

  if (render_histogram == NULL) {
    if (histogram_type ==  EXPONENTIAL) {
      render_histogram = new Histogram(histogram_name.c_str(),
                                       declared_min,
                                       declared_max,
                                       bucket_count);
    } else if (histogram_type == LINEAR) {
      render_histogram = reinterpret_cast<Histogram*>
        (new LinearHistogram(histogram_name.c_str(),
                             declared_min,
                             declared_max,
                             bucket_count));
    } else {
      LOG(ERROR) << "Error Deserializing Histogram Unknown histogram_type: " <<
          histogram_type;
      return false;
    }
    DCHECK(!(flags & kRendererHistogramFlag));
    render_histogram->SetFlags(flags | kRendererHistogramFlag);
  }

  DCHECK(declared_min == render_histogram->declared_min());
  DCHECK(declared_max == render_histogram->declared_max());
  DCHECK(bucket_count == render_histogram->bucket_count());
  DCHECK(histogram_type == render_histogram->histogram_type());

  if (render_histogram->flags() & kRendererHistogramFlag) {
    render_histogram->AddSampleSet(sample);
  } else {
    DLOG(INFO) << "Single thread mode, histogram observed and not copied: " <<
        histogram_name;
  }

  return true;
}






Histogram::SampleSet::SampleSet()
    : counts_(),
      sum_(0),
      square_sum_(0) {
}

void Histogram::SampleSet::Resize(const Histogram& histogram) {
  counts_.resize(histogram.bucket_count(), 0);
}

void Histogram::SampleSet::CheckSize(const Histogram& histogram) const {
  DCHECK(counts_.size() == histogram.bucket_count());
}


void Histogram::SampleSet::Accumulate(Sample value,  Count count,
                                      size_t index) {
  DCHECK(count == 1 || count == -1);
  counts_[index] += count;
  sum_ += count * value;
  square_sum_ += (count * value) * static_cast<int64>(value);
  DCHECK(counts_[index] >= 0);
  DCHECK(sum_ >= 0);
  DCHECK(square_sum_ >= 0);
}

Count Histogram::SampleSet::TotalCount() const {
  Count total = 0;
  for (Counts::const_iterator it = counts_.begin();
       it != counts_.end();
       ++it) {
    total += *it;
  }
  return total;
}

void Histogram::SampleSet::Add(const SampleSet& other) {
  DCHECK(counts_.size() == other.counts_.size());
  sum_ += other.sum_;
  square_sum_ += other.square_sum_;
  for (size_t index = 0; index < counts_.size(); ++index)
    counts_[index] += other.counts_[index];
}

void Histogram::SampleSet::Subtract(const SampleSet& other) {
  DCHECK(counts_.size() == other.counts_.size());
  
  
  
  sum_ -= other.sum_;
  square_sum_ -= other.square_sum_;
  for (size_t index = 0; index < counts_.size(); ++index) {
    counts_[index] -= other.counts_[index];
    DCHECK(counts_[index] >= 0);
  }
}

bool Histogram::SampleSet::Serialize(Pickle* pickle) const {
  pickle->WriteInt64(sum_);
  pickle->WriteInt64(square_sum_);
  pickle->WriteSize(counts_.size());

  for (size_t index = 0; index < counts_.size(); ++index) {
    pickle->WriteInt(counts_[index]);
  }

  return true;
}

bool Histogram::SampleSet::Deserialize(void** iter, const Pickle& pickle) {
  DCHECK(counts_.size() == 0);
  DCHECK(sum_ == 0);
  DCHECK(square_sum_ == 0);

  size_t counts_size;

  if (!pickle.ReadInt64(iter, &sum_) ||
      !pickle.ReadInt64(iter, &square_sum_) ||
      !pickle.ReadSize(iter, &counts_size)) {
    return false;
  }

  if (counts_size <= 0)
    return false;

  counts_.resize(counts_size, 0);
  for (size_t index = 0; index < counts_size; ++index) {
    if (!pickle.ReadInt(iter, &counts_[index])) {
      return false;
    }
  }

  return true;
}






LinearHistogram::LinearHistogram(const char* name, Sample minimum,
    Sample maximum, size_t bucket_count)
    : Histogram(name, minimum >= 1 ? minimum : 1, maximum, bucket_count) {
  InitializeBucketRange();
  DCHECK(ValidateBucketRanges());
}

LinearHistogram::LinearHistogram(const char* name,
    TimeDelta minimum, TimeDelta maximum, size_t bucket_count)
    : Histogram(name, minimum >= TimeDelta::FromMilliseconds(1) ?
                                 minimum : TimeDelta::FromMilliseconds(1),
                maximum, bucket_count) {
  
  InitializeBucketRange();
  DCHECK(ValidateBucketRanges());
}

void LinearHistogram::SetRangeDescriptions(
    const DescriptionPair descriptions[]) {
  for (int i =0; descriptions[i].description; ++i) {
    bucket_description_[descriptions[i].sample] = descriptions[i].description;
  }
}

const std::string LinearHistogram::GetAsciiBucketRange(size_t i) const {
  int range = ranges(i);
  BucketDescriptionMap::const_iterator it = bucket_description_.find(range);
  if (it == bucket_description_.end())
    return Histogram::GetAsciiBucketRange(i);
  return it->second;
}

bool LinearHistogram::PrintEmptyBucket(size_t index) const {
  return bucket_description_.find(ranges(index)) == bucket_description_.end();
}


void LinearHistogram::InitializeBucketRange() {
  DCHECK(0 < declared_min());  
  double min = declared_min();
  double max = declared_max();
  size_t i;
  for (i = 1; i < bucket_count(); ++i) {
    double linear_range = (min * (bucket_count() -1 - i) + max * (i - 1)) /
                          (bucket_count() - 2);
    SetBucketRange(i, static_cast<int> (linear_range + 0.5));
  }
}


size_t LinearHistogram::BucketIndex(Sample value) const {
  if (value < declared_min()) return 0;
  if (value >= declared_max()) return bucket_count() - 1;
  size_t index;
  index = static_cast<size_t>(((value - declared_min()) * (bucket_count() - 2))
                              / (declared_max() - declared_min()) + 1);
  DCHECK(1 <= index && bucket_count() > index);
  return index;
}

double LinearHistogram::GetBucketSize(Count current, size_t i) const {
  DCHECK(ranges(i + 1) > ranges(i));
  
  
  double denominator = ranges(i + 1) - ranges(i);
  return current/denominator;
}





ThreadSafeHistogram::ThreadSafeHistogram(const char* name, Sample minimum,
                                         Sample maximum, size_t bucket_count)
    : Histogram(name, minimum, maximum, bucket_count),
      lock_() {
  }

void ThreadSafeHistogram::Remove(int value) {
  if (value >= kSampleType_MAX)
    value = kSampleType_MAX - 1;
  size_t index = BucketIndex(value);
  Accumulate(value, -1, index);
}

void ThreadSafeHistogram::Accumulate(Sample value, Count count, size_t index) {
  AutoLock lock(lock_);
  Histogram::Accumulate(value, count, index);
}

void ThreadSafeHistogram::SnapshotSample(SampleSet* sample) const {
  AutoLock lock(lock_);
  Histogram::SnapshotSample(sample);
};










StatisticsRecorder::StatisticsRecorder() {
  DCHECK(!histograms_);
  lock_ = new Lock;
  histograms_ = new HistogramMap;
}

StatisticsRecorder::~StatisticsRecorder() {
  DCHECK(histograms_);

  if (dump_on_exit_) {
    std::string output;
    WriteGraph("", &output);
    LOG(INFO) << output;
  }

  
  delete histograms_;
  histograms_ = NULL;
  delete lock_;
  lock_ = NULL;
}


bool StatisticsRecorder::WasStarted() {
  return NULL != histograms_;
}


bool StatisticsRecorder::Register(Histogram* histogram) {
  if (!histograms_)
    return false;
  const std::string name = histogram->histogram_name();
  AutoLock auto_lock(*lock_);

  DCHECK(histograms_->end() == histograms_->find(name)) << name << " is already"
      "registered as a histogram.  Check for duplicate use of the name, or a "
      "race where a static initializer could be run by several threads.";
  (*histograms_)[name] = histogram;
  return true;
}


void StatisticsRecorder::UnRegister(Histogram* histogram) {
  if (!histograms_)
    return;
  const std::string name = histogram->histogram_name();
  AutoLock auto_lock(*lock_);
  DCHECK(histograms_->end() != histograms_->find(name));
  histograms_->erase(name);
  if (dump_on_exit_) {
    std::string output;
    histogram->WriteAscii(true, "\n", &output);
    LOG(INFO) << output;
  }
}


void StatisticsRecorder::WriteHTMLGraph(const std::string& query,
                                        std::string* output) {
  if (!histograms_)
    return;
  output->append("<html><head><title>About Histograms");
  if (!query.empty())
    output->append(" - " + query);
  output->append("</title>"
                 
                 
                 "</head><body>");

  Histograms snapshot;
  GetSnapshot(query, &snapshot);
  for (Histograms::iterator it = snapshot.begin();
       it != snapshot.end();
       ++it) {
    (*it)->WriteHTMLGraph(output);
    output->append("<br><hr><br>");
  }
  output->append("</body></html>");
}


void StatisticsRecorder::WriteGraph(const std::string& query,
                                    std::string* output) {
  if (!histograms_)
    return;
  if (query.length())
    StringAppendF(output, "Collections of histograms for %s\n", query.c_str());
  else
    output->append("Collections of all histograms\n");

  Histograms snapshot;
  GetSnapshot(query, &snapshot);
  for (Histograms::iterator it = snapshot.begin();
       it != snapshot.end();
       ++it) {
    (*it)->WriteAscii(true, "\n", output);
    output->append("\n");
  }
}


void StatisticsRecorder::GetHistograms(Histograms* output) {
  if (!histograms_)
    return;
  AutoLock auto_lock(*lock_);
  for (HistogramMap::iterator it = histograms_->begin();
       histograms_->end() != it;
       ++it) {
    output->push_back(it->second);
  }
}

Histogram* StatisticsRecorder::GetHistogram(const std::string& query) {
  if (!histograms_)
    return NULL;
  AutoLock auto_lock(*lock_);
  for (HistogramMap::iterator it = histograms_->begin();
       histograms_->end() != it;
       ++it) {
    if (it->first.find(query) != std::string::npos)
      return it->second;
  }
  return NULL;
}


void StatisticsRecorder::GetSnapshot(const std::string& query,
                                     Histograms* snapshot) {
  AutoLock auto_lock(*lock_);
  for (HistogramMap::iterator it = histograms_->begin();
       histograms_->end() != it;
       ++it) {
    if (it->first.find(query) != std::string::npos)
      snapshot->push_back(it->second);
  }
}


StatisticsRecorder::HistogramMap* StatisticsRecorder::histograms_ = NULL;

Lock* StatisticsRecorder::lock_ = NULL;

bool StatisticsRecorder::dump_on_exit_ = false;
