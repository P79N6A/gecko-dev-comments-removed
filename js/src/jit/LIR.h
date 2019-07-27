





#ifndef jit_LIR_h
#define jit_LIR_h




#include "mozilla/Array.h"

#include "jit/Bailouts.h"
#include "jit/InlineList.h"
#include "jit/IonAllocPolicy.h"
#include "jit/LOpcodes.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "jit/Registers.h"
#include "jit/Safepoints.h"

namespace js {
namespace jit {

class LUse;
class LGeneralReg;
class LFloatReg;
class LStackSlot;
class LArgument;
class LConstantIndex;
class MBasicBlock;
class MTableSwitch;
class MIRGenerator;
class MSnapshot;

static const uint32_t VREG_INCREMENT = 1;

static const uint32_t THIS_FRAME_ARGSLOT = 0;

#if defined(JS_NUNBOX32)
# define BOX_PIECES         2
static const uint32_t VREG_TYPE_OFFSET = 0;
static const uint32_t VREG_DATA_OFFSET = 1;
static const uint32_t TYPE_INDEX = 0;
static const uint32_t PAYLOAD_INDEX = 1;
#elif defined(JS_PUNBOX64)
# define BOX_PIECES         1
#else
# error "Unknown!"
#endif



class LAllocation : public TempObject
{
    uintptr_t bits_;

    
    
    
    static const uintptr_t KIND_BITS = 3;
    static const uintptr_t KIND_SHIFT = 0;
    static const uintptr_t KIND_MASK = (1 << KIND_BITS) - 1;

  protected:
    static const uintptr_t DATA_BITS = (sizeof(uint32_t) * 8) - KIND_BITS;
    static const uintptr_t DATA_SHIFT = KIND_SHIFT + KIND_BITS;
    static const uintptr_t DATA_MASK = (1 << DATA_BITS) - 1;

  public:
    enum Kind {
        CONSTANT_VALUE, 
        CONSTANT_INDEX, 
        USE,            
        GPR,            
        FPU,            
        STACK_SLOT,     
        ARGUMENT_SLOT   
    };

  protected:
    uint32_t data() const {
        return uint32_t(bits_) >> DATA_SHIFT;
    }
    void setData(uint32_t data) {
        JS_ASSERT(data <= DATA_MASK);
        bits_ &= ~(DATA_MASK << DATA_SHIFT);
        bits_ |= (data << DATA_SHIFT);
    }
    void setKindAndData(Kind kind, uint32_t data) {
        JS_ASSERT(data <= DATA_MASK);
        bits_ = (uint32_t(kind) << KIND_SHIFT) | data << DATA_SHIFT;
    }

    LAllocation(Kind kind, uint32_t data) {
        setKindAndData(kind, data);
    }
    explicit LAllocation(Kind kind) {
        setKindAndData(kind, 0);
    }

  public:
    LAllocation() : bits_(0)
    {
        JS_ASSERT(isBogus());
    }

    static LAllocation *New(TempAllocator &alloc) {
        return new(alloc) LAllocation();
    }
    template <typename T>
    static LAllocation *New(TempAllocator &alloc, const T &other) {
        return new(alloc) LAllocation(other);
    }

    
    explicit LAllocation(const Value *vp) {
        JS_ASSERT(vp);
        bits_ = uintptr_t(vp);
        JS_ASSERT((bits_ & (KIND_MASK << KIND_SHIFT)) == 0);
        bits_ |= CONSTANT_VALUE << KIND_SHIFT;
    }
    inline explicit LAllocation(AnyRegister reg);

    Kind kind() const {
        return (Kind)((bits_ >> KIND_SHIFT) & KIND_MASK);
    }

    bool isBogus() const {
        return bits_ == 0;
    }
    bool isUse() const {
        return kind() == USE;
    }
    bool isConstant() const {
        return isConstantValue() || isConstantIndex();
    }
    bool isConstantValue() const {
        return kind() == CONSTANT_VALUE;
    }
    bool isConstantIndex() const {
        return kind() == CONSTANT_INDEX;
    }
    bool isGeneralReg() const {
        return kind() == GPR;
    }
    bool isFloatReg() const {
        return kind() == FPU;
    }
    bool isStackSlot() const {
        return kind() == STACK_SLOT;
    }
    bool isArgument() const {
        return kind() == ARGUMENT_SLOT;
    }
    bool isRegister() const {
        return isGeneralReg() || isFloatReg();
    }
    bool isRegister(bool needFloat) const {
        return needFloat ? isFloatReg() : isGeneralReg();
    }
    bool isMemory() const {
        return isStackSlot() || isArgument();
    }
    inline LUse *toUse();
    inline const LUse *toUse() const;
    inline const LGeneralReg *toGeneralReg() const;
    inline const LFloatReg *toFloatReg() const;
    inline const LStackSlot *toStackSlot() const;
    inline const LArgument *toArgument() const;
    inline const LConstantIndex *toConstantIndex() const;
    inline AnyRegister toRegister() const;

    const Value *toConstant() const {
        JS_ASSERT(isConstantValue());
        return reinterpret_cast<const Value *>(bits_ & ~(KIND_MASK << KIND_SHIFT));
    }

    bool operator ==(const LAllocation &other) const {
        return bits_ == other.bits_;
    }

    bool operator !=(const LAllocation &other) const {
        return bits_ != other.bits_;
    }

    HashNumber hash() const {
        return bits_;
    }

#ifdef DEBUG
    const char *toString() const;
#else
    const char *toString() const { return "???"; }
#endif
    bool aliases(const LAllocation &other) const;
    void dump() const;

};

class LUse : public LAllocation
{
    static const uint32_t POLICY_BITS = 3;
    static const uint32_t POLICY_SHIFT = 0;
    static const uint32_t POLICY_MASK = (1 << POLICY_BITS) - 1;
    static const uint32_t REG_BITS = 6;
    static const uint32_t REG_SHIFT = POLICY_SHIFT + POLICY_BITS;
    static const uint32_t REG_MASK = (1 << REG_BITS) - 1;

    
    static const uint32_t USED_AT_START_BITS = 1;
    static const uint32_t USED_AT_START_SHIFT = REG_SHIFT + REG_BITS;
    static const uint32_t USED_AT_START_MASK = (1 << USED_AT_START_BITS) - 1;

