




#ifndef mozilla_DeadlockDetector_h
#define mozilla_DeadlockDetector_h

#include "mozilla/Attributes.h"

#include <stdlib.h>

#include "plhash.h"
#include "prlock.h"

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
  typedef nsTArray<PLHashEntry*> HashEntryArray;
  typedef typename HashEntryArray::index_type index_type;
  typedef typename HashEntryArray::size_type size_type;
  static const index_type NoIndex = HashEntryArray::NoIndex;

  






  struct OrderingEntry
  {
    OrderingEntry(const T* aResource)
      : mFirstSeen(CallStack::kNone)
      , mOrderedLT()        
      , mResource(aResource)
    {
    }
    ~OrderingEntry()
    {
    }

    CallStack mFirstSeen; 
    HashEntryArray mOrderedLT; 
    const T* mResource;
  };

  static void* TableAlloc(void* , size_t aSize)
  {
    return operator new(aSize);
  }
  static void TableFree(void* , void* aItem)
  {
    operator delete(aItem);
  }
  static PLHashEntry* EntryAlloc(void* , const void* aKey)
  {
    return new PLHashEntry;
  }
  static void EntryFree(void* , PLHashEntry* aEntry, unsigned aFlag)
  {
    delete static_cast<T*>(const_cast<void*>(aEntry->key));
    delete static_cast<OrderingEntry*>(aEntry->value);
    aEntry->value = 0;
    if (aFlag == HT_FREE_ENTRY) {
      delete aEntry;
    }
  }
  static PLHashNumber HashKey(const void* aKey)
  {
    return static_cast<PLHashNumber>(NS_PTR_TO_INT32(aKey) >> 2);
  }
  static const PLHashAllocOps kAllocOps;

  

  PLHashEntry** GetEntry(const T* aKey)
  {
    return PL_HashTableRawLookup(mOrdering, HashKey(aKey), aKey);
  }

  void PutEntry(T* aKey)
  {
    PL_HashTableAdd(mOrdering, aKey, new OrderingEntry(aKey));
  }

  
  

  








  void AddOrder(PLHashEntry* aLT, PLHashEntry* aGT)
  {
    static_cast<OrderingEntry*>(aLT->value)->mOrderedLT
      .InsertElementSorted(aGT);
  }

  





  bool IsOrdered(const PLHashEntry* aFirst, const PLHashEntry* aSecond)
  const
  {
    const OrderingEntry* entry =
      static_cast<const OrderingEntry*>(aFirst->value);
    return entry->mOrderedLT.BinaryIndexOf(aSecond) != NoIndex;
  }

  





  PLHashEntry* const* GetOrders(const PLHashEntry* aEntry) const
  {
    return
      static_cast<const OrderingEntry*>(aEntry->value)->mOrderedLT.Elements();
  }

  





  size_type NumOrders(const PLHashEntry* aEntry) const
  {
    return
      static_cast<const OrderingEntry*>(aEntry->value)->mOrderedLT.Length();
  }

  
  ResourceAcquisition MakeResourceAcquisition(const PLHashEntry* aEntry) const
  {
    return ResourceAcquisition(
      static_cast<const T*>(aEntry->key),
      static_cast<const OrderingEntry*>(aEntry->value)->mFirstSeen);
  }

  
  struct PRAutoLock
  {
    explicit PRAutoLock(PRLock* aLock) : mLock(aLock) { PR_Lock(mLock); }
    ~PRAutoLock() { PR_Unlock(mLock); }
    PRLock* mLock;
  };

