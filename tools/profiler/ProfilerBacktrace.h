





#ifndef __PROFILER_BACKTRACE_H
#define __PROFILER_BACKTRACE_H

class SyncProfile;

class ProfilerBacktrace
{
public:
  explicit ProfilerBacktrace(SyncProfile* aProfile);
  ~ProfilerBacktrace();

  void StreamJSObject(JSStreamWriter& b);

private:
  ProfilerBacktrace(const ProfilerBacktrace&);
  ProfilerBacktrace& operator=(const ProfilerBacktrace&);

  SyncProfile*  mProfile;
};

#endif 

