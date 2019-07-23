



#include "base/tracked_objects.h"

#include <math.h>

#include "base/string_util.h"

using base::TimeDelta;

namespace tracked_objects {



TLSSlot ThreadData::tls_index_(base::LINKER_INITIALIZED);




void DeathData::RecordDeath(const TimeDelta& duration) {
  ++count_;
  life_duration_ += duration;
  int64 milliseconds = duration.InMilliseconds();
  square_duration_ += milliseconds * milliseconds;
}

int DeathData::AverageMsDuration() const {
  return static_cast<int>(life_duration_.InMilliseconds() / count_);
}

double DeathData::StandardDeviation() const {
  double average = AverageMsDuration();
  double variance = static_cast<float>(square_duration_)/count_
                    - average * average;
  return sqrt(variance);
}


void DeathData::AddDeathData(const DeathData& other) {
  count_ += other.count_;
  life_duration_ += other.life_duration_;
  square_duration_ += other.square_duration_;
}

void DeathData::Write(std::string* output) const {
  if (!count_)
    return;
  if (1 == count_)
    StringAppendF(output, "(1)Life in %dms ", AverageMsDuration());
  else
    StringAppendF(output, "(%d)Lives %dms/life ", count_, AverageMsDuration());
}

void DeathData::Clear() {
  count_ = 0;
  life_duration_ = TimeDelta();
  square_duration_ = 0;
}



BirthOnThread::BirthOnThread(const Location& location)
    : location_(location),
      birth_thread_(ThreadData::current()) { }


Births::Births(const Location& location)
    : BirthOnThread(location),
      birth_count_(0) { }





ThreadData* ThreadData::first_ = NULL;

Lock ThreadData::list_lock_;


ThreadData::Status ThreadData::status_ = ThreadData::UNINITIALIZED;

ThreadData::ThreadData() : next_(NULL), message_loop_(MessageLoop::current()) {}


ThreadData* ThreadData::current() {
  if (!tls_index_.initialized())
    return NULL;

  ThreadData* registry = static_cast<ThreadData*>(tls_index_.Get());
  if (!registry) {
    
    bool too_late_to_create = false;
    {
      registry = new ThreadData;
      AutoLock lock(list_lock_);
      
      if (!IsActive()) {
        too_late_to_create = true;
      } else {
        
        registry->next_ = first_;
        first_ = registry;
      }
    }  
    if (too_late_to_create) {
      delete registry;
      registry = NULL;
    } else {
      tls_index_.Set(registry);
    }
  }
  return registry;
}


static std::string UnescapeQuery(const std::string& query) {
  std::string result;
  for (size_t i = 0; i < query.size(); i++) {
    char next = query[i];
    if ('%' == next && i + 2 < query.size()) {
      std::string hex = query.substr(i + 1, 2);
      char replacement = '\0';
      
      if (LowerCaseEqualsASCII(hex, "3c"))
        replacement ='<';
      else if (LowerCaseEqualsASCII(hex, "3e"))
        replacement = '>';
      else if (hex == "20")
        replacement = ' ';
      if (replacement) {
        next = replacement;
        i += 2;
      }
    }
    result.push_back(next);
  }
  return result;
}


void ThreadData::WriteHTML(const std::string& query, std::string* output) {
  if (!ThreadData::IsActive())
    return;  

  DCHECK(ThreadData::current());

  output->append("<html><head><title>About Objects");
  std::string escaped_query = UnescapeQuery(query);
  if (!escaped_query.empty())
    output->append(" - " + escaped_query);
  output->append("</title></head><body><pre>");

  DataCollector collected_data;  
  collected_data.AddListOfLivingObjects();  

  
  DataCollector::Collection* collection = collected_data.collection();

  
  Comparator comparator;
  comparator.ParseQuery(escaped_query);

  
  DataCollector::Collection match_array;
  for (DataCollector::Collection::iterator it = collection->begin();
       it != collection->end(); ++it) {
    if (comparator.Acceptable(*it))
      match_array.push_back(*it);
  }

  comparator.Sort(&match_array);

  WriteHTMLTotalAndSubtotals(match_array, comparator, output);

  comparator.Clear();  

  output->append("</pre></body></html>");
}


void ThreadData::WriteHTMLTotalAndSubtotals(
    const DataCollector::Collection& match_array,
    const Comparator& comparator,
    std::string* output) {
  if (!match_array.size()) {
    output->append("There were no tracked matches.");
  } else {
    
    Aggregation totals;
    for (size_t i = 0; i < match_array.size(); ++i) {
      totals.AddDeathSnapshot(match_array[i]);
    }
    output->append("Aggregate Stats: ");
    totals.Write(output);
    output->append("<hr><hr>");

    Aggregation subtotals;
    for (size_t i = 0; i < match_array.size(); ++i) {
      if (0 == i || !comparator.Equivalent(match_array[i - 1],
                                           match_array[i])) {
        
        comparator.WriteSortGrouping(match_array[i], output);
        output->append("<br><br>");
      }
      comparator.WriteSnapshot(match_array[i], output);
      output->append("<br>");
      subtotals.AddDeathSnapshot(match_array[i]);
      if (i + 1 >= match_array.size() ||
          !comparator.Equivalent(match_array[i],
                                 match_array[i + 1])) {
        
        output->append("<br>");
        subtotals.Write(output);
        output->append("<br><hr><br>");
        subtotals.Clear();
      }
    }
  }
}

Births* ThreadData::FindLifetime(const Location& location) {
  if (!message_loop_)  
    message_loop_ = MessageLoop::current();  

  BirthMap::iterator it = birth_map_.find(location);
  if (it != birth_map_.end())
    return it->second;
  Births* tracker = new Births(location);

  
  
  AutoLock lock(lock_);
  birth_map_[location] = tracker;
  return tracker;
}

void ThreadData::TallyADeath(const Births& lifetimes,
                             const TimeDelta& duration) {
  if (!message_loop_)  
    message_loop_ = MessageLoop::current();  

  DeathMap::iterator it = death_map_.find(&lifetimes);
  if (it != death_map_.end()) {
    it->second.RecordDeath(duration);
    return;
  }

  AutoLock lock(lock_);  
  death_map_[&lifetimes].RecordDeath(duration);
}


ThreadData* ThreadData::first() {
  AutoLock lock(list_lock_);
  return first_;
}

const std::string ThreadData::ThreadName() const {
  if (message_loop_)
    return message_loop_->thread_name();
  return "ThreadWithoutMessageLoop";
}


void ThreadData::SnapshotBirthMap(BirthMap *output) const {
  AutoLock lock(*const_cast<Lock*>(&lock_));
  for (BirthMap::const_iterator it = birth_map_.begin();
       it != birth_map_.end(); ++it)
    (*output)[it->first] = it->second;
}


void ThreadData::SnapshotDeathMap(DeathMap *output) const {
  AutoLock lock(*const_cast<Lock*>(&lock_));
  for (DeathMap::const_iterator it = death_map_.begin();
       it != death_map_.end(); ++it)
    (*output)[it->first] = it->second;
}

#ifdef OS_WIN
void ThreadData::RunOnAllThreads(void (*function)()) {
  ThreadData* list = first();  

  std::vector<MessageLoop*> message_loops;
  for (ThreadData* it = list; it; it = it->next()) {
    if (current() != it && it->message_loop())
      message_loops.push_back(it->message_loop());
  }

  ThreadSafeDownCounter* counter =
    new ThreadSafeDownCounter(message_loops.size() + 1);  

  HANDLE completion_handle = CreateEvent(NULL, false, false, NULL);
  
  for (size_t i = 0; i < message_loops.size(); ++i)
    message_loops[i]->PostTask(FROM_HERE,
        new RunTheStatic(function, completion_handle, counter));

  
  RunTheStatic local_task(function, completion_handle, counter);
  local_task.Run();

  WaitForSingleObject(completion_handle, INFINITE);
  int ret_val = CloseHandle(completion_handle);
  DCHECK(ret_val);
}
#endif


bool ThreadData::StartTracking(bool status) {
#ifndef TRACK_ALL_TASK_OBJECTS
  return false;  
#endif

  if (!status) {
    AutoLock lock(list_lock_);
    DCHECK(status_ == ACTIVE || status_ == SHUTDOWN);
    status_ = SHUTDOWN;
    return true;
  }
  AutoLock lock(list_lock_);
  DCHECK(status_ == UNINITIALIZED);
  CHECK(tls_index_.Initialize(NULL));
  status_ = ACTIVE;
  return true;
}


bool ThreadData::IsActive() {
  return status_ == ACTIVE;
}

#ifdef OS_WIN

void ThreadData::ShutdownMultiThreadTracking() {
  
  if (!StartTracking(false))
    return;

  RunOnAllThreads(ShutdownDisablingFurtherTracking);

  
  
  
  
  
  
  return;
}
#endif


void ThreadData::ShutdownSingleThreadedCleanup() {
  
  if (!StartTracking(false))
    return;
  ThreadData* thread_data_list;
  {
    AutoLock lock(list_lock_);
    thread_data_list = first_;
    first_ = NULL;
  }

  while (thread_data_list) {
    ThreadData* next_thread_data = thread_data_list;
    thread_data_list = thread_data_list->next();

    for (BirthMap::iterator it = next_thread_data->birth_map_.begin();
         next_thread_data->birth_map_.end() != it; ++it)
      delete it->second;  
    next_thread_data->birth_map_.clear();
    next_thread_data->death_map_.clear();
    delete next_thread_data;  
  }

  CHECK(tls_index_.initialized());
  tls_index_.Free();
  DCHECK(!tls_index_.initialized());
  status_ = UNINITIALIZED;
}


void ThreadData::ShutdownDisablingFurtherTracking() {
  
  if (!StartTracking(false))
    return;
}




ThreadData::ThreadSafeDownCounter::ThreadSafeDownCounter(size_t count)
    : remaining_count_(count) {
  DCHECK(remaining_count_ > 0);
}

bool ThreadData::ThreadSafeDownCounter::LastCaller() {
  {
    AutoLock lock(lock_);
    if (--remaining_count_)
      return false;
  }  
  delete this;
  return true;
}


#ifdef OS_WIN
ThreadData::RunTheStatic::RunTheStatic(FunctionPointer function,
                                       HANDLE completion_handle,
                                       ThreadSafeDownCounter* counter)
    : function_(function),
      completion_handle_(completion_handle),
      counter_(counter) {
}

void ThreadData::RunTheStatic::Run() {
  function_();
  if (counter_->LastCaller())
    SetEvent(completion_handle_);
}
#endif





Snapshot::Snapshot(const BirthOnThread& birth_on_thread,
                   const ThreadData& death_thread,
                   const DeathData& death_data)
    : birth_(&birth_on_thread),
      death_thread_(&death_thread),
      death_data_(death_data) {
}

Snapshot::Snapshot(const BirthOnThread& birth_on_thread, int count)
    : birth_(&birth_on_thread),
      death_thread_(NULL),
      death_data_(DeathData(count)) {
}

const std::string Snapshot::DeathThreadName() const {
  if (death_thread_)
    return death_thread_->ThreadName();
  return "Still_Alive";
}

void Snapshot::Write(std::string* output) const {
  death_data_.Write(output);
  StringAppendF(output, "%s->%s ",
                birth_->birth_thread()->ThreadName().c_str(),
                death_thread_->ThreadName().c_str());
  birth_->location().Write(true, true, output);
}

void Snapshot::Add(const Snapshot& other) {
  death_data_.AddDeathData(other.death_data_);
}




DataCollector::DataCollector() {
  DCHECK(ThreadData::IsActive());

  ThreadData* my_list = ThreadData::current()->first();

  count_of_contributing_threads_ = 0;
  for (ThreadData* thread_data = my_list;
       thread_data;
       thread_data = thread_data->next()) {
    ++count_of_contributing_threads_;
  }

  
  
  for (ThreadData* thread_data = my_list;
       thread_data;
       thread_data = thread_data->next()) {
    Append(*thread_data);
  }
}

void DataCollector::Append(const ThreadData& thread_data) {
  
  ThreadData::BirthMap birth_map;
  thread_data.SnapshotBirthMap(&birth_map);
  ThreadData::DeathMap death_map;
  thread_data.SnapshotDeathMap(&death_map);

  
  AutoLock lock(accumulation_lock_);

  DCHECK(count_of_contributing_threads_);

  for (ThreadData::DeathMap::const_iterator it = death_map.begin();
       it != death_map.end(); ++it) {
    collection_.push_back(Snapshot(*it->first, thread_data, it->second));
    global_birth_count_[it->first] -= it->first->birth_count();
  }

  for (ThreadData::BirthMap::const_iterator it = birth_map.begin();
       it != birth_map.end(); ++it) {
    global_birth_count_[it->second] += it->second->birth_count();
  }

  --count_of_contributing_threads_;
}

DataCollector::Collection* DataCollector::collection() {
  DCHECK(!count_of_contributing_threads_);
  return &collection_;
}

void DataCollector::AddListOfLivingObjects() {
  DCHECK(!count_of_contributing_threads_);
  for (BirthCount::iterator it = global_birth_count_.begin();
       it != global_birth_count_.end(); ++it) {
    if (it->second > 0)
      collection_.push_back(Snapshot(*it->first, it->second));
  }
}




void Aggregation::AddDeathSnapshot(const Snapshot& snapshot) {
  AddBirth(snapshot.birth());
  death_threads_[snapshot.death_thread()]++;
  AddDeathData(snapshot.death_data());
}

void Aggregation::AddBirths(const Births& births) {
  AddBirth(births);
  birth_count_ += births.birth_count();
}
void Aggregation::AddBirth(const BirthOnThread& birth) {
  AddBirthPlace(birth.location());
  birth_threads_[birth.birth_thread()]++;
}

void Aggregation::AddBirthPlace(const Location& location) {
  locations_[location]++;
  birth_files_[location.file_name()]++;
}

void Aggregation::Write(std::string* output) const {
  if (locations_.size() == 1) {
    locations_.begin()->first.Write(true, true, output);
  } else {
    StringAppendF(output, "%d Locations. ", locations_.size());
    if (birth_files_.size() > 1)
      StringAppendF(output, "%d Files. ", birth_files_.size());
    else
      StringAppendF(output, "All born in %s. ",
                    birth_files_.begin()->first.c_str());
  }

  if (birth_threads_.size() > 1)
    StringAppendF(output, "%d BirthingThreads. ", birth_threads_.size());
  else
    StringAppendF(output, "All born on %s. ",
                  birth_threads_.begin()->first->ThreadName().c_str());

  if (death_threads_.size() > 1) {
    StringAppendF(output, "%d DeathThreads. ", death_threads_.size());
  } else {
    if (death_threads_.begin()->first)
      StringAppendF(output, "All deleted on %s. ",
                  death_threads_.begin()->first->ThreadName().c_str());
    else
      output->append("All these objects are still alive.");
  }

  if (birth_count_ > 1)
    StringAppendF(output, "Births=%d ", birth_count_);

  DeathData::Write(output);
}

void Aggregation::Clear() {
  birth_count_ = 0;
  birth_files_.clear();
  locations_.clear();
  birth_threads_.clear();
  DeathData::Clear();
  death_threads_.clear();
}




Comparator::Comparator()
    : selector_(NIL),
      tiebreaker_(NULL),
      combined_selectors_(0),
      use_tiebreaker_for_sort_only_(false) {}

void Comparator::Clear() {
  if (tiebreaker_) {
    tiebreaker_->Clear();
    delete tiebreaker_;
    tiebreaker_ = NULL;
  }
  use_tiebreaker_for_sort_only_ = false;
  selector_ = NIL;
}

void Comparator::Sort(DataCollector::Collection* collection) const {
  std::sort(collection->begin(), collection->end(), *this);
}


bool Comparator::operator()(const Snapshot& left,
                            const Snapshot& right) const {
  switch (selector_) {
    case BIRTH_THREAD:
      if (left.birth_thread() != right.birth_thread() &&
          left.birth_thread()->ThreadName() !=
          right.birth_thread()->ThreadName())
        return left.birth_thread()->ThreadName() <
            right.birth_thread()->ThreadName();
      break;

    case DEATH_THREAD:
      if (left.death_thread() != right.death_thread() &&
          left.DeathThreadName() !=
          right.DeathThreadName()) {
        if (!left.death_thread())
          return true;
        if (!right.death_thread())
          return false;
        return left.DeathThreadName() <
             right.DeathThreadName();
      }
      break;

    case BIRTH_FILE:
      if (left.location().file_name() != right.location().file_name()) {
        int comp = strcmp(left.location().file_name(),
                          right.location().file_name());
        if (comp)
          return 0 > comp;
      }
      break;

    case BIRTH_FUNCTION:
      if (left.location().function_name() != right.location().function_name()) {
        int comp = strcmp(left.location().function_name(),
                          right.location().function_name());
        if (comp)
          return 0 > comp;
      }
      break;

    case BIRTH_LINE:
      if (left.location().line_number() != right.location().line_number())
        return left.location().line_number() <
            right.location().line_number();
      break;

    case COUNT:
      if (left.count() != right.count())
        return left.count() > right.count();  
      break;

    case AVERAGE_DURATION:
      if (left.AverageMsDuration() != right.AverageMsDuration())
        return left.AverageMsDuration() > right.AverageMsDuration();
      break;

    default:
      break;
  }
  if (tiebreaker_)
    return tiebreaker_->operator()(left, right);
  return false;
}

bool Comparator::Equivalent(const Snapshot& left,
                            const Snapshot& right) const {
  switch (selector_) {
    case BIRTH_THREAD:
      if (left.birth_thread() != right.birth_thread() &&
          left.birth_thread()->ThreadName() !=
              right.birth_thread()->ThreadName())
        return false;
      break;

    case DEATH_THREAD:
      if (left.death_thread() != right.death_thread() &&
          left.DeathThreadName() != right.DeathThreadName())
        return false;
      break;

    case BIRTH_FILE:
      if (left.location().file_name() != right.location().file_name()) {
        int comp = strcmp(left.location().file_name(),
                          right.location().file_name());
        if (comp)
          return false;
      }
      break;

    case BIRTH_FUNCTION:
      if (left.location().function_name() != right.location().function_name()) {
        int comp = strcmp(left.location().function_name(),
                          right.location().function_name());
        if (comp)
          return false;
      }
      break;

    case COUNT:
      if (left.count() != right.count())
        return false;
      break;

    case AVERAGE_DURATION:
      if (left.life_duration() != right.life_duration())
        return false;
      break;

    default:
      break;
  }
  if (tiebreaker_ && !use_tiebreaker_for_sort_only_)
    return tiebreaker_->Equivalent(left, right);
  return true;
}

bool Comparator::Acceptable(const Snapshot& sample) const {
  if (required_.size()) {
    switch (selector_) {
      case BIRTH_THREAD:
        if (sample.birth_thread()->ThreadName().find(required_) ==
            std::string::npos)
          return false;
        break;

      case DEATH_THREAD:
        if (sample.DeathThreadName().find(required_) == std::string::npos)
          return false;
        break;

      case BIRTH_FILE:
        if (!strstr(sample.location().file_name(), required_.c_str()))
          return false;
        break;

      case BIRTH_FUNCTION:
        if (!strstr(sample.location().function_name(), required_.c_str()))
          return false;
        break;

      default:
        break;
    }
  }
  if (tiebreaker_ && !use_tiebreaker_for_sort_only_)
    return tiebreaker_->Acceptable(sample);
  return true;
}

void Comparator::SetTiebreaker(Selector selector, const std::string required) {
  if (selector == selector_ || NIL == selector)
    return;
  combined_selectors_ |= selector;
  if (NIL == selector_) {
    selector_ = selector;
    if (required.size())
      required_ = required;
    return;
  }
  if (tiebreaker_) {
    if (use_tiebreaker_for_sort_only_) {
      Comparator* temp = new Comparator;
      temp->tiebreaker_ = tiebreaker_;
      tiebreaker_ = temp;
    }
  } else {
    tiebreaker_ = new Comparator;
    DCHECK(!use_tiebreaker_for_sort_only_);
  }
  tiebreaker_->SetTiebreaker(selector, required);
}

bool Comparator::IsGroupedBy(Selector selector) const {
  return 0 != (selector & combined_selectors_);
}

void Comparator::SetSubgroupTiebreaker(Selector selector) {
  if (selector == selector_ || NIL == selector)
    return;
  if (!tiebreaker_) {
    use_tiebreaker_for_sort_only_ = true;
    tiebreaker_ = new Comparator;
    tiebreaker_->SetTiebreaker(selector, "");
  } else {
    tiebreaker_->SetSubgroupTiebreaker(selector);
  }
}

void Comparator::ParseKeyphrase(const std::string key_phrase) {
  static std::map<const std::string, Selector> key_map;
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    key_map["count"]    = COUNT;
    key_map["duration"] = AVERAGE_DURATION;
    key_map["birth"]    = BIRTH_THREAD;
    key_map["death"]    = DEATH_THREAD;
    key_map["file"]     = BIRTH_FILE;
    key_map["function"] = BIRTH_FUNCTION;
    key_map["line"]     = BIRTH_LINE;
  }

