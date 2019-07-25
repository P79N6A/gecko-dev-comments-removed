







































#include "mozilla/Assertions.h"



extern "C" {

MOZ_EXPORT_API(void)
MOZ_Assert(const char* s, const char* file, int ln)
{
  MOZ_OutputAssertMessage(s, file, ln);
  MOZ_CRASH();
}

}
