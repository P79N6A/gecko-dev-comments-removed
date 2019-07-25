








































#ifndef jsion_lir_h__
#define jsion_lir_h__




#include "jscntxt.h"
#include "IonAllocPolicy.h"
#include "InlineList.h"
#include "IonAssembler.h"
#include "FixedArityList.h"
#include "LOpcodes.h"

namespace js {
namespace ion {

class LUse;
class LGeneralReg;
class LFloatReg;
class LStackSlot;
class LArgument;
class LConstantIndex;
class MBasicBlock;
class MIRGenerator;
class MSnapshot;

static const uint32 MAX_VIRTUAL_REGISTERS = (1 << 21) - 1;

#if defined(JS_NUNBOX32)
# define BOX_PIECES         2
#elif defined(JS_PUNBOX64)
# define BOX_PIECES         1
#else
# error "Unknown!"
#endif



class LAllocation
{
    uintptr_t bits_;

  protected:
    static const uintptr_t TAG_BIT = 1;
    static const uintptr_t TAG_SHIFT = 0;
    static const uintptr_t TAG_MASK = (1 << TAG_BIT) - 1;
    static const uintptr_t KIND_BITS = 3;
    static const uintptr_t KIND_SHIFT = TAG_SHIFT + TAG_BIT;
    static const uintptr_t KIND_MASK = (1 << KIND_BITS) - 1;
    static const uintptr_t DATA_BITS = (sizeof(uint32) * 8) - KIND_BITS;
    static const uintptr_t DATA_SHIFT = KIND_SHIFT + KIND_BITS;
    static const uintptr_t DATA_MASK = (1 << DATA_BITS) - 1;

  public:
    enum Kind {
        USE,            
        CONSTANT_VALUE, 
        CONSTANT_INDEX, 
        GPR,            
        FPU,            
        STACK_SLOT,     
        DOUBLE_SLOT,    
        ARGUMENT        
    };

  protected:
    bool isTagged() const {
        return !!(bits_ & TAG_MASK);
    }

    int32 data() const {
        return int32(bits_) >> DATA_SHIFT;
    }
    void setData(int32 data) {
        JS_ASSERT(int32(data) <= int32(DATA_MASK));
        bits_ &= ~(DATA_MASK << DATA_SHIFT);
        bits_ |= (data << DATA_SHIFT);
    }
    void setKindAndData(Kind kind, uint32 data) {
        JS_ASSERT(int32(data) <= int32(DATA_MASK));
        bits_ = (uint32(kind) << KIND_SHIFT) | data << DATA_SHIFT;
    }

    LAllocation(Kind kind, uint32 data) {
        setKindAndData(kind, data);
    }
    explicit LAllocation(Kind kind) {
        setKindAndData(kind, 0);
    }

  public:
    LAllocation() : bits_(0)
    { }

    
    explicit LAllocation(const Value *vp) {
        bits_ = uintptr_t(vp);
        JS_ASSERT(!isTagged());
        bits_ |= TAG_MASK;
    }

    Kind kind() const {
        if (isTagged())
            return CONSTANT_VALUE;
        return (Kind)((bits_ >> KIND_SHIFT) & KIND_MASK);
    }

    bool isUse() const {
        return kind() == USE;
    }
    bool isConstantValue() const {
        return kind() == CONSTANT_VALUE;
    }
    bool isConstantIndex() const {
        return kind() == CONSTANT_INDEX;
    }
    bool isValue() const {
        return kind() == CONSTANT_VALUE;
    }
    bool isGeneralReg() const {
        return kind() == GPR;
    }
    bool isFloatReg() const {
        return kind() == FPU;
    }
    bool isStackSlot() const {
        return kind() == STACK_SLOT || kind() == DOUBLE_SLOT;
    }
    bool isArgument() const {
        return kind() == ARGUMENT;
    }
    inline LUse *toUse();
    inline const LUse *toUse() const;
    inline const LGeneralReg *toGeneralReg() const;
    inline const LFloatReg *toFloatReg() const;
    inline const LStackSlot *toStackSlot() const;
    inline const LArgument *toArgument() const;
    inline const LConstantIndex *toConstantIndex() const;