  public:
    
    static const uint32_t VREG_BITS = DATA_BITS - (USED_AT_START_SHIFT + USED_AT_START_BITS);
    static const uint32_t VREG_SHIFT = USED_AT_START_SHIFT + USED_AT_START_BITS;
    static const uint32_t VREG_MASK = (1 << VREG_BITS) - 1;

    enum Policy {
        
        ANY,

        
        REGISTER,

        
        FIXED,

        
        
        
        KEEPALIVE,

        
        
        
        
        
        RECOVERED_INPUT
    };

    void set(Policy policy, uint32_t reg, bool usedAtStart) {
        setKindAndData(USE, (policy << POLICY_SHIFT) |
                            (reg << REG_SHIFT) |
                            ((usedAtStart ? 1 : 0) << USED_AT_START_SHIFT));
    }

  public:
    LUse(uint32_t vreg, Policy policy, bool usedAtStart = false) {
        set(policy, 0, usedAtStart);
        setVirtualRegister(vreg);
    }
    explicit LUse(Policy policy, bool usedAtStart = false) {
        set(policy, 0, usedAtStart);
    }
    explicit LUse(Register reg, bool usedAtStart = false) {
        set(FIXED, reg.code(), usedAtStart);
    }
    explicit LUse(FloatRegister reg, bool usedAtStart = false) {
        set(FIXED, reg.code(), usedAtStart);
    }
    LUse(Register reg, uint32_t virtualRegister) {
        set(FIXED, reg.code(), false);
        setVirtualRegister(virtualRegister);
    }
    LUse(FloatRegister reg, uint32_t virtualRegister) {
        set(FIXED, reg.code(), false);
        setVirtualRegister(virtualRegister);
    }

    void setVirtualRegister(uint32_t index) {
        JS_ASSERT(index < VREG_MASK);

        uint32_t old = data() & ~(VREG_MASK << VREG_SHIFT);
        setData(old | (index << VREG_SHIFT));
    }

    Policy policy() const {
        Policy policy = (Policy)((data() >> POLICY_SHIFT) & POLICY_MASK);
        return policy;
    }
    uint32_t virtualRegister() const {
        uint32_t index = (data() >> VREG_SHIFT) & VREG_MASK;
        JS_ASSERT(index != 0);
        return index;
    }
    uint32_t registerCode() const {
        JS_ASSERT(policy() == FIXED);
        return (data() >> REG_SHIFT) & REG_MASK;
    }
    bool isFixedRegister() const {
        return policy() == FIXED;
    }
    bool usedAtStart() const {
        return !!((data() >> USED_AT_START_SHIFT) & USED_AT_START_MASK);
    }
};

static const uint32_t MAX_VIRTUAL_REGISTERS = LUse::VREG_MASK;

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
    explicit LConstantIndex(uint32_t index)
      : LAllocation(CONSTANT_INDEX, index)
    { }

  public:
    static LConstantIndex FromIndex(uint32_t index) {
        return LConstantIndex(index);
    }

    uint32_t index() const {
        return data();
    }
};


class LStackSlot : public LAllocation
{
  public:
    explicit LStackSlot(uint32_t slot)
      : LAllocation(STACK_SLOT, slot)
    { }

    uint32_t slot() const {
        return data();
    }
};


class LArgument : public LAllocation
{
  public:
    explicit LArgument(int32_t index)
      : LAllocation(ARGUMENT_SLOT, index)
    { }

    int32_t index() const {
        return data();
    }
};


class LDefinition
{
    
    uint32_t bits_;

    
    
    
    
    
    
    
    LAllocation output_;

    static const uint32_t TYPE_BITS = 4;
    static const uint32_t TYPE_SHIFT = 0;
    static const uint32_t TYPE_MASK = (1 << TYPE_BITS) - 1;
    static const uint32_t POLICY_BITS = 2;
    static const uint32_t POLICY_SHIFT = TYPE_SHIFT + TYPE_BITS;
    static const uint32_t POLICY_MASK = (1 << POLICY_BITS) - 1;

    static const uint32_t VREG_BITS = (sizeof(uint32_t) * 8) - (POLICY_BITS + TYPE_BITS);
    static const uint32_t VREG_SHIFT = POLICY_SHIFT + POLICY_BITS;
    static const uint32_t VREG_MASK = (1 << VREG_BITS) - 1;

  public:
    
    
    
    enum Policy {
        
        
        
        
        
        
        FIXED,

        
        REGISTER,

        
        
        MUST_REUSE_INPUT
    };

    enum Type {
        GENERAL,    
        INT32,      
        OBJECT,     
        SLOTS,      
        FLOAT32,    
        DOUBLE,     
        INT32X4,    
        FLOAT32X4,  
#ifdef JS_NUNBOX32
        
        
        TYPE,
        PAYLOAD
#else
        BOX         
#endif
    };

    void set(uint32_t index, Type type, Policy policy) {
        JS_STATIC_ASSERT(MAX_VIRTUAL_REGISTERS <= VREG_MASK);
        bits_ = (index << VREG_SHIFT) | (policy << POLICY_SHIFT) | (type << TYPE_SHIFT);
        JS_ASSERT_IF(!SupportsSimd, !isSimdType());
    }

  public:
    LDefinition(uint32_t index, Type type, Policy policy = REGISTER) {
        set(index, type, policy);
    }

    explicit LDefinition(Type type, Policy policy = REGISTER) {
        set(0, type, policy);
    }

    LDefinition(Type type, const LAllocation &a)
      : output_(a)
    {
        set(0, type, FIXED);
    }

    LDefinition(uint32_t index, Type type, const LAllocation &a)
      : output_(a)
    {
        set(index, type, FIXED);
    }