  std::string required;
  size_t equal_offset = key_phrase.find('=', 0);
  if (key_phrase.npos != equal_offset)
    required = key_phrase.substr(equal_offset + 1, key_phrase.npos);
  std::string keyword(key_phrase.substr(0, equal_offset));
  keyword = StringToLowerASCII(keyword);
  if (key_map.end() == key_map.find(keyword))
    return;
  SetTiebreaker(key_map[keyword], required);
}

bool Comparator::ParseQuery(const std::string query) {
  for (size_t i = 0; i < query.size();) {
    size_t slash_offset = query.find('/', i);
    ParseKeyphrase(query.substr(i, slash_offset - i));
    if (query.npos == slash_offset)
      break;
    i = slash_offset + 1;
  }

  
  SetSubgroupTiebreaker(COUNT);
  SetSubgroupTiebreaker(AVERAGE_DURATION);
  SetSubgroupTiebreaker(BIRTH_THREAD);
  SetSubgroupTiebreaker(DEATH_THREAD);
  SetSubgroupTiebreaker(BIRTH_FUNCTION);
  SetSubgroupTiebreaker(BIRTH_FILE);
  SetSubgroupTiebreaker(BIRTH_LINE);

  return true;
}

bool Comparator::WriteSortGrouping(const Snapshot& sample,
                                       std::string* output) const {
  bool wrote_data = false;
  switch (selector_) {
    case BIRTH_THREAD:
      StringAppendF(output, "All new on %s ",
                    sample.birth_thread()->ThreadName().c_str());
      wrote_data = true;
      break;

    case DEATH_THREAD:
      if (sample.death_thread())
        StringAppendF(output, "All deleted on %s ",
                      sample.DeathThreadName().c_str());
      else
        output->append("All still alive ");
      wrote_data = true;
      break;

    case BIRTH_FILE:
      StringAppendF(output, "All born in %s ",
                    sample.location().file_name());
      break;

    case BIRTH_FUNCTION:
      output->append("All born in ");
      sample.location().WriteFunctionName(output);
      output->push_back(' ');
      break;

    default:
      break;
  }
  if (tiebreaker_ && !use_tiebreaker_for_sort_only_) {
    wrote_data |= tiebreaker_->WriteSortGrouping(sample, output);
  }
  return wrote_data;
}

void Comparator::WriteSnapshot(const Snapshot& sample,
                               std::string* output) const {
  sample.death_data().Write(output);
  if (!(combined_selectors_ & BIRTH_THREAD) ||
      !(combined_selectors_ & DEATH_THREAD))
    StringAppendF(output, "%s->%s ",
                  (combined_selectors_ & BIRTH_THREAD) ? "*" :
                    sample.birth().birth_thread()->ThreadName().c_str(),
                  (combined_selectors_ & DEATH_THREAD) ? "*" :
                    sample.DeathThreadName().c_str());
  sample.birth().location().Write(!(combined_selectors_ & BIRTH_FILE),
                                  !(combined_selectors_ & BIRTH_FUNCTION),
                                  output);
}

}  
