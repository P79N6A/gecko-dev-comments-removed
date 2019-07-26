





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

  private:
    void initFromLookup(Lookup &lookup);

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

struct SavedFrame::Lookup {
    Lookup(JSAtom *source, size_t line, size_t column, JSAtom *functionDisplayName,
           Handle<SavedFrame*> parent, JSPrincipals *principals)
        : source(source),
          line(line),
          column(column),
          functionDisplayName(functionDisplayName),
          parent(parent),
          principals(principals)
    {
        JS_ASSERT(source);
    }

    JSAtom              *source;
    size_t              line;
    size_t              column;
    JSAtom              *functionDisplayName;
    Handle<SavedFrame*> parent;
    JSPrincipals        *principals;
};

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
    bool     saveCurrentStack(JSContext *cx, MutableHandle<SavedFrame*> frame);
    void     sweep(JSRuntime *rt);
    uint32_t count();
    void     clear();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

  private:
    SavedFrame::Set          frames;
    JSObject                 *savedFrameProto;

    bool       insertFrames(JSContext *cx, ScriptFrameIter &iter, MutableHandle<SavedFrame*> frame);
    SavedFrame *getOrCreateSavedFrame(JSContext *cx, SavedFrame::Lookup &lookup);
    
    
    JSObject   *getOrCreateSavedFramePrototype(JSContext *cx);
    SavedFrame *createFrameFromLookup(JSContext *cx, SavedFrame::Lookup &lookup);
};

bool SavedStacksMetadataCallback(JSContext *cx, JSObject **pmetadata);

} 

#endif 
