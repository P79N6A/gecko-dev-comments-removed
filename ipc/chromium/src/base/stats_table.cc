



#include "base/stats_table.h"

#include "base/logging.h"
#include "base/platform_thread.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/shared_memory.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"
#include "base/thread_local_storage.h"

#if defined(OS_POSIX)
#include "errno.h"
#endif















































COMPILE_ASSERT(sizeof(int)==4, expect_4_byte_ints);

namespace {



const int kTableVersion = 0x13131313;


const char kUnknownName[] = "<unknown>";


inline int AlignOffset(int offset) {
  return (sizeof(int) - (offset % sizeof(int))) % sizeof(int);
}

inline int AlignedSize(int size) {
  return size + AlignOffset(size);
}







struct StatsTableTLSData {
  StatsTable* table;
  int slot;
};

}  




class StatsTablePrivate {
 public:
  
  struct TableHeader {
    int version;
    int size;
    int max_counters;
    int max_threads;
  };

  
  
  static StatsTablePrivate* New(const std::string& name, int size,
                                int max_threads, int max_counters);

  base::SharedMemory* shared_memory() { return &shared_memory_; }

  
  TableHeader* table_header() const { return table_header_; }
  int version() const { return table_header_->version; }
  int size() const { return table_header_->size; }
  int max_counters() const { return table_header_->max_counters; }
  int max_threads() const { return table_header_->max_threads; }

  
  char* thread_name(int slot_id) const {
    return &thread_names_table_[
      (slot_id-1) * (StatsTable::kMaxThreadNameLength)];
  }
  PlatformThreadId* thread_tid(int slot_id) const {
    return &(thread_tid_table_[slot_id-1]);
  }
  int* thread_pid(int slot_id) const {
    return &(thread_pid_table_[slot_id-1]);
  }
  char* counter_name(int counter_id) const {
    return &counter_names_table_[
      (counter_id-1) * (StatsTable::kMaxCounterNameLength)];
  }
  int* row(int counter_id) const {
    return &data_table_[(counter_id-1) * max_threads()];
  }

 private:
  
  StatsTablePrivate() {}

  
  
  void InitializeTable(void* memory, int size, int max_counters,
                       int max_threads);

  
  void ComputeMappedPointers(void* memory);

  base::SharedMemory shared_memory_;
  TableHeader* table_header_;
  char* thread_names_table_;
  PlatformThreadId* thread_tid_table_;
  int* thread_pid_table_;
  char* counter_names_table_;
  int* data_table_;
};


