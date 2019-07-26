





#ifndef nsISupportsImpl_h__
#define nsISupportsImpl_h__

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsISupportsBase_h__
#include "nsISupportsBase.h"
#endif

#ifndef nsISupportsUtils_h__
#include "nsISupportsUtils.h"
#endif


#if !defined(XPCOM_GLUE_AVOID_NSPR)
#include "prthread.h" 
#include "nsAtomicRefcnt.h" 
#ifdef DEBUG
#include "nsCycleCollectorUtils.h" 
#endif 
#endif 

#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"

inline nsISupports*
ToSupports(nsISupports* p)
{
    return p;
}

inline nsISupports*
ToCanonicalSupports(nsISupports* p)
{
    return NULL;
}




#if defined(DEBUG) && !defined(XPCOM_GLUE_AVOID_NSPR)

class nsAutoOwningThread {
public:
    nsAutoOwningThread() { mThread = PR_GetCurrentThread(); }
    void *GetThread() const { return mThread; }

private:
    void *mThread;
};

#define NS_DECL_OWNINGTHREAD            nsAutoOwningThread _mOwningThread;
#define NS_ASSERT_OWNINGTHREAD(_class) \
  NS_CheckThreadSafe(_mOwningThread.GetThread(), #_class " not thread-safe")
#define NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class) \
  do { \
    if (NS_IsCycleCollectorThread()) { \
      MOZ_CRASH("Changing refcount of " #_class " object during Traverse is " \
                "not permitted!"); \
    } \
    else { \
      NS_ASSERT_OWNINGTHREAD(_class); \
    } \
  } while (0)

#else 

#define NS_DECL_OWNINGTHREAD
#define NS_ASSERT_OWNINGTHREAD(_class)  ((void)0)
#define NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class)  ((void)0)

#endif 



struct nsPurpleBufferEntry {
  union {
    void *mObject;                        
    nsPurpleBufferEntry *mNextInFreeList; 
  };

  nsCycleCollectingAutoRefCnt *mRefCnt;

  nsCycleCollectionParticipant *mParticipant; 
};

#define NS_NUMBER_OF_FLAGS_IN_REFCNT 2
#define NS_IN_PURPLE_BUFFER (1 << 0)
#define NS_IS_PURPLE (1 << 1)
#define NS_REFCOUNT_CHANGE (1 << NS_NUMBER_OF_FLAGS_IN_REFCNT)
#define NS_REFCOUNT_VALUE(_val) (_val >> NS_NUMBER_OF_FLAGS_IN_REFCNT)

class nsCycleCollectingAutoRefCnt {

public:
  nsCycleCollectingAutoRefCnt()
    : mRefCntAndFlags(0)
  {}

  nsCycleCollectingAutoRefCnt(uintptr_t aValue)
    : mRefCntAndFlags(aValue << NS_NUMBER_OF_FLAGS_IN_REFCNT)
  {
  }

  MOZ_ALWAYS_INLINE uintptr_t incr()
  {
    mRefCntAndFlags += NS_REFCOUNT_CHANGE;
    mRefCntAndFlags &= ~NS_IS_PURPLE;
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE void stabilizeForDeletion()
  {
    
    
    mRefCntAndFlags = NS_REFCOUNT_CHANGE | NS_IN_PURPLE_BUFFER;
  }

  MOZ_ALWAYS_INLINE uintptr_t decr(nsISupports *owner,
                                   bool *shouldDelete = nullptr)
  {
    return decr(owner, nullptr, shouldDelete);
  }

  MOZ_ALWAYS_INLINE uintptr_t decr(void *owner, nsCycleCollectionParticipant *p,
                                   bool *shouldDelete = nullptr)
  {
    MOZ_ASSERT(get() > 0);
    if (!IsInPurpleBuffer()) {
      mRefCntAndFlags -= NS_REFCOUNT_CHANGE;
      mRefCntAndFlags |= (NS_IN_PURPLE_BUFFER | NS_IS_PURPLE);
      uintptr_t retval = NS_REFCOUNT_VALUE(mRefCntAndFlags);
      
      NS_CycleCollectorSuspect3(owner, p, this, shouldDelete);
      return retval;
    }
    mRefCntAndFlags -= NS_REFCOUNT_CHANGE;
    mRefCntAndFlags |= (NS_IN_PURPLE_BUFFER | NS_IS_PURPLE);
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE void RemovePurple()
  {
    MOZ_ASSERT(IsPurple(), "must be purple");
    mRefCntAndFlags &= ~NS_IS_PURPLE;
  }

  MOZ_ALWAYS_INLINE void RemoveFromPurpleBuffer()
  {
    MOZ_ASSERT(IsInPurpleBuffer());
    mRefCntAndFlags &= ~(NS_IS_PURPLE | NS_IN_PURPLE_BUFFER);
  }

  MOZ_ALWAYS_INLINE bool IsPurple() const
  {
    return !!(mRefCntAndFlags & NS_IS_PURPLE);
  }

  MOZ_ALWAYS_INLINE bool IsInPurpleBuffer() const
  {
    return !!(mRefCntAndFlags & NS_IN_PURPLE_BUFFER);
  }

  MOZ_ALWAYS_INLINE nsrefcnt get() const
  {
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE operator nsrefcnt() const
  {
    return get();
  }

 private:
  uintptr_t mRefCntAndFlags;
};

class nsAutoRefCnt {

 public:
    nsAutoRefCnt() : mValue(0) {}
    nsAutoRefCnt(nsrefcnt aValue) : mValue(aValue) {}

    
    nsrefcnt operator++() { return ++mValue; }
    nsrefcnt operator--() { return --mValue; }

    nsrefcnt operator=(nsrefcnt aValue) { return (mValue = aValue); }
    operator nsrefcnt() const { return mValue; }
    nsrefcnt get() const { return mValue; }
 private:
    
    nsrefcnt operator++(int);
    nsrefcnt operator--(int);
    nsrefcnt mValue;
};








#define NS_DECL_ISUPPORTS                                                     \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr);                             \
  NS_IMETHOD_(nsrefcnt) AddRef(void);                                         \
  NS_IMETHOD_(nsrefcnt) Release(void);                                        \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
  NS_DECL_OWNINGTHREAD                                                        \
public:

#define NS_DECL_CYCLE_COLLECTING_ISUPPORTS                                    \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr);                             \
  NS_IMETHOD_(nsrefcnt) AddRef(void);                                         \
  NS_IMETHOD_(nsrefcnt) Release(void);                                        \
  NS_IMETHOD_(void) DeleteCycleCollectable(void);                             \
protected:                                                                    \
  nsCycleCollectingAutoRefCnt mRefCnt;                                        \
  NS_DECL_OWNINGTHREAD                                                        \
public:









#define NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                 \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                          \
    nsrefcnt count = mRefCnt.incr();                                          \
    NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                       \
    return count;

#define NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                          \
    nsrefcnt count =                                                          \
      mRefCnt.decr(static_cast<void*>(this),                                  \
                   _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant()); \
    NS_LOG_RELEASE(this, count, #_class);                                     \
    return count;

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_ADDREF(_class)                        \
NS_METHOD_(nsrefcnt) _class::AddRef(void)                                     \
{                                                                             \
  NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                       \
}

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE_WITH_LAST_RELEASE(_class, _last) \
NS_METHOD_(nsrefcnt) _class::Release(void)                                       \
{                                                                                \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                             \
    NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                             \
    bool shouldDelete = false;                                                   \
    nsrefcnt count =                                                             \
      mRefCnt.decr(static_cast<void*>(this),                                     \
                   _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant(),     \
                   &shouldDelete);                                               \
    NS_LOG_RELEASE(this, count, #_class);                                        \
    if (count == 0) {                                                            \
        mRefCnt.incr();                                                          \
        _last;                                                                   \
        mRefCnt.decr(static_cast<void*>(this),                                   \
                     _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant());  \
        if (shouldDelete) {                                                      \
            mRefCnt.stabilizeForDeletion();                                      \
            DeleteCycleCollectable();                                            \
        }                                                                        \
    }                                                                            \
    return count;                                                                \
}

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE(_class)                       \
NS_METHOD_(nsrefcnt) _class::Release(void)                                    \
{                                                                             \
  NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                      \
}

#define NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(_class)            \
public:                                                                       \
  NS_METHOD_(nsrefcnt) AddRef(void) {                                         \
    NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                     \
  }                                                                           \
  NS_METHOD_(nsrefcnt) Release(void) {                                        \
    NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                    \
  }                                                                           \
protected:                                                                    \
  nsCycleCollectingAutoRefCnt mRefCnt;                                        \
  NS_DECL_OWNINGTHREAD                                                        \
public:









#define NS_INIT_ISUPPORTS() ((void)0)







#define NS_INLINE_DECL_REFCOUNTING(_class)                                    \
public:                                                                       \
  NS_METHOD_(nsrefcnt) AddRef(void) {                                         \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                          \
    ++mRefCnt;                                                                \
    NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                     \
    return mRefCnt;                                                           \
  }                                                                           \
  NS_METHOD_(nsrefcnt) Release(void) {                                        \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                          \
    --mRefCnt;                                                                \
    NS_LOG_RELEASE(this, mRefCnt, #_class);                                   \
    if (mRefCnt == 0) {                                                       \
      NS_ASSERT_OWNINGTHREAD(_class);                                         \
      mRefCnt = 1; /* stabilize */                                            \
      delete this;                                                            \
      return 0;                                                               \
    }                                                                         \
    return mRefCnt;                                                           \
  }                                                                           \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
  NS_DECL_OWNINGTHREAD                                                        \
public:









#define NS_INLINE_DECL_THREADSAFE_REFCOUNTING(_class)                         \
public:                                                                       \
  NS_METHOD_(nsrefcnt) AddRef(void) {                                         \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    nsrefcnt count = NS_AtomicIncrementRefcnt(mRefCnt);                       \
    NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                       \
    return (nsrefcnt) count;                                                  \
  }                                                                           \
  NS_METHOD_(nsrefcnt) Release(void) {                                        \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    nsrefcnt count = NS_AtomicDecrementRefcnt(mRefCnt);                       \
    NS_LOG_RELEASE(this, count, #_class);                                     \
    if (count == 0) {                                                         \
      delete (this);                                                          \
      return 0;                                                               \
    }                                                                         \
    return count;                                                             \
  }                                                                           \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
public:





#define NS_IMPL_ADDREF(_class)                                                \
NS_IMETHODIMP_(nsrefcnt) _class::AddRef(void)                                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                        \
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                            \
  ++mRefCnt;                                                                  \
  NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                       \
  return mRefCnt;                                                             \
}








#define NS_IMPL_ADDREF_USING_AGGREGATOR(_class, _aggregator)                  \
NS_IMETHODIMP_(nsrefcnt) _class::AddRef(void)                                 \
{                                                                             \
  NS_PRECONDITION(_aggregator, "null aggregator");                            \
  return (_aggregator)->AddRef();                                             \
}




















#define NS_IMPL_RELEASE_WITH_DESTROY(_class, _destroy)                        \
NS_IMETHODIMP_(nsrefcnt) _class::Release(void)                                \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                            \
  --mRefCnt;                                                                  \
  NS_LOG_RELEASE(this, mRefCnt, #_class);                                     \
  if (mRefCnt == 0) {                                                         \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    mRefCnt = 1; /* stabilize */                                              \
    _destroy;                                                                 \
    return 0;                                                                 \
  }                                                                           \
  return mRefCnt;                                                             \
}














#define NS_IMPL_RELEASE(_class) \
  NS_IMPL_RELEASE_WITH_DESTROY(_class, delete (this))








#define NS_IMPL_RELEASE_USING_AGGREGATOR(_class, _aggregator)                 \
NS_IMETHODIMP_(nsrefcnt) _class::Release(void)                                \
{                                                                             \
  NS_PRECONDITION(_aggregator, "null aggregator");                            \
  return (_aggregator)->Release();                                            \
}


#define NS_IMPL_CYCLE_COLLECTING_ADDREF(_class)                               \
NS_IMETHODIMP_(nsrefcnt) _class::AddRef(void)                                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                        \
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                            \
  nsrefcnt count = mRefCnt.incr();                                            \
  NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                         \
  return count;                                                               \
}

#define NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(_class, _destroy)       \
NS_IMETHODIMP_(nsrefcnt) _class::Release(void)                                \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                            \
  nsISupports *base = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);    \
  nsrefcnt count = mRefCnt.decr(base);                                        \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  return count;                                                               \
}                                                                             \
NS_IMETHODIMP_(void) _class::DeleteCycleCollectable(void)                     \
{                                                                             \
  _destroy;                                                                   \
}

#define NS_IMPL_CYCLE_COLLECTING_RELEASE(_class)                              \
  NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(_class, delete (this))



#define NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_LAST_RELEASE(_class, _last)     \
NS_IMETHODIMP_(nsrefcnt) _class::Release(void)                                \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(_class);                            \
  bool shouldDelete = false;                                                  \
  nsISupports *base = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);    \
  nsrefcnt count = mRefCnt.decr(base, &shouldDelete);                         \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  if (count == 0) {                                                           \
      mRefCnt.incr();                                                         \
      _last;                                                                  \
      mRefCnt.decr(base);                                                     \
      if (shouldDelete) {                                                     \
          mRefCnt.stabilizeForDeletion();                                     \
          DeleteCycleCollectable();                                           \
      }                                                                       \
  }                                                                           \
  return count;                                                               \
}                                                                             \
NS_IMETHODIMP_(void) _class::DeleteCycleCollectable(void)                     \
{                                                                             \
  delete this;                                                                \
}


















struct QITableEntry
{
  const nsIID *iid;     
  int32_t   offset;
};

NS_COM_GLUE nsresult NS_FASTCALL
NS_TableDrivenQI(void* aThis, const QITableEntry* entries,
                 REFNSIID aIID, void **aInstancePtr);





#define NS_INTERFACE_TABLE_HEAD(_class)                                       \
NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
{                                                                             \
  NS_ASSERTION(aInstancePtr,                                                  \
               "QueryInterface requires a non-NULL destination!");            \
  nsresult rv = NS_ERROR_FAILURE;

#define NS_INTERFACE_TABLE_BEGIN                                              \
  static const QITableEntry table[] = {

#define NS_INTERFACE_TABLE_ENTRY(_class, _interface)                          \
  { &_interface::COMTypeInfo<int>::kIID,                                      \
    int32_t(reinterpret_cast<char*>(                                          \
                        static_cast<_interface*>((_class*) 0x1000)) -         \
               reinterpret_cast<char*>((_class*) 0x1000))                     \
  },

#define NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, _interface, _implClass)    \
  { &_interface::COMTypeInfo<int>::kIID,                                      \
    int32_t(reinterpret_cast<char*>(                                          \
                        static_cast<_interface*>(                             \
                                       static_cast<_implClass*>(              \
                                                      (_class*) 0x1000))) -   \
               reinterpret_cast<char*>((_class*) 0x1000))                     \
  },

#define NS_INTERFACE_TABLE_END_WITH_PTR(_ptr)                                 \
  { nullptr, 0 } };                                                           \
  rv = NS_TableDrivenQI(static_cast<void*>(_ptr),                             \
                        table, aIID, aInstancePtr);

#define NS_INTERFACE_TABLE_END                                                \
  NS_INTERFACE_TABLE_END_WITH_PTR(this)

#define NS_INTERFACE_TABLE_TAIL                                               \
  return rv;                                                                  \
}

#define NS_INTERFACE_TABLE_TAIL_INHERITING(_baseclass)                        \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
  return _baseclass::QueryInterface(aIID, aInstancePtr);                      \
}

#define NS_INTERFACE_TABLE_TAIL_USING_AGGREGATOR(_aggregator)                 \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
  NS_ASSERTION(_aggregator, "null aggregator");                               \
  return _aggregator->QueryInterface(aIID, aInstancePtr)                      \
}