public:
  static const uint32_t kDefaultNumBuckets;

  






  explicit DeadlockDetector(uint32_t aNumResourcesGuess = kDefaultNumBuckets)
  {
    mOrdering = PL_NewHashTable(aNumResourcesGuess,
                                HashKey,
                                PL_CompareValues, PL_CompareValues,
                                &kAllocOps, 0);
    if (!mOrdering) {
      NS_RUNTIMEABORT("couldn't initialize resource ordering table");
    }

    mLock = PR_NewLock();
    if (!mLock) {
      NS_RUNTIMEABORT("couldn't allocate deadlock detector lock");
    }
  }

  




  ~DeadlockDetector()
  {
    PL_HashTableDestroy(mOrdering);
    PR_DestroyLock(mLock);
  }

  









  void Add(T* aResource)
  {
    PRAutoLock _(mLock);
    PutEntry(aResource);
  }

  
  
  
  
  
  

  





















  ResourceAcquisitionArray* CheckAcquisition(const T* aLast,
                                             const T* aProposed,
                                             const CallStack& aCallContext)
  {
    NS_ASSERTION(aProposed, "null resource");
    PRAutoLock _(mLock);

    PLHashEntry* second = *GetEntry(aProposed);
    OrderingEntry* e = static_cast<OrderingEntry*>(second->value);
    if (CallStack::kNone == e->mFirstSeen) {
      e->mFirstSeen = aCallContext;
    }

    if (!aLast) {
      
      return 0;
    }

    PLHashEntry* first = *GetEntry(aLast);

    

    if (first == second) {
      
      
      ResourceAcquisitionArray* cycle = new ResourceAcquisitionArray();
      if (!cycle) {
        NS_RUNTIMEABORT("can't allocate dep. cycle array");
      }
      cycle->AppendElement(MakeResourceAcquisition(first));
      cycle->AppendElement(ResourceAcquisition(aProposed,
                                               aCallContext));
      return cycle;
    }
    if (InTransitiveClosure(first, second)) {
      
      return 0;
    }
    if (InTransitiveClosure(second, first)) {
      
      
      
      
      
      ResourceAcquisitionArray* cycle = GetDeductionChain(second, first);
      
      cycle->AppendElement(ResourceAcquisition(aProposed,
                                               aCallContext));
      return cycle;
    }
    
    
    
    AddOrder(first, second);
    return 0;
  }

  





  bool InTransitiveClosure(const PLHashEntry* aStart,
                           const PLHashEntry* aTarget) const
  {
    if (IsOrdered(aStart, aTarget)) {
      return true;
    }

    index_type i = 0;
    size_type len = NumOrders(aStart);
    for (const PLHashEntry* const* it = GetOrders(aStart); i < len; ++i, ++it) {
      if (InTransitiveClosure(*it, aTarget)) {
        return true;
      }
    }
    return false;
  }

  















  ResourceAcquisitionArray* GetDeductionChain(const PLHashEntry* aStart,
                                              const PLHashEntry* aTarget)
  {
    ResourceAcquisitionArray* chain = new ResourceAcquisitionArray();
    if (!chain) {
      NS_RUNTIMEABORT("can't allocate dep. cycle array");
    }
    chain->AppendElement(MakeResourceAcquisition(aStart));

    NS_ASSERTION(GetDeductionChain_Helper(aStart, aTarget, chain),
                 "GetDeductionChain called when there's no deadlock");
    return chain;
  }

  
  
  bool GetDeductionChain_Helper(const PLHashEntry* aStart,
                                const PLHashEntry* aTarget,
                                ResourceAcquisitionArray* aChain)
  {
    if (IsOrdered(aStart, aTarget)) {
      aChain->AppendElement(MakeResourceAcquisition(aTarget));
      return true;
    }

    index_type i = 0;
    size_type len = NumOrders(aStart);
    for (const PLHashEntry* const* it = GetOrders(aStart); i < len; ++i, ++it) {
      aChain->AppendElement(MakeResourceAcquisition(*it));
      if (GetDeductionChain_Helper(*it, aTarget, aChain)) {
        return true;
      }
      aChain->RemoveElementAt(aChain->Length() - 1);
    }
    return false;
  }

  



  PLHashTable* mOrdering;     

  




  PRLock* mLock;

private:
  DeadlockDetector(const DeadlockDetector& aDD) MOZ_DELETE;
  DeadlockDetector& operator=(const DeadlockDetector& aDD) MOZ_DELETE;
};


template<typename T>
const PLHashAllocOps DeadlockDetector<T>::kAllocOps = {
  DeadlockDetector<T>::TableAlloc, DeadlockDetector<T>::TableFree,
  DeadlockDetector<T>::EntryAlloc, DeadlockDetector<T>::EntryFree
};


template<typename T>

const uint32_t DeadlockDetector<T>::kDefaultNumBuckets = 64;


} 

#endif 
