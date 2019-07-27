





#ifndef jspubtd_h
#define jspubtd_h





#include "mozilla/Assertions.h"
#include "mozilla/LinkedList.h"
#include "mozilla/PodOperations.h"

#include "jsprototypes.h"
#include "jstypes.h"

#include "js/TypeDecls.h"

#if defined(JS_GC_ZEAL) || defined(DEBUG)
# define JSGC_HASH_TABLE_CHECKS
#endif

namespace JS {

template <typename T>
class AutoVectorRooter;
typedef AutoVectorRooter<jsid> AutoIdVector;
class CallArgs;

template <typename T>
class Rooted;

class JS_FRIEND_API(CompileOptions);
class JS_FRIEND_API(ReadOnlyCompileOptions);
class JS_FRIEND_API(OwningCompileOptions);
class JS_PUBLIC_API(CompartmentOptions);

class Value;
struct Zone;

} 

namespace js {
struct ContextFriendFields;
class Shape;
} 






enum JSVersion {
    JSVERSION_ECMA_3  = 148,
    JSVERSION_1_6     = 160,
    JSVERSION_1_7     = 170,
    JSVERSION_1_8     = 180,
    JSVERSION_ECMA_5  = 185,
    JSVERSION_DEFAULT = 0,
    JSVERSION_UNKNOWN = -1,
    JSVERSION_LATEST  = JSVERSION_ECMA_5
};


enum JSType {
    JSTYPE_VOID,                
    JSTYPE_OBJECT,              
    JSTYPE_FUNCTION,            
    JSTYPE_STRING,              
    JSTYPE_NUMBER,              
    JSTYPE_BOOLEAN,             
    JSTYPE_NULL,                
    JSTYPE_SYMBOL,              
    JSTYPE_LIMIT
};


enum JSProtoKey {
#define PROTOKEY_AND_INITIALIZER(name,code,init,clasp) JSProto_##name = code,
    JS_FOR_EACH_PROTOTYPE(PROTOKEY_AND_INITIALIZER)
#undef PROTOKEY_AND_INITIALIZER
    JSProto_LIMIT
};


struct JSClass;
struct JSCompartment;
struct JSCrossCompartmentCall;
class JSErrorReport;
struct JSExceptionState;
struct JSFunctionSpec;
struct JSIdArray;
struct JSLocaleCallbacks;
struct JSObjectMap;
struct JSPrincipals;
struct JSPropertyDescriptor;
struct JSPropertyName;
struct JSPropertySpec;
struct JSRuntime;
struct JSSecurityCallbacks;
struct JSStructuredCloneCallbacks;
struct JSStructuredCloneReader;
struct JSStructuredCloneWriter;
class JS_PUBLIC_API(JSTracer);

class JSFlatString;

typedef struct PRCallOnceType   JSCallOnceType;
typedef bool                    (*JSInitCallback)(void);

template<typename T> struct JSConstScalarSpec;
typedef JSConstScalarSpec<double> JSConstDoubleSpec;
typedef JSConstScalarSpec<int32_t> JSConstIntegerSpec;





typedef void
(* JSTraceDataOp)(JSTracer* trc, void* data);

namespace js {

void FinishGC(JSRuntime* rt);

namespace gc {
class AutoTraceSession;
class StoreBuffer;
void MarkPersistentRootedChains(JSTracer*);
void FinishPersistentRootedChains(JSRuntime*);
} 
} 

namespace JS {

typedef void (*OffThreadCompileCallback)(void* token, void* callbackData);

enum class HeapState {
    Idle,             
    Tracing,          
    MajorCollecting,  
    MinorCollecting   
};

namespace shadow {

struct Runtime
{
  protected:
    
    friend class js::gc::AutoTraceSession;
    JS::HeapState heapState_;

    js::gc::StoreBuffer* gcStoreBufferPtr_;

  public:
    Runtime()
      : heapState_(JS::HeapState::Idle)
      , gcStoreBufferPtr_(nullptr)
    {}

    bool isHeapBusy() const { return heapState_ != JS::HeapState::Idle; }

    js::gc::StoreBuffer* gcStoreBufferPtr() { return gcStoreBufferPtr_; }

    static JS::shadow::Runtime* asShadowRuntime(JSRuntime* rt) {
        return reinterpret_cast<JS::shadow::Runtime*>(rt);
    }

  protected:
    void setGCStoreBufferPtr(js::gc::StoreBuffer* storeBuffer) {
        gcStoreBufferPtr_ = storeBuffer;
    }

    
  private:
    template <typename Referent> friend class JS::PersistentRooted;
    friend void js::gc::MarkPersistentRootedChains(JSTracer*);
    friend void js::gc::FinishPersistentRootedChains(JSRuntime* rt);

