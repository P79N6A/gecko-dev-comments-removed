







































#ifndef jsion_assembler_shared_h__
#define jsion_assembler_shared_h__

#include <limits.h>
#include "ion/IonRegisters.h"

namespace js {
namespace ion {




static const uint32 MAX_BUFFER_SIZE = INT_MAX / 2;


struct Imm32
{
    int32_t value;

    Imm32(int32_t value) : value(value)
    { }
};

struct ImmWord
{
    uintptr_t value;

    ImmWord(uintptr_t value) : value(value)
    { }
};


struct ImmGCPtr
{
    uintptr_t value;

    ImmGCPtr(uintptr_t value) : value(value)
    { }
    ImmGCPtr(void *ptr) : value(reinterpret_cast<uintptr_t>(ptr))
    { }
};

class Relocation {
  public:
    enum Kind {
        
        
        EXTERNAL,

        
        
        CODE
    };
};









struct Label
{
  private:
    
    
    int32 offset_ : 31;
    bool bound_   : 1;

    
    void operator =(const Label &label);

  public:
    static const int32 INVALID_OFFSET = -1;

    Label() : offset_(INVALID_OFFSET), bound_(false)
    { }
    Label(const Label &label)
      : offset_(label.offset_),
        bound_(label.bound_)
    { }
    ~Label()
    {
        JS_ASSERT(!used());
    }

    
    
    bool bound() const {
        return bound_;
    }
    int32 offset() const {
        JS_ASSERT(bound() || used());
        return offset_;
    }
    
    bool used() const {
        return !bound() && offset_ > INVALID_OFFSET;
    }
    
    void bind(int32 offset) {
        JS_ASSERT(!bound());
        offset_ = offset;
        bound_ = true;
        JS_ASSERT(offset_ == offset);
    }
    
    
    int32 use(int32 offset) {
        JS_ASSERT(!bound());

        int32 old = offset_;
        offset_ = offset;
        JS_ASSERT(offset_ == offset);

        return old;
    }
};

} 
} 

#endif 