    LDefinition() : bits_(0)
    {
        JS_ASSERT(isBogusTemp());
    }

    static LDefinition BogusTemp() {
        return LDefinition();
    }

    Policy policy() const {
        return (Policy)((bits_ >> POLICY_SHIFT) & POLICY_MASK);
    }
    Type type() const {
        return (Type)((bits_ >> TYPE_SHIFT) & TYPE_MASK);
    }
    bool isSimdType() const {
        return type() == INT32X4 || type() == FLOAT32X4;
    }
    bool isCompatibleReg(const AnyRegister &r) const {
        if (isFloatReg() && r.isFloat()) {
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
            if (type() == FLOAT32)
                return r.fpu().isSingle();
            return r.fpu().isDouble();
#else
            return true;
#endif
        }
        return !isFloatReg() && !r.isFloat();
    }
    bool isCompatibleDef(const LDefinition &other) const {
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        if (isFloatReg() && other.isFloatReg())
            return type() == other.type();
        return !isFloatReg() && !other.isFloatReg();
#else
        return isFloatReg() == other.isFloatReg();
#endif
    }

    bool isFloatReg() const {
        return type() == FLOAT32 || type() == DOUBLE || isSimdType();
    }
    uint32_t virtualRegister() const {
        uint32_t index = (bits_ >> VREG_SHIFT) & VREG_MASK;
        
        return index;
    }
    LAllocation *output() {
        return &output_;
    }
    const LAllocation *output() const {
        return &output_;
    }
    bool isFixed() const {
        return policy() == FIXED;
    }
    bool isBogusTemp() const {
        return isFixed() && output()->isBogus();
    }
    void setVirtualRegister(uint32_t index) {
        JS_ASSERT(index < VREG_MASK);
        bits_ &= ~(VREG_MASK << VREG_SHIFT);
        bits_ |= index << VREG_SHIFT;
    }
    void setOutput(const LAllocation &a) {
        output_ = a;
        if (!a.isUse()) {
            bits_ &= ~(POLICY_MASK << POLICY_SHIFT);
            bits_ |= FIXED << POLICY_SHIFT;
        }
    }
    void setReusedInput(uint32_t operand) {
        output_ = LConstantIndex::FromIndex(operand);
    }
    uint32_t getReusedInput() const {
        JS_ASSERT(policy() == LDefinition::MUST_REUSE_INPUT);
        return output_.toConstantIndex()->index();
    }

    static inline Type TypeFrom(MIRType type) {
        switch (type) {
          case MIRType_Boolean:
          case MIRType_Int32:
            
            
            static_assert(sizeof(bool) <= sizeof(int32_t), "bool doesn't fit in an int32 slot");
            return LDefinition::INT32;
          case MIRType_String:
          case MIRType_Symbol:
          case MIRType_Object:
            return LDefinition::OBJECT;
          case MIRType_Double:
            return LDefinition::DOUBLE;
          case MIRType_Float32:
            return LDefinition::FLOAT32;
#if defined(JS_PUNBOX64)
          case MIRType_Value:
            return LDefinition::BOX;
#endif
          case MIRType_Slots:
          case MIRType_Elements:
            return LDefinition::SLOTS;
          case MIRType_Pointer:
            return LDefinition::GENERAL;
          case MIRType_ForkJoinContext:
            return LDefinition::GENERAL;
          case MIRType_Int32x4:
            return LDefinition::INT32X4;
          case MIRType_Float32x4:
            return LDefinition::FLOAT32X4;
          default:
            MOZ_CRASH("unexpected type");
        }
    }

#ifdef DEBUG
    const char *toString() const;
#else
    const char *toString() const { return "???"; }
#endif

    void dump() const;
};


#define LIROP(op) class L##op;
    LIR_OPCODE_LIST(LIROP)
#undef LIROP

class LSnapshot;
class LSafepoint;
class LInstructionVisitor;

class LInstruction
  : public TempObject,
    public InlineListNode<LInstruction>
{
    uint32_t id_;

    
    
    LSnapshot *snapshot_;

    
    
    LSafepoint *safepoint_;

  protected:
    MDefinition *mir_;

    LInstruction()
      : id_(0),
        snapshot_(nullptr),
        safepoint_(nullptr),
        mir_(nullptr)
    { }

  public:
    class InputIterator;
    enum Opcode {
#   define LIROP(name) LOp_##name,
        LIR_OPCODE_LIST(LIROP)
#   undef LIROP
        LOp_Invalid
    };

    const char *opName() {
        switch (op()) {
#   define LIR_NAME_INS(name)                   \
            case LOp_##name: return #name;
            LIR_OPCODE_LIST(LIR_NAME_INS)
#   undef LIR_NAME_INS
          default:
            return "Invalid";
        }
    }

    
    
    virtual const char *extraName() const {
        return nullptr;
    }

  public:
    virtual Opcode op() const = 0;

    
    
    virtual size_t numDefs() const = 0;
    virtual LDefinition *getDef(size_t index) = 0;
    virtual void setDef(size_t index, const LDefinition &def) = 0;

    
    virtual size_t numOperands() const = 0;
    virtual LAllocation *getOperand(size_t index) = 0;
    virtual void setOperand(size_t index, const LAllocation &a) = 0;

    
    
    virtual size_t numTemps() const = 0;
    virtual LDefinition *getTemp(size_t index) = 0;
    virtual void setTemp(size_t index, const LDefinition &a) = 0;

    
    
    virtual size_t numSuccessors() const = 0;
    virtual MBasicBlock *getSuccessor(size_t i) const = 0;
    virtual void setSuccessor(size_t i, MBasicBlock *successor) = 0;

    virtual bool isCall() const {
        return false;
    }
    uint32_t id() const {
        return id_;
    }
    void setId(uint32_t id) {
        JS_ASSERT(!id_);
        JS_ASSERT(id);
        id_ = id;
    }
    LSnapshot *snapshot() const {
        return snapshot_;
    }
    LSafepoint *safepoint() const {
        return safepoint_;
    }
    void setMir(MDefinition *mir) {
        mir_ = mir;
    }
    MDefinition *mirRaw() const {
        
        return mir_;
    }
    void assignSnapshot(LSnapshot *snapshot);
    void initSafepoint(TempAllocator &alloc);

    
    
    virtual bool recoversInput() const {
        return false;
    }

    virtual void dump(FILE *fp);
    void dump();
    static void printName(FILE *fp, Opcode op);
    virtual void printName(FILE *fp);
    virtual void printOperands(FILE *fp);
    virtual void printInfo(FILE *fp) { }

  public:
    
#   define LIROP(name)                                                      \
    bool is##name() const {                                                 \
        return op() == LOp_##name;                                          \
    }                                                                       \
    inline L##name *to##name();
    LIR_OPCODE_LIST(LIROP)
#   undef LIROP

