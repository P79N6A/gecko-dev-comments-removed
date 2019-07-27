




#ifndef mozilla_DeadlockDetector_h
#define mozilla_DeadlockDetector_h

#include "mozilla/Attributes.h"

#include <stdlib.h>

#include "prlock.h"

#include "nsClassHashtable.h"
#include "nsTArray.h"

namespace mozilla {













































template<typename T>
class DeadlockDetector
{
public:
  typedef nsTArray<const T*> ResourceAcquisitionArray;

private:
  struct OrderingEntry;
  typedef nsTArray<OrderingEntry*> HashEntryArray;
  typedef typename HashEntryArray::index_type index_type;
  typedef typename HashEntryArray::size_type size_type;
  static const index_type NoIndex = HashEntryArray::NoIndex;

  






  struct OrderingEntry
  {
    explicit OrderingEntry(const T* aResource)
      : mOrderedLT()        
      , mExternalRefs()
      , mResource(aResource)
    {
    }
    ~OrderingEntry()
    {
    }

    size_t
    SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
    {
      size_t n = aMallocSizeOf(this);
      n += mOrderedLT.SizeOfExcludingThis(aMallocSizeOf);
      n += mExternalRefs.SizeOfExcludingThis(aMallocSizeOf);
      return n;
    }

    HashEntryArray mOrderedLT; 
    HashEntryArray mExternalRefs; 
    const T* mResource;
  };

  
  struct PRAutoLock
  {
    explicit PRAutoLock(PRLock* aLock) : mLock(aLock) { PR_Lock(mLock); }
    ~PRAutoLock() { PR_Unlock(mLock); }
    PRLock* mLock;
  };

public:
  static const uint32_t kDefaultNumBuckets;

  






  explicit DeadlockDetector(uint32_t aNumResourcesGuess = kDefaultNumBuckets)
    : mOrdering(aNumResourcesGuess)
  {
    mLock = PR_NewLock();
    if (!mLock) {
      NS_RUNTIMEABORT("couldn't allocate deadlock detector lock");
    }
  }

  




  ~DeadlockDetector()
  {
    PR_DestroyLock(mLock);
  }

  static size_t
  SizeOfEntryExcludingThis(const T* aKey, const nsAutoPtr<OrderingEntry>& aEntry,
                           MallocSizeOf aMallocSizeOf, void* aUserArg)
  {
    
    size_t n = aEntry->SizeOfIncludingThis(aMallocSizeOf);
    return n;
  }

  size_t
  SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t n = aMallocSizeOf(this);

    {
      PRAutoLock _(mLock);
      n += mOrdering.SizeOfExcludingThis(SizeOfEntryExcludingThis, aMallocSizeOf);
    }

    return n;
  }

  









  void Add(const T* aResource)
  {
    PRAutoLock _(mLock);
    mOrdering.Put(aResource, new OrderingEntry(aResource));
  }

  void Remove(const T* aResource)
  {
    PRAutoLock _(mLock);

    OrderingEntry* entry = mOrdering.Get(aResource);

    
    HashEntryArray& refs = entry->mExternalRefs;
    for (index_type i = 0; i < refs.Length(); i++) {
      refs[i]->mOrderedLT.RemoveElementSorted(entry);
    }

    
    HashEntryArray& orders = entry->mOrderedLT;
    for (index_type i = 0; i < orders.Length(); i++) {
      orders[i]->mExternalRefs.RemoveElementSorted(entry);
    }

    
    mOrdering.Remove(aResource);
  }

  




















  ResourceAcquisitionArray* CheckAcquisition(const T* aLast,
                                             const T* aProposed)
  {
    if (!aLast) {
      
      return 0;
    }

    NS_ASSERTION(aProposed, "null resource");
    PRAutoLock _(mLock);

    OrderingEntry* proposed = mOrdering.Get(aProposed);
    NS_ASSERTION(proposed, "missing ordering entry");

    OrderingEntry* current = mOrdering.Get(aLast);
    NS_ASSERTION(current, "missing ordering entry");

    

    if (current == proposed) {
      
      
      ResourceAcquisitionArray* cycle = new ResourceAcquisitionArray();
      if (!cycle) {
        NS_RUNTIMEABORT("can't allocate dep. cycle array");
      }
      cycle->AppendElement(current->mResource);
      cycle->AppendElement(aProposed);
      return cycle;
    }
    if (InTransitiveClosure(current, proposed)) {
      
      return 0;
    }
    if (InTransitiveClosure(proposed, current)) {
      
      
      
      
      
      ResourceAcquisitionArray* cycle = GetDeductionChain(proposed, current);
      
      cycle->AppendElement(aProposed);
      return cycle;
    }
    
    
    
    current->mOrderedLT.InsertElementSorted(proposed);
    proposed->mExternalRefs.InsertElementSorted(current);
    return 0;
  }

  





  bool InTransitiveClosure(const OrderingEntry* aStart,
                           const OrderingEntry* aTarget) const
  {
    
    
    static nsDefaultComparator<const OrderingEntry*, const OrderingEntry*> comp;
    if (aStart->mOrderedLT.BinaryIndexOf(aTarget, comp) != NoIndex) {
      return true;
    }

    index_type i = 0;
    size_type len = aStart->mOrderedLT.Length();
    for (auto it = aStart->mOrderedLT.Elements(); i < len; ++i, ++it) {
      if (InTransitiveClosure(*it, aTarget)) {
        return true;
      }
    }
    return false;
  }

  















  ResourceAcquisitionArray* GetDeductionChain(const OrderingEntry* aStart,
                                              const OrderingEntry* aTarget)
  {
    ResourceAcquisitionArray* chain = new ResourceAcquisitionArray();
    if (!chain) {
      NS_RUNTIMEABORT("can't allocate dep. cycle array");
    }
    chain->AppendElement(aStart->mResource);

    NS_ASSERTION(GetDeductionChain_Helper(aStart, aTarget, chain),
                 "GetDeductionChain called when there's no deadlock");
    return chain;
  }

  
  
  bool GetDeductionChain_Helper(const OrderingEntry* aStart,
                                const OrderingEntry* aTarget,
                                ResourceAcquisitionArray* aChain)
  {
    if (aStart->mOrderedLT.BinaryIndexOf(aTarget) != NoIndex) {
      aChain->AppendElement(aTarget->mResource);
      return true;
    }

    index_type i = 0;
    size_type len = aStart->mOrderedLT.Length();
    for (auto it = aStart->mOrderedLT.Elements(); i < len; ++i, ++it) {
      aChain->AppendElement((*it)->mResource);
      if (GetDeductionChain_Helper(*it, aTarget, aChain)) {
        return true;
      }
      aChain->RemoveElementAt(aChain->Length() - 1);
    }
    return false;
  }

  



  nsClassHashtable<nsPtrHashKey<const T>, OrderingEntry> mOrdering;


  




  PRLock* mLock;

private:
  DeadlockDetector(const DeadlockDetector& aDD) = delete;
  DeadlockDetector& operator=(const DeadlockDetector& aDD) = delete;
};


template<typename T>

const uint32_t DeadlockDetector<T>::kDefaultNumBuckets = 32;


} 

#endif 
