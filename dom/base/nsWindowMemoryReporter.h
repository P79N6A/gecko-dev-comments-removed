




































#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

class nsWindowSizes {
public:
    nsWindowSizes(nsMallocSizeOfFun aMallocSizeOf) {
      memset(this, 0, sizeof(nsWindowSizes));
      mMallocSizeOf = aMallocSizeOf;
    }
    nsMallocSizeOfFun mMallocSizeOf;
    size_t mDOM;
    size_t mStyleSheets;
    size_t mLayoutArenas;
    size_t mLayoutStyleSets;
    size_t mLayoutTextRuns;
};

class nsWindowMemoryReporter: public nsIMemoryMultiReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYMULTIREPORTER

  static void Init();

private:
  
  nsWindowMemoryReporter();
};

#endif 

