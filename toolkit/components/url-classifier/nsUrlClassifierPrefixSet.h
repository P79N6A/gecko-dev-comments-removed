







































#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsToolkitCompsCID.h"

class nsUrlClassifierPrefixSet : public nsIUrlClassifierPrefixSet
{
public:
  nsUrlClassifierPrefixSet();
  virtual ~nsUrlClassifierPrefixSet() {};

  
  virtual nsresult SetPrefixes(const PRUint32*, PRUint32);
  
  
  virtual nsresult AddPrefixes(const PRUint32*, PRUint32);
  virtual nsresult Contains(PRUint32, PRBool*);
  virtual nsresult EstimateSize(PRUint32*);

  NS_DECL_ISUPPORTS

protected:
  static const PRUint32 DELTAS_LIMIT = 100;
  static const PRUint32 MAX_INDEX_DIFF = (1 << 16);

  PRUint32 BinSearch(PRUint32 start, PRUint32 end, PRUint32 target);

 
  
  PRBool mHasPrefixes;
  
  nsTArray<PRUint16> mDeltas;
  
  nsTArray<PRUint32> mIndexPrefixes;
  
  
  nsTArray<PRUint32> mIndexStarts;
};

#endif
