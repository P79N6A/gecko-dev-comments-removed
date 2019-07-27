




#ifndef NS_SVGATTRTEAROFFTABLE_H_
#define NS_SVGATTRTEAROFFTABLE_H_

#include "nsDataHashtable.h"
#include "nsDebug.h"
#include "nsHashKeys.h"









template<class SimpleType, class TearoffType>
class nsSVGAttrTearoffTable
{
public:
#ifdef DEBUG
  ~nsSVGAttrTearoffTable()
  {
    NS_ABORT_IF_FALSE(!mTable, "Tear-off objects remain in hashtable at shutdown.");
  }
#endif

  TearoffType* GetTearoff(SimpleType* aSimple);

  void AddTearoff(SimpleType* aSimple, TearoffType* aTearoff);

  void RemoveTearoff(SimpleType* aSimple);

private:
  typedef nsPtrHashKey<SimpleType> SimpleTypePtrKey;
  typedef nsDataHashtable<SimpleTypePtrKey, TearoffType* > TearoffTable;

  TearoffTable* mTable;
};

template<class SimpleType, class TearoffType>
TearoffType*
nsSVGAttrTearoffTable<SimpleType, TearoffType>::GetTearoff(SimpleType* aSimple)
{
  if (!mTable)
    return nullptr;

  TearoffType *tearoff = nullptr;

#ifdef DEBUG
  bool found =
#endif
    mTable->Get(aSimple, &tearoff);
  NS_ABORT_IF_FALSE(!found || tearoff,
      "NULL pointer stored in attribute tear-off map");

  return tearoff;
}

template<class SimpleType, class TearoffType>
void
nsSVGAttrTearoffTable<SimpleType, TearoffType>::AddTearoff(SimpleType* aSimple,
                                                          TearoffType* aTearoff)
{
  if (!mTable) {
    mTable = new TearoffTable;
  }

  
  
  if (mTable->Get(aSimple, nullptr)) {
    NS_ABORT_IF_FALSE(false, "There is already a tear-off for this object.");
    return;
  }

  mTable->Put(aSimple, aTearoff);
}

template<class SimpleType, class TearoffType>
void
nsSVGAttrTearoffTable<SimpleType, TearoffType>::RemoveTearoff(
    SimpleType* aSimple)
{
  if (!mTable) {
    
    
    return;
  }

  mTable->Remove(aSimple);
  if (mTable->Count() == 0) {
    delete mTable;
    mTable = nullptr;
  }
}

#endif 
