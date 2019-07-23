




#ifndef BASE_STATS_COUNTERS_H__
#define BASE_STATS_COUNTERS_H__

#include <string>
#include "base/stats_table.h"
#include "base/time.h"

































#define STATS_COUNTER(name, delta) do { \
  static StatsCounter counter(name); \
  counter.Add(delta); \
} while (0)

#define SIMPLE_STATS_COUNTER(name) STATS_COUNTER(name, 1)

#define RATE_COUNTER(name, duration) do { \
  static StatsRate hit_count(name); \
  hit_count.AddTime(duration); \
} while (0)


#ifndef NDEBUG

#define DSTATS_COUNTER(name, delta) STATS_COUNTER(name, delta)
#define DSIMPLE_STATS_COUNTER(name) SIMPLE_STATS_COUNTER(name)
#define DRATE_COUNTER(name, duration) RATE_COUNTER(name, duration)

#else  

#define DSTATS_COUNTER(name, delta) do {} while (0)
#define DSIMPLE_STATS_COUNTER(name) do {} while (0)
#define DRATE_COUNTER(name, duration) do {} while (0)

#endif  



class StatsCounter {
 public:
  
  explicit StatsCounter(const std::string& name)
       : counter_id_(-1) {
    
    name_ = "c:";
    name_.append(name);
  };

  virtual ~StatsCounter() {}

  
  void Set(int value) {
    int* loc = GetPtr();
    if (loc) *loc = value;
  }

  
  void Increment() {
    Add(1);
  }

  virtual void Add(int value) {
    int* loc = GetPtr();
    if (loc)
      (*loc) += value;
  }

  
  void Decrement() {
    Add(-1);
  }

  void Subtract(int value) {
    Add(-value);
  }

  
  
  bool Enabled() {
    return GetPtr() != NULL;
  }

  int value() {
    int* loc = GetPtr();
    if (loc) return *loc;
    return 0;
  }

 protected:
  StatsCounter()
    : counter_id_(-1) {
  }

  
  int* GetPtr() {
    StatsTable* table = StatsTable::current();
    if (!table)
      return NULL;

    
    if (counter_id_ == -1) {
      counter_id_ = table->FindCounter(name_);
      if (table->GetSlot() == 0) {
        if (!table->RegisterThread("")) {
          
          
          counter_id_ = 0;
          return NULL;
        }
      }
    }

    
    if (counter_id_ > 0)
      return table->GetLocation(counter_id_, table->GetSlot());

    
    return NULL;
  }

  std::string name_;
  
  
  
  int32 counter_id_;
};





class StatsCounterTimer : protected StatsCounter {
 public:
  
  explicit StatsCounterTimer(const std::string& name) {
    
    name_ = "t:";
    name_.append(name);
  }

  
  void Start() {
    if (!Enabled())
      return;
    start_time_ = base::TimeTicks::Now();
    stop_time_ = base::TimeTicks();
  }

  
  void Stop() {
    if (!Enabled() || !Running())
      return;
    stop_time_ = base::TimeTicks::Now();
    Record();
  }

  
  bool Running() {
    return Enabled() && !start_time_.is_null() && stop_time_.is_null();
  }

  
  virtual void AddTime(base::TimeDelta time) {
    Add(static_cast<int>(time.InMilliseconds()));
  }

 protected:
  
  void Record() {
    AddTime(stop_time_ - start_time_);
  }

  base::TimeTicks start_time_;
  base::TimeTicks stop_time_;
};




class StatsRate : public StatsCounterTimer {
 public:
  
  explicit StatsRate(const char* name)
      : StatsCounterTimer(name),
      counter_(name),
      largest_add_(std::string(" ").append(name).append("MAX").c_str()) {
  }

  virtual void Add(int value) {
    counter_.Increment();
    StatsCounterTimer::Add(value);
    if (value > largest_add_.value())
      largest_add_.Set(value);
  }

 private:
  StatsCounter counter_;
  StatsCounter largest_add_;
};



template<class T> class StatsScope {
 public:
  explicit StatsScope<T>(T& timer)
      : timer_(timer) {
    timer_.Start();
  }

  ~StatsScope() {
    timer_.Stop();
  }

  void Stop() {
    timer_.Stop();
  }

 private:
  T& timer_;
};

#endif  
