




































#ifndef nsDOMMemoryReporter_h__
#define nsDOMMemoryReporter_h__

#include "nsIMemoryReporter.h"


class nsDOMMemoryReporter: public nsIMemoryReporter {
public:
  NS_DECL_ISUPPORTS

  static void Init();

  NS_IMETHOD GetProcess(char** aProcess);
  NS_IMETHOD GetPath(char** aMemoryPath);
  NS_IMETHOD GetKind(int* aKnd);
  NS_IMETHOD GetDescription(char** aDescription);
  NS_IMETHOD GetMemoryUsed(PRInt64* aMemoryUsed);

private:
  
  nsDOMMemoryReporter();
};

#endif 