StatsTablePrivate* StatsTablePrivate::New(const std::string& name,
                                          int size,
                                          int max_threads,
                                          int max_counters) {
  scoped_ptr<StatsTablePrivate> priv(new StatsTablePrivate());
#ifdef CHROMIUM_MOZILLA_BUILD
  if (!priv->shared_memory_.Create(name, false, true,
#else
  if (!priv->shared_memory_.Create(base::SysUTF8ToWide(name), false, true,
#endif
                                   size))
    return NULL;
  if (!priv->shared_memory_.Map(size))
    return NULL;
  void* memory = priv->shared_memory_.memory();

  TableHeader* header = static_cast<TableHeader*>(memory);

  
  
  if (header->version != kTableVersion)
    priv->InitializeTable(memory, size, max_counters, max_threads);

  
  priv->ComputeMappedPointers(memory);

  return priv.release();
}

void StatsTablePrivate::InitializeTable(void* memory, int size,
                                        int max_counters,
                                        int max_threads) {
  
  memset(memory, 0, size);

  
  TableHeader* header = static_cast<TableHeader*>(memory);
  header->version = kTableVersion;
  header->size = size;
  header->max_counters = max_counters;
  header->max_threads = max_threads;
}

void StatsTablePrivate::ComputeMappedPointers(void* memory) {
  char* data = static_cast<char*>(memory);
  int offset = 0;

  table_header_ = reinterpret_cast<TableHeader*>(data);
  offset += sizeof(*table_header_);
  offset += AlignOffset(offset);

  
  DCHECK_EQ(table_header_->version, kTableVersion);

  thread_names_table_ = reinterpret_cast<char*>(data + offset);
  offset += sizeof(char) *
            max_threads() * StatsTable::kMaxThreadNameLength;
  offset += AlignOffset(offset);

  thread_tid_table_ = reinterpret_cast<PlatformThreadId*>(data + offset);
  offset += sizeof(int) * max_threads();
  offset += AlignOffset(offset);

  thread_pid_table_ = reinterpret_cast<int*>(data + offset);
  offset += sizeof(int) * max_threads();
  offset += AlignOffset(offset);

  counter_names_table_ = reinterpret_cast<char*>(data + offset);
  offset += sizeof(char) *
            max_counters() * StatsTable::kMaxCounterNameLength;
  offset += AlignOffset(offset);

  data_table_ = reinterpret_cast<int*>(data + offset);
  offset += sizeof(int) * max_threads() * max_counters();

  DCHECK_EQ(offset, size());
}




StatsTable* StatsTable::global_table_ = NULL;

StatsTable::StatsTable(const std::string& name, int max_threads,
                       int max_counters)
    : impl_(NULL),
      tls_index_(SlotReturnFunction) {
  int table_size =
    AlignedSize(sizeof(StatsTablePrivate::TableHeader)) +
    AlignedSize((max_counters * sizeof(char) * kMaxCounterNameLength)) +
    AlignedSize((max_threads * sizeof(char) * kMaxThreadNameLength)) +
    AlignedSize(max_threads * sizeof(int)) +
    AlignedSize(max_threads * sizeof(int)) +
    AlignedSize((sizeof(int) * (max_counters * max_threads)));

  impl_ = StatsTablePrivate::New(name, table_size, max_threads, max_counters);

  
#if defined(OS_WIN)
  if (!impl_)
    LOG(ERROR) << "StatsTable did not initialize:" << GetLastError();
#elif defined(OS_POSIX)
  if (!impl_)
    LOG(ERROR) << "StatsTable did not initialize:" << strerror(errno);
#endif
}

StatsTable::~StatsTable() {
  
  
  UnregisterThread();

  
  
  tls_index_.Free();

  
  delete impl_;

  
  if (global_table_ == this)
    global_table_ = NULL;
}

int StatsTable::RegisterThread(const std::string& name) {
  int slot = 0;

  
  
  
  {
    base::SharedMemoryAutoLock lock(impl_->shared_memory());
    slot = FindEmptyThread();
    if (!slot) {
      return 0;
    }

    DCHECK(impl_);

    
    std::string thread_name = name;
    if (name.empty())
      thread_name = kUnknownName;
    base::strlcpy(impl_->thread_name(slot), thread_name.c_str(),
                  kMaxThreadNameLength);
    *(impl_->thread_tid(slot)) = PlatformThread::CurrentId();
    *(impl_->thread_pid(slot)) = base::GetCurrentProcId();
  }

  
  StatsTableTLSData* data = new StatsTableTLSData;
  data->table = this;
  data->slot = slot;
  tls_index_.Set(data);
  return slot;
}

StatsTableTLSData* StatsTable::GetTLSData() const {
  StatsTableTLSData* data =
    static_cast<StatsTableTLSData*>(tls_index_.Get());
  if (!data)
    return NULL;

  DCHECK(data->slot);
  DCHECK_EQ(data->table, this);
  return data;
}

void StatsTable::UnregisterThread() {
  UnregisterThread(GetTLSData());
}

void StatsTable::UnregisterThread(StatsTableTLSData* data) {
  if (!data)
    return;
  DCHECK(impl_);

  
  char* name = impl_->thread_name(data->slot);
  *name = '\0';

  
  tls_index_.Set(NULL);
  delete data;
}

void StatsTable::SlotReturnFunction(void* data) {
  
  
  
  StatsTableTLSData* tls_data = static_cast<StatsTableTLSData*>(data);
  if (tls_data) {
    DCHECK(tls_data->table);
    tls_data->table->UnregisterThread(tls_data);
  }
}

int StatsTable::CountThreadsRegistered() const {
  if (!impl_)
    return 0;

  
  
  int count = 0;
  for (int index = 1; index <= impl_->max_threads(); index++) {
    char* name = impl_->thread_name(index);
    if (*name != '\0')
      count++;
  }
  return count;
}

int StatsTable::GetSlot() const {
  StatsTableTLSData* data = GetTLSData();
  if (!data)
    return 0;
  return data->slot;
}

int StatsTable::FindEmptyThread() const {
  
  
  
  
  
  
  
  
  if (!impl_)
    return 0;

  int index = 1;
  for (; index <= impl_->max_threads(); index++) {
    char* name = impl_->thread_name(index);
    if (!*name)
      break;
  }
  if (index > impl_->max_threads())
    return 0;  
  return index;
}

int StatsTable::FindCounterOrEmptyRow(const std::string& name) const {
  
  
  
  
  
  
  
  if (!impl_)
    return 0;

  int free_slot = 0;
  for (int index = 1; index <= impl_->max_counters(); index++) {
    char* row_name = impl_->counter_name(index);
    if (!*row_name && !free_slot)
      free_slot = index;  
    else if (!strncmp(row_name, name.c_str(), kMaxCounterNameLength))
      return index;
  }
  return free_slot;
}

int StatsTable::FindCounter(const std::string& name) {
  
  
  
  if (!impl_)
    return 0;

  
  {
    AutoLock scoped_lock(counters_lock_);

    
    CountersMap::const_iterator iter;
    iter = counters_.find(name);
    if (iter != counters_.end())
      return iter->second;
  }

  
  return AddCounter(name);
}

int StatsTable::AddCounter(const std::string& name) {
  DCHECK(impl_);

  if (!impl_)
    return 0;

  int counter_id = 0;
  {
    
    
    base::SharedMemoryAutoLock lock(impl_->shared_memory());

    
    counter_id = FindCounterOrEmptyRow(name);
    if (!counter_id)
      return 0;

    std::string counter_name = name;
    if (name.empty())
      counter_name = kUnknownName;
    base::strlcpy(impl_->counter_name(counter_id), counter_name.c_str(),
                  kMaxCounterNameLength);
  }

  
  {
    AutoLock lock(counters_lock_);
    counters_[name] = counter_id;
  }
  return counter_id;
}

int* StatsTable::GetLocation(int counter_id, int slot_id) const {
  if (!impl_)
    return NULL;
  if (slot_id > impl_->max_threads())
    return NULL;

  int* row = impl_->row(counter_id);
  return &(row[slot_id-1]);
}

const char* StatsTable::GetRowName(int index) const {
  if (!impl_)
    return NULL;

  return impl_->counter_name(index);
}

int StatsTable::GetRowValue(int index, int pid) const {
  if (!impl_)
    return 0;

  int rv = 0;
  int* row = impl_->row(index);
  for (int slot_id = 0; slot_id < impl_->max_threads(); slot_id++) {
    if (pid == 0 || *impl_->thread_pid(slot_id) == pid)
      rv += row[slot_id];
  }
  return rv;
}

int StatsTable::GetRowValue(int index) const {
  return GetRowValue(index, 0);
}

int StatsTable::GetCounterValue(const std::string& name, int pid) {
  if (!impl_)
    return 0;

  int row = FindCounter(name);
  if (!row)
    return 0;
  return GetRowValue(row, pid);
}

int StatsTable::GetCounterValue(const std::string& name) {
  return GetCounterValue(name, 0);
}

int StatsTable::GetMaxCounters() const {
  if (!impl_)
    return 0;
  return impl_->max_counters();
}

int StatsTable::GetMaxThreads() const {
  if (!impl_)
    return 0;
  return impl_->max_threads();
}

int* StatsTable::FindLocation(const char* name) {
  
  StatsTable *table = StatsTable::current();
  if (!table)
    return NULL;

  
  
  int slot = table->GetSlot();
  if (!slot && !(slot = table->RegisterThread("")))
      return NULL;

  
  std::string str_name(name);
  int counter = table->FindCounter(str_name);

  
  return table->GetLocation(counter, slot);
}
