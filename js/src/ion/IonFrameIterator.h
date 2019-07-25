








































#ifndef jsion_frame_iterator_h__
#define jsion_frame_iterator_h__

#include "jstypes.h"
#include "IonCode.h"

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
class IonFrameIterator;

class InlineFrameIterator
{
    
    
    
    const IonFrameIterator *bottom_;
    size_t frameCount_;
    JSScript *script_;
    jsbytecode *pc_;

  private:
    size_t getInlinedFrame(size_t nb);

  public:
    InlineFrameIterator(const IonFrameIterator *bottom)
      : bottom_(bottom)
    {
        if (bottom_)
            frameCount_ = getInlinedFrame(-1);
    }

    inline JSScript *script() const {
        JS_ASSERT(bottom_);
        return script_;
    }
    inline jsbytecode *pc() const {
        JS_ASSERT(bottom_);
        return pc_;
    }

    inline InlineFrameIterator &operator++() {
        JS_ASSERT(bottom_);
        JS_ASSERT(more());
        getInlinedFrame(--frameCount_);
        return *this;
    }
    inline bool more() const {
        JS_ASSERT(bottom_);
        return frameCount_;
    }
};

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

    
    FrameType type() const {
        return type_;
    }
    uint8 *fp() const {
        return current_;
    }

    inline IonCommonFrameLayout *current() const;
    inline uint8 *returnAddress() const;

    IonJSFrameLayout *jsFrame() {
        JS_ASSERT(type() == IonFrame_JS);
        return (IonJSFrameLayout *) fp();
    }

    
    
    bool checkInvalidation(IonScript **ionScript) const;
    bool checkInvalidation() const;

    inline bool isScripted() const {
        return type_ == IonFrame_JS;
    }
    void *calleeToken() const;
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

} 
} 

#endif 