    virtual bool accept(LInstructionVisitor *visitor) = 0;
};

class LInstructionVisitor
{
    LInstruction *ins_;

  protected:
    jsbytecode *lastPC_;
    jsbytecode *lastNotInlinedPC_;

    LInstruction *instruction() {
        return ins_;
    }

  public:
    void setInstruction(LInstruction *ins) {
        ins_ = ins;
        if (ins->mirRaw()) {
            lastPC_ = ins->mirRaw()->trackedPc();
            if (ins->mirRaw()->trackedTree())
                lastNotInlinedPC_ = ins->mirRaw()->profilerLeavePc();
        }
    }

    LInstructionVisitor()
      : ins_(nullptr),
        lastPC_(nullptr),
        lastNotInlinedPC_(nullptr)
    {}

  public:
#define VISIT_INS(op) virtual bool visit##op(L##op *) { MOZ_CRASH("NYI: " #op); }
    LIR_OPCODE_LIST(VISIT_INS)
#undef VISIT_INS
};

typedef InlineList<LInstruction>::iterator LInstructionIterator;
typedef InlineList<LInstruction>::reverse_iterator LInstructionReverseIterator;

class LPhi;
class LMoveGroup;
class LBlock : public TempObject
{
    MBasicBlock *block_;
    FixedList<LPhi> phis_;
    InlineList<LInstruction> instructions_;
    LMoveGroup *entryMoveGroup_;
    LMoveGroup *exitMoveGroup_;
    Label label_;

    explicit LBlock(MBasicBlock *block)
      : block_(block),
        phis_(),
        entryMoveGroup_(nullptr),
        exitMoveGroup_(nullptr)
    { }

  public:
    static LBlock *New(TempAllocator &alloc, MBasicBlock *from);
    void add(LInstruction *ins) {
        instructions_.pushBack(ins);
    }
    size_t numPhis() const {
        return phis_.length();
    }
    LPhi *getPhi(size_t index) {
        return &phis_[index];
    }
    MBasicBlock *mir() const {
        return block_;
    }
    LInstructionIterator begin() {
        return instructions_.begin();
    }
    LInstructionIterator begin(LInstruction *at) {
        return instructions_.begin(at);
    }
    LInstructionIterator end() {
        return instructions_.end();
    }
    LInstructionReverseIterator rbegin() {
        return instructions_.rbegin();
    }
    LInstructionReverseIterator rbegin(LInstruction *at) {
        return instructions_.rbegin(at);
    }
    LInstructionReverseIterator rend() {
        return instructions_.rend();
    }
    InlineList<LInstruction> &instructions() {
        return instructions_;
    }
    void insertAfter(LInstruction *at, LInstruction *ins) {
        instructions_.insertAfter(at, ins);
    }
    void insertBefore(LInstruction *at, LInstruction *ins) {
        JS_ASSERT(!at->isLabel());
        instructions_.insertBefore(at, ins);
    }
    uint32_t firstId() const;
    uint32_t lastId() const;

    
    Label *label() {
        JS_ASSERT(!isTrivial());
        return &label_;
    }

    LMoveGroup *getEntryMoveGroup(TempAllocator &alloc);
    LMoveGroup *getExitMoveGroup(TempAllocator &alloc);

    
    
    bool isTrivial() {
        LInstructionIterator ins(begin());
        while (ins->isLabel())
            ++ins;
        return ins->isGoto() && !mir()->isLoopHeader();
    }

    void dump(FILE *fp);
    void dump();
};

template <size_t Defs, size_t Operands, size_t Temps>
class LInstructionHelper : public LInstruction
{
    mozilla::Array<LDefinition, Defs> defs_;
    mozilla::Array<LAllocation, Operands> operands_;
    mozilla::Array<LDefinition, Temps> temps_;

  public:
    size_t numDefs() const MOZ_FINAL MOZ_OVERRIDE {
        return Defs;
    }
    LDefinition *getDef(size_t index) MOZ_FINAL MOZ_OVERRIDE {
        return &defs_[index];
    }
    size_t numOperands() const MOZ_FINAL MOZ_OVERRIDE {
        return Operands;
    }
    LAllocation *getOperand(size_t index) MOZ_FINAL MOZ_OVERRIDE {
        return &operands_[index];
    }
    size_t numTemps() const MOZ_FINAL MOZ_OVERRIDE {
        return Temps;
    }
    LDefinition *getTemp(size_t index) MOZ_FINAL MOZ_OVERRIDE {
        return &temps_[index];
    }

    void setDef(size_t index, const LDefinition &def) MOZ_FINAL MOZ_OVERRIDE {
        defs_[index] = def;
    }
    void setOperand(size_t index, const LAllocation &a) MOZ_FINAL MOZ_OVERRIDE {
        operands_[index] = a;
    }
    void setTemp(size_t index, const LDefinition &a) MOZ_FINAL MOZ_OVERRIDE {
        temps_[index] = a;
    }

    size_t numSuccessors() const {
        return 0;
    }
    MBasicBlock *getSuccessor(size_t i) const {
        JS_ASSERT(false);
        return nullptr;
    }
    void setSuccessor(size_t i, MBasicBlock *successor) {
        JS_ASSERT(false);
    }

    
    const LAllocation *input() {
        JS_ASSERT(numOperands() == 1);
        return getOperand(0);
    }
    const LDefinition *output() {
        JS_ASSERT(numDefs() == 1);
        return getDef(0);
    }