#define NS_IMPL_QUERY_HEAD(_class)                                            \
NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
{                                                                             \
  NS_ASSERTION(aInstancePtr,                                                  \
               "QueryInterface requires a non-NULL destination!");            \
  nsISupports* foundInterface;

#define NS_IMPL_QUERY_BODY(_interface)                                        \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(this);                          \
  else

#define NS_IMPL_QUERY_BODY_CONDITIONAL(_interface, condition)                 \
  if ( (condition) && aIID.Equals(NS_GET_IID(_interface)))                    \
    foundInterface = static_cast<_interface*>(this);                          \
  else

#define NS_IMPL_QUERY_BODY_AMBIGUOUS(_interface, _implClass)                  \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(                                \
                                    static_cast<_implClass*>(this));          \
  else

#define NS_IMPL_QUERY_BODY_AGGREGATED(_interface, _aggregate)                 \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(_aggregate);                    \
  else

#define NS_IMPL_QUERY_TAIL_GUTS                                               \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface )                                                      \
    {                                                                         \
      /* nsISupports should be handled by this point. If not, fail. */        \
      MOZ_ASSERT(!aIID.Equals(NS_GET_IID(nsISupports)));                      \
      status = NS_NOINTERFACE;                                                \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL_INHERITING(_baseclass)                             \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface )                                                      \
    status = _baseclass::QueryInterface(aIID, (void**)&foundInterface);       \
  else                                                                        \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL_USING_AGGREGATOR(_aggregator)                      \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface ) {                                                    \
    NS_ASSERTION(_aggregator, "null aggregator");                             \
    status = _aggregator->QueryInterface(aIID, (void**)&foundInterface);      \
  } else                                                                      \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL(_supports_interface)                               \
  NS_IMPL_QUERY_BODY_AMBIGUOUS(nsISupports, _supports_interface)              \
  NS_IMPL_QUERY_TAIL_GUTS


  





