






#ifndef jsion_assembler_shared_h__
#define jsion_assembler_shared_h__

#include <limits.h>
#include "ion/IonAllocPolicy.h"
#include "ion/Registers.h"
#include "ion/RegisterSets.h"
#if defined(JS_CPU_X64) || defined(JS_CPU_ARM)


#    define JS_SMALL_BRANCH
#endif
namespace js {
namespace ion {

enum Scale {
    TimesOne,
    TimesTwo,
    TimesFour,
    TimesEight
};

static inline Scale
ScaleFromShift(int shift)
{
    switch (shift) {
      case 1:
        return TimesOne;
      case 2:
        return TimesTwo;
      case 4:
        return TimesFour;
      case 8:
        return TimesEight;
    }

    JS_NOT_REACHED("Invalid scale");
    return TimesOne;
}


struct Imm32
{
    int32_t value;

    explicit Imm32(int32_t value) : value(value)
    { }

    static inline Imm32 ShiftOf(enum Scale s) {
        switch (s) {
          case TimesOne:
            return Imm32(0);
          case TimesTwo:
            return Imm32(1);
          case TimesFour:
            return Imm32(2);
          case TimesEight:
            return Imm32(3);
        };
        JS_NOT_REACHED("Invalid scale");
        return Imm32(-1);
    }

    static inline Imm32 FactorOf(enum Scale s) {
        return Imm32(1 << ShiftOf(s).value);
    }
};


struct ImmWord
{
    uintptr_t value;

    explicit ImmWord(uintptr_t value) : value(value)
    { }
    explicit ImmWord(const void *ptr) : value(reinterpret_cast<uintptr_t>(ptr))
    { }

    
    explicit ImmWord(gc::Cell *cell);

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


struct AbsoluteAddress {
    void *addr;

    explicit AbsoluteAddress(void *addr)
      : addr(addr)
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
    static int id_count;
  public:
    DebugOnly <int> id;
    static const int32 INVALID_OFFSET = -1;

    LabelBase() : offset_(INVALID_OFFSET), bound_(false), id(id_count++)
    { }
    LabelBase(const LabelBase &label)
      : offset_(label.offset_),
        bound_(label.bound_),
        id(id_count++)
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
        
        
        JS_ASSERT_IF(OOM_counter < OOM_maxAllocations, !used());
    }
};

class RepatchLabel
{
    static const int32 INVALID_OFFSET = 0xC0000000;
    int32 offset_ : 31;
    uint32 bound_ : 1;
  public:

    RepatchLabel() : offset_(INVALID_OFFSET), bound_(0) {}

    void use(uint32 newOffset) {
        JS_ASSERT(offset_ == INVALID_OFFSET);
        JS_ASSERT(newOffset != (uint32)INVALID_OFFSET);
        offset_ = newOffset;
    }
    bool bound() const {
        return bound_;
    }
    void bind(int32 dest) {
        JS_ASSERT(!bound_);
        JS_ASSERT(dest != INVALID_OFFSET);
        offset_ = dest;
        bound_ = true;
    }
    int32 target() {
        JS_ASSERT(bound());
        int32 ret = offset_;
        offset_ = INVALID_OFFSET;
        return ret;
    }
    int32 offset() {
        JS_ASSERT(!bound());
        return offset_;
    }
    bool used() const {
        return !bound() && offset_ != (INVALID_OFFSET);
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

#ifdef JS_SMALL_BRANCH
    size_t jumpTableIndex_;
#endif

  public:

#ifdef JS_SMALL_BRANCH
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
    void fixup(MacroAssembler *masm);
};

class CodeOffsetCall
{
    size_t offset_;

#ifdef JS_SMALL_BRANCH
    size_t jumpTableIndex_;
#endif

  public:

#ifdef JS_SMALL_BRANCH
    CodeOffsetCall(size_t offset, size_t jumpTableIndex)
        : offset_(offset), jumpTableIndex_(jumpTableIndex)
    {}
    size_t jumpTableIndex() const {
        return jumpTableIndex_;
    }
#else
    CodeOffsetCall(size_t offset) : offset_(offset) {}
#endif

    CodeOffsetCall() {
        PodZero(this);
    }

    size_t offset() const {
        return offset_;
    }
    void fixup(MacroAssembler *masm);
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
    void fixup(MacroAssembler *masm);

};






class CodeLocationJump
{
    uint8 *raw_;
    DebugOnly<bool> absolute_;

#ifdef JS_SMALL_BRANCH
    uint8 *jumpTableEntry_;
#endif

  public:
    CodeLocationJump() {}
    CodeLocationJump(IonCode *code, CodeOffsetJump base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetJump base) {
        raw_ = (uint8 *) base.offset();
        absolute_ = false;
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8 *) base.jumpTableIndex();
#endif
    }

    void repoint(IonCode *code, MacroAssembler* masm = NULL);

    uint8 *raw() const {
        JS_ASSERT(absolute_);
        return raw_;
    }
    uint8 *offset() const {
        JS_ASSERT(!absolute_);
        return raw_;
    }

#ifdef JS_SMALL_BRANCH
    uint8 *jumpTableEntry() {
        JS_ASSERT(absolute_);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationCall
{
    uint8 *raw_;
    DebugOnly<bool> absolute_;

#ifdef JS_SMALL_BRANCH
    uint8 *jumpTableEntry_;
#endif

  public:
    CodeLocationCall() {}
    CodeLocationCall(IonCode *code, CodeOffsetCall base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetCall base) {
        raw_ = (uint8 *) base.offset();
        absolute_ = false;
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8 *) base.jumpTableIndex();
#endif
    }

    void repoint(IonCode *code, MacroAssembler* masm = NULL);

    uint8 *raw() const {
        JS_ASSERT(absolute_);
        return raw_;
    }
    uint8 *offset() const {
        JS_ASSERT(!absolute_);
        return raw_;
    }

#ifdef JS_SMALL_BRANCH
    uint8 *jumpTableEntry() {
        JS_ASSERT(absolute_);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationLabel
{
    uint8 *raw_;
    DebugOnly<bool> absolute_;

  public:
    CodeLocationLabel() {}
    CodeLocationLabel(IonCode *code, CodeOffsetLabel base) {
        *this = base;
        repoint(code);
    }
    CodeLocationLabel(IonCode *code) {
        raw_ = code->raw();
        absolute_ = true;
    }
    CodeLocationLabel(uint8 *raw) {
        raw_ = raw;
        absolute_ = true;
    }

    void operator = (CodeOffsetLabel base) {
        raw_ = (uint8 *)base.offset();
        absolute_ = false;
    }
    ptrdiff_t operator - (const CodeLocationLabel &other) {
        return raw_ - other.raw_;
    }

    void repoint(IonCode *code, MacroAssembler *masm = NULL);

    uint8 *raw() {
        JS_ASSERT(absolute_);
        return raw_;
    }
    uint8 *offset() {
        JS_ASSERT(!absolute_);
        return raw_;
    }
};


} 
} 

#endif 

