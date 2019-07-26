




#ifndef nsCycleCollectionParticipant_h__
#define nsCycleCollectionParticipant_h__

#include "mozilla/TypeTraits.h"
#include "nsISupports.h"

#define NS_CYCLECOLLECTIONPARTICIPANT_IID                                      \
{                                                                              \
    0x9674489b,                                                                \
    0x1f6f,                                                                    \
    0x4550,                                                                    \
    { 0xa7, 0x30, 0xcc, 0xae, 0xdd, 0x10, 0x4c, 0xf9 }                         \
}








#define NS_CYCLECOLLECTIONISUPPORTS_IID                                        \
{                                                                              \
    0xc61eac14,                                                                \
    0x5f7a,                                                                    \
    0x4481,                                                                    \
    { 0x96, 0x5e, 0x7e, 0xaa, 0x6e, 0xff, 0xa8, 0x5f }                         \
}




class nsCycleCollectionISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_CYCLECOLLECTIONISUPPORTS_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCycleCollectionISupports, 
                              NS_CYCLECOLLECTIONISUPPORTS_IID)




class nsCycleCollectionParticipant;
class nsScriptObjectTracer;
class nsXPCOMCycleCollectionParticipant;




typedef void
(* TraceCallback)(void *p, const char *name, void *closure);

class NS_NO_VTABLE nsCycleCollectionTraversalCallback
{
public:
    
    
    
    
    NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refcount,
                                             size_t objsz,
                                             const char *objname) = 0;
    NS_IMETHOD_(void) DescribeGCedNode(bool ismarked,
                                       size_t objsz,
                                       const char *objname) = 0;

    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) = 0;
    NS_IMETHOD_(void) NoteJSRoot(void *root) = 0;
    NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant) = 0;

    NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child) = 0;
    NS_IMETHOD_(void) NoteJSChild(void *child) = 0;
    NS_IMETHOD_(void) NoteNativeChild(void *child,
                                      nsCycleCollectionParticipant *helper) = 0;

    
    
    
    
    NS_IMETHOD_(void) NoteNextEdgeName(const char* name) = 0;

    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *val) = 0;

    enum {
        

        
        
        
        WANT_DEBUG_INFO = (1<<0),

        
        
        WANT_ALL_TRACES = (1<<1)
    };
    PRUint32 Flags() const { return mFlags; }
    bool WantDebugInfo() const { return (mFlags & WANT_DEBUG_INFO) != 0; }
    bool WantAllTraces() const { return (mFlags & WANT_ALL_TRACES) != 0; }
protected:
    nsCycleCollectionTraversalCallback() : mFlags(0) {}

    PRUint32 mFlags;
};

























template <typename T>
struct nsCycleCollectionParticipantVTableCommon
{
    nsresult (NS_STDCALL *TraverseReal)
        (T *that, void *p, nsCycleCollectionTraversalCallback &cb);

    nsresult (NS_STDCALL *Root)(void *p);
    nsresult (NS_STDCALL *Unlink)(void *p);
    nsresult (NS_STDCALL *Unroot)(void *p);

    bool (NS_STDCALL *CanSkipReal)(void *p, bool aRemovingAllowed);
    bool (NS_STDCALL *CanSkipInCCReal)(void *p);
    bool (NS_STDCALL *CanSkipThisReal)(void *p);
};

typedef nsCycleCollectionParticipantVTableCommon<nsCycleCollectionParticipant>
    nsCycleCollectionParticipantVTable;


struct nsScriptObjectTracerVTable
{
    void (NS_STDCALL *Trace)(void *p, TraceCallback cb, void *closure);
};


struct nsXPCOMCycleCollectionParticipantVTable
{
    void (NS_STDCALL *UnmarkIfPurple)(nsISupports *p);
};























enum nsCycleCollectionParticipantType
{
  eInvalid,
  eCycleCollectionParticipant,
  eScriptObjectTracer,
  eXPCOMCycleCollectionParticipant
};

template <typename T, enum nsCycleCollectionParticipantType ParticipantType>
struct CCParticipantVTableImpl { };