#define NS_INTERFACE_MAP_BEGIN(_implClass)      NS_IMPL_QUERY_HEAD(_implClass)
#define NS_INTERFACE_MAP_ENTRY(_interface)      NS_IMPL_QUERY_BODY(_interface)
#define NS_INTERFACE_MAP_ENTRY_CONDITIONAL(_interface, condition)             \
  NS_IMPL_QUERY_BODY_CONDITIONAL(_interface, condition)
#define NS_INTERFACE_MAP_ENTRY_AGGREGATED(_interface,_aggregate)              \
  NS_IMPL_QUERY_BODY_AGGREGATED(_interface,_aggregate)

#define NS_INTERFACE_MAP_END                    NS_IMPL_QUERY_TAIL_GUTS
#define NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(_interface, _implClass)              \
  NS_IMPL_QUERY_BODY_AMBIGUOUS(_interface, _implClass)
#define NS_INTERFACE_MAP_END_INHERITING(_baseClass)                           \
  NS_IMPL_QUERY_TAIL_INHERITING(_baseClass)
#define NS_INTERFACE_MAP_END_AGGREGATED(_aggregator)                          \
  NS_IMPL_QUERY_TAIL_USING_AGGREGATOR(_aggregator)

#define NS_INTERFACE_TABLE0(_class)                                           \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, nsISupports)                             \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE1(_class, _i1)                                      \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE2(_class, _i1, _i2)                                 \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE3(_class, _i1, _i2, _i3)                            \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE4(_class, _i1, _i2, _i3, _i4)                       \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE5(_class, _i1, _i2, _i3, _i4, _i5)                  \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)             \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)        \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE8(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8)   \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE9(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7,        \
                            _i8, _i9)                                         \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE10(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7,       \
                             _i8, _i9, _i10)                                  \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i10)                                    \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE11(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7,       \
                             _i8, _i9, _i10, _i11)                            \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i10)                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, _i11)                                    \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _i1)              \
  NS_INTERFACE_TABLE_END

