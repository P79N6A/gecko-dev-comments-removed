





#ifndef DMD_h___
#define DMD_h___

#include <string.h>
#include <stdarg.h>

#include "mozilla/DebugOnly.h"
#include "mozilla/Move.h"
#include "mozilla/Types.h"
#include "mozilla/UniquePtr.h"

#include "replace_malloc_bridge.h"

namespace mozilla {

class JSONWriteFunc;

namespace dmd {

struct Sizes
{
  size_t mStackTracesUsed;
  size_t mStackTracesUnused;
  size_t mStackTraceTable;
  size_t mLiveBlockTable;
  size_t mDeadBlockTable;

  Sizes() { Clear(); }
  void Clear() { memset(this, 0, sizeof(Sizes)); }
};




struct DMDFuncs
{
  virtual void Report(const void*);

  virtual void ReportOnAlloc(const void*);

  virtual void ClearReports();

  virtual void Analyze(UniquePtr<JSONWriteFunc>);

  virtual void SizeOf(Sizes*);

  virtual void StatusMsg(const char*, va_list);

  virtual void ResetEverything(const char*);

#ifndef REPLACE_MALLOC_IMPL
  
  
  
  
  
  
  
  
  
  
  
  
  
  static DMDFuncs* Get() { return sSingleton.Get(); }

private:
  
  
  
  
  
  class Singleton
  {
  public:
    Singleton() : mValue(ReplaceMalloc::GetDMDFuncs()), mInitialized(true) {}

    DMDFuncs* Get()
    {
      MOZ_ASSERT(mInitialized);
      return mValue;
    }

  private:
    DMDFuncs* mValue;
    DebugOnly<bool> mInitialized;
  };

  
  
  static Singleton sSingleton;
#endif
};

#ifndef REPLACE_MALLOC_IMPL

inline void
Report(const void* aPtr)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->Report(aPtr);
  }
}


inline void
ReportOnAlloc(const void* aPtr)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->ReportOnAlloc(aPtr);
  }
}







inline void
ClearReports()
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->ClearReports();
  }
}




























































































































template <typename JSONWriteFunc>
inline void
Analyze(UniquePtr<JSONWriteFunc> aWriteFunc)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->Analyze(Move(aWriteFunc));
  }
}



inline void
SizeOf(Sizes* aSizes)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->SizeOf(aSizes);
  }
}


inline void
StatusMsg(const char* aFmt, ...)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    va_list ap;
    va_start(ap, aFmt);
    funcs->StatusMsg(aFmt, ap);
    va_end(ap);
  }
}


inline bool
IsRunning()
{
  return !!DMDFuncs::Get();
}




inline void
ResetEverything(const char* aOptions)
{
  DMDFuncs* funcs = DMDFuncs::Get();
  if (funcs) {
    funcs->ResetEverything(aOptions);
  }
}
#endif

} 
} 

#endif 