    mozilla::LinkedList<PersistentRootedFunction> functionPersistentRooteds;
    mozilla::LinkedList<PersistentRootedId>       idPersistentRooteds;
    mozilla::LinkedList<PersistentRootedObject>   objectPersistentRooteds;
    mozilla::LinkedList<PersistentRootedScript>   scriptPersistentRooteds;
    mozilla::LinkedList<PersistentRootedString>   stringPersistentRooteds;
    mozilla::LinkedList<PersistentRootedValue>    valuePersistentRooteds;

    
    template<typename Referent>
    inline mozilla::LinkedList<PersistentRooted<Referent> >& getPersistentRootedList();
};

template<>
inline mozilla::LinkedList<PersistentRootedFunction>
&Runtime::getPersistentRootedList<JSFunction*>() { return functionPersistentRooteds; }

template<>
inline mozilla::LinkedList<PersistentRootedId>
&Runtime::getPersistentRootedList<jsid>() { return idPersistentRooteds; }

template<>
inline mozilla::LinkedList<PersistentRootedObject>
&Runtime::getPersistentRootedList<JSObject*>() { return objectPersistentRooteds; }

template<>
inline mozilla::LinkedList<PersistentRootedScript>
&Runtime::getPersistentRootedList<JSScript*>() { return scriptPersistentRooteds; }

template<>
inline mozilla::LinkedList<PersistentRootedString>
&Runtime::getPersistentRootedList<JSString*>() { return stringPersistentRooteds; }

template<>
inline mozilla::LinkedList<PersistentRootedValue>
&Runtime::getPersistentRootedList<Value>() { return valuePersistentRooteds; }

} 

class JS_PUBLIC_API(AutoGCRooter)
{
  public:
    AutoGCRooter(JSContext* cx, ptrdiff_t tag);
    AutoGCRooter(js::ContextFriendFields* cx, ptrdiff_t tag);

    ~AutoGCRooter() {
        MOZ_ASSERT(this == *stackTop);
        *stackTop = down;
    }

    
    inline void trace(JSTracer* trc);
    static void traceAll(JSTracer* trc);
    static void traceAllWrappers(JSTracer* trc);

    
    template<typename T>
    static void traceAllInContext(T* cx, JSTracer* trc) {
        for (AutoGCRooter* gcr = cx->roots.autoGCRooters_; gcr; gcr = gcr->down)
            gcr->trace(trc);
    }

  protected:
    AutoGCRooter * const down;

    






    ptrdiff_t tag_;

    enum {
        VALARRAY =     -2, 
        PARSER =       -3, 
        SHAPEVECTOR =  -4, 
        IDARRAY =      -6, 
        DESCVECTOR =   -7, 
        VALVECTOR =   -10, 
        IDVECTOR =    -11, 
        IDVALVECTOR = -12, 
        OBJVECTOR =   -14, 
        STRINGVECTOR =-15, 
        SCRIPTVECTOR =-16, 
        NAMEVECTOR =  -17, 
        HASHABLEVALUE=-18, 
        IONMASM =     -19, 
        WRAPVECTOR =  -20, 
        WRAPPER =     -21, 
        JSONPARSER =  -25, 
        CUSTOM =      -26  
    };

    static ptrdiff_t GetTag(const Value& value) { return VALVECTOR; }
    static ptrdiff_t GetTag(const jsid& id) { return IDVECTOR; }
    static ptrdiff_t GetTag(JSObject* obj) { return OBJVECTOR; }
    static ptrdiff_t GetTag(JSScript* script) { return SCRIPTVECTOR; }
    static ptrdiff_t GetTag(JSString* string) { return STRINGVECTOR; }
    static ptrdiff_t GetTag(js::Shape* shape) { return SHAPEVECTOR; }
    static ptrdiff_t GetTag(const JSPropertyDescriptor& pd) { return DESCVECTOR; }

  private:
    AutoGCRooter ** const stackTop;

    
    AutoGCRooter(AutoGCRooter& ida) = delete;
    void operator=(AutoGCRooter& ida) = delete;
};

} 