    virtual void printInfo(FILE *fp) {
        printOperands(fp);
    }
};

template <size_t Defs, size_t Operands, size_t Temps>
class LCallInstructionHelper : public LInstructionHelper<Defs, Operands, Temps>
{
  public:
    virtual bool isCall() const {
        return true;
    }
};

class LRecoverInfo : public TempObject
{
  public:
    typedef Vector<MNode *, 2, IonAllocPolicy> Instructions;

  private:
    
    
    Instructions instructions_;

    
    RecoverOffset recoverOffset_;

    explicit LRecoverInfo(TempAllocator &alloc);
    bool init(MResumePoint *mir);

    
    
    bool appendOperands(MNode *ins);
    bool appendDefinition(MDefinition *def);
    bool appendResumePoint(MResumePoint *rp);
  public:
    static LRecoverInfo *New(MIRGenerator *gen, MResumePoint *mir);

    
    MResumePoint *mir() const {
        return instructions_.back()->toResumePoint();
    }
    RecoverOffset recoverOffset() const {
        return recoverOffset_;
    }
    void setRecoverOffset(RecoverOffset offset) {
        JS_ASSERT(recoverOffset_ == INVALID_RECOVER_OFFSET);
        recoverOffset_ = offset;
    }

    MNode **begin() {
        return instructions_.begin();
    }
    MNode **end() {
        return instructions_.end();
    }
    size_t numInstructions() const {
        return instructions_.length();
    }

    class OperandIter
    {
      private:
        MNode **it_;
        MNode **end_;
        size_t op_;

      public:
        explicit OperandIter(LRecoverInfo *recoverInfo)
          : it_(recoverInfo->begin()), end_(recoverInfo->end()), op_(0)
        {
            settle();
        }

        void settle() {
            while ((*it_)->numOperands() == 0) {
                ++it_;
                op_ = 0;
            }
        }

        MDefinition *operator *() {
            return (*it_)->getOperand(op_);
        }
        MDefinition *operator ->() {
            return (*it_)->getOperand(op_);
        }

        OperandIter &operator ++() {
            ++op_;
            if (op_ == (*it_)->numOperands()) {
                op_ = 0;
                ++it_;
            }
            if (!*this)
                settle();

            return *this;
        }

        operator bool() const {
            return it_ == end_;
        }

#ifdef DEBUG
        bool canOptimizeOutIfUnused();
#endif
    };
};






class LSnapshot : public TempObject
{
  private:
    uint32_t numSlots_;
    LAllocation *slots_;
    LRecoverInfo *recoverInfo_;
    SnapshotOffset snapshotOffset_;
    BailoutId bailoutId_;
    BailoutKind bailoutKind_;

    LSnapshot(LRecoverInfo *recover, BailoutKind kind);
    bool init(MIRGenerator *gen);

  public:
    static LSnapshot *New(MIRGenerator *gen, LRecoverInfo *recover, BailoutKind kind);

    size_t numEntries() const {
        return numSlots_;
    }
    size_t numSlots() const {
        return numSlots_ / BOX_PIECES;
    }
    LAllocation *payloadOfSlot(size_t i) {
        JS_ASSERT(i < numSlots());
        size_t entryIndex = (i * BOX_PIECES) + (BOX_PIECES - 1);
        return getEntry(entryIndex);
    }
#ifdef JS_NUNBOX32
    LAllocation *typeOfSlot(size_t i) {
        JS_ASSERT(i < numSlots());
        size_t entryIndex = (i * BOX_PIECES) + (BOX_PIECES - 2);
        return getEntry(entryIndex);
    }
#endif
    LAllocation *getEntry(size_t i) {
        JS_ASSERT(i < numSlots_);
        return &slots_[i];
    }
    void setEntry(size_t i, const LAllocation &alloc) {
        JS_ASSERT(i < numSlots_);
        slots_[i] = alloc;
    }
    LRecoverInfo *recoverInfo() const {
        return recoverInfo_;
    }
    MResumePoint *mir() const {
        return recoverInfo()->mir();
    }
    SnapshotOffset snapshotOffset() const {
        return snapshotOffset_;
    }
    BailoutId bailoutId() const {
        return bailoutId_;
    }
    void setSnapshotOffset(SnapshotOffset offset) {
        JS_ASSERT(snapshotOffset_ == INVALID_SNAPSHOT_OFFSET);
        snapshotOffset_ = offset;
    }
    void setBailoutId(BailoutId id) {
        JS_ASSERT(bailoutId_ == INVALID_BAILOUT_ID);
        bailoutId_ = id;
    }
    BailoutKind bailoutKind() const {
        return bailoutKind_;
    }
    void setBailoutKind(BailoutKind kind) {
        bailoutKind_ = kind;
    }
    void rewriteRecoveredInput(LUse input);
};

struct SafepointNunboxEntry {
    LAllocation type;
    LAllocation payload;

    SafepointNunboxEntry() { }
    SafepointNunboxEntry(LAllocation type, LAllocation payload)
      : type(type), payload(payload)
    { }
};

class LSafepoint : public TempObject
{
    typedef SafepointNunboxEntry NunboxEntry;

  public:
    typedef Vector<uint32_t, 0, IonAllocPolicy> SlotList;
    typedef Vector<NunboxEntry, 0, IonAllocPolicy> NunboxList;

  private:
    
    

    
    
    
    
    
    
    
    
    
    
    RegisterSet liveRegs_;

    
    GeneralRegisterSet gcRegs_;

#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    RegisterSet clobberedRegs_;
#endif

    
    
    uint32_t safepointOffset_;

    
    uint32_t osiCallPointOffset_;

    
    SlotList gcSlots_;

    
    SlotList valueSlots_;

#ifdef JS_NUNBOX32
    
    NunboxList nunboxParts_;

    
    uint32_t partialNunboxes_;
#elif JS_PUNBOX64
    
