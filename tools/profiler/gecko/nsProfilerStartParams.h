




#ifndef _NSPROFILERSTARTPARAMS_H_
#define _NSPROFILERSTARTPARAMS_H_

#include "nsIProfiler.h"
#include "nsString.h"
#include "nsTArray.h"

class nsProfilerStartParams : public nsIProfilerStartParams
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROFILERSTARTPARAMS

  nsProfilerStartParams(uint32_t aEntries,
                        double aInterval,
                        const nsTArray<nsCString>& aFeatures,
                        const nsTArray<nsCString>& aThreadFilterNames);

private:
  virtual ~nsProfilerStartParams();
  uint32_t mEntries;
  double mInterval;
  nsTArray<nsCString> mFeatures;
  nsTArray<nsCString> mThreadFilterNames;
};

#endif
