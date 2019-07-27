




#ifndef mozilla_DeadlockDetector_h
#define mozilla_DeadlockDetector_h

#include "mozilla/Attributes.h"

#include <stdlib.h>

#include "prlock.h"

#include "nsClassHashtable.h"
#include "nsTArray.h"

#ifdef NS_TRACE_MALLOC
#  include "nsTraceMalloc.h"
#endif  

namespace mozilla {




class NS_COM_GLUE CallStack
{
private:
#ifdef NS_TRACE_MALLOC
  typedef nsTMStackTraceID callstack_id;
  
#   define NS_GET_BACKTRACE() NS_TraceMallocGetStackTrace()
#   define NS_DEADLOCK_DETECTOR_CONSTEXPR
#else
  typedef void* callstack_id;
#   define NS_GET_BACKTRACE() 0
#   define NS_DEADLOCK_DETECTOR_CONSTEXPR MOZ_CONSTEXPR
#endif  

  callstack_id mCallStack;

public:
  











  NS_DEADLOCK_DETECTOR_CONSTEXPR
  explicit CallStack(const callstack_id aCallStack = NS_GET_BACKTRACE())
    : mCallStack(aCallStack)
  {
  }
  NS_DEADLOCK_DETECTOR_CONSTEXPR
  CallStack(const CallStack& aFrom)
    : mCallStack(aFrom.mCallStack)
  {
  }
  CallStack& operator=(const CallStack& aFrom)
  {
    mCallStack = aFrom.mCallStack;
    return *this;
  }
  bool operator==(const CallStack& aOther) const
  {
    return mCallStack == aOther.mCallStack;
  }
  bool operator!=(const CallStack& aOther) const
  {
    return mCallStack != aOther.mCallStack;
  }

  
  
  
  void Print(FILE* aFile) const
  {
#ifdef NS_TRACE_MALLOC
    if (this != &kNone && mCallStack) {
      NS_TraceMallocPrintStackTrace(aFile, mCallStack);
      return;
    }
#endif
    fputs("  [stack trace unavailable]\n", aFile);
  }

  
  static const CallStack kNone;
};














































template<typename T>
class DeadlockDetector
{
public:
  






  struct ResourceAcquisition
  {
    const T* mResource;
    CallStack mCallContext;

    explicit ResourceAcquisition(const T* aResource,
                                 const CallStack aCallContext = CallStack::kNone)
      : mResource(aResource)
      , mCallContext(aCallContext)
    {
    }
    ResourceAcquisition(const ResourceAcquisition& aFrom)
      : mResource(aFrom.mResource)
      , mCallContext(aFrom.mCallContext)
    {
    }
    ResourceAcquisition& operator=(const ResourceAcquisition& aFrom)
    {
      mResource = aFrom.mResource;
      mCallContext = aFrom.mCallContext;
      return *this;
    }
  };
  typedef nsTArray<ResourceAcquisition> ResourceAcquisitionArray;

private:
  struct OrderingEntry;
  typedef nsTArray<OrderingEntry*> HashEntryArray;
  typedef typename HashEntryArray::index_type index_type;
  typedef typename HashEntryArray::size_type size_type;
  static const index_type NoIndex = HashEntryArray::NoIndex;

  






  struct OrderingEntry
  {
    OrderingEntry(const T* aResource)
      : mFirstSeen(CallStack::kNone)
      , mOrderedLT()        
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
      n += mResource->SizeOfIncludingThis(aMallocSizeOf);
      return n;
    }

    CallStack mFirstSeen; 
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
    n += mOrdering.SizeOfExcludingThis(SizeOfEntryExcludingThis, aMallocSizeOf);
    return n;
  }

  









  void Add(T* aResource)
  {
    PRAutoLock _(mLock);
    mOrdering.Put(aResource, new OrderingEntry(aResource));
  }

  void Remove(T* aResource)
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
    delete aResource;
  }

  





















  ResourceAcquisitionArray* CheckAcquisition(const T* aLast,
                                             const T* aProposed,
                                             const CallStack& aCallContext)
  {
    NS_ASSERTION(aProposed, "null resource");
    PRAutoLock _(mLock);

    OrderingEntry* proposed = mOrdering.Get(aProposed);
    NS_ASSERTION(proposed, "missing ordering entry");

    if (CallStack::kNone == proposed->mFirstSeen) {
      proposed->mFirstSeen = aCallContext;
    }

    if (!aLast) {
      
      return 0;
    }

    OrderingEntry* current = mOrdering.Get(aLast);
    NS_ASSERTION(current, "missing ordering entry");

    

    if (current == proposed) {
      
      
      ResourceAcquisitionArray* cycle = new ResourceAcquisitionArray();
      if (!cycle) {
        NS_RUNTIMEABORT("can't allocate dep. cycle array");
      }
      cycle->AppendElement(ResourceAcquisition(current->mResource,
                                               current->mFirstSeen));
      cycle->AppendElement(ResourceAcquisition(aProposed,
                                               aCallContext));
      return cycle;
    }
    if (InTransitiveClosure(current, proposed)) {
      
      return 0;
    }
    if (InTransitiveClosure(proposed, current)) {
      
      
      
      
      
      ResourceAcquisitionArray* cycle = GetDeductionChain(proposed, current);
      
      cycle->AppendElement(ResourceAcquisition(aProposed,
                                               aCallContext));
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
    for (const OrderingEntry* const* it = aStart->mOrderedLT.Elements(); i < len; ++i, ++it) {
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
    chain->AppendElement(ResourceAcquisition(aStart->mResource,
                                             aStart->mFirstSeen));

    NS_ASSERTION(GetDeductionChain_Helper(aStart, aTarget, chain),
                 "GetDeductionChain called when there's no deadlock");
    return chain;
  }

  
  
  bool GetDeductionChain_Helper(const OrderingEntry* aStart,
                                const OrderingEntry* aTarget,
                                ResourceAcquisitionArray* aChain)
  {
    if (aStart->mOrderedLT.BinaryIndexOf(aTarget) != NoIndex) {
      aChain->AppendElement(ResourceAcquisition(aTarget->mResource,
                                                aTarget->mFirstSeen));
      return true;
    }

    index_type i = 0;
    size_type len = aStart->mOrderedLT.Length();
    for (const OrderingEntry* const* it = aStart->mOrderedLT.Elements(); i < len; ++i, ++it) {
      aChain->AppendElement(ResourceAcquisition((*it)->mResource,
                                                (*it)->mFirstSeen));
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
  DeadlockDetector(const DeadlockDetector& aDD) MOZ_DELETE;
  DeadlockDetector& operator=(const DeadlockDetector& aDD) MOZ_DELETE;
};


template<typename T>

const uint32_t DeadlockDetector<T>::kDefaultNumBuckets = 64;


} 

#endif 
