





#ifndef vm_SavedStacks_h
#define vm_SavedStacks_h

#include "jscntxt.h"
#include "jsmath.h"
#include "jswrapper.h"
#include "js/HashTable.h"
#include "vm/Stack.h"



































































































































namespace js {

class SavedFrame;
typedef JS::Handle<SavedFrame*> HandleSavedFrame;
typedef JS::MutableHandle<SavedFrame*> MutableHandleSavedFrame;
typedef JS::Rooted<SavedFrame*> RootedSavedFrame;

class SavedFrame : public NativeObject {
    friend class SavedStacks;

  public:
    static const Class          class_;
    static void finalize(FreeOp* fop, JSObject* obj);
    static const JSPropertySpec protoAccessors[];
    static const JSFunctionSpec protoFunctions[];
    static const JSFunctionSpec staticFunctions[];

    
    static bool construct(JSContext* cx, unsigned argc, Value* vp);
    static bool sourceProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool lineProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool columnProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool functionDisplayNameProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool asyncCauseProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool asyncParentProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool parentProperty(JSContext* cx, unsigned argc, Value* vp);
    static bool toStringMethod(JSContext* cx, unsigned argc, Value* vp);

    
    JSAtom*      getSource();
    uint32_t     getLine();
    uint32_t     getColumn();
    JSAtom*      getFunctionDisplayName();
    JSAtom*      getAsyncCause();
    SavedFrame*  getParent();
    JSPrincipals* getPrincipals();

    bool         isSelfHosted();

    static bool isSavedFrameAndNotProto(JSObject& obj) {
        return obj.is<SavedFrame>() &&
               !obj.as<SavedFrame>().getReservedSlot(JSSLOT_SOURCE).isNull();
    }

    struct Lookup;
    struct HashPolicy;

    typedef HashSet<js::ReadBarriered<SavedFrame*>,
                    HashPolicy,
                    SystemAllocPolicy> Set;

    class AutoLookupVector;

    class MOZ_STACK_CLASS HandleLookup {
        friend class AutoLookupVector;

        Lookup& lookup;

        explicit HandleLookup(Lookup& lookup) : lookup(lookup) { }

      public:
        inline Lookup& get() { return lookup; }
        inline Lookup* operator->() { return &lookup; }
    };

  private:
    static bool finishSavedFrameInit(JSContext* cx, HandleObject ctor, HandleObject proto);
    void initFromLookup(HandleLookup lookup);

    enum {
        
        JSSLOT_SOURCE,
        JSSLOT_LINE,
        JSSLOT_COLUMN,
        JSSLOT_FUNCTIONDISPLAYNAME,
        JSSLOT_ASYNCCAUSE,
        JSSLOT_PARENT,
        JSSLOT_PRINCIPALS,
        JSSLOT_PRIVATE_PARENT,

        
        JSSLOT_COUNT
    };

    
    
    
    
    
    
    
    
    bool parentMoved();
    void updatePrivateParent();

    static bool checkThis(JSContext* cx, CallArgs& args, const char* fnName,
                          MutableHandleObject frame);
};

struct SavedFrame::HashPolicy
{
    typedef SavedFrame::Lookup               Lookup;
    typedef PointerHasher<SavedFrame*, 3>   SavedFramePtrHasher;
    typedef PointerHasher<JSPrincipals*, 3> JSPrincipalsPtrHasher;

    static HashNumber hash(const Lookup& lookup);
    static bool       match(SavedFrame* existing, const Lookup& lookup);

    typedef ReadBarriered<SavedFrame*> Key;
    static void rekey(Key& key, const Key& newKey);
};



inline void AssertObjectIsSavedFrameOrWrapper(JSContext* cx, HandleObject stack);

class SavedStacks {
    friend JSObject* SavedStacksMetadataCallback(JSContext* cx);

  public:
    SavedStacks()
      : frames(),
        allocationSamplingProbability(1.0),
        allocationSkipCount(0),
        rngState(0),
        creatingSavedFrame(false)
    { }