    GeneralRegisterSet valueRegs_;
#endif

    
    GeneralRegisterSet slotsOrElementsRegs_;

    
    SlotList slotsOrElementsSlots_;

  public:
    void assertInvariants() {
        
#ifndef JS_NUNBOX32
        JS_ASSERT((valueRegs().bits() & ~liveRegs().gprs().bits()) == 0);
#endif
        JS_ASSERT((gcRegs().bits() & ~liveRegs().gprs().bits()) == 0);
    }

    explicit LSafepoint(TempAllocator &alloc)
      : safepointOffset_(INVALID_SAFEPOINT_OFFSET)
      , osiCallPointOffset_(0)
      , gcSlots_(alloc)
      , valueSlots_(alloc)
#ifdef JS_NUNBOX32
      , nunboxParts_(alloc)
      , partialNunboxes_(0)
#endif
      , slotsOrElementsSlots_(alloc)
    {
      assertInvariants();
    }
    void addLiveRegister(AnyRegister reg) {
        liveRegs_.addUnchecked(reg);
        assertInvariants();
    }
    const RegisterSet &liveRegs() const {
        return liveRegs_;
    }
#ifdef CHECK_OSIPOINT_REGISTERS
    void addClobberedRegister(AnyRegister reg) {
        clobberedRegs_.addUnchecked(reg);
        assertInvariants();
    }
    const RegisterSet &clobberedRegs() const {
        return clobberedRegs_;
    }
#endif
    void addGcRegister(Register reg) {
        gcRegs_.addUnchecked(reg);
        assertInvariants();
    }
    GeneralRegisterSet gcRegs() const {
        return gcRegs_;
    }
    bool addGcSlot(uint32_t slot) {
        bool result = gcSlots_.append(slot);
        if (result)
            assertInvariants();
        return result;
    }
    SlotList &gcSlots() {
        return gcSlots_;
    }

    SlotList &slotsOrElementsSlots() {
        return slotsOrElementsSlots_;
    }
    GeneralRegisterSet slotsOrElementsRegs() const {
        return slotsOrElementsRegs_;
    }
    void addSlotsOrElementsRegister(Register reg) {
        slotsOrElementsRegs_.addUnchecked(reg);
        assertInvariants();
    }
    bool addSlotsOrElementsSlot(uint32_t slot) {
        bool result = slotsOrElementsSlots_.append(slot);
        if (result)
            assertInvariants();
        return result;
    }
    bool addSlotsOrElementsPointer(LAllocation alloc) {
        if (alloc.isStackSlot())
            return addSlotsOrElementsSlot(alloc.toStackSlot()->slot());
        JS_ASSERT(alloc.isRegister());
        addSlotsOrElementsRegister(alloc.toRegister().gpr());
        assertInvariants();
        return true;
    }
    bool hasSlotsOrElementsPointer(LAllocation alloc) const {
        if (alloc.isRegister())
            return slotsOrElementsRegs().has(alloc.toRegister().gpr());
        if (alloc.isStackSlot()) {
            for (size_t i = 0; i < slotsOrElementsSlots_.length(); i++) {
                if (slotsOrElementsSlots_[i] == alloc.toStackSlot()->slot())
                    return true;
            }
            return false;
        }
        return false;
    }

    bool addGcPointer(LAllocation alloc) {
        if (alloc.isStackSlot())
            return addGcSlot(alloc.toStackSlot()->slot());
        if (alloc.isRegister())
            addGcRegister(alloc.toRegister().gpr());
        assertInvariants();
        return true;
    }

    bool hasGcPointer(LAllocation alloc) const {
        if (alloc.isRegister())
            return gcRegs().has(alloc.toRegister().gpr());
        if (alloc.isStackSlot()) {
            for (size_t i = 0; i < gcSlots_.length(); i++) {
                if (gcSlots_[i] == alloc.toStackSlot()->slot())
                    return true;
            }
            return false;
        }
        JS_ASSERT(alloc.isArgument());
        return true;
    }

    bool addValueSlot(uint32_t slot) {
        bool result = valueSlots_.append(slot);
        if (result)
            assertInvariants();
        return result;
    }
    SlotList &valueSlots() {
        return valueSlots_;
    }

    bool hasValueSlot(uint32_t slot) const {
        for (size_t i = 0; i < valueSlots_.length(); i++) {
            if (valueSlots_[i] == slot)
                return true;
        }
        return false;
    }

#ifdef JS_NUNBOX32

    bool addNunboxParts(LAllocation type, LAllocation payload) {
        bool result = nunboxParts_.append(NunboxEntry(type, payload));
        if (result)
            assertInvariants();
        return result;
    }

    bool addNunboxType(uint32_t typeVreg, LAllocation type) {
        for (size_t i = 0; i < nunboxParts_.length(); i++) {
            if (nunboxParts_[i].type == type)
                return true;
            if (nunboxParts_[i].type == LUse(typeVreg, LUse::ANY)) {
                nunboxParts_[i].type = type;
                partialNunboxes_--;
                return true;
            }
        }
        partialNunboxes_++;

        
        uint32_t payloadVreg = typeVreg + 1;
        bool result = nunboxParts_.append(NunboxEntry(type, LUse(payloadVreg, LUse::ANY)));
        if (result)
            assertInvariants();
        return result;
    }

    bool hasNunboxType(LAllocation type) const {
        if (type.isArgument())
            return true;
        if (type.isStackSlot() && hasValueSlot(type.toStackSlot()->slot() + 1))
            return true;
        for (size_t i = 0; i < nunboxParts_.length(); i++) {
            if (nunboxParts_[i].type == type)
                return true;
        }
        return false;
    }