    const Value *toConstant() const {
        JS_ASSERT(isConstantValue());
        return reinterpret_cast<const Value *>(bits_ & ~TAG_MASK);
    }
};

class LUse : public LAllocation
{
    static const uint32 POLICY_BITS = 2;
    static const uint32 POLICY_SHIFT = 0;
    static const uint32 POLICY_MASK = (1 << POLICY_BITS) - 1;
    static const uint32 FLAG_BITS = 1;
    static const uint32 FLAG_SHIFT = POLICY_SHIFT + POLICY_BITS;
    static const uint32 FLAG_MASK = (1 << FLAG_BITS) - 1;
    static const uint32 REG_BITS = 5;
    static const uint32 REG_SHIFT = FLAG_SHIFT + FLAG_BITS;
    static const uint32 REG_MASK = (1 << REG_BITS) - 1;

    
    static const uint32 VREG_BITS = DATA_BITS - (REG_SHIFT + REG_BITS);
    static const uint32 VREG_SHIFT = REG_SHIFT + REG_BITS;
    static const uint32 VREG_MASK = (1 << VREG_BITS) - 1;

  public:
    enum Policy {
        ANY,                
        REGISTER,           
        FIXED               
    };

    
    
    
    static const uint32 KILLED_AT_START = (1 << 0);

    void set(Policy policy, uint32 reg, uint32 flags) {
        JS_ASSERT(flags <= KILLED_AT_START);
        setKindAndData(USE, (policy << POLICY_SHIFT) |
                            (flags << FLAG_SHIFT) |
                            (reg << REG_SHIFT));
    }

  public:
    LUse(uint32 vreg, Policy policy, uint32 flags = 0) {
        set(policy, 0, flags);
        setVirtualRegister(vreg);
    }
    LUse(Policy policy, uint32 flags = 0) {
        set(policy, 0, flags);
    }
    LUse(Register reg, uint32 flags = 0) {
        set(FIXED, reg.code(), flags);
    }
    LUse(FloatRegister reg, uint32 flags = 0) {
        set(FIXED, reg.code(), flags);
    }

    void setVirtualRegister(uint32 index) {
        JS_STATIC_ASSERT(VREG_MASK <= MAX_VIRTUAL_REGISTERS);
        JS_ASSERT(index < VREG_MASK);

        uint32 old = data() & ~(VREG_MASK << VREG_SHIFT);
        setData(old | (index << VREG_SHIFT));
    }

    Policy policy() const {
        Policy policy = (Policy)((data() >> POLICY_SHIFT) & POLICY_MASK);
        return policy;
    }
    bool killedAtStart() const {
        return !!(flags() & KILLED_AT_START);
    }
    uint32 virtualRegister() const {
        uint32 index = (data() >> VREG_SHIFT) & VREG_MASK;
        return index;
    }
    uint32 flags() const {
        return (data() >> FLAG_SHIFT) & FLAG_MASK;
    }
    uint32 registerCode() const {
        JS_ASSERT(policy() == FIXED);
        return (data() >> REG_SHIFT) & REG_MASK;
    }
};

class LGeneralReg : public LAllocation
{
  public:
    explicit LGeneralReg(Register reg)
      : LAllocation(GPR, reg.code())
    { }

    Register reg() const {
        return Register::FromCode(data());
    }
};

class LFloatReg : public LAllocation
{
  public:
    explicit LFloatReg(FloatRegister reg)
      : LAllocation(FPU, reg.code())
    { }

    FloatRegister reg() const {
        return FloatRegister::FromCode(data());
    }
};


class LConstantIndex : public LAllocation
{
  public:
    explicit LConstantIndex(uint32 index)
      : LAllocation(CONSTANT_INDEX, index)
    { }

