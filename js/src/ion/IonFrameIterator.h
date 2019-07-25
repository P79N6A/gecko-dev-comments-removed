








































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
    IonFrame_Exit
};

class IonCommonFrameLayout;
struct InvalidationRecord;
class IonActivation;
class IonJSFrameLayout;

class IonFrameIterator
{
    uint8 *current_;
    FrameType type_;
    uint8 **returnAddressToFp_;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit),
        returnAddressToFp_(NULL)
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
    void *calleeToken() const;
    bool hasScript() const;
    JSScript *script() const;

    
    
    uint8 *returnAddressToFp() const {
        return *returnAddressToFp_;
    }
    uint8 **addressOfReturnToFp() const {
        return returnAddressToFp_;
    }

    
    inline size_t prevFrameLocalSize() const;
    inline FrameType prevType() const;
    uint8 *prevFp() const;

    
    
    inline bool more() const;
    IonFrameIterator &operator++();

    
    uint8 **returnAddressPtr();
};

class IonActivationIterator
{
    uint8 *top_;
    IonActivation *activation_;

  public:
    IonActivationIterator(JSContext *cx);
    IonActivationIterator(ThreadData *td);

    IonActivationIterator &operator++();

    uint8 *top() const {
        return top_;
    }
    bool more() const;
};

} 
} 

#endif 

