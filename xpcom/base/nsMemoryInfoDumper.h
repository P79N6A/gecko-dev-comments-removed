





#ifndef mozilla_nsMemoryInfoDumper_h
#define mozilla_nsMemoryInfoDumper_h

#include "nsIMemoryInfoDumper.h"
#include <stdio.h>








class nsMemoryInfoDumper : public nsIMemoryInfoDumper
{
  virtual ~nsMemoryInfoDumper();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYINFODUMPER

  nsMemoryInfoDumper();

  static void Initialize();

#ifdef MOZ_DMD
  
  
  static nsresult OpenDMDFile(const nsAString& aIdentifier, int aPid,
                              FILE** aOutFile);
  
  static nsresult DumpDMDToFile(FILE* aFile);
#endif
};

#define NS_MEMORY_INFO_DUMPER_CID \
{ 0x00bd71fb, 0x7f09, 0x4ec3, \
{ 0x96, 0xaf, 0xa0, 0xb5, 0x22, 0xb7, 0x79, 0x69 } }

#endif
