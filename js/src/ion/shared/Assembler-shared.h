





#ifndef ion_shared_Assembler_shared_h
#define ion_shared_Assembler_shared_h

#include <limits.h>

#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"

#include "ion/IonAllocPolicy.h"
#include "ion/Registers.h"
#include "ion/RegisterSets.h"
#if defined(JS_CPU_X64) || defined(JS_CPU_ARM)


#    define JS_SMALL_BRANCH
#endif
namespace js {
namespace ion {

enum Scale {
    TimesOne = 0,
    TimesTwo = 1,
    TimesFour = 2,
    TimesEight = 3
};

static inline unsigned
ScaleToShift(Scale scale)
{
    return unsigned(scale);
}

static inline bool
IsShiftInScaleRange(int i)
{
    return i >= TimesOne && i <= TimesEight;
}

static inline Scale
ShiftToScale(int i)
{
    JS_ASSERT(IsShiftInScaleRange(i));
    return Scale(i);
}

static inline Scale
ScaleFromElemWidth(int shift)
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

    MOZ_ASSUME_UNREACHABLE("Invalid scale");
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
        MOZ_ASSUME_UNREACHABLE("Invalid scale");
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
    {
        JS_ASSERT(!IsPoisonedPtr(ptr));
        JS_ASSERT_IF(ptr, ptr->isTenured());
    }

  protected:
    ImmGCPtr() : value(0) {}
};


struct ImmMaybeNurseryPtr : public ImmGCPtr
{
    explicit ImmMaybeNurseryPtr(gc::Cell *ptr)
    {
        this->value = reinterpret_cast<uintptr_t>(ptr);
        JS_ASSERT(!IsPoisonedPtr(ptr));
    }
};


struct AbsoluteAddress {
    void *addr;

    explicit AbsoluteAddress(void *addr)
      : addr(addr)
    { }

    AbsoluteAddress offset(ptrdiff_t delta) {
        return AbsoluteAddress(((uint8_t *) addr) + delta);
    }
};



struct Address
{
    Register base;
    int32_t offset;

    Address(Register base, int32_t offset) : base(base), offset(offset)
    { }

    Address() { mozilla::PodZero(this); }
};



struct BaseIndex
{
    Register base;
    Register index;
    Scale scale;
    int32_t offset;

    BaseIndex(Register base, Register index, Scale scale, int32_t offset = 0)
      : base(base), index(index), scale(scale), offset(offset)
    { }

    BaseIndex() { mozilla::PodZero(this); }
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
    
    
    int32_t offset_ : 31;
    bool bound_   : 1;

    
    void operator =(const LabelBase &label);
    static int id_count;
  public:
    mozilla::DebugOnly <int> id;
    static const int32_t INVALID_OFFSET = -1;

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
    int32_t offset() const {
        JS_ASSERT(bound() || used());
        return offset_;
    }
    
    bool used() const {
        return !bound() && offset_ > INVALID_OFFSET;
    }
    
    void bind(int32_t offset) {
        JS_ASSERT(!bound());
        offset_ = offset;
        bound_ = true;
        JS_ASSERT(offset_ == offset);
    }
    
    void reset() {
        offset_ = INVALID_OFFSET;
        bound_ = false;
    }
    
    
    int32_t use(int32_t offset) {
        JS_ASSERT(!bound());

        int32_t old = offset_;
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
#ifdef DEBUG
        
        
        if (!js_IonOptions.parallelCompilation)
            JS_ASSERT_IF(MaybeGetIonContext() && !GetIonContext()->runtime->hadOutOfMemory, !used());
#endif
    }
};




class NonAssertingLabel : public Label
{
  public:
    ~NonAssertingLabel()
    {
#ifdef DEBUG
        if (used())
            bind(0);
#endif
    }
};

class RepatchLabel
{
    static const int32_t INVALID_OFFSET = 0xC0000000;
    int32_t offset_ : 31;
    uint32_t bound_ : 1;
  public:

    RepatchLabel() : offset_(INVALID_OFFSET), bound_(0) {}