#define NS_IMPL_QUERY_INTERFACE0(_class)                                      \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE0(_class)                                                 \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE1(_class, _i1)                                 \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE1(_class, _i1)                                            \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE2(_class, _i1, _i2)                            \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE2(_class, _i1, _i2)                                       \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE3(_class, _i1, _i2, _i3)                       \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE3(_class, _i1, _i2, _i3)                                  \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE4(_class, _i1, _i2, _i3, _i4)                  \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE4(_class, _i1, _i2, _i3, _i4)                             \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE5(_class, _i1, _i2, _i3, _i4, _i5)             \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE5(_class, _i1, _i2, _i3, _i4, _i5)                        \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)        \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)                   \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)   \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)              \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE8(_class, _i1, _i2, _i3, _i4, _i5, _i6,        \
                                 _i7, _i8)                                    \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE8(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8)         \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE9(_class, _i1, _i2, _i3, _i4, _i5, _i6,        \
                                 _i7, _i8, _i9)                               \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE9(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8, _i9)    \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE10(_class, _i1, _i2, _i3, _i4, _i5, _i6,       \
                                  _i7, _i8, _i9, _i10)                        \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE10(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,        \
                       _i9, _i10)                                             \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE11(_class, _i1, _i2, _i3, _i4, _i5, _i6,       \
                                  _i7, _i8, _i9, _i10, _i11)                  \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE11(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,        \
                       _i9, _i10, _i11)                                       \
  NS_INTERFACE_TABLE_TAIL