    bool     init();
    bool     initialized() const { return frames.initialized(); }
    bool     saveCurrentStack(JSContext* cx, MutableHandleSavedFrame frame, unsigned maxFrameCount = 0);
    void     sweep(JSRuntime* rt);
    void     trace(JSTracer* trc);
    uint32_t count();
    void     clear();
    void     setRNGState(uint64_t state) { rngState = state; }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

  private:
    SavedFrame::Set frames;
    double          allocationSamplingProbability;
    uint32_t        allocationSkipCount;
    uint64_t        rngState;
    bool            creatingSavedFrame;

    
    
    
    struct MOZ_STACK_CLASS AutoReentrancyGuard {
        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;
        SavedStacks& stacks;

        explicit AutoReentrancyGuard(SavedStacks& stacks MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
            : stacks(stacks)
        {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            stacks.creatingSavedFrame = true;
        }

        ~AutoReentrancyGuard()
        {
            stacks.creatingSavedFrame = false;
        }
    };

    bool       insertFrames(JSContext* cx, FrameIter& iter, MutableHandleSavedFrame frame,
                            unsigned maxFrameCount = 0);
    bool       adoptAsyncStack(JSContext* cx, HandleSavedFrame asyncStack,
                               HandleString asyncCause,
                               MutableHandleSavedFrame adoptedStack,
                               unsigned maxFrameCount);
    SavedFrame* getOrCreateSavedFrame(JSContext* cx, SavedFrame::HandleLookup lookup);
    SavedFrame* createFrameFromLookup(JSContext* cx, SavedFrame::HandleLookup lookup);
    void       chooseSamplingProbability(JSContext* cx);

    

    struct PCKey {
        PCKey(JSScript* script, jsbytecode* pc) : script(script), pc(pc) { }

        PreBarrieredScript script;
        jsbytecode*        pc;
    };

    struct LocationValue {
        LocationValue() : source(nullptr), line(0), column(0) { }
        LocationValue(JSAtom* source, size_t line, uint32_t column)
            : source(source),
              line(line),
              column(column)
        { }

        void trace(JSTracer* trc) {
            if (source)
                TraceEdge(trc, &source, "SavedStacks::LocationValue::source");
        }

        PreBarrieredAtom source;
        size_t           line;
        uint32_t         column;
    };

    class MOZ_STACK_CLASS AutoLocationValueRooter : public JS::CustomAutoRooter
    {
      public:
        explicit AutoLocationValueRooter(JSContext* cx)
            : JS::CustomAutoRooter(cx),
              value() {}

        inline LocationValue* operator->() { return &value; }
        void set(LocationValue& loc) { value = loc; }
        LocationValue& get() { return value; }

      private:
        virtual void trace(JSTracer* trc) {
            value.trace(trc);
        }

        SavedStacks::LocationValue value;
    };

    class MOZ_STACK_CLASS MutableHandleLocationValue
    {
      public:
        inline MOZ_IMPLICIT MutableHandleLocationValue(AutoLocationValueRooter* location)
            : location(location) {}

        inline LocationValue* operator->() { return &location->get(); }
        void set(LocationValue& loc) { location->set(loc); }

      private:
        AutoLocationValueRooter* location;
    };

    struct PCLocationHasher : public DefaultHasher<PCKey> {
        typedef PointerHasher<JSScript*, 3>   ScriptPtrHasher;
        typedef PointerHasher<jsbytecode*, 3> BytecodePtrHasher;

        static HashNumber hash(const PCKey& key) {
            return mozilla::AddToHash(ScriptPtrHasher::hash(key.script),
                                      BytecodePtrHasher::hash(key.pc));
        }

        static bool match(const PCKey& l, const PCKey& k) {
            return l.script == k.script && l.pc == k.pc;
        }
    };

    typedef HashMap<PCKey, LocationValue, PCLocationHasher, SystemAllocPolicy> PCLocationMap;

    PCLocationMap pcLocationMap;

    void sweepPCLocationMap();
    bool getLocation(JSContext* cx, const FrameIter& iter, MutableHandleLocationValue locationp);
};

JSObject* SavedStacksMetadataCallback(JSContext* cx);

} 

#endif 
