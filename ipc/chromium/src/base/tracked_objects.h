



#ifndef BASE_TRACKED_OBJECTS_H_
#define BASE_TRACKED_OBJECTS_H_


#include <map>
#include <string>
#include <vector>

#include "base/lock.h"
#include "base/message_loop.h"
#include "base/thread_local_storage.h"
#include "base/tracked.h"


namespace tracked_objects {




class ThreadData;
class BirthOnThread {
 public:
  explicit BirthOnThread(const Location& location);

  const Location location() const { return location_; }
  const ThreadData* birth_thread() const { return birth_thread_; }

 private:
  
  
  
  const Location location_;

  
  
  const ThreadData* birth_thread_;  

  DISALLOW_COPY_AND_ASSIGN(BirthOnThread);
};




class Births: public BirthOnThread {
 public:
  explicit Births(const Location& location);

  int birth_count() const { return birth_count_; }

  
  void RecordBirth() { ++birth_count_; }

  
  
  void ForgetBirth() { --birth_count_; }  

 private:
  
  int birth_count_;

  DISALLOW_COPY_AND_ASSIGN(Births);
};






class DeathData {
 public:
  
  DeathData() : count_(0), square_duration_(0) {}

  
  
  
  explicit DeathData(int count) : count_(count), square_duration_(0) {}

  void RecordDeath(const base::TimeDelta& duration);

  
  int count() const { return count_; }
  base::TimeDelta life_duration() const { return life_duration_; }
  int64 square_duration() const { return square_duration_; }
  int AverageMsDuration() const;
  double StandardDeviation() const;

  
  void AddDeathData(const DeathData& other);

  
  void Write(std::string* output) const;

  void Clear();

 private:
  int count_;                
  base::TimeDelta life_duration_;    
  int64 square_duration_;  
};








class Snapshot {
 public:
  
  Snapshot(const BirthOnThread& birth_on_thread, const ThreadData& death_thread,
           const DeathData& death_data);

  
  Snapshot(const BirthOnThread& birth_on_thread, int count);


  const ThreadData* birth_thread() const { return birth_->birth_thread(); }
  const Location location() const { return birth_->location(); }
  const BirthOnThread& birth() const { return *birth_; }
  const ThreadData* death_thread() const {return death_thread_; }
  const DeathData& death_data() const { return death_data_; }
  const std::string DeathThreadName() const;

  int count() const { return death_data_.count(); }
  base::TimeDelta life_duration() const { return death_data_.life_duration(); }
  int64 square_duration() const { return death_data_.square_duration(); }
  int AverageMsDuration() const { return death_data_.AverageMsDuration(); }

  void Write(std::string* output) const;

  void Add(const Snapshot& other);

 private:
  const BirthOnThread* birth_;  
  const ThreadData* death_thread_;
  DeathData death_data_;
};





class DataCollector {
 public:
  typedef std::vector<Snapshot> Collection;

  
  
  DataCollector();

  
  
  
  void Append(const ThreadData& thread_data);

  
  Collection* collection();

  
  
  void AddListOfLivingObjects();

 private:
  
  
  
  
  int count_of_contributing_threads_;

  
  Collection collection_;

  
  
  typedef std::map<const BirthOnThread*, int> BirthCount;
  BirthCount global_birth_count_;

  Lock accumulation_lock_;  

  DISALLOW_COPY_AND_ASSIGN(DataCollector);
};





class Aggregation: public DeathData {
 public:
  Aggregation() : birth_count_(0) {}

  void AddDeathSnapshot(const Snapshot& snapshot);
  void AddBirths(const Births& births);
  void AddBirth(const BirthOnThread& birth);
  void AddBirthPlace(const Location& location);
  void Write(std::string* output) const;
  void Clear();

 private:
  int birth_count_;
  std::map<std::string, int> birth_files_;
  std::map<Location, int> locations_;
  std::map<const ThreadData*, int> birth_threads_;
  DeathData death_data_;
  std::map<const ThreadData*, int> death_threads_;

