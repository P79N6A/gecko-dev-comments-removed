





#ifndef __PROFILER_BACKTRACE_H
#define __PROFILER_BACKTRACE_H

class SyncProfile;
class SpliceableJSONWriter;
class UniqueStacks;

class ProfilerBacktrace
{
public:
  explicit ProfilerBacktrace(SyncProfile* aProfile);
  ~ProfilerBacktrace();

  
  
  
  
  
  
  void StreamJSON(SpliceableJSONWriter& aWriter, UniqueStacks& aUniqueStacks);

private:
  ProfilerBacktrace(const ProfilerBacktrace&);
  ProfilerBacktrace& operator=(const ProfilerBacktrace&);

  SyncProfile*  mProfile;
};

#endif 