#define NS_IMPL_THREADSAFE_QUERY_INTERFACE0  NS_IMPL_QUERY_INTERFACE0
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE1  NS_IMPL_QUERY_INTERFACE1
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE2  NS_IMPL_QUERY_INTERFACE2
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE3  NS_IMPL_QUERY_INTERFACE3
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE4  NS_IMPL_QUERY_INTERFACE4
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE5  NS_IMPL_QUERY_INTERFACE5
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE6  NS_IMPL_QUERY_INTERFACE6
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE7  NS_IMPL_QUERY_INTERFACE7
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE8  NS_IMPL_QUERY_INTERFACE8
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE9  NS_IMPL_QUERY_INTERFACE9
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE10  NS_IMPL_QUERY_INTERFACE10
#define NS_IMPL_THREADSAFE_QUERY_INTERFACE11  NS_IMPL_QUERY_INTERFACE11













#define NS_DECL_ISUPPORTS_INHERITED                                           \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr);                             \
  NS_IMETHOD_(nsrefcnt) AddRef(void);                                         \
  NS_IMETHOD_(nsrefcnt) Release(void);                                        \









#define NS_IMPL_ADDREF_INHERITED(Class, Super)                                \
NS_IMETHODIMP_(nsrefcnt) Class::AddRef(void)                                  \
{                                                                             \
  nsrefcnt r = Super::AddRef();                                               \
  NS_LOG_ADDREF(this, r, #Class, sizeof(*this));                              \
  return r;                                                                   \
}

#define NS_IMPL_RELEASE_INHERITED(Class, Super)                               \
NS_IMETHODIMP_(nsrefcnt) Class::Release(void)                                 \
{                                                                             \
  nsrefcnt r = Super::Release();                                              \
  NS_LOG_RELEASE(this, r, #Class);                                            \
  return r;                                                                   \
}





#define NS_IMPL_NONLOGGING_ADDREF_INHERITED(Class, Super)                     \
NS_IMETHODIMP_(nsrefcnt) Class::AddRef(void)                                  \
{                                                                             \
  return Super::AddRef();                                                     \
}

#define NS_IMPL_NONLOGGING_RELEASE_INHERITED(Class, Super)                    \
NS_IMETHODIMP_(nsrefcnt) Class::Release(void)                                 \
{                                                                             \
  return Super::Release();                                                    \
}

#define NS_INTERFACE_TABLE_INHERITED0(Class)

#define NS_INTERFACE_TABLE_INHERITED1(Class, i1)                              \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED2(Class, i1, i2)                          \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED3(Class, i1, i2, i3)                      \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED4(Class, i1, i2, i3, i4)                  \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED5(Class, i1, i2, i3, i4, i5)              \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED6(Class, i1, i2, i3, i4, i5, i6)          \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED7(Class, i1, i2, i3, i4, i5, i6, i7)      \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED8(Class, i1, i2, i3, i4, i5, i6, i7, i8)  \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i8)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED9(Class, i1, i2, i3, i4, i5, i6, i7,      \
                                      i8, i9)                                 \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i8)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i9)                                       \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED10(Class, i1, i2, i3, i4, i5, i6, i7,     \
                                      i8, i9, i10)                            \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i8)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i9)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i10)                                      \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED11(Class, i1, i2, i3, i4, i5, i6, i7,     \
                                      i8, i9, i10, i11)                       \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i8)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i9)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i10)                                      \
    NS_INTERFACE_TABLE_ENTRY(Class, i11)                                      \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE_INHERITED12(Class, i1, i2, i3, i4, i5, i6, i7,     \
                                      i8, i9, i10, i11, i12)                  \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(Class, i1)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i2)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i3)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i4)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i5)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i6)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i7)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i8)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i9)                                       \
    NS_INTERFACE_TABLE_ENTRY(Class, i10)                                      \
    NS_INTERFACE_TABLE_ENTRY(Class, i11)                                      \
    NS_INTERFACE_TABLE_ENTRY(Class, i12)                                      \
  NS_INTERFACE_TABLE_END

