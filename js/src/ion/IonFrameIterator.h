








































#ifndef jsion_frame_iterator_h__
#define jsion_frame_iterator_h__

#include "jstypes.h"

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

class IonFrameIterator
{
    uint8 *current_;
    FrameType type_;

    IonCommonFrameLayout *current() const;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit)
    { }

    
    FrameType type() const {
        return type_;
    }
    uint8 *fp() const {
        return current_;
    }
    uint8 *returnAddress() const;

    
    size_t prevFrameLocalSize() const;
    FrameType prevType() const;
    uint8 *prevFp() const;

    
    
    bool more() const {
        return prevType() != IonFrame_Entry;
    }
    IonFrameIterator &operator++();
};

class IonActivationIterator
{
    JSContext *cx_;
    uint8 *top_;
    IonActivation *activation_;

  public:
    IonActivationIterator(JSContext *cx);

    IonActivationIterator &operator++();

    uint8 *top() const {
        return top_;
    }
};

} 
} 

#endif 

