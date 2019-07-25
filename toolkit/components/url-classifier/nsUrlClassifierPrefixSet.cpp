







































#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsUrlClassifierPrefixSet.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsToolkitCompsCID.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "mozilla/Mutex.h"
#include "prlog.h"


#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierPrefixSetLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierPrefixSetLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierPrefixSetLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (PR_FALSE)
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUrlClassifierPrefixSet, nsIUrlClassifierPrefixSet);

nsUrlClassifierPrefixSet::nsUrlClassifierPrefixSet()
  : mHasPrefixes(PR_FALSE)
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierPrefixSetLog)
    gUrlClassifierPrefixSetLog = PR_NewLogModule("UrlClassifierPrefixSet");
#endif
  LOG(("Instantiating PrefixSet"));
}

nsresult
nsUrlClassifierPrefixSet::SetPrefixes(const PRUint32 * aArray, PRUint32 aLength)
{
  if (mHasPrefixes) {
    mDeltas.Clear();
    mIndexPrefixes.Clear();
    mIndexStarts.Clear();
    mHasPrefixes = PR_FALSE;
  }
  if (aLength > 0) {
    
    nsTArray<PRUint32> prefixes;
    prefixes.AppendElements(aArray, aLength);
    prefixes.Sort();
    AddPrefixes(prefixes.Elements(), prefixes.Length());
  }

  return NS_OK;
}

nsresult
nsUrlClassifierPrefixSet::AddPrefixes(const PRUint32 * prefixes, PRUint32 aLength)
{
  if (aLength == 0) {
    return NS_OK;
  }

  mIndexPrefixes.AppendElement(prefixes[0]);
  mIndexStarts.AppendElement(mDeltas.Length());

  PRUint32 numOfDeltas = 0;
  PRUint32 currentItem = prefixes[0];
  for (PRUint32 i = 1; i < aLength; i++) {
    if ((numOfDeltas >= DELTAS_LIMIT) ||
          (prefixes[i] - currentItem >= MAX_INDEX_DIFF)) {
      mIndexStarts.AppendElement(mDeltas.Length());
      mIndexPrefixes.AppendElement(prefixes[i]);
      numOfDeltas = 0;
    } else {
      PRUint16 delta = prefixes[i] - currentItem;
      mDeltas.AppendElement(delta);
      numOfDeltas++;
    }
    currentItem = prefixes[i];
  }

  mIndexPrefixes.Compact();
  mIndexStarts.Compact();
  mDeltas.Compact();

  LOG(("Total number of indices: %d", mIndexPrefixes.Length()));
  LOG(("Total number of deltas: %d", mDeltas.Length()));

  mHasPrefixes = PR_TRUE;

  return NS_OK;
}

PRUint32 nsUrlClassifierPrefixSet::BinSearch(PRUint32 start,
                                              PRUint32 end,
                                              PRUint32 target)
{
  while (start != end && end >= start) {
    int i = start + ((end - start) >> 1);
    int value = mIndexPrefixes[i];
    if (value < target) {
      start = i + 1;
    } else if (value > target) {
      end = i - 1;
    } else {
      return i;
    }
  }
  return end;
}

nsresult
nsUrlClassifierPrefixSet::Contains(PRUint32 aPrefix, PRBool * aFound)
{
  *aFound = PR_FALSE;

  if (!mHasPrefixes) {
    return NS_OK;
  }

  PRUint32 target = aPrefix;

  
  
  
  
  if (target < mIndexPrefixes[0]) {
    return NS_OK;
  }

  
  
  
  
  

  int i = BinSearch(0, mIndexPrefixes.Length() - 1, target);
  if (mIndexPrefixes[i] > target && i > 0) {
    i--;
  }

  
  PRUint32 diff = target - mIndexPrefixes[i];
  PRUint32 deltaIndex = mIndexStarts[i];
  PRUint32 end = (i + 1 < mIndexStarts.Length()) ? mIndexStarts[i+1]
                                                 : mDeltas.Length();
  while (diff > 0 && deltaIndex < end) {
    diff -= mDeltas[deltaIndex];
    deltaIndex++;
  }

  if (diff == 0) {
    *aFound = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierPrefixSet::EstimateSize(PRUint32 * aSize)
{
  *aSize = sizeof(PRBool);
  if (mHasPrefixes) {
    *aSize += sizeof(PRUint16) * mDeltas.Length();
    *aSize += sizeof(PRUint32) * mIndexPrefixes.Length();
    *aSize += sizeof(PRUint32) * mIndexStarts.Length();
  }
  return NS_OK;
}
