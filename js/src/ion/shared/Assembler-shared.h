







































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

    Address() { PodZero(this); }
};

enum Scale {
    TimesOne,
    TimesTwo,
    TimesFour,
    TimesEight
};



struct BaseIndex
{
    Register base;
    Register index;
    Scale scale;
    int32 offset;

    BaseIndex(Register base, Register index, Scale scale, int32 offset = 0)
      : base(base), index(index), scale(scale), offset(offset)
    { }

    BaseIndex() { PodZero(this); }
};

class Relocation {
  public:
    enum Kind {
        
        
        HARDCODED,

        
        
        IONCODE
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




class CodeOffsetJump
{
    size_t offset_;

#ifdef JS_CPU_X64
    size_t jumpTableIndex_;
#endif

  public:

#ifdef JS_CPU_X64
    CodeOffsetJump(size_t offset, size_t jumpTableIndex)
        : offset_(offset), jumpTableIndex_(jumpTableIndex)
    {}
    size_t jumpTableIndex() const {
        return jumpTableIndex_;
    }
#else
    CodeOffsetJump(size_t offset) : offset_(offset) {}
#endif

    CodeOffsetJump() {
        PodZero(this);
    }

    size_t offset() const {
        return offset_;
    }
};

class CodeOffsetLabel
{
    size_t offset_;

  public:
    CodeOffsetLabel(size_t offset) : offset_(offset) {}
    CodeOffsetLabel() : offset_(0) {}

    size_t offset() const {
        return offset_;
    }
};






class CodeLocationJump
{
    uint8 *raw_;

#ifdef JS_CPU_X64
    uint8 *jumpTableEntry_;
#endif

#ifdef DEBUG
    bool absolute;
    void markAbsolute(bool value) {
        absolute = value;
    }
#else
    void markAbsolute(bool value) {}
#endif

  public:
    CodeLocationJump() {}
    CodeLocationJump(IonCode *code, CodeOffsetJump base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetJump base) {
        raw_ = (uint8 *) base.offset();
#ifdef JS_CPU_X64
        jumpTableEntry_ = (uint8 *) base.jumpTableIndex();
#endif
        markAbsolute(false);
    }

    void repoint(IonCode *code, MacroAssembler* masm = NULL);

    uint8 *raw() const {
        JS_ASSERT(absolute);
        return raw_;
    }
    uint8 *offset() const {
        JS_ASSERT(!absolute);
        return raw_;
    }

#ifdef JS_CPU_X64
    uint8 *jumpTableEntry() {
        JS_ASSERT(absolute);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationLabel
{
    uint8 *raw_;
#ifdef DEBUG
    bool absolute;
    void markAbsolute(bool value) {
        absolute = value;
    }
#else
    void markAbsolute(bool value) {}
#endif

  public:
    CodeLocationLabel() {}
    CodeLocationLabel(IonCode *code, CodeOffsetLabel base) {
        *this = base;
        repoint(code);
    }
    CodeLocationLabel(IonCode *code) {
        raw_ = code->raw();
        markAbsolute(true);
    }
    CodeLocationLabel(uint8 *raw) {
        raw_ = raw;
        markAbsolute(true);
    }

    void operator = (CodeOffsetLabel base) {
        raw_ = (uint8 *) base.offset();
        markAbsolute(false);
    }
    ptrdiff_t operator - (const CodeLocationLabel &other) {
        return raw_ - other.raw_;
    }

    void repoint(IonCode *code, MacroAssembler *masm = NULL);

    uint8 *raw() {
        JS_ASSERT(absolute);
        return raw_;
    }
    uint8 *offset() {
        JS_ASSERT(!absolute);
        return raw_;
    }
};

} 
} 

#endif 

