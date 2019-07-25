








































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
    IonFrame_Entry,
    IonFrame_Rectifier,
    IonFrame_Bailed_Rectifier,
    IonFrame_Exit
};

class IonCommonFrameLayout;
class IonActivation;
class IonJSFrameLayout;

class IonFrameIterator
{
    uint8 *current_;
    FrameType type_;
    uint8 *returnAddressToFp_;
    size_t frameSize_;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit),
        returnAddressToFp_(NULL),
        frameSize_(0)
    { }

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

    
    
    bool checkInvalidation(IonScript **ionScript) const;
    bool checkInvalidation() const;

    inline bool isScripted() const {
        return type_ == IonFrame_JS;
    }
    bool isFunctionFrame() const;

    void *calleeToken() const;
    JSFunction *callee() const;
    JSFunction *maybeCallee() const;
    JSScript *script() const;

    
    
    uint8 *returnAddressToFp() const {
        return returnAddressToFp_;
    }

    
    inline size_t prevFrameLocalSize() const;
    inline FrameType prevType() const;
    uint8 *prevFp() const;

    
    
    inline size_t frameSize() const;

    
    
    inline bool more() const {
        return type_ != IonFrame_Entry;
    }
    IonFrameIterator &operator++();

    
    IonScript *ionScript() const;

    
    
    const SafepointIndex *safepoint() const;

    
    
    const OsiIndex *osiIndex() const;
};

class IonActivationIterator
{
    uint8 *top_;
    IonActivation *activation_;

  public:
    IonActivationIterator(JSContext *cx);
    IonActivationIterator(JSRuntime *rt);

    IonActivationIterator &operator++();

    IonActivation *activation() {
        return activation_;
    }
    uint8 *top() const {
        return top_;
    }
    bool more() const;
};

class FrameRecovery;
class IonJSFrameLayout;



class SnapshotIterator : public SnapshotReader
{
    IonJSFrameLayout *fp_;
    MachineState machine_;
    IonScript *ionScript_;

  private:
    uintptr_t fromLocation(const SnapshotReader::Location &loc);
    static Value FromTypedPayload(JSValueType type, uintptr_t payload);

    Value slotValue(const Slot &slot);

  public:
    SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                     IonJSFrameLayout *fp, const MachineState &machine);
    SnapshotIterator(const IonFrameIterator &iter, const MachineState &machine);
    SnapshotIterator();

    Value read() {
        return slotValue(readSlot());
    }
};



class InlineFrameIterator
{
    const IonFrameIterator *frame_;
    MachineState machine_;
    SnapshotIterator start_;
    SnapshotIterator si_;
    unsigned framesRead_;
    HeapPtr<JSFunction> callee_;
    HeapPtr<JSScript> script_;
    jsbytecode *pc_;

  private:
    void findNextFrame();

  public:
    InlineFrameIterator(const IonFrameIterator *iter, const MachineState &machine);

    bool more() const {
        return frame_ && framesRead_ < start_.frameCount();
    }
    JSFunction *callee() const {
        JS_ASSERT(callee_);
        return callee_;
    }
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
    InlineFrameIterator operator++();
};

} 
} 

#endif 