template <typename T>
struct CCParticipantVTableImpl<T, eCycleCollectionParticipant>
{
    nsCycleCollectionParticipant *GetParticipant() const
    {
        return const_cast<nsCycleCollectionParticipant *>
               (reinterpret_cast<const nsCycleCollectionParticipant *>(this));
    }
    nsCycleCollectionParticipantVTableCommon<T> cycleCollectionParticipant;
};


template <typename T>
struct CCParticipantVTableImpl<T, eScriptObjectTracer>
{
    nsScriptObjectTracer *GetParticipant() const
    {
        return const_cast<nsScriptObjectTracer *>
               (reinterpret_cast<const nsScriptObjectTracer *>(this));
    }
    nsCycleCollectionParticipantVTableCommon<T> cycleCollectionParticipant;
    nsScriptObjectTracerVTable scriptObjectTracer;
};


template <typename T>
struct CCParticipantVTableImpl<T, eXPCOMCycleCollectionParticipant>
{
    nsXPCOMCycleCollectionParticipant *GetParticipant() const
    {
        return const_cast<nsXPCOMCycleCollectionParticipant *>
               (reinterpret_cast<const nsXPCOMCycleCollectionParticipant *>(this));
    }
    nsCycleCollectionParticipantVTableCommon<T> cycleCollectionParticipant;
    nsScriptObjectTracerVTable scriptObjectTracer;
    nsXPCOMCycleCollectionParticipantVTable XPCOMCycleCollectionParticipant;
};

template <typename T>
struct CCParticipantVTable
{
    static const enum nsCycleCollectionParticipantType ParticipantType =
        mozilla::IsBaseOf<nsXPCOMCycleCollectionParticipant, T>::value ? eXPCOMCycleCollectionParticipant :
        mozilla::IsBaseOf<nsScriptObjectTracer, T>::value ? eScriptObjectTracer :
        mozilla::IsBaseOf<nsCycleCollectionParticipant, T>::value ? eCycleCollectionParticipant :
        eInvalid;
    typedef CCParticipantVTableImpl<T, ParticipantType> Type;
};




class nsCycleCollectionParticipant : public nsCycleCollectionParticipantVTable
{
public:
    static const bool isSkippable = false;

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_CYCLECOLLECTIONPARTICIPANT_IID)

    
    
    NS_METHOD Traverse(void *p, nsCycleCollectionTraversalCallback &cb) {
        return TraverseReal(this, p, cb);
    }
    
    
    
    
    bool CanSkip(void *p, bool aRemovingAllowed)
    {
        return CanSkipReal ? CanSkipReal(p, aRemovingAllowed) : false;
    }

    
    
    
    bool CanSkipInCC(void *p)
    {
        return CanSkipInCCReal ? CanSkipInCCReal(p) : false;
    }

    
    
    
    bool CanSkipThis(void *p)
    {
        return CanSkipThisReal ? CanSkipThisReal(p) : false;
    }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCycleCollectionParticipant, 
                              NS_CYCLECOLLECTIONPARTICIPANT_IID)

class nsScriptObjectTracer
    : public nsCycleCollectionParticipant, public nsScriptObjectTracerVTable
{
public:
    static void NS_COM_GLUE NoteJSChild(void *aScriptThing, const char *name,
                                        void *aClosure);
};

class nsXPCOMCycleCollectionParticipant
    : public nsScriptObjectTracer, public nsXPCOMCycleCollectionParticipantVTable
{
public:
    static NS_METHOD TraverseImpl(nsXPCOMCycleCollectionParticipant *that,
                                  void *p, nsCycleCollectionTraversalCallback &cb);

    static NS_METHOD RootImpl(void *p);
    static NS_METHOD UnlinkImpl(void *p);
    static NS_METHOD UnrootImpl(void *p);

    static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb, void *closure);

    static NS_METHOD_(void) UnmarkIfPurpleImpl(nsISupports *p);

    static bool CheckForRightISupports(nsISupports *s);
};





#define NS_CYCLE_COLLECTION_INNERCLASS                                         \
        cycleCollection

#define NS_CYCLE_COLLECTION_CLASSNAME(_class)                                  \
        _class::NS_CYCLE_COLLECTION_INNERCLASS

#define NS_CYCLE_COLLECTION_INNERNAME                                          \
        _cycleCollectorGlobal