    uint32 index() const {
        return data();
    }
};



class LStackSlot : public LAllocation
{
  public:
    explicit LStackSlot(uint32 slot)
      : LAllocation(STACK_SLOT, slot)
    { }

    bool isDouble() const {
        return kind() == DOUBLE_SLOT;
    }

    uint32 slot() const {
        return data();
    }
};



class LArgument : public LAllocation
{
  public:
    explicit LArgument(int32 index)
      : LAllocation(ARGUMENT, index)
    { }

    int32 index() const {
        return data();
    }
};


class LDefinition
{
    
    uint32 bits_;

    
    
    
    LAllocation output_;

    static const uint32 TYPE_BITS = 3;
    static const uint32 TYPE_SHIFT = 0;
    static const uint32 TYPE_MASK = (1 << TYPE_BITS) - 1;
    static const uint32 POLICY_BITS = 2;
    static const uint32 POLICY_SHIFT = TYPE_SHIFT + TYPE_BITS;
    static const uint32 POLICY_MASK = (1 << POLICY_BITS) - 1;

    static const uint32 VREG_BITS = (sizeof(bits_) * 8) - (POLICY_BITS + TYPE_BITS);
    static const uint32 VREG_SHIFT = POLICY_SHIFT + POLICY_BITS;
    static const uint32 VREG_MASK = (1 << VREG_BITS) - 1;

  public:
    
    
    
    enum Policy {
        
        DEFAULT,

        
        
        
        
        
        
        
        PRESET,

        
        
        CAN_REUSE_INPUT,
        MUST_REUSE_INPUT
    };

    enum Type {
        INTEGER,    
        POINTER,    
        OBJECT,     
        DOUBLE,     
        TYPE,       
        PAYLOAD,    
        BOX         
    };

    void set(uint32 index, Type type, Policy policy) {
        JS_STATIC_ASSERT(MAX_VIRTUAL_REGISTERS <= VREG_MASK);
        bits_ = (index << VREG_SHIFT) | (policy << POLICY_SHIFT) | (type << TYPE_SHIFT);
    }

  public:
    LDefinition(uint32 index, Type type, Policy policy = DEFAULT) {
        set(index, type, policy);
    }

    LDefinition(Type type, Policy policy = DEFAULT) {
        set(0, type, policy);
    }

    LDefinition(Type type, const LAllocation &a)
      : output_(a)
    {
        set(0, type, PRESET);
    }

    LDefinition() : bits_(0)
    { }

    Policy policy() const {
        return (Policy)((bits_ >> POLICY_SHIFT) & POLICY_MASK);
    }
    Type type() const {
        return (Type)((bits_ >> TYPE_SHIFT) & TYPE_MASK);
    }
    uint32 virtualRegister() const {
        return (bits_ >> VREG_SHIFT) & VREG_MASK;
    }
    const LAllocation *output() const {
        return &output_;
    }

