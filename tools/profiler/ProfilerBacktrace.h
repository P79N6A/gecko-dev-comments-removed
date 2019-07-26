





#ifndef __PROFILER_BACKTRACE_H
#define __PROFILER_BACKTRACE_H

class SyncProfile;

class ProfilerBacktrace
{
public:
  ProfilerBacktrace(SyncProfile* aProfile);
  ~ProfilerBacktrace();

  template<typename Builder> void
  BuildJSObject(Builder& aObjBuilder, typename Builder::ObjectHandle aScope);

private:
  ProfilerBacktrace(const ProfilerBacktrace&);
  ProfilerBacktrace& operator=(const ProfilerBacktrace&);

  SyncProfile*  mProfile;
};

#endif 