#define NS_CYCLE_COLLECTION_PARTICIPANT(_class)                                \
        _class::NS_CYCLE_COLLECTION_INNERNAME.GetParticipant()

#define NS_IMPL_QUERY_CYCLE_COLLECTION(_class)                                 \
  if ( aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant)) ) {          \
    *aInstancePtr = NS_CYCLE_COLLECTION_PARTICIPANT(_class);                   \
    return NS_OK;                                                              \
  } else

#define NS_IMPL_QUERY_CYCLE_COLLECTION_ISUPPORTS(_class)                       \
  if ( aIID.Equals(NS_GET_IID(nsCycleCollectionISupports)) ) {                 \
    *aInstancePtr = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);       \
    return NS_OK;                                                              \
  } else

#define NS_INTERFACE_MAP_ENTRY_CYCLE_COLLECTION(_class)                        \
  NS_IMPL_QUERY_CYCLE_COLLECTION(_class)

#define NS_INTERFACE_MAP_ENTRY_CYCLE_COLLECTION_ISUPPORTS(_class)              \
  NS_IMPL_QUERY_CYCLE_COLLECTION_ISUPPORTS(_class)

#define NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(_class)                      \
  NS_INTERFACE_MAP_ENTRY_CYCLE_COLLECTION(_class)                              \
  NS_INTERFACE_MAP_ENTRY_CYCLE_COLLECTION_ISUPPORTS(_class)

#define NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(_class)                        \
  NS_INTERFACE_MAP_BEGIN(_class)                                               \
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(_class)

#define NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(_class)              \
  NS_INTERFACE_MAP_BEGIN(_class)                                               \
    NS_INTERFACE_MAP_ENTRY_CYCLE_COLLECTION(_class)

#define NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(_class)  \
  if (rv == NS_OK) return rv; \
  nsISupports* foundInterface; \
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(_class)

#define NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(_class)            \
  NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)    \
  {                                                                           \
    NS_PRECONDITION(aInstancePtr, "null out param");                          \
                                                                              \
    if ( aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant)) ) {       \
      *aInstancePtr = NS_CYCLE_COLLECTION_PARTICIPANT(_class);                \
      return NS_OK;                                                           \
    }                                                                         \
    nsresult rv;

#define NS_CYCLE_COLLECTION_UPCAST(obj, clazz)                                 \
  NS_CYCLE_COLLECTION_CLASSNAME(clazz)::Upcast(obj)





#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(_class)                        \
  NS_METHOD_(bool)                                                             \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::CanSkipImpl(void *p,                  \
                                                     bool aRemovingAllowed)    \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = Downcast(s);

#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END                                  \
    (void)tmp;                                                                 \
    return false;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(_class)                  \
  NS_METHOD_(bool)                                                             \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::CanSkipInCCImpl(void *p)              \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = Downcast(s);

#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END                            \
    (void)tmp;                                                                 \
    return false;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(_class)                   \
  NS_METHOD_(bool)                                                             \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::CanSkipThisImpl(void *p)              \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = Downcast(s);

#define NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END                             \
    (void)tmp;                                                                 \
    return false;                                                              \
  }