  DISALLOW_COPY_AND_ASSIGN(Aggregation);
};







class Comparator {
 public:
  enum Selector {
    NIL = 0,
    BIRTH_THREAD = 1,
    DEATH_THREAD = 2,
    BIRTH_FILE = 4,
    BIRTH_FUNCTION = 8,
    BIRTH_LINE = 16,
    COUNT = 32,
    AVERAGE_DURATION = 64,
    TOTAL_DURATION = 128,
  };

  explicit Comparator();

  
  
  
  
  
  void Clear();

  
  bool operator()(const Snapshot& left, const Snapshot& right) const;

  void Sort(DataCollector::Collection* collection) const;

  
  bool Equivalent(const Snapshot& left, const Snapshot& right) const;

  
  bool Acceptable(const Snapshot& sample) const;

  
  
  
  void SetTiebreaker(Selector selector, const std::string required);

  
  
  
  bool IsGroupedBy(Selector selector) const;

  
  
  
  void SetSubgroupTiebreaker(Selector selector);

  
  void ParseKeyphrase(const std::string key_phrase);

  
  bool ParseQuery(const std::string query);

  
  
  
  bool WriteSortGrouping(const Snapshot& sample, std::string* output) const;

  
  void WriteSnapshot(const Snapshot& sample, std::string* output) const;

 private:
  
  
  enum Selector selector_;

  
  
  std::string required_;

  
  
  Comparator* tiebreaker_;

  
  
  
  int combined_selectors_;

  
  
  
  
  bool use_tiebreaker_for_sort_only_;
};






class ThreadData {
 public:
  typedef std::map<Location, Births*> BirthMap;
  typedef std::map<const Births*, DeathData> DeathMap;

  ThreadData();

  
  
  
  
  
  static ThreadData* current();

  
  
  static void WriteHTML(const std::string& query, std::string* output);

  
  
  static void WriteHTMLTotalAndSubtotals(
      const DataCollector::Collection& match_array,
      const Comparator& comparator, std::string* output);

  
  Births* FindLifetime(const Location& location);

  
  void TallyADeath(const Births& lifetimes, const base::TimeDelta& duration);

  
  static ThreadData* first();
  
  ThreadData* next() const { return next_; }

  MessageLoop* message_loop() const { return message_loop_; }
  const std::string ThreadName() const;

  
  
  void SnapshotBirthMap(BirthMap *output) const;
  void SnapshotDeathMap(DeathMap *output) const;

  static void RunOnAllThreads(void (*Func)());

  
  
  
  static bool StartTracking(bool status);
  static bool IsActive();

#ifdef OS_WIN
  
  
  
  
  
  
  
  
  
  
  static void ShutdownMultiThreadTracking();
#endif

  
  
  
  
  
  static void ShutdownSingleThreadedCleanup();

 private:
  
  
  enum Status {
    UNINITIALIZED,
    ACTIVE,
    SHUTDOWN,
  };

  
  
  
  class ThreadSafeDownCounter {
   public:
    
    explicit ThreadSafeDownCounter(size_t count);

    
    
    bool LastCaller();

   private:
    size_t remaining_count_;
    Lock lock_;  
  };

#ifdef OS_WIN
  
  
  
  class RunTheStatic : public Task {
   public:
    typedef void (*FunctionPointer)();
    RunTheStatic(FunctionPointer function,
                 HANDLE completion_handle,
                 ThreadSafeDownCounter* counter);
    
    void Run();

   private:
    FunctionPointer function_;
    HANDLE completion_handle_;
    
    ThreadSafeDownCounter* counter_;

    DISALLOW_COPY_AND_ASSIGN(RunTheStatic);
  };
#endif

  
  
  
  
  static void ShutdownDisablingFurtherTracking();

  
  static TLSSlot tls_index_ ;

  
  static ThreadData* first_;
  
  static Lock list_lock_;


  
  
  
  
  static Status status_;

  
  
  
  ThreadData* next_;

  
  
  
  MessageLoop* message_loop_;

  
  
  
  
  BirthMap birth_map_;

  
  
  DeathMap death_map_;

  
  
  
  
  
  Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(ThreadData);
};








class AutoTracking {
 public:
  AutoTracking() { ThreadData::StartTracking(true); }

  ~AutoTracking() {
#ifndef NDEBUG  
    
    
    
    
    ThreadData::ShutdownSingleThreadedCleanup();
#endif
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoTracking);
};


}  

#endif
