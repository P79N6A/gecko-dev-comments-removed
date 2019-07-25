







































#ifndef jsion_assembler_shared_h__
#define jsion_assembler_shared_h__

#include <limits.h>
#include "ion/IonAllocPolicy.h"
#include "ion/IonRegisters.h"

namespace js {
namespace ion {


struct Imm32
{
    int32_t value;

    explicit Imm32(int32_t value) : value(value)
    { }
};


struct ImmWord
{
    uintptr_t value;

    explicit ImmWord(uintptr_t value) : value(value)
    { }
    explicit ImmWord(const void *ptr) : value(reinterpret_cast<uintptr_t>(ptr))
    { }

    void *asPointer() {
        return reinterpret_cast<void *>(value);
    }
};


struct ImmGCPtr
{
    uintptr_t value;

    explicit ImmGCPtr(const gc::Cell *ptr) : value(reinterpret_cast<uintptr_t>(ptr))
    { }
};



struct Address
{
    Register base;
    int32 offset;

    Address(Register base, int32 offset) : base(base), offset(offset)
    { }
};

class Relocation {
  public:
    enum Kind {
        
        
        EXTERNAL,

        
        
        CODE
    };
};

struct LabelBase
{
  protected:
    
    
    int32 offset_ : 31;
    bool bound_   : 1;

    
    void operator =(const LabelBase &label);

  public:
    static const int32 INVALID_OFFSET = -1;

    LabelBase() : offset_(INVALID_OFFSET), bound_(false)
    { }
    LabelBase(const LabelBase &label)
      : offset_(label.offset_),
        bound_(label.bound_)
    { }

    
    
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
    
    void reset() {
        offset_ = INVALID_OFFSET;
        bound_ = false;
    }
    
    
    int32 use(int32 offset) {
        JS_ASSERT(!bound());

        int32 old = offset_;
        offset_ = offset;
        JS_ASSERT(offset_ == offset);

        return old;
    }
};









class Label : public LabelBase
{
  public:
    Label()
    { }
    Label(const Label &label) : LabelBase(label)
    { }
    ~Label()
    {
        JS_ASSERT(!used());
    }
};




struct AbsoluteLabel : public LabelBase
{
  public:
    AbsoluteLabel()
    { }
    AbsoluteLabel(const AbsoluteLabel &label) : LabelBase(label)
    { }
    int32 prev() const {
        JS_ASSERT(!bound());
        if (!used())
            return INVALID_OFFSET;
        return offset();
    }
    void setPrev(int32 offset) {
        use(offset);
    }
    void bind() {
        bound_ = true;

        
        offset_ = -1;
    }
};



class CodeLabel : public TempObject
{
    
    AbsoluteLabel dest_;
    
    
    
    Label src_;

  public:
    CodeLabel()
    { }
    AbsoluteLabel *dest() {
        return &dest_;
    }
    Label *src() {
        return &src_;
    }
};





class DeferredData : public TempObject
{
    
    AbsoluteLabel label_;

    
    int32 offset_;

  public:
    DeferredData() : offset_(-1)
    { }
    int32 offset() const {
        JS_ASSERT(offset_ > -1);
        return offset_;
    }
    void setOffset(int32 offset) {
        offset_ = offset;
    }
    AbsoluteLabel *label() {
        return &label_;
    }

    
    virtual void copy(IonCode *code, uint8 *buffer) const = 0;
};

} 
} 

#endif 