#define NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                          \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnlinkImpl(void *p)                   \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = Downcast(s);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(_class, _base_class)   \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnlinkImpl(void *p)                   \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = static_cast<_class*>(Downcast(s));                           \
    NS_CYCLE_COLLECTION_CLASSNAME(_base_class)::UnlinkImpl(s);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(_class)                   \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnlinkImpl(void *p)                   \
  {                                                                            \
    _class *tmp = static_cast<_class*>(p);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_field)                       \
    tmp->_field = NULL;    

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(_field)                     \
    tmp->_field.Clear();

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(_field)                       \
    tmp->_field.Clear();

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                    \
    (void)tmp;                                                                 \
    return NS_OK;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_0(_class)                              \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnlinkImpl(void *p)                   \
  {                                                                            \
    NS_ASSERTION(CheckForRightISupports(static_cast<nsISupports*>(p)),         \
                 "not the nsISupports pointer we expect");                     \
    return NS_OK;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_NATIVE_0(_class)                       \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnlinkImpl(void *p)                   \
  {                                                                            \
    return NS_OK;                                                              \
  }






#define NS_IMPL_CYCLE_COLLECTION_DESCRIBE(_class, _refcnt)                     \
    cb.DescribeRefCountedNode(_refcnt, sizeof(_class), #_class);

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(_class)               \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::TraverseImpl                          \
                         (NS_CYCLE_COLLECTION_CLASSNAME(_class) *that, void *p,\
                          nsCycleCollectionTraversalCallback &cb)              \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = static_cast<_class*>(Downcast(s));

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                        \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(_class)                     \
  NS_IMPL_CYCLE_COLLECTION_DESCRIBE(_class, tmp->mRefCnt.get())





#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(_class, _base_class) \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(_class)                     \
    if (NS_CYCLE_COLLECTION_CLASSNAME(_base_class)::TraverseImpl(that, s, cb)  \
        == NS_SUCCESS_INTERRUPTED_TRAVERSE) {                                  \
      return NS_SUCCESS_INTERRUPTED_TRAVERSE;                                  \
    }

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(_class)                 \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::TraverseImpl                          \
                         (NS_CYCLE_COLLECTION_CLASSNAME(_class) *that, void *p,\
                          nsCycleCollectionTraversalCallback &cb)              \
  {                                                                            \
    _class *tmp = static_cast<_class*>(p);                                     \
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(_class, tmp->mRefCnt.get())

#define NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(_cb, _name)                         \
  PR_BEGIN_MACRO                                                               \
    if (NS_UNLIKELY((_cb).WantDebugInfo())) {                                  \
      (_cb).NoteNextEdgeName(_name);                                           \
    }                                                                          \
  PR_END_MACRO

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(_field)                       \
  PR_BEGIN_MACRO                                                               \
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, #_field);                           \
    cb.NoteXPCOMChild(tmp->_field);                                            \
  PR_END_MACRO;

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_field)                     \
  PR_BEGIN_MACRO                                                               \
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, #_field);                           \
    cb.NoteXPCOMChild(tmp->_field.get());                                      \
  PR_END_MACRO;

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(_field, _base)    \
  PR_BEGIN_MACRO                                                               \
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, #_field);                           \
    cb.NoteXPCOMChild(NS_ISUPPORTS_CAST(_base*, tmp->_field));                 \
  PR_END_MACRO;

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(_field)                   \
    {                                                                          \
      PRInt32 i;                                                               \
      for (i = 0; i < tmp->_field.Count(); ++i) {                              \
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, #_field "[i]");                 \
        cb.NoteXPCOMChild(tmp->_field[i]);                                     \
      }                                                                        \
    }

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(_ptr, _ptr_class, _name)  \
  PR_BEGIN_MACRO                                                               \
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, _name);                             \
    cb.NoteNativeChild(_ptr, NS_CYCLE_COLLECTION_PARTICIPANT(_ptr_class));     \
  PR_END_MACRO;

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(_field, _field_class)  \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->_field, _field_class,      \
                                               #_field)

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY(_array, _element_class,     \
                                                   _name)                      \
    {                                                                          \
      PRUint32 i, length = (_array).Length();                                  \
      for (i = 0; i < length; ++i)                                             \
        NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR((_array)[i],              \
                                                     _element_class,           \
                                                     _name "[i]");             \
    }

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_OF_NSCOMPTR(_field)         \
    {                                                                          \
      PRUint32 i, length = tmp->_field.Length();                               \
      for (i = 0; i < length; ++i) {                                           \
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, #_field "[i]");                 \
        cb.NoteXPCOMChild(tmp->_field[i].get());                               \
      }                                                                        \
    }

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(_field,              \
                                                          _element_class)      \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY(tmp->_field, _element_class,    \
                                               #_field)

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS                       \
    that->Trace(p, &nsScriptObjectTracer::NoteJSChild, &cb);

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                                  \
    (void)tmp;                                                                 \
    return NS_OK;                                                              \
  }





#define NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(_class)                           \
  void                                                                         \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::TraceImpl(void *p,                    \
                                                   TraceCallback aCallback,    \
                                                   void *aClosure)             \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = Downcast(s);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(_class, _base_class)    \
  void                                                                         \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::TraceImpl(void *p,                    \
                                                   TraceCallback aCallback,    \
                                                   void *aClosure)             \
  {                                                                            \
    nsISupports *s = static_cast<nsISupports*>(p);                             \
    NS_ASSERTION(CheckForRightISupports(s),                                    \
                 "not the nsISupports pointer we expect");                     \
    _class *tmp = static_cast<_class*>(Downcast(s));                           \
    NS_CYCLE_COLLECTION_CLASSNAME(_base_class)::TraceImpl(s,                   \
                                                          aCallback,           \
                                                          aClosure);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_NATIVE_BEGIN(_class)                    \
  void                                                                         \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::TraceImpl(void *p,                    \
                                                   TraceCallback aCallback,    \
                                                   void *aClosure)             \
  {                                                                            \
    _class *tmp = static_cast<_class*>(p);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(_object, _name)             \
  if (_object)                                                                 \
    aCallback(_object, _name, aClosure);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(_field)              \
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->_field, #_field)

#define NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(_field)           \
  if (JSVAL_IS_TRACEABLE(tmp->_field)) {                                       \
    void *gcThing = JSVAL_TO_TRACEABLE(tmp->_field);                           \
    aCallback(gcThing, #_field, aClosure);                                     \
  }




#define NS_IMPL_CYCLE_COLLECTION_TRACE_END                                     \
      (void)tmp;                                                               \
  }





#define NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE                               \
  static const CCParticipantVTable<NS_CYCLE_COLLECTION_INNERCLASS>::Type       \
      NS_CYCLE_COLLECTION_INNERNAME;

#define NS_DECL_CYCLE_COLLECTION_CLASS_BODY_NO_UNLINK(_class, _base)           \
public:                                                                        \
  static NS_METHOD TraverseImpl(NS_CYCLE_COLLECTION_CLASSNAME(_class) *that,   \
                            void *p, nsCycleCollectionTraversalCallback &cb);  \
  static NS_METHOD_(void) UnmarkIfPurpleImpl(nsISupports *s)                   \
  {                                                                            \
    Downcast(s)->UnmarkIfPurple();                                             \
  }                                                                            \
  static _class* Downcast(nsISupports* s)                                      \
  {                                                                            \
    return static_cast<_class*>(static_cast<_base*>(s));                       \
  }                                                                            \
  static nsISupports* Upcast(_class *p)                                        \
  {                                                                            \
    return NS_ISUPPORTS_CAST(_base*, p);                                       \
  }

#define NS_DECL_CYCLE_COLLECTION_CLASS_BODY(_class, _base)                     \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY_NO_UNLINK(_class, _base)                 \
  static NS_METHOD UnlinkImpl(void *p);

#define NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(_class, _base)                \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public nsXPCOMCycleCollectionParticipant                                    \
{                                                                              \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY(_class, _base)                           \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_CLASS(_class)                                 \
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(_class, _class)


#define NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS_AMBIGUOUS(_class, _base)        \
class NS_CYCLE_COLLECTION_INNERCLASS                                             \
 : public nsXPCOMCycleCollectionParticipant                                      \
{                                                                                \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY(_class, _base)                             \
  static const bool isSkippable = true;                                          \
  static NS_METHOD_(bool) CanSkipImpl(void *p, bool aRemovingAllowed);           \
  static NS_METHOD_(bool) CanSkipInCCImpl(void *p);                              \
  static NS_METHOD_(bool) CanSkipThisImpl(void *p);                              \
};                                                                               \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS(_class)                       \
        NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS_AMBIGUOUS(_class, _class)




#define NS_DECL_CYCLE_COLLECTION_CLASS_NO_UNLINK(_class)                       \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public nsXPCOMCycleCollectionParticipant                                    \
{                                                                              \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY_NO_UNLINK(_class, _class)                \
  static NS_METHOD RootImpl(void *p)                                           \
  {                                                                            \
    return NS_OK;                                                              \
  }                                                                            \
  static NS_METHOD UnlinkImpl(void *p)                                         \
  {                                                                            \
    return NS_OK;                                                              \
  }                                                                            \
  static NS_METHOD UnrootImpl(void *p)                                         \
  {                                                                            \
    return NS_OK;                                                              \
  }                                                                            \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(_class, _base)  \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public nsXPCOMCycleCollectionParticipant                                    \
{                                                                              \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY(_class, _base)                           \
  static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb, void *closure); \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(_class, _base)   \
class NS_CYCLE_COLLECTION_INNERCLASS                                                      \
 : public nsXPCOMCycleCollectionParticipant                                               \
{                                                                                         \
  NS_DECL_CYCLE_COLLECTION_CLASS_BODY(_class, _base)                                      \
  static const bool isSkippable = true;                                                   \
  static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb, void *closure);            \
  static NS_METHOD_(bool) CanSkipImpl(void *p, bool aRemovingAllowed);                    \
  static NS_METHOD_(bool) CanSkipInCCImpl(void *p);                                       \
  static NS_METHOD_(bool) CanSkipThisImpl(void *p);                                       \
};                                                                                        \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(_class)  \
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(_class, _class)

#define NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(_class,      \
                                                                         _base_class) \
class NS_CYCLE_COLLECTION_INNERCLASS                                                  \
 : public NS_CYCLE_COLLECTION_CLASSNAME(_base_class)                                  \
{                                                                                     \
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY(_class, _base_class)                  \
  static const bool isSkippable = true;                                               \
  static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb, void *closure);        \
  static NS_METHOD_(bool) CanSkipImpl(void *p, bool aRemovingAllowed);                \
  static NS_METHOD_(bool) CanSkipInCCImpl(void *p);                                   \
  static NS_METHOD_(bool) CanSkipThisImpl(void *p);                                   \
};                                                                                    \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(_class)  \
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(_class, _class)

#define NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY_NO_UNLINK(_class,        \
                                                                _base_class)   \
public:                                                                        \
  static NS_METHOD TraverseImpl(NS_CYCLE_COLLECTION_CLASSNAME(_class) *that,   \
                         void *p, nsCycleCollectionTraversalCallback &cb);     \
  static _class* Downcast(nsISupports* s)                                      \
  {                                                                            \
    return static_cast<_class*>(static_cast<_base_class*>(                     \
      NS_CYCLE_COLLECTION_CLASSNAME(_base_class)::Downcast(s)));               \
  }

#define NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY(_class, _base_class)     \
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY_NO_UNLINK(_class, _base_class) \
  static NS_METHOD UnlinkImpl(void *p);

#define NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(_class, _base_class)          \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public NS_CYCLE_COLLECTION_CLASSNAME(_base_class)                           \
{                                                                              \
public:                                                                        \
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY(_class, _base_class)           \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(_class,             \
                                                           _base_class)        \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public NS_CYCLE_COLLECTION_CLASSNAME(_base_class)                           \
{                                                                              \
public:                                                                        \
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY_NO_UNLINK(_class, _base_class) \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(_class,         \
                                                               _base_class)    \
class NS_CYCLE_COLLECTION_INNERCLASS                                           \
 : public NS_CYCLE_COLLECTION_CLASSNAME(_base_class)                           \
{                                                                              \
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY(_class, _base_class)           \
  static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb, void *closure); \
};                                                                             \
NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE







#define NS_DECL_CYCLE_COLLECTION_UNMARK_PURPLE_STUB(_class)                    \
  NS_IMETHODIMP_(void) UnmarkIfPurple()                                        \
  {                                                                            \
  }                                                                            \






struct SkippableDummy
{
  static NS_METHOD_(bool) CanSkipImpl(void *p, bool aRemovingAllowed) { return false; }
  static NS_METHOD_(bool) CanSkipInCCImpl(void *p) { return false; }
  static NS_METHOD_(bool) CanSkipThisImpl(void *p) { return false; }
};







template <class T>
struct Skippable
 : public mozilla::Conditional<T::isSkippable, T, SkippableDummy>::Type
{ };

#define NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(_class)                         \
  {                                                                            \
    &_class::TraverseImpl,                                                     \
    &_class::RootImpl,                                                         \
    &_class::UnlinkImpl,                                                       \
    &_class::UnrootImpl,                                                       \
    _class::isSkippable ? &Skippable<_class>::CanSkipImpl : NULL,              \
    _class::isSkippable ? &Skippable<_class>::CanSkipInCCImpl : NULL,          \
    _class::isSkippable ? &Skippable<_class>::CanSkipThisImpl : NULL           \
  }
 
#define NS_IMPL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_VTABLE(_class)           \
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(_class),                              \
  { &_class::TraceImpl }

#define NS_IMPL_CYCLE_COLLECTION_VTABLE(_class)                                \
  NS_IMPL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_VTABLE(_class),                \
  { &_class::UnmarkIfPurpleImpl }

#define NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(_class)                          \
  const CCParticipantVTable<NS_CYCLE_COLLECTION_CLASSNAME(_class)>             \
    ::Type _class::NS_CYCLE_COLLECTION_INNERNAME =                             \
  { NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(NS_CYCLE_COLLECTION_CLASSNAME(_class)) };

#define NS_IMPL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(_class)            \
  const CCParticipantVTable<NS_CYCLE_COLLECTION_CLASSNAME(_class)>             \
    ::Type _class::NS_CYCLE_COLLECTION_INNERNAME =                             \
  { NS_IMPL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_VTABLE(NS_CYCLE_COLLECTION_CLASSNAME(_class)) };

#define NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                 \
  const CCParticipantVTable<NS_CYCLE_COLLECTION_CLASSNAME(_class)>             \
    ::Type _class::NS_CYCLE_COLLECTION_INNERNAME =                             \
  { NS_IMPL_CYCLE_COLLECTION_VTABLE(NS_CYCLE_COLLECTION_CLASSNAME(_class)) };

#define NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS_BODY(_class)                     \
  public:                                                                      \
    static NS_METHOD RootImpl(void *n);                                        \
    static NS_METHOD UnlinkImpl(void *n);                                      \
    static NS_METHOD UnrootImpl(void *n);                                      \
    static NS_METHOD TraverseImpl(NS_CYCLE_COLLECTION_CLASSNAME(_class) *that, \
                           void *n, nsCycleCollectionTraversalCallback &cb);

#define NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(_class)                          \
  class NS_CYCLE_COLLECTION_INNERCLASS                                         \
   : public nsCycleCollectionParticipant                                       \
  {                                                                            \
     NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS_BODY(_class)                        \
  };                                                                           \
  NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(_class)            \
  class NS_CYCLE_COLLECTION_INNERCLASS                                         \
   : public nsScriptObjectTracer                                               \
  {                                                                            \
    NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS_BODY(_class)                         \
    static NS_METHOD_(void) TraceImpl(void *p, TraceCallback cb,               \
                                      void *closure);                          \
  };                                                                           \
  NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

#define NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(_class, _root_function)           \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::RootImpl(void *p)                     \
  {                                                                            \
    _class *tmp = static_cast<_class*>(p);                                     \
    tmp->_root_function();                                                     \
    return NS_OK;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(_class, _unroot_function)       \
  NS_METHOD                                                                    \
  NS_CYCLE_COLLECTION_CLASSNAME(_class)::UnrootImpl(void *p)                   \
  {                                                                            \
    _class *tmp = static_cast<_class*>(p);                                     \
    tmp->_unroot_function();                                                   \
    return NS_OK;                                                              \
  }

#define NS_IMPL_CYCLE_COLLECTION_0(_class)                                     \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_0(_class)                                     \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_1(_class, _f)                                 \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f)                                  \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f)                                \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_2(_class, _f1, _f2)                           \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_3(_class, _f1, _f2, _f3)                      \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_4(_class, _f1, _f2, _f3, _f4)                 \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f4)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f4)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_5(_class, _f1, _f2, _f3, _f4, _f5)            \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f4)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f5)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f4)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f5)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_6(_class, _f1, _f2, _f3, _f4, _f5, _f6)       \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f4)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f5)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f6)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f4)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f5)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f6)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_7(_class, _f1, _f2, _f3, _f4, _f5, _f6, _f7)  \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f4)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f5)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f6)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f7)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f4)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f5)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f6)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f7)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_IMPL_CYCLE_COLLECTION_8(_class, _f1, _f2, _f3, _f4, _f5, _f6, _f7, _f8) \
 NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                                        \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f1)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f2)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f3)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f4)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f5)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f6)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f7)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(_f8)                                 \
 NS_IMPL_CYCLE_COLLECTION_UNLINK_END                                           \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f1)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f2)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f3)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f4)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f5)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f6)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f7)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(_f8)                               \
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#endif 
