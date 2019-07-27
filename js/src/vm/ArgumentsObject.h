





#ifndef vm_ArgumentsObject_h
#define vm_ArgumentsObject_h

#include "mozilla/MemoryReporting.h"

#include "gc/Barrier.h"
#include "vm/NativeObject.h"

namespace js {

class AbstractFramePtr;
class ScriptFrameIter;

namespace jit {
class JitFrameLayout;
}







struct ArgumentsData
{
    



    unsigned    numArgs;

    



    HeapValue   callee;

    
    JSScript*   script;

    



    size_t*     deletedBits;

    







    HeapValue   args[1];

    
    static ptrdiff_t offsetOfArgs() { return offsetof(ArgumentsData, args); }

    
    HeapValue* begin() { return args; }
    const HeapValue* begin() const { return args; }
    HeapValue* end() { return args + numArgs; }
    const HeapValue* end() const { return args + numArgs; }
};





static const unsigned ARGS_LENGTH_MAX = 500 * 1000;






































class ArgumentsObject : public NativeObject
{
  protected:
    static const uint32_t INITIAL_LENGTH_SLOT = 0;
    static const uint32_t DATA_SLOT = 1;
    static const uint32_t MAYBE_CALL_SLOT = 2;

  public:
    static const uint32_t LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32_t PACKED_BITS_COUNT = 1;

  protected:
    template <typename CopyArgs>
    static ArgumentsObject* create(JSContext* cx, HandleScript script, HandleFunction callee,
                                   unsigned numActuals, CopyArgs& copy);

    ArgumentsData* data() const {
        return reinterpret_cast<ArgumentsData*>(getFixedSlot(DATA_SLOT).toPrivate());
    }

  public:
    static const uint32_t RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT4_BACKGROUND;

    
    static ArgumentsObject* createExpected(JSContext* cx, AbstractFramePtr frame);

    





    static ArgumentsObject* createUnexpected(JSContext* cx, ScriptFrameIter& iter);
    static ArgumentsObject* createUnexpected(JSContext* cx, AbstractFramePtr frame);
    static ArgumentsObject* createForIon(JSContext* cx, jit::JitFrameLayout* frame,
                                         HandleObject scopeChain);

    



    uint32_t initialLength() const {
        uint32_t argc = uint32_t(getFixedSlot(INITIAL_LENGTH_SLOT).toInt32()) >> PACKED_BITS_COUNT;
        MOZ_ASSERT(argc <= ARGS_LENGTH_MAX);
        return argc;
    }

    
    JSScript* containingScript() const {
        return data()->script;
    }

    
    bool hasOverriddenLength() const {
        const Value& v = getFixedSlot(INITIAL_LENGTH_SLOT);
        return v.toInt32() & LENGTH_OVERRIDDEN_BIT;
    }

    void markLengthOverridden() {
        uint32_t v = getFixedSlot(INITIAL_LENGTH_SLOT).toInt32() | LENGTH_OVERRIDDEN_BIT;
        setFixedSlot(INITIAL_LENGTH_SLOT, Int32Value(v));
    }

    













    bool isElementDeleted(uint32_t i) const {
        MOZ_ASSERT(i < data()->numArgs);
        if (i >= initialLength())
            return false;
        return IsBitArrayElementSet(data()->deletedBits, initialLength(), i);
    }

    bool isAnyElementDeleted() const {
        return IsAnyBitArrayElementSet(data()->deletedBits, initialLength());
    }

    void markElementDeleted(uint32_t i) {
        SetBitArrayElement(data()->deletedBits, initialLength(), i);
    }

    














    const Value& element(uint32_t i) const;

    inline void setElement(JSContext* cx, uint32_t i, const Value& v);

    const Value& arg(unsigned i) const {
        MOZ_ASSERT(i < data()->numArgs);
        const Value& v = data()->args[i];
        MOZ_ASSERT(!v.isMagic());
        return v;
    }

    void setArg(unsigned i, const Value& v) {
        MOZ_ASSERT(i < data()->numArgs);
        HeapValue& lhs = data()->args[i];
        MOZ_ASSERT(!lhs.isMagic());
        lhs = v;
    }

    








    bool maybeGetElement(uint32_t i, MutableHandleValue vp) {
        if (i >= initialLength() || isElementDeleted(i))
            return false;
        vp.set(element(i));
        return true;
    }

    inline bool maybeGetElements(uint32_t start, uint32_t count, js::Value* vp);

    



    size_t sizeOfMisc(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(data());
    }

    static void finalize(FreeOp* fop, JSObject* obj);
    static void trace(JSTracer* trc, JSObject* obj);

    
    static size_t getDataSlotOffset() {
        return getFixedSlotOffset(DATA_SLOT);
    }
    static size_t getInitialLengthSlotOffset() {
        return getFixedSlotOffset(INITIAL_LENGTH_SLOT);
    }

    static Value MagicScopeSlotValue(uint32_t slot) {
        
        
        
        
        
        
        
        JS_STATIC_ASSERT(UINT32_MAX - JS_WHY_MAGIC_COUNT > ARGS_LENGTH_MAX);
        return JS::MagicValueUint32(slot + JS_WHY_MAGIC_COUNT);
    }
    static uint32_t SlotFromMagicScopeSlotValue(const Value& v) {
        JS_STATIC_ASSERT(UINT32_MAX - JS_WHY_MAGIC_COUNT > ARGS_LENGTH_MAX);
        return v.magicUint32() - JS_WHY_MAGIC_COUNT;
    }
    static bool IsMagicScopeSlotValue(const Value& v) {
        return v.isMagic() && v.magicUint32() > JS_WHY_MAGIC_COUNT;
    }

    static void MaybeForwardToCallObject(AbstractFramePtr frame, ArgumentsObject* obj,
                                         ArgumentsData* data);
    static void MaybeForwardToCallObject(jit::JitFrameLayout* frame, HandleObject callObj,
                                         ArgumentsObject* obj, ArgumentsData* data);
};

class NormalArgumentsObject : public ArgumentsObject
{
  public:
    static const Class class_;

    



    const js::Value& callee() const {
        return data()->callee;
    }

    
    void clearCallee() {
        data()->callee = MagicValue(JS_OVERWRITTEN_CALLEE);
    }
};

class StrictArgumentsObject : public ArgumentsObject
{
  public:
    static const Class class_;
};

} 

template<>
inline bool
JSObject::is<js::ArgumentsObject>() const
{
    return is<js::NormalArgumentsObject>() || is<js::StrictArgumentsObject>();
}

#endif 
