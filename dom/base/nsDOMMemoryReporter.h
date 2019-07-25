




































#ifndef nsDOMMemoryReporter_h__
#define nsDOMMemoryReporter_h__

#include "nsIMemoryReporter.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

class nsWindowSizes {
public:
    nsWindowSizes(nsMallocSizeOfFun aMallocSizeOf)
    : mMallocSizeOf(aMallocSizeOf),
      mDOM(0),
      mStyleSheets(0)
    {}
    nsMallocSizeOfFun mMallocSizeOf;
    size_t mDOM;
    size_t mStyleSheets;
};

class nsDOMMemoryMultiReporter: public nsIMemoryMultiReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYMULTIREPORTER

  static void Init();

private:
  
  nsDOMMemoryMultiReporter();
};

#endif 