    void use(uint32_t newOffset) {
        JS_ASSERT(offset_ == INVALID_OFFSET);
        JS_ASSERT(newOffset != (uint32_t)INVALID_OFFSET);
        offset_ = newOffset;
    }
    bool bound() const {
        return bound_;
    }
    void bind(int32_t dest) {
        JS_ASSERT(!bound_);
        JS_ASSERT(dest != INVALID_OFFSET);
        offset_ = dest;
        bound_ = true;
    }
    int32_t target() {
        JS_ASSERT(bound());
        int32_t ret = offset_;
        offset_ = INVALID_OFFSET;
        return ret;
    }
    int32_t offset() {
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
    int32_t prev() const {
        JS_ASSERT(!bound());
        if (!used())
            return INVALID_OFFSET;
        return offset();
    }
    void setPrev(int32_t offset) {
        use(offset);
    }
    void bind() {
        bound_ = true;

        
        offset_ = -1;
    }
};



class CodeLabel
{
    
    AbsoluteLabel dest_;

    
    
    Label src_;

  public:
    CodeLabel()
    { }
    CodeLabel(const AbsoluteLabel &dest)
       : dest_(dest)
    { }
    AbsoluteLabel *dest() {
        return &dest_;
    }
    Label *src() {
        return &src_;
    }
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
        mozilla::PodZero(this);
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
    uint8_t *raw_;
#ifdef DEBUG
    bool absolute_;
    void setAbsolute() {
        absolute_ = true;
    }
    void setRelative() {
        absolute_ = false;
    }
#else
    void setAbsolute() const {
    }
    void setRelative() const {
    }
#endif

#ifdef JS_SMALL_BRANCH
    uint8_t *jumpTableEntry_;
#endif

  public:
    CodeLocationJump() {
        raw_ = (uint8_t *) 0xdeadc0de;
        setAbsolute();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t *) 0xdeadab1e;
#endif
    }
    CodeLocationJump(IonCode *code, CodeOffsetJump base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetJump base) {
        raw_ = (uint8_t *) base.offset();
        setRelative();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t *) base.jumpTableIndex();
#endif
    }

    void repoint(IonCode *code, MacroAssembler* masm = NULL);

    bool isSet() const {
        return raw_ != (uint8_t *) 0xdeadc0de;
    }

    uint8_t *raw() const {
        JS_ASSERT(absolute_ && isSet());
        return raw_;
    }
    uint8_t *offset() const {
        JS_ASSERT(!absolute_ && isSet());
        return raw_;
    }

#ifdef JS_SMALL_BRANCH
    uint8_t *jumpTableEntry() {
        JS_ASSERT(absolute_);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationLabel
{
    uint8_t *raw_;
#ifdef DEBUG
    bool absolute_;
    void setAbsolute() {
        absolute_ = true;
    }
    void setRelative() {
        absolute_ = false;
    }
#else
    void setAbsolute() const {
    }
    void setRelative() const {
    }
#endif

  public:
    CodeLocationLabel() {
        raw_ = (uint8_t *) 0xdeadc0de;
        setAbsolute();
    }
    CodeLocationLabel(IonCode *code, CodeOffsetLabel base) {
        *this = base;
        repoint(code);
    }
    CodeLocationLabel(IonCode *code) {
        raw_ = code->raw();
        setAbsolute();
    }
    CodeLocationLabel(uint8_t *raw) {
        raw_ = raw;
        setAbsolute();
    }

    void operator = (CodeOffsetLabel base) {
        raw_ = (uint8_t *)base.offset();
        setRelative();
    }
    ptrdiff_t operator - (const CodeLocationLabel &other) {
        return raw_ - other.raw_;
    }

    void repoint(IonCode *code, MacroAssembler *masm = NULL);

    bool isSet() {
        return raw_ != (uint8_t *) 0xdeadc0de;
    }

    uint8_t *raw() {
        JS_ASSERT(absolute_ && isSet());
        return raw_;
    }
    uint8_t *offset() {
        JS_ASSERT(!absolute_ && isSet());
        return raw_;
    }
};


} 
} 

#endif 
