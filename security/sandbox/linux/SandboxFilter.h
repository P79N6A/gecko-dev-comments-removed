





#ifndef mozilla_SandboxFilter_h
#define mozilla_SandboxFilter_h

struct sock_fprog;
struct sock_filter;

namespace mozilla {
  class SandboxFilter {
    sock_filter *mFilter;
    sock_fprog *mProg;
    const sock_fprog **mStored;
  public:
  
  
  
    SandboxFilter(const sock_fprog** aStored, bool aVerbose = false);
    ~SandboxFilter();
  };
}

#endif
