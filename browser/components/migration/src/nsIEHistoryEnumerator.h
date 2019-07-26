



#ifndef iehistoryenumerator___h___
#define iehistoryenumerator___h___

#include <urlhist.h>

#include "mozilla/Attributes.h"
#include "nsISimpleEnumerator.h"
#include "nsIWritablePropertyBag2.h"
#include "nsAutoPtr.h"

class nsIEHistoryEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  nsIEHistoryEnumerator();

private:
  ~nsIEHistoryEnumerator();

  


  void EnsureInitialized();

  nsRefPtr<IUrlHistoryStg2> mIEHistory;
  nsRefPtr<IEnumSTATURL> mURLEnumerator;

  nsCOMPtr<nsIWritablePropertyBag2> mCachedNextEntry;
};

#endif
