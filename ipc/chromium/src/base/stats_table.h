


















#ifndef BASE_STATS_TABLE_H__
#define BASE_STATS_TABLE_H__

#include <string>
#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/lock.h"
#include "base/thread_local_storage.h"

class StatsTablePrivate;

namespace {
struct StatsTableTLSData;
}

class StatsTable {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  StatsTable(const std::string& name, int max_threads, int max_counters);

  
  
  ~StatsTable();

  
  
  static StatsTable* current() { return global_table_; }

  
  static void set_current(StatsTable* value) { global_table_ = value; }

  
  
  int GetSlot() const;

  
  
  
  
  
  
  
  
  
  
  
  int RegisterThread(const std::string& name);

  
  
  int CountThreadsRegistered() const;

  
  
  
  
  
  
  int FindCounter(const std::string& name);

  

  
  
  int* GetLocation(int counter_id, int slot_id) const;

  
  
  const char* GetRowName(int index) const;

  
  int GetRowValue(int index) const;

  
  int GetRowValue(int index, int pid) const;

  
  
  int GetCounterValue(const std::string& name);

  
  
  int GetCounterValue(const std::string& name, int pid);

  
  int GetMaxCounters() const;

  
  int GetMaxThreads() const;

  
  
  static const int kMaxThreadNameLength = 32;

  
  
  static const int kMaxCounterNameLength = 32;

  
  
  
  static int* FindLocation(const char *name);

 private:
  
  
  
  
  void UnregisterThread();

  
  
  void UnregisterThread(StatsTableTLSData* tls_data);

  
  
  static void SlotReturnFunction(void* data);

  
  
  
  int FindEmptyThread() const;

  
  
  
  int FindCounterOrEmptyRow(const std::string& name) const;

  
  
  
  
  
  
  
  
  int AddCounter(const std::string& name);

  
  
  StatsTableTLSData* GetTLSData() const;

  typedef base::hash_map<std::string, int> CountersMap;

  StatsTablePrivate* impl_;
  
  Lock counters_lock_;
  
  
  
  
  
  CountersMap counters_;
  TLSSlot tls_index_;

  static StatsTable* global_table_;

  DISALLOW_EVIL_CONSTRUCTORS(StatsTable);
};

#endif  
