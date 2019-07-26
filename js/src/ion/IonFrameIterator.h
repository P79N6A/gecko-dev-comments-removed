






#ifndef jsion_frame_iterator_h__
#define jsion_frame_iterator_h__

#include "jstypes.h"
#include "IonCode.h"
#include "SnapshotReader.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {

enum FrameType
{
    
    
    IonFrame_JS,

    IonFrame_BaselineJS,

    
    
    IonFrame_Entry,

    
    
    IonFrame_Rectifier,

    
    
    IonFrame_Bailed_JS,

    
    
    IonFrame_Bailed_Rectifier,

    
    
    
    IonFrame_Exit,

    
    
    
    IonFrame_Osr
};

class IonCommonFrameLayout;
class IonJSFrameLayout;
class IonExitFrameLayout;

class IonActivation;
class IonActivationIterator;

class IonFrameIterator
{
  protected:
    uint8 *current_;
    FrameType type_;
    uint8 *returnAddressToFp_;
    size_t frameSize_;

  private:
    mutable const SafepointIndex *cachedSafepointIndex_;
    const IonActivation *activation_;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit),
        returnAddressToFp_(NULL),
        frameSize_(0),
        cachedSafepointIndex_(NULL),
        activation_(NULL)
    { }

    IonFrameIterator(const IonActivationIterator &activations);
    IonFrameIterator(IonJSFrameLayout *fp);

    
    FrameType type() const {
        return type_;
    }
    uint8 *fp() const {
        return current_;
    }

    inline IonCommonFrameLayout *current() const;
    inline uint8 *returnAddress() const;

    IonJSFrameLayout *jsFrame() const {
        JS_ASSERT(type() == IonFrame_JS);
        return (IonJSFrameLayout *) fp();
    }

    IonExitFrameLayout *exitFrame() const {
        JS_ASSERT(type() == IonFrame_Exit);
        return (IonExitFrameLayout *) fp();
    }

    
    
    bool checkInvalidation(IonScript **ionScript) const;
    bool checkInvalidation() const;

    bool isScripted() const {
        return type_ == IonFrame_JS;
    }
    bool isNative() const;
    bool isOOLNativeGetter() const;
    bool isOOLPropertyOp() const;
    bool isDOMExit() const;
    bool isEntry() const {
        return type_ == IonFrame_Entry;
    }
    bool isFunctionFrame() const;

    bool isConstructing() const;

    bool isEntryJSFrame() const;

    void *calleeToken() const;
    JSFunction *callee() const;
    JSFunction *maybeCallee() const;
    unsigned numActualArgs() const;
    JSScript *script() const;
    Value *nativeVp() const;
    Value *actualArgs() const;

    
    
    uint8 *returnAddressToFp() const {
        return returnAddressToFp_;
    }

    
    inline size_t prevFrameLocalSize() const;
    inline FrameType prevType() const;
    uint8 *prevFp() const;

    
    
    inline size_t frameSize() const;

    
    
    inline bool done() const {
        return type_ == IonFrame_Entry;
    }
    IonFrameIterator &operator++();

    
    IonScript *ionScript() const;

    
    
    const SafepointIndex *safepoint() const;

    
    
    const OsiIndex *osiIndex() const;

    uintptr_t *spillBase() const;
    MachineState machineState() const;

    void dump() const;
};

class IonActivationIterator
{
    uint8 *top_;
    IonActivation *activation_;

  private:
    void settle();

  public:
    IonActivationIterator(JSContext *cx);
    IonActivationIterator(JSRuntime *rt);

    IonActivationIterator &operator++();

    IonActivation *activation() const {
        return activation_;
    }
    uint8 *top() const {
        return top_;
    }
    bool more() const;

    
    void ionStackRange(uintptr_t *&min, uintptr_t *&end);
};

class IonJSFrameLayout;
class IonBailoutIterator;



class SnapshotIterator : public SnapshotReader
{
    IonJSFrameLayout *fp_;
    MachineState machine_;
    IonScript *ionScript_;

  private:
    bool hasLocation(const SnapshotReader::Location &loc);
    uintptr_t fromLocation(const SnapshotReader::Location &loc);
    static Value FromTypedPayload(JSValueType type, uintptr_t payload);

    Value slotValue(const Slot &slot);
    bool slotReadable(const Slot &slot);
    void warnUnreadableSlot();

  public:
    SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                     IonJSFrameLayout *fp, const MachineState &machine);
    SnapshotIterator(const IonFrameIterator &iter);
    SnapshotIterator(const IonBailoutIterator &iter);
    SnapshotIterator();

    Value read() {
        return slotValue(readSlot());
    }
    Value maybeRead(bool silentFailure = false) {
        Slot s = readSlot();
        if (slotReadable(s))
            return slotValue(s);
        if (!silentFailure)
            warnUnreadableSlot();
        return UndefinedValue();
    }

    template <class Op>
    inline void readFrameArgs(Op op, const Value *argv, Value *scopeChain, Value *thisv,
                              unsigned start, unsigned formalEnd, unsigned iterEnd);

    Value maybeReadSlotByIndex(size_t index) {
        while (index--) {
            JS_ASSERT(moreSlots());
            skip();
        }

        Value s = maybeRead(true);

        while (moreSlots())
            skip();

        return s;
    }
};



class InlineFrameIterator
{
    const IonFrameIterator *frame_;
    SnapshotIterator start_;
    SnapshotIterator si_;
    unsigned framesRead_;
    HeapPtr<JSFunction> callee_;
    HeapPtr<JSScript> script_;
    jsbytecode *pc_;
    uint32 numActualArgs_;

  private:
    void findNextFrame();

  public:
    InlineFrameIterator(const IonFrameIterator *iter);
    InlineFrameIterator(const IonBailoutIterator *iter);

    bool more() const {
        return frame_ && framesRead_ < start_.frameCount();
    }
    JSFunction *callee() const {
        JS_ASSERT(callee_);
        return callee_;
    }
    JSFunction *maybeCallee() const {
        return callee_;
    }
    unsigned numActualArgs() const;

    template <class Op>
    inline void forEachCanonicalActualArg(Op op, unsigned start, unsigned count) const;

    JSScript *script() const {
        return script_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
    SnapshotIterator snapshotIterator() const {
        return si_;
    }
    bool isFunctionFrame() const;
    bool isConstructing() const;
    JSObject *scopeChain() const;
    JSObject *thisObject() const;
    InlineFrameIterator operator++();

    void dump() const;
};

} 
} 

#endif 

