





#ifndef vm_SavedStacks_h
#define vm_SavedStacks_h

#include "jscntxt.h"
#include "js/HashTable.h"
#include "vm/Stack.h"

namespace js {

class SavedFrame : public JSObject {
    friend class SavedStacks;

  public:
    static const Class          class_;
    static void finalize(FreeOp *fop, JSObject *obj);

    
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
    static bool sourceProperty(JSContext *cx, unsigned argc, Value *vp);
    static bool lineProperty(JSContext *cx, unsigned argc, Value *vp);
    static bool columnProperty(JSContext *cx, unsigned argc, Value *vp);
    static bool functionDisplayNameProperty(JSContext *cx, unsigned argc, Value *vp);
    static bool parentProperty(JSContext *cx, unsigned argc, Value *vp);
    static bool toStringMethod(JSContext *cx, unsigned argc, Value *vp);

    
    JSAtom       *getSource();
    size_t       getLine();
    size_t       getColumn();
    JSAtom       *getFunctionDisplayName();
    SavedFrame   *getParent();
    JSPrincipals *getPrincipals();

    bool         isSelfHosted();

    struct Lookup;
    struct HashPolicy;

    typedef HashSet<js::ReadBarriered<SavedFrame *>,
                    HashPolicy,
                    SystemAllocPolicy> Set;

    class AutoLookupRooter;

  private:
    void initFromLookup(const Lookup &lookup);

    enum {
        
        JSSLOT_SOURCE,
        JSSLOT_LINE,
        JSSLOT_COLUMN,
        JSSLOT_FUNCTIONDISPLAYNAME,
        JSSLOT_PARENT,
        JSSLOT_PRINCIPALS,
        JSSLOT_PRIVATE_PARENT,

        
        JSSLOT_COUNT
    };

    
    
    
    
    
    
    
    
    bool         parentMoved();
    void         updatePrivateParent();

    static SavedFrame *checkThis(JSContext *cx, CallArgs &args, const char *fnName);
};

typedef JS::Handle<SavedFrame*> HandleSavedFrame;
typedef JS::MutableHandle<SavedFrame*> MutableHandleSavedFrame;
typedef JS::Rooted<SavedFrame*> RootedSavedFrame;

struct SavedFrame::HashPolicy
{
    typedef SavedFrame::Lookup               Lookup;
    typedef PointerHasher<SavedFrame *, 3>   SavedFramePtrHasher;
    typedef PointerHasher<JSPrincipals *, 3> JSPrincipalsPtrHasher;

    static HashNumber hash(const Lookup &lookup);
    static bool       match(SavedFrame *existing, const Lookup &lookup);

    typedef ReadBarriered<SavedFrame*> Key;
    static void rekey(Key &key, const Key &newKey);
};

class SavedStacks {
  public:
    SavedStacks() : frames(), savedFrameProto(nullptr) { }

    bool     init();
    bool     initialized() const { return frames.initialized(); }
    bool     saveCurrentStack(JSContext *cx, MutableHandleSavedFrame frame);
    void     sweep(JSRuntime *rt);
    void     trace(JSTracer *trc);
    uint32_t count();
    void     clear();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

  private:
    SavedFrame::Set          frames;
    JSObject                 *savedFrameProto;

    bool       insertFrames(JSContext *cx, ScriptFrameIter &iter, MutableHandleSavedFrame frame);
    SavedFrame *getOrCreateSavedFrame(JSContext *cx, const SavedFrame::Lookup &lookup);
    
    
    JSObject   *getOrCreateSavedFramePrototype(JSContext *cx);
    SavedFrame *createFrameFromLookup(JSContext *cx, const SavedFrame::Lookup &lookup);

    

    struct PCKey {
        PCKey(JSScript *script, jsbytecode *pc) : script(script), pc(pc) { }

        PreBarrieredScript script;
        jsbytecode         *pc;
    };

    struct LocationValue {
        LocationValue() : source(nullptr), line(0), column(0) { }
        LocationValue(JSAtom *source, size_t line, size_t column)
            : source(source),
              line(line),
              column(column)
        { }

        PreBarrieredAtom source;
        size_t           line;
        size_t           column;
    };

    class MOZ_STACK_CLASS AutoLocationValueRooter : public JS::CustomAutoRooter
    {
      public:
        AutoLocationValueRooter(JSContext *cx)
            : JS::CustomAutoRooter(cx),
              value() {}

        void set(LocationValue &loc) {
            value = loc;
        }

        LocationValue &get() {
            return value;
        }

      private:
        virtual void trace(JSTracer *trc) {
            if (value.source)
                gc::MarkString(trc, &value.source, "SavedStacks::LocationValue::source");
        }

        SavedStacks::LocationValue value;
    };

    class MOZ_STACK_CLASS MutableHandleLocationValue
    {
      public:
        inline MOZ_IMPLICIT MutableHandleLocationValue(AutoLocationValueRooter *location)
            : location(location) {}

        void set(LocationValue &loc) {
            location->set(loc);
        }

      private:
        AutoLocationValueRooter *location;
    };

    struct PCLocationHasher : public DefaultHasher<PCKey> {
        typedef PointerHasher<JSScript *, 3>   ScriptPtrHasher;
        typedef PointerHasher<jsbytecode *, 3> BytecodePtrHasher;

        static HashNumber hash(const PCKey &key) {
            return mozilla::AddToHash(ScriptPtrHasher::hash(key.script),
                                      BytecodePtrHasher::hash(key.pc));
        }

        static bool match(const PCKey &l, const PCKey &k) {
            return l.script == k.script && l.pc == k.pc;
        }
    };

    typedef HashMap<PCKey, LocationValue, PCLocationHasher, SystemAllocPolicy> PCLocationMap;

    PCLocationMap pcLocationMap;

    void sweepPCLocationMap();
    bool getLocation(JSContext *cx, JSScript *script, jsbytecode *pc,
                     MutableHandleLocationValue locationp);
};

bool SavedStacksMetadataCallback(JSContext *cx, JSObject **pmetadata);

} 

#endif 