    void setVirtualRegister(uint32 index) {
        JS_ASSERT(index < VREG_MASK);
        bits_ &= ~(VREG_MASK << VREG_SHIFT);
        bits_ |= index << VREG_SHIFT;
    }
    void setOutput(const LAllocation &a) {
        output_ = a;
    }
};


#define LIROP(op) class L##op;
    LIR_OPCODE_LIST(LIROP)
#undef LIROP

class LSnapshot;

class LInstruction : public TempObject,
                     public InlineListNode<LInstruction>
{
    uint32 id_;
    LSnapshot *snapshot_;

  protected:
    LInstruction()
      : id_(0),
        snapshot_(NULL)
    { }

  public:
    enum Opcode {
#   define LIROP(name) LOp_##name,
        LIR_OPCODE_LIST(LIROP)
#   undef LIROP
        LOp_Invalid
    };

  public:
    virtual Opcode op() const = 0;

    
    
    virtual size_t numDefs() const = 0;
    virtual LDefinition *getDef(size_t index) = 0;

    
    
    virtual size_t numOperands() const = 0;
    virtual LAllocation *getOperand(size_t index) = 0;

    
    
    virtual size_t numTemps() const = 0;
    virtual LDefinition *getTemp(size_t index) = 0;

    uint32 id() const {
        return id_;
    }
    void setId(uint32 id) {
        JS_ASSERT(!id_);
        JS_ASSERT(id);
        id_ = id;
    }
    LSnapshot *snapshot() const {
        return snapshot_;
    }
    void assignSnapshot(LSnapshot *snapshot) {
        JS_ASSERT(!snapshot_);
        snapshot_ = snapshot;
    }

    virtual void print(FILE *fp);
    virtual void printName(FILE *fp);
    virtual void printOperands(FILE *fp);
    virtual void printInfo(FILE *fp) {
    }

  public:
    
#   define LIROP(name)                                                      \
    bool is##name() const {                                                 \
        return op() == LOp_##name;                                          \
    }                                                                       \
    inline L##name *to##name();
    LIR_OPCODE_LIST(LIROP)
#   undef LIROP
};

typedef InlineList<LInstruction>::iterator LInstructionIterator;

class LBlock : public TempObject
{
    MBasicBlock *block_;
    InlineList<LInstruction> instructions_;

    LBlock(MBasicBlock *block)
      : block_(block)
    { }

  public:
    static LBlock *New(MBasicBlock *from) {
        return new LBlock(from);
    }
    void add(LInstruction *ins) {
        instructions_.insert(ins);
    }
    MBasicBlock *mir() const {
        return block_;
    }
    LInstructionIterator begin() {
        return instructions_.begin();
    }
    LInstructionIterator end() {
        return instructions_.end();
    }
};

template <size_t Defs, size_t Operands, size_t Temps>
class LInstructionHelper : public LInstruction
{
    FixedArityList<LDefinition, Defs> defs_;
    FixedArityList<LAllocation, Operands> operands_;
    FixedArityList<LDefinition, Temps> temps_;

  public:
    size_t numDefs() const {
        return Defs;
    }
    LDefinition *getDef(size_t index) {
        return &defs_[index];
    }
    size_t numOperands() const {
        return Operands;
    }
    LAllocation *getOperand(size_t index) {
        return &operands_[index];
    }
    size_t numTemps() const {
        return Temps;
    }
    LDefinition *getTemp(size_t index) {
        return &temps_[index];
    }

    void setDef(size_t index, const LDefinition &def) {
        defs_[index] = def;
    }
    void setOperand(size_t index, const LAllocation &a) {
        operands_[index] = a;
    }
    void setTemp(size_t index, const LDefinition &a) {
        temps_[index] = a;
    }

    virtual void printInfo(FILE *fp) {
        printOperands(fp);
    }
};



class LSnapshot : public TempObject
{
    uint32 numSlots_;
    LAllocation *slots_;
    MSnapshot *mir_;

    LSnapshot(MSnapshot *mir);
    bool init(MIRGenerator *gen);

  public:
    static LSnapshot *New(MIRGenerator *gen, MSnapshot *snapshot);

    size_t numEntries() const {
        return numSlots_;
    }
    LAllocation *getEntry(size_t i) {
        JS_ASSERT(i < numSlots_);
        return &slots_[i];
    }
    MSnapshot *mir() const {
        return mir_;
    }
};

} 
} 

#define LIR_HEADER(opcode)                                                  \
    Opcode op() const {                                                     \
        return LInstruction::LOp_##opcode;                                  \
    }

#include "LIR-Common.h"
#if defined(JS_CPU_X86)
# include "x86/LIR-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/LIR-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/LIR-ARM.h"
#endif

#undef LIR_HEADER

#endif

