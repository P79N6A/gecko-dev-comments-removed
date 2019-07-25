







































#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsToolkitCompsCID.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

class nsUrlClassifierPrefixSet : public nsIUrlClassifierPrefixSet
{
public:
  nsUrlClassifierPrefixSet();
  virtual ~nsUrlClassifierPrefixSet() {};

  
  NS_IMETHOD SetPrefixes(const PRUint32* aArray, PRUint32 aLength);
  
  
  NS_IMETHOD AddPrefixes(const PRUint32* aArray, PRUint32 aLength);
  
  NS_IMETHOD Contains(PRUint32 aPrefix, PRBool* aFound);
  
  
  
  NS_IMETHOD Probe(PRUint32 aPrefix, PRBool* aReady, PRBool* aFound);
  NS_IMETHOD EstimateSize(PRUint32* aSize);

  NS_DECL_ISUPPORTS

protected:
  static const PRUint32 DELTAS_LIMIT = 100;
  static const PRUint32 MAX_INDEX_DIFF = (1 << 16);

  mozilla::Mutex mPrefixTreeLock;
  mozilla::CondVar mTreeIsReady;

  PRUint32 BinSearch(PRUint32 start, PRUint32 end, PRUint32 target);

  
  
  PRBool mHasPrefixes;
  
  nsTArray<PRUint32> mIndexPrefixes;
  
  
  nsTArray<PRUint32> mIndexStarts;
  
  nsTArray<PRUint16> mDeltas;
};

#endif