#define NS_IMPL_QUERY_INTERFACE_INHERITED0(Class, Super)                      \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED0(Class)                                        \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED1(Class, Super, i1)                  \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED1(Class, i1)                                    \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED2(Class, Super, i1, i2)              \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED2(Class, i1, i2)                                \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED3(Class, Super, i1, i2, i3)          \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED3(Class, i1, i2, i3)                            \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED4(Class, Super, i1, i2, i3, i4)      \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED4(Class, i1, i2, i3, i4)                        \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED5(Class,Super,i1,i2,i3,i4,i5)        \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED5(Class, i1, i2, i3, i4, i5)                    \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED6(Class,Super,i1,i2,i3,i4,i5,i6)     \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED6(Class, i1, i2, i3, i4, i5, i6)                \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED7(Class,Super,i1,i2,i3,i4,i5,i6,i7)  \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED7(Class, i1, i2, i3, i4, i5, i6, i7)            \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED8(Class,Super,i1,i2,i3,i4,i5,i6,     \
                                           i7,i8)                             \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED8(Class, i1, i2, i3, i4, i5, i6, i7, i8)        \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED9(Class,Super,i1,i2,i3,i4,i5,i6,     \
                                           i7,i8,i9)                          \
  NS_INTERFACE_TABLE_HEAD(Class)                                              \
  NS_INTERFACE_TABLE_INHERITED9(Class, i1, i2, i3, i4, i5, i6, i7, i8, i9)    \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)

#define NS_IMPL_QUERY_INTERFACE_INHERITED10(Class,Super,i1,i2,i3,i4,i5,i6,       \
                                            i7,i8,i9,i10)                        \
  NS_INTERFACE_TABLE_HEAD(Class)                                                 \
  NS_INTERFACE_TABLE_INHERITED10(Class, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10) \
  NS_INTERFACE_TABLE_TAIL_INHERITING(Super)