    bool addNunboxPayload(uint32_t payloadVreg, LAllocation payload) {
        for (size_t i = 0; i < nunboxParts_.length(); i++) {
            if (nunboxParts_[i].payload == payload)
                return true;
            if (nunboxParts_[i].payload == LUse(payloadVreg, LUse::ANY)) {
                partialNunboxes_--;
                nunboxParts_[i].payload = payload;
                return true;
            }
        }
        partialNunboxes_++;

        
        uint32_t typeVreg = payloadVreg - 1;
        bool result = nunboxParts_.append(NunboxEntry(LUse(typeVreg, LUse::ANY), payload));
        if (result)
            assertInvariants();
        return result;
    }

    bool hasNunboxPayload(LAllocation payload) const {
        if (payload.isArgument())
            return true;
        if (payload.isStackSlot() && hasValueSlot(payload.toStackSlot()->slot()))
            return true;
        for (size_t i = 0; i < nunboxParts_.length(); i++) {
            if (nunboxParts_[i].payload == payload)
                return true;
        }
        return false;
    }

    NunboxList &nunboxParts() {
        return nunboxParts_;
    }

    uint32_t partialNunboxes() {
        return partialNunboxes_;
    }

#elif JS_PUNBOX64

    void addValueRegister(Register reg) {
        valueRegs_.add(reg);
        assertInvariants();
    }
    GeneralRegisterSet valueRegs() const {
        return valueRegs_;
    }

    bool addBoxedValue(LAllocation alloc) {
        if (alloc.isRegister()) {
            Register reg = alloc.toRegister().gpr();
            if (!valueRegs().has(reg))
                addValueRegister(reg);
            return true;
        }
        if (alloc.isStackSlot()) {
            uint32_t slot = alloc.toStackSlot()->slot();
            for (size_t i = 0; i < valueSlots().length(); i++) {
                if (valueSlots()[i] == slot)
                    return true;
            }
            return addValueSlot(slot);
        }
        JS_ASSERT(alloc.isArgument());
        return true;
    }

    bool hasBoxedValue(LAllocation alloc) const {
        if (alloc.isRegister())
            return valueRegs().has(alloc.toRegister().gpr());
        if (alloc.isStackSlot())
            return hasValueSlot(alloc.toStackSlot()->slot());
        JS_ASSERT(alloc.isArgument());
        return true;
    }

#endif 

    bool encoded() const {
        return safepointOffset_ != INVALID_SAFEPOINT_OFFSET;
    }
    uint32_t offset() const {
        JS_ASSERT(encoded());
        return safepointOffset_;
    }
    void setOffset(uint32_t offset) {
        safepointOffset_ = offset;
    }
    uint32_t osiReturnPointOffset() const {
        
        
        
        return osiCallPointOffset_ + Assembler::PatchWrite_NearCallSize();
    }
    uint32_t osiCallPointOffset() const {
        return osiCallPointOffset_;
    }
    void setOsiCallPointOffset(uint32_t osiCallPointOffset) {
        JS_ASSERT(!osiCallPointOffset_);
        osiCallPointOffset_ = osiCallPointOffset;
    }
    void fixupOffset(MacroAssembler *masm) {
        osiCallPointOffset_ = masm->actualOffset(osiCallPointOffset_);
    }
};

class LInstruction::InputIterator
{
  private:
    LInstruction &ins_;
    size_t idx_;
    bool snapshot_;

    void handleOperandsEnd() {
        
        if (!snapshot_ && idx_ == ins_.numOperands() && ins_.snapshot()) {
            idx_ = 0;
            snapshot_ = true;
        }
    }

public:
    explicit InputIterator(LInstruction &ins) :
      ins_(ins),
      idx_(0),
      snapshot_(false)
    {
        handleOperandsEnd();
    }

    bool more() const {
        if (snapshot_)
            return idx_ < ins_.snapshot()->numEntries();
        if (idx_ < ins_.numOperands())
            return true;
        if (ins_.snapshot() && ins_.snapshot()->numEntries())
            return true;
        return false;
    }

    bool isSnapshotInput() const {
        return snapshot_;
    }

    void next() {
        JS_ASSERT(more());
        idx_++;
        handleOperandsEnd();
    }

    void replace(const LAllocation &alloc) {
        if (snapshot_)
            ins_.snapshot()->setEntry(idx_, alloc);
        else
            ins_.setOperand(idx_, alloc);
    }

    LAllocation *operator *() const {
        if (snapshot_)
            return ins_.snapshot()->getEntry(idx_);
        return ins_.getOperand(idx_);
    }

    LAllocation *operator ->() const {
        return **this;
    }
};

class LIRGraph
{
    struct ValueHasher
    {
        typedef Value Lookup;
        static HashNumber hash(const Value &v) {
            return HashNumber(v.asRawBits());
        }
        static bool match(const Value &lhs, const Value &rhs) {
            return lhs == rhs;
        }
    };

    FixedList<LBlock *> blocks_;
    Vector<Value, 0, IonAllocPolicy> constantPool_;
    typedef HashMap<Value, uint32_t, ValueHasher, IonAllocPolicy> ConstantPoolMap;
    ConstantPoolMap constantPoolMap_;
    Vector<LInstruction *, 0, IonAllocPolicy> safepoints_;
    Vector<LInstruction *, 0, IonAllocPolicy> nonCallSafepoints_;
    uint32_t numVirtualRegisters_;
    uint32_t numInstructions_;

    
    uint32_t localSlotCount_;
    
    uint32_t argumentSlotCount_;

    
    LSnapshot *entrySnapshot_;

    MIRGraph &mir_;

  public:
    explicit LIRGraph(MIRGraph *mir);

    bool init() {
        return constantPoolMap_.init() && blocks_.init(mir_.alloc(), mir_.numBlocks());
    }
    MIRGraph &mir() const {
        return mir_;
    }
    size_t numBlocks() const {
        return blocks_.length();
    }
    LBlock *getBlock(size_t i) const {
        return blocks_[i];
    }
    uint32_t numBlockIds() const {
        return mir_.numBlockIds();
    }
    void setBlock(size_t index, LBlock *block) {
        blocks_[index] = block;
    }
    uint32_t getVirtualRegister() {
        numVirtualRegisters_ += VREG_INCREMENT;
        return numVirtualRegisters_;
    }
    uint32_t numVirtualRegisters() const {
        
        
        return numVirtualRegisters_ + 1;
    }
    uint32_t getInstructionId() {
        return numInstructions_++;
    }
    uint32_t numInstructions() const {
        return numInstructions_;
    }
    void setLocalSlotCount(uint32_t localSlotCount) {
        localSlotCount_ = localSlotCount;
    }
    uint32_t localSlotCount() const {
        return localSlotCount_;
    }
    
    
    
