





#ifndef mozilla_MemoryInfoDumper_h
#define mozilla_MemoryInfoDumper_h

#include "nsString.h"
#include "mozilla/StandardInteger.h"
class nsIGZFileWriter;

namespace mozilla {








class MemoryInfoDumper
{
public:
  static void Initialize();

  





























































































  static void
  DumpMemoryReportsToFile(const nsAString& aIdentifier,
                          bool aMinimizeMemoryUsage,
                          bool aDumpChildProcesses);

  















  static void
  DumpGCAndCCLogsToFile(const nsAString& aIdentifier,
                        bool aDumpChildProcesses);

private:
  static nsresult
  DumpMemoryReportsToFileImpl(const nsAString& aIdentifier);
};

} 
#endif
