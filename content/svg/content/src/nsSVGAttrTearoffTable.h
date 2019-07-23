
































#ifndef NS_SVGATTRTEAROFFTABLE_H_
#define NS_SVGATTRTEAROFFTABLE_H_

#include "nsDataHashtable.h"









template<class SimpleType, class TearoffType>
class nsSVGAttrTearoffTable
{
public:
  ~nsSVGAttrTearoffTable()
  {
    NS_ABORT_IF_FALSE(mTable.Count() == 0,
        "Tear-off objects remain in hashtable at shutdown.");
  }

  TearoffType* GetTearoff(SimpleType* aSimple);

  void AddTearoff(SimpleType* aSimple, TearoffType* aTearoff);

  void RemoveTearoff(SimpleType* aSimple);

private:
  typedef nsPtrHashKey<SimpleType> SimpleTypePtrKey;
  typedef nsDataHashtable<SimpleTypePtrKey, TearoffType* > TearoffTable;

  TearoffTable mTable;
};

template<class SimpleType, class TearoffType>
TearoffType*
nsSVGAttrTearoffTable<SimpleType, TearoffType>::GetTearoff(SimpleType* aSimple)
{
  if (!mTable.IsInitialized())
    return nsnull;

  TearoffType *tearoff = nsnull;
  PRBool found = mTable.Get(aSimple, &tearoff);
  NS_ABORT_IF_FALSE(!found || tearoff,
      "NULL pointer stored in attribute tear-off map");

  return tearoff;
}

template<class SimpleType, class TearoffType>
void
nsSVGAttrTearoffTable<SimpleType, TearoffType>::AddTearoff(SimpleType* aSimple,
                                                          TearoffType* aTearoff)
{
  if (!mTable.IsInitialized()) {
    mTable.Init();
  }

  
  
  if (mTable.Get(aSimple, nsnull)) {
    NS_ABORT_IF_FALSE(PR_FALSE, "There is already a tear-off for this object.");
    return;
  }

  PRBool result = mTable.Put(aSimple, aTearoff);
  NS_ABORT_IF_FALSE(result, "Out of memory.");
}

template<class SimpleType, class TearoffType>
void
nsSVGAttrTearoffTable<SimpleType, TearoffType>::RemoveTearoff(
    SimpleType* aSimple)
{
  if (!mTable.IsInitialized()) {
    
    
    return;
  }

  mTable.Remove(aSimple);
}

#endif 
