




































#ifndef nsDOMMemoryReporter_h__
#define nsDOMMemoryReporter_h__

#include "nsIMemoryReporter.h"


class nsDOMMemoryReporter: public nsIMemoryReporter {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  static void Init();

private:
  
  nsDOMMemoryReporter();
};

#endif 