namespace js {

class ExclusiveContext;






enum StackKind
{
    StackForSystemCode,      
    StackForTrustedScript,   
    StackForUntrustedScript, 
    StackKindCount
};

enum ThingRootKind
{
    THING_ROOT_OBJECT,
    THING_ROOT_SHAPE,
    THING_ROOT_BASE_SHAPE,
    THING_ROOT_OBJECT_GROUP,
    THING_ROOT_STRING,
    THING_ROOT_SYMBOL,
    THING_ROOT_JIT_CODE,
    THING_ROOT_SCRIPT,
    THING_ROOT_LAZY_SCRIPT,
    THING_ROOT_ID,
    THING_ROOT_VALUE,
    THING_ROOT_PROPERTY_DESCRIPTOR,
    THING_ROOT_PROP_DESC,
    THING_ROOT_STATIC_TRACEABLE,
    THING_ROOT_DYNAMIC_TRACEABLE,
    THING_ROOT_LIMIT
};

template <typename T>
struct RootKind;






template<typename T, ThingRootKind Kind>
struct SpecificRootKind
{
    static ThingRootKind rootKind() { return Kind; }
};

template <> struct RootKind<JSObject*> : SpecificRootKind<JSObject*, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSFlatString*> : SpecificRootKind<JSFlatString*, THING_ROOT_STRING> {};
template <> struct RootKind<JSFunction*> : SpecificRootKind<JSFunction*, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSString*> : SpecificRootKind<JSString*, THING_ROOT_STRING> {};
template <> struct RootKind<JS::Symbol*> : SpecificRootKind<JS::Symbol*, THING_ROOT_SYMBOL> {};
template <> struct RootKind<JSScript*> : SpecificRootKind<JSScript*, THING_ROOT_SCRIPT> {};
template <> struct RootKind<jsid> : SpecificRootKind<jsid, THING_ROOT_ID> {};
template <> struct RootKind<JS::Value> : SpecificRootKind<JS::Value, THING_ROOT_VALUE> {};



class RootLists
{
    
    JS::Rooted<void*>* stackRoots_[THING_ROOT_LIMIT];
    template <typename T> friend class JS::Rooted;

    
    JS::AutoGCRooter* autoGCRooters_;
    friend class JS::AutoGCRooter;

  public:
    RootLists() : autoGCRooters_(nullptr) {
        mozilla::PodArrayZero(stackRoots_);
    }

    template <class T>
    inline JS::Rooted<T>* gcRooters() {
        js::ThingRootKind kind = RootKind<T>::rootKind();
        return reinterpret_cast<JS::Rooted<T>*>(stackRoots_[kind]);
    }

    void checkNoGCRooters();
};

struct ContextFriendFields
{
  protected:
    JSRuntime* const     runtime_;

    
    JSCompartment*      compartment_;

    
    JS::Zone*           zone_;

  public:
    
    RootLists           roots;

    explicit ContextFriendFields(JSRuntime* rt)
      : runtime_(rt), compartment_(nullptr), zone_(nullptr)
    {}

    static const ContextFriendFields* get(const JSContext* cx) {
        return reinterpret_cast<const ContextFriendFields*>(cx);
    }

    static ContextFriendFields* get(JSContext* cx) {
        return reinterpret_cast<ContextFriendFields*>(cx);
    }

    friend JSRuntime* GetRuntime(const JSContext* cx);
    friend JSCompartment* GetContextCompartment(const JSContext* cx);
    friend JS::Zone* GetContextZone(const JSContext* cx);
    template <typename T> friend class JS::Rooted;
};











inline JSRuntime*
GetRuntime(const JSContext* cx)
{
    return ContextFriendFields::get(cx)->runtime_;
}

inline JSCompartment*
GetContextCompartment(const JSContext* cx)
{
    return ContextFriendFields::get(cx)->compartment_;
}

inline JS::Zone*
GetContextZone(const JSContext* cx)
{
    return ContextFriendFields::get(cx)->zone_;
}

class PerThreadData;

struct PerThreadDataFriendFields
{
  private:
    
    
    
    struct RuntimeDummy : JS::shadow::Runtime
    {
        struct PerThreadDummy {
            void* field1;
            uintptr_t field2;
#ifdef JS_DEBUG
            uint64_t field3;
#endif
        } mainThread;
    };

  public:
    
    RootLists roots;

    PerThreadDataFriendFields();

    
    uintptr_t nativeStackLimit[js::StackKindCount];

    static const size_t RuntimeMainThreadOffset = offsetof(RuntimeDummy, mainThread);

    static inline PerThreadDataFriendFields* get(js::PerThreadData* pt) {
        return reinterpret_cast<PerThreadDataFriendFields*>(pt);
    }

    static inline PerThreadDataFriendFields* getMainThread(JSRuntime* rt) {
        
        
        return reinterpret_cast<PerThreadDataFriendFields*>(
            reinterpret_cast<char*>(rt) + RuntimeMainThreadOffset);
    }

    static inline const PerThreadDataFriendFields* getMainThread(const JSRuntime* rt) {
        
        
        return reinterpret_cast<const PerThreadDataFriendFields*>(
            reinterpret_cast<const char*>(rt) + RuntimeMainThreadOffset);
    }

    template <typename T> friend class JS::Rooted;
};

} 

#endif 
