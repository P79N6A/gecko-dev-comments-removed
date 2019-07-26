





#ifndef DMD_h___
#define DMD_h___

#include <stdarg.h>
#include <string.h>

#include "mozilla/Types.h"

namespace mozilla {
namespace dmd {


MOZ_EXPORT void
Report(const void* aPtr, const char* aReporterName);


MOZ_EXPORT void
ReportOnAlloc(const void* aPtr, const char* aReporterName);

class Writer
{
public:
  typedef void (*WriterFun)(void* aWriteState, const char* aFmt, va_list aAp);

  Writer(WriterFun aWriterFun, void* aWriteState)
    : mWriterFun(aWriterFun), mWriteState(aWriteState)
  {}

  void Write(const char* aFmt, ...) const;

private:
  WriterFun mWriterFun;
  void*     mWriteState;
};




MOZ_EXPORT void
Dump(Writer aWriter);



MOZ_EXPORT void
FpWrite(void* aFp, const char* aFmt, va_list aAp);

struct Sizes
{
  size_t mStackTraces;
  size_t mStackTraceTable;
  size_t mLiveBlockTable;
  size_t mDoubleReportTable;

  Sizes() { Clear(); }
  void Clear() { memset(this, 0, sizeof(Sizes)); }
};



MOZ_EXPORT void
SizeOf(Sizes* aSizes);

} 
} 

#endif 