#define NS_IMPL_ISUPPORTS0(_class)                                            \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE0(_class)

#define NS_IMPL_ISUPPORTS1(_class, _interface)                                \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE1(_class, _interface)

#define NS_IMPL_ISUPPORTS2(_class, _i1, _i2)                                  \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE2(_class, _i1, _i2)

#define NS_IMPL_ISUPPORTS3(_class, _i1, _i2, _i3)                             \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE3(_class, _i1, _i2, _i3)

#define NS_IMPL_ISUPPORTS4(_class, _i1, _i2, _i3, _i4)                        \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE4(_class, _i1, _i2, _i3, _i4)

#define NS_IMPL_ISUPPORTS5(_class, _i1, _i2, _i3, _i4, _i5)                   \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE5(_class, _i1, _i2, _i3, _i4, _i5)

#define NS_IMPL_ISUPPORTS6(_class, _i1, _i2, _i3, _i4, _i5, _i6)              \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)

#define NS_IMPL_ISUPPORTS7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)         \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)

#define NS_IMPL_ISUPPORTS8(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8)    \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE8(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8)

#define NS_IMPL_ISUPPORTS9(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,    \
                           _i9)                                               \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE9(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8, _i9)

#define NS_IMPL_ISUPPORTS10(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,   \
                            _i9, _i10)                                        \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE10(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,   \
                            _i9, _i10)

#define NS_IMPL_ISUPPORTS11(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,   \
                            _i9, _i10, _i11)                                  \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE11(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7, _i8,   \
                            _i9, _i10, _i11)

#define NS_IMPL_ISUPPORTS_INHERITED0(Class, Super)                            \
    NS_IMPL_QUERY_INTERFACE_INHERITED0(Class, Super)                          \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED1(Class, Super, i1)                        \
    NS_IMPL_QUERY_INTERFACE_INHERITED1(Class, Super, i1)                      \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED2(Class, Super, i1, i2)                    \
    NS_IMPL_QUERY_INTERFACE_INHERITED2(Class, Super, i1, i2)                  \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED3(Class, Super, i1, i2, i3)                \
    NS_IMPL_QUERY_INTERFACE_INHERITED3(Class, Super, i1, i2, i3)              \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED4(Class, Super, i1, i2, i3, i4)            \
    NS_IMPL_QUERY_INTERFACE_INHERITED4(Class, Super, i1, i2, i3, i4)          \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED5(Class, Super, i1, i2, i3, i4, i5)        \
    NS_IMPL_QUERY_INTERFACE_INHERITED5(Class, Super, i1, i2, i3, i4, i5)      \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED6(Class, Super, i1, i2, i3, i4, i5, i6)    \
    NS_IMPL_QUERY_INTERFACE_INHERITED6(Class, Super, i1, i2, i3, i4, i5, i6)  \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED7(Class, Super, i1, i2, i3, i4, i5, i6, i7) \
    NS_IMPL_QUERY_INTERFACE_INHERITED7(Class, Super, i1, i2, i3, i4, i5, i6, i7) \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED8(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8) \
    NS_IMPL_QUERY_INTERFACE_INHERITED8(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8) \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED9(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8, i9) \
    NS_IMPL_QUERY_INTERFACE_INHERITED9(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8, i9) \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \

#define NS_IMPL_ISUPPORTS_INHERITED10(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10) \
    NS_IMPL_QUERY_INTERFACE_INHERITED10(Class, Super, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10) \
    NS_IMPL_ADDREF_INHERITED(Class, Super)                                    \
    NS_IMPL_RELEASE_INHERITED(Class, Super)                                   \
/*
 * Macro to glue together a QI that starts with an interface table
 * and segues into an interface map (e.g. it uses singleton classinfo
 * or tearoffs).
 */
#define NS_INTERFACE_TABLE_TO_MAP_SEGUE \
  if (rv == NS_OK) return rv; \
  nsISupports* foundInterface;











#if !defined(XPCOM_GLUE_AVOID_NSPR)






#define NS_IMPL_THREADSAFE_ADDREF(_class)                                     \
NS_IMETHODIMP_(nsrefcnt) _class::AddRef(void)                                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                        \
  nsrefcnt count = NS_AtomicIncrementRefcnt(mRefCnt);                         \
  NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                         \
  return (nsrefcnt) count;                                                    \
}