    uint32_t paddedLocalSlotCount() const {
        
        
        
        size_t Alignment = Max(size_t(ABIStackAlignment), sizeof(Value));
        return AlignBytes(localSlotCount(), Alignment);
    }
    size_t paddedLocalSlotsSize() const {
        return paddedLocalSlotCount();
    }
    void setArgumentSlotCount(uint32_t argumentSlotCount) {
        argumentSlotCount_ = argumentSlotCount;
    }
    uint32_t argumentSlotCount() const {
        return argumentSlotCount_;
    }
    size_t argumentsSize() const {
        return argumentSlotCount() * sizeof(Value);
    }
    uint32_t totalSlotCount() const {
        return paddedLocalSlotCount() + argumentsSize();
    }
    bool addConstantToPool(const Value &v, uint32_t *index);
    size_t numConstants() const {
        return constantPool_.length();
    }
    Value *constantPool() {
        return &constantPool_[0];
    }
    void setEntrySnapshot(LSnapshot *snapshot) {
        JS_ASSERT(!entrySnapshot_);
        JS_ASSERT(snapshot->bailoutKind() == Bailout_InitialState);
        snapshot->setBailoutKind(Bailout_ArgumentCheck);
        entrySnapshot_ = snapshot;
    }
    LSnapshot *entrySnapshot() const {
        JS_ASSERT(entrySnapshot_);
        return entrySnapshot_;
    }
    bool noteNeedsSafepoint(LInstruction *ins);
    size_t numNonCallSafepoints() const {
        return nonCallSafepoints_.length();
    }
    LInstruction *getNonCallSafepoint(size_t i) const {
        return nonCallSafepoints_[i];
    }
    size_t numSafepoints() const {
        return safepoints_.length();
    }
    LInstruction *getSafepoint(size_t i) const {
        return safepoints_[i];
    }

    void dump(FILE *fp) const;
    void dump() const;
};

LAllocation::LAllocation(AnyRegister reg)
{
    if (reg.isFloat())
        *this = LFloatReg(reg.fpu());
    else
        *this = LGeneralReg(reg.gpr());
}

AnyRegister
LAllocation::toRegister() const
{
    JS_ASSERT(isRegister());
    if (isFloatReg())
        return AnyRegister(toFloatReg()->reg());
    return AnyRegister(toGeneralReg()->reg());
}

} 
} 

#define LIR_HEADER(opcode)                                                  \
    Opcode op() const {                                                     \
        return LInstruction::LOp_##opcode;                                  \
    }                                                                       \
    bool accept(LInstructionVisitor *visitor) {                             \
        visitor->setInstruction(this);                                      \
        return visitor->visit##opcode(this);                                \
    }

#include "jit/LIR-Common.h"
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
# if defined(JS_CODEGEN_X86)
#  include "jit/x86/LIR-x86.h"
# elif defined(JS_CODEGEN_X64)
#  include "jit/x64/LIR-x64.h"
# endif
# include "jit/shared/LIR-x86-shared.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/LIR-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/LIR-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/LIR-none.h"
#else
# error "Unknown architecture!"
#endif

#undef LIR_HEADER

namespace js {
namespace jit {

#define LIROP(name)                                                         \
    L##name *LInstruction::to##name()                                       \
    {                                                                       \
        JS_ASSERT(is##name());                                              \
        return static_cast<L##name *>(this);                                \
    }
    LIR_OPCODE_LIST(LIROP)
#undef LIROP

#define LALLOC_CAST(type)                                                   \
    L##type *LAllocation::to##type() {                                      \
        JS_ASSERT(is##type());                                              \
        return static_cast<L##type *>(this);                                \
    }
#define LALLOC_CONST_CAST(type)                                             \
    const L##type *LAllocation::to##type() const {                          \
        JS_ASSERT(is##type());                                              \
        return static_cast<const L##type *>(this);                          \
    }

LALLOC_CAST(Use)
LALLOC_CONST_CAST(Use)
LALLOC_CONST_CAST(GeneralReg)
LALLOC_CONST_CAST(FloatReg)
LALLOC_CONST_CAST(StackSlot)
LALLOC_CONST_CAST(Argument)
LALLOC_CONST_CAST(ConstantIndex)

#undef LALLOC_CAST

#ifdef JS_NUNBOX32
static inline signed
OffsetToOtherHalfOfNunbox(LDefinition::Type type)
{
    JS_ASSERT(type == LDefinition::TYPE || type == LDefinition::PAYLOAD);
    signed offset = (type == LDefinition::TYPE)
                    ? PAYLOAD_INDEX - TYPE_INDEX
                    : TYPE_INDEX - PAYLOAD_INDEX;
    return offset;
}

static inline void
AssertTypesFormANunbox(LDefinition::Type type1, LDefinition::Type type2)
{
    JS_ASSERT((type1 == LDefinition::TYPE && type2 == LDefinition::PAYLOAD) ||
              (type2 == LDefinition::TYPE && type1 == LDefinition::PAYLOAD));
}

static inline unsigned
OffsetOfNunboxSlot(LDefinition::Type type)
{
    if (type == LDefinition::PAYLOAD)
        return NUNBOX32_PAYLOAD_OFFSET;
    return NUNBOX32_TYPE_OFFSET;
}



static inline unsigned
BaseOfNunboxSlot(LDefinition::Type type, unsigned slot)
{
    if (type == LDefinition::PAYLOAD)
        return slot + NUNBOX32_PAYLOAD_OFFSET;
    return slot + NUNBOX32_TYPE_OFFSET;
}
#endif

} 
} 

#endif 
