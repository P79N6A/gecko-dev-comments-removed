





































#ifndef mozilla_DeadlockDetector_h
#define mozilla_DeadlockDetector_h

#include <stdlib.h>

#include "plhash.h"
#include "prlock.h"

#include "nsTArray.h"

#ifdef NS_TRACE_MALLOC
#  include "tracemalloc/nsTraceMalloc.h"
#endif  

namespace mozilla {




class NS_COM_GLUE CallStack
{
private:
#ifdef NS_TRACE_MALLOC
    typedef nsTMStackTraceID callstack_id;
    
#   define NS_GET_BACKTRACE() NS_TraceMallocGetStackTrace()
#else
    typedef void* callstack_id;
#   define NS_GET_BACKTRACE() 0
#endif  

    callstack_id mCallStack;

public:
    











    CallStack(const callstack_id aCallStack = NS_GET_BACKTRACE()) :
        mCallStack(aCallStack)
    {
    }
    CallStack(const CallStack& aFrom) :
        mCallStack(aFrom.mCallStack)
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

    
    
    
    void Print(FILE* f) const
    {
#ifdef NS_TRACE_MALLOC
        if (this != &kNone && mCallStack) {
            NS_TraceMallocPrintStackTrace(f, mCallStack);
            return;
        }
#endif
        fputs("  [stack trace unavailable]\n", f);
    }

    
    static const CallStack kNone;
};














































template <typename T>
class DeadlockDetector
{
public:
    






    struct ResourceAcquisition
    {
        const T* mResource;
        CallStack mCallContext;

        ResourceAcquisition(
            const T* aResource,
            const CallStack aCallContext=CallStack::kNone) :
            mResource(aResource),
            mCallContext(aCallContext)
        {
        }
        ResourceAcquisition(const ResourceAcquisition& aFrom) :
            mResource(aFrom.mResource),
            mCallContext(aFrom.mCallContext)
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
    enum {
        NoIndex = HashEntryArray::NoIndex
    };

    






    struct OrderingEntry
    {
        OrderingEntry() :
            mFirstSeen(CallStack::kNone),
            mOrderedLT()        
        {                       
        }
        ~OrderingEntry()
        {
        }

        CallStack mFirstSeen; 
        HashEntryArray mOrderedLT; 
    };

    static void* TableAlloc(void* , PRSize size)
    {
        return operator new(size);
    }
    static void TableFree(void* , void* item)
    {
        operator delete(item);
    }
    static PLHashEntry* EntryAlloc(void* , const void* key)
    {
        return new PLHashEntry;
    }
    static void EntryFree(void* , PLHashEntry* entry, PRUintn flag)
    {
        delete static_cast<T*>(const_cast<void*>(entry->key));
        delete static_cast<OrderingEntry*>(entry->value);
        entry->value = 0;
        if (HT_FREE_ENTRY == flag)
            delete entry;
    }
    static PLHashNumber HashKey(const void* aKey)
    {
        return NS_PTR_TO_INT32(aKey) >> 2;
    }
    static const PLHashAllocOps kAllocOps;

    

    PLHashEntry** GetEntry(const T* aKey)
    {
        return PL_HashTableRawLookup(mOrdering, HashKey(aKey), aKey);
    }

    void PutEntry(T* aKey)
    {
        PL_HashTableAdd(mOrdering, aKey, new OrderingEntry());
    }

    
    

    








    void AddOrder(PLHashEntry* aLT, PLHashEntry* aGT)
    {
        static_cast<OrderingEntry*>(aLT->value)->mOrderedLT
            .InsertElementSorted(aGT);
    }

    





    bool IsOrdered(const PLHashEntry* aFirst, const PLHashEntry* aSecond)
        const
    {
        return NoIndex !=
            static_cast<const OrderingEntry*>(aFirst->value)->mOrderedLT
                .BinaryIndexOf(aSecond);
    }

    





    PLHashEntry* const* GetOrders(const PLHashEntry* aEntry) const
    {
        return static_cast<const OrderingEntry*>(aEntry->value)->mOrderedLT
            .Elements();
    }

    





    size_type NumOrders(const PLHashEntry* aEntry) const
    {
        return static_cast<const OrderingEntry*>(aEntry->value)->mOrderedLT
            .Length();
    }

    
    ResourceAcquisition MakeResourceAcquisition(const PLHashEntry* aEntry)
        const
    {
        return ResourceAcquisition(
            static_cast<const T*>(aEntry->key),
            static_cast<const OrderingEntry*>(aEntry->value)->mFirstSeen);
    }

    
    struct PRAutoLock
    {
        PRAutoLock(PRLock* aLock) : mLock(aLock) { PR_Lock(mLock); }
        ~PRAutoLock() { PR_Unlock(mLock); }
        PRLock* mLock;
    };

public:
    static const PRUint32 kDefaultNumBuckets;

    






    DeadlockDetector(PRUint32 aNumResourcesGuess = kDefaultNumBuckets)
    {
        mOrdering = PL_NewHashTable(aNumResourcesGuess,
                                    HashKey,
                                    PL_CompareValues, PL_CompareValues,
                                    &kAllocOps, 0);
        if (!mOrdering)
            NS_RUNTIMEABORT("couldn't initialize resource ordering table");

        mLock = PR_NewLock();
        if (!mLock)
            NS_RUNTIMEABORT("couldn't allocate deadlock detector lock");
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
        if (CallStack::kNone == e->mFirstSeen)
            e->mFirstSeen = aCallContext;

        if (!aLast)
            
            return 0;

        PLHashEntry* first = *GetEntry(aLast);

        

        if (first == second) {
            
            
            ResourceAcquisitionArray* cycle = new ResourceAcquisitionArray();
            if (!cycle)
                NS_RUNTIMEABORT("can't allocate dep. cycle array");
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
        if (IsOrdered(aStart, aTarget))
            return true;

        index_type i = 0;
        size_type len = NumOrders(aStart);
        for (const PLHashEntry* const* it = GetOrders(aStart);
             i < len; ++i, ++it)
            if (InTransitiveClosure(*it, aTarget))
                return true;
        return false;
    }

    















    ResourceAcquisitionArray* GetDeductionChain(
        const PLHashEntry* aStart,
        const PLHashEntry* aTarget)
    {
        ResourceAcquisitionArray* chain = new ResourceAcquisitionArray();
        if (!chain)
            NS_RUNTIMEABORT("can't allocate dep. cycle array");
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
        for (const PLHashEntry* const* it = GetOrders(aStart);
             i < len; ++i, ++it) {
            aChain->AppendElement(MakeResourceAcquisition(*it));
            if (GetDeductionChain_Helper(*it, aTarget, aChain))
                return true;
            aChain->RemoveElementAt(aChain->Length() - 1);
        }
        return false;
    }

    



    PLHashTable* mOrdering;     

    




    PRLock* mLock;

    DeadlockDetector(const DeadlockDetector& aDD);
    DeadlockDetector& operator=(const DeadlockDetector& aDD);
};


template<typename T>
const PLHashAllocOps DeadlockDetector<T>::kAllocOps = {
    DeadlockDetector<T>::TableAlloc, DeadlockDetector<T>::TableFree,
    DeadlockDetector<T>::EntryAlloc, DeadlockDetector<T>::EntryFree
};


template<typename T>

const PRUint32 DeadlockDetector<T>::kDefaultNumBuckets = 64;


} 

#endif 