#define NS_IMPL_THREADSAFE_RELEASE(_class)                                    \
NS_IMETHODIMP_(nsrefcnt) _class::Release(void)                                \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  nsrefcnt count = NS_AtomicDecrementRefcnt(mRefCnt);                         \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  if (0 == count) {                                                           \
    mRefCnt = 1; /* stabilize */                                              \
    /* enable this to find non-threadsafe destructors: */                     \
    /* NS_ASSERT_OWNINGTHREAD(_class); */                                     \
    delete (this);                                                            \
    return 0;                                                                 \
  }                                                                           \
  return count;                                                               \
}

#else 

#define NS_IMPL_THREADSAFE_ADDREF(_class)                                     \
  THREADSAFE_ISUPPORTS_NOT_AVAILABLE_IN_STANDALONE_GLUE;

#define NS_IMPL_THREADSAFE_RELEASE(_class)                                    \
  THREADSAFE_ISUPPORTS_NOT_AVAILABLE_IN_STANDALONE_GLUE;

#endif

#define NS_IMPL_THREADSAFE_ISUPPORTS0(_class)                                 \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE0(_class)

#define NS_IMPL_THREADSAFE_ISUPPORTS1(_class, _interface)                     \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE1(_class, _interface)

#define NS_IMPL_THREADSAFE_ISUPPORTS2(_class, _i1, _i2)                       \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE2(_class, _i1, _i2)

#define NS_IMPL_THREADSAFE_ISUPPORTS3(_class, _i1, _i2, _i3)                  \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE3(_class, _i1, _i2, _i3)

#define NS_IMPL_THREADSAFE_ISUPPORTS4(_class, _i1, _i2, _i3, _i4)             \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE4(_class, _i1, _i2, _i3, _i4)

#define NS_IMPL_THREADSAFE_ISUPPORTS5(_class, _i1, _i2, _i3, _i4, _i5)        \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE5(_class, _i1, _i2, _i3, _i4, _i5)

#define NS_IMPL_THREADSAFE_ISUPPORTS6(_class, _i1, _i2, _i3, _i4, _i5, _i6)   \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)

#define NS_IMPL_THREADSAFE_ISUPPORTS7(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7)                                    \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE7(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7)

#define NS_IMPL_THREADSAFE_ISUPPORTS8(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7, _i8)                               \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE8(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7, _i8)

#define NS_IMPL_THREADSAFE_ISUPPORTS9(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7, _i8, _i9)                          \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE9(_class, _i1, _i2, _i3, _i4, _i5, _i6,   \
                                      _i7, _i8, _i9)

#define NS_IMPL_THREADSAFE_ISUPPORTS10(_class, _i1, _i2, _i3, _i4, _i5, _i6,  \
                                       _i7, _i8, _i9, _i10)                   \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE10(_class, _i1, _i2, _i3, _i4, _i5, _i6,  \
                                       _i7, _i8, _i9, _i10)

#define NS_IMPL_THREADSAFE_ISUPPORTS11(_class, _i1, _i2, _i3, _i4, _i5, _i6,  \
                                       _i7, _i8, _i9, _i10, _i11)             \
  NS_IMPL_THREADSAFE_ADDREF(_class)                                           \
  NS_IMPL_THREADSAFE_RELEASE(_class)                                          \
  NS_IMPL_THREADSAFE_QUERY_INTERFACE11(_class, _i1, _i2, _i3, _i4, _i5, _i6,  \
                                       _i7, _i8, _i9, _i10, _i11)

#define NS_INTERFACE_MAP_END_THREADSAFE NS_IMPL_QUERY_TAIL_GUTS





#define NS_IMPL_THREADSAFE_CI(_class)                                         \
NS_IMETHODIMP                                                                 \
_class::GetInterfaces(uint32_t* _count, nsIID*** _array)                      \
{                                                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)(_count, _array);                 \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetHelperForLanguage(uint32_t _language, nsISupports** _retval)       \
{                                                                             \
  *_retval = nullptr;                                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetContractID(char** _contractID)                                     \
{                                                                             \
  *_contractID = nullptr;                                                     \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassDescription(char** _classDescription)                         \
{                                                                             \
  *_classDescription = nullptr;                                               \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassID(nsCID** _classID)                                          \
{                                                                             \
  *_classID = nullptr;                                                        \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetImplementationLanguage(uint32_t* _language)                        \
{                                                                             \
  *_language = nsIProgrammingLanguage::CPLUSPLUS;                             \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetFlags(uint32_t* _flags)                                            \
{                                                                             \
  *_flags = nsIClassInfo::THREADSAFE;                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassIDNoAlloc(nsCID* _classIDNoAlloc)                             \
{                                                                             \
  return NS_ERROR_NOT_AVAILABLE;                                              \
}

#endif
