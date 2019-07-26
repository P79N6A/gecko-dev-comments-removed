






#ifndef jsion_lir_h__
#define jsion_lir_h__




#include "jscntxt.h"
#include "IonAllocPolicy.h"
#include "InlineList.h"
#include "FixedArityList.h"
#include "LOpcodes.h"
#include "TypeOracle.h"
#include "Registers.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "shared/Assembler-shared.h"
#include "Safepoints.h"
#include "Bailouts.h"
#include "VMFunctions.h"

namespace js {
namespace ion {

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

static const uint32_t THIS_FRAME_SLOT = 0;

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

  protected:
    static const uintptr_t TAG_BIT = 1;
    static const uintptr_t TAG_SHIFT = 0;
    static const uintptr_t TAG_MASK = 1 << TAG_SHIFT;
    static const uintptr_t KIND_BITS = 4;
    static const uintptr_t KIND_SHIFT = TAG_SHIFT + TAG_BIT;
    static const uintptr_t KIND_MASK = (1 << KIND_BITS) - 1;
    static const uintptr_t DATA_BITS = (sizeof(uint32_t) * 8) - KIND_BITS - TAG_BIT;
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
        INT_ARGUMENT,   
        DOUBLE_ARGUMENT 
    };

  protected:
    bool isTagged() const {
        return !!(bits_ & TAG_MASK);
    }

    int32_t data() const {
        return int32_t(bits_) >> DATA_SHIFT;
    }
    void setData(int32_t data) {
        JS_ASSERT(int32_t(data) <= int32_t(DATA_MASK));
        bits_ &= ~(DATA_MASK << DATA_SHIFT);
        bits_ |= (data << DATA_SHIFT);
    }
    void setKindAndData(Kind kind, uint32_t data) {
        JS_ASSERT(int32_t(data) <= int32_t(DATA_MASK));
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
    { }

    static LAllocation *New() {
        return new LAllocation();
    }
    template <typename T>
    static LAllocation *New(const T &other) {
        return new LAllocation(other);
    }

    
    explicit LAllocation(const Value *vp) {
        bits_ = uintptr_t(vp);
        JS_ASSERT(!isTagged());
        bits_ |= TAG_MASK;
    }
    inline explicit LAllocation(const AnyRegister &reg);

    Kind kind() const {
        if (isTagged())
            return CONSTANT_VALUE;
        return (Kind)((bits_ >> KIND_SHIFT) & KIND_MASK);
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
        return kind() == INT_ARGUMENT || kind() == DOUBLE_ARGUMENT;
    }
    bool isRegister() const {
        return isGeneralReg() || isFloatReg();
    }
    bool isMemory() const {
        return isStackSlot() || isArgument();
    }
    bool isDouble() const {
        return kind() == DOUBLE_SLOT || kind() == FPU || kind() == DOUBLE_ARGUMENT;
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
        return reinterpret_cast<const Value *>(bits_ & ~TAG_MASK);
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
};

class LUse : public LAllocation
{
    static const uint32_t POLICY_BITS = 3;
    static const uint32_t POLICY_SHIFT = 0;
    static const uint32_t POLICY_MASK = (1 << POLICY_BITS) - 1;
    static const uint32_t REG_BITS = 5;
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
    LUse(Policy policy, bool usedAtStart = false) {
        set(policy, 0, usedAtStart);
    }
    LUse(Register reg, bool usedAtStart = false) {
        set(FIXED, reg.code(), usedAtStart);
    }
    LUse(FloatRegister reg, bool usedAtStart = false) {
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
    
    static LConstantIndex Bogus() {
        return LConstantIndex(0);
    }

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
    explicit LStackSlot(uint32_t slot, bool isDouble = false)
      : LAllocation(isDouble ? DOUBLE_SLOT : STACK_SLOT, slot)
    { }

    bool isDouble() const {
        return kind() == DOUBLE_SLOT;
    }

    uint32_t slot() const {
        return data();
    }
};




class LArgument : public LAllocation
{
  public:
    explicit LArgument(LAllocation::Kind kind, int32_t index)
      : LAllocation(kind, index)
    {
        JS_ASSERT(kind == INT_ARGUMENT || kind == DOUBLE_ARGUMENT);
    }

    int32_t index() const {
        return data();
    }
};


class LDefinition
{
    
    uint32_t bits_;

    
    
    
    
    
    
    
    LAllocation output_;

    static const uint32_t TYPE_BITS = 3;
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
        
        DEFAULT,

        
        
        
        
        
        
        PRESET,

        
        
        MUST_REUSE_INPUT,

        
        
        
        
        PASSTHROUGH
    };

    enum Type {
        GENERAL,    
        OBJECT,     
        DOUBLE,     
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
    }

  public:
    LDefinition(uint32_t index, Type type, Policy policy = DEFAULT) {
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

    LDefinition(uint32_t index, Type type, const LAllocation &a)
      : output_(a)
    {
        set(index, type, PRESET);
    }

    LDefinition() : bits_(0)
    { }

    static LDefinition BogusTemp() {
        return LDefinition(GENERAL, LConstantIndex::Bogus());
    }

    Policy policy() const {
        return (Policy)((bits_ >> POLICY_SHIFT) & POLICY_MASK);
    }
    Type type() const {
        return (Type)((bits_ >> TYPE_SHIFT) & TYPE_MASK);
    }
    uint32_t virtualRegister() const {
        return (bits_ >> VREG_SHIFT) & VREG_MASK;
    }
    LAllocation *output() {
        return &output_;
    }
    const LAllocation *output() const {
        return &output_;
    }
    bool isPreset() const {
        return policy() == PRESET;
    }
    bool isBogusTemp() const {
        return isPreset() && output()->isConstantIndex();
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
            bits_ |= PRESET << POLICY_SHIFT;
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
            return LDefinition::GENERAL;
          case MIRType_String:
          case MIRType_Object:
            return LDefinition::OBJECT;
          case MIRType_Double:
            return LDefinition::DOUBLE;
#if defined(JS_PUNBOX64)
          case MIRType_Value:
            return LDefinition::BOX;
#endif
          case MIRType_Slots:
          case MIRType_Elements:
            
            
            return LDefinition::GENERAL;
          case MIRType_Pointer:
            return LDefinition::GENERAL;
          case MIRType_ForkJoinSlice:
            return LDefinition::GENERAL;
          default:
            JS_NOT_REACHED("unexpected type");
            return LDefinition::GENERAL;
        }
    }
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
        snapshot_(NULL),
        safepoint_(NULL),
        mir_(NULL)
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
    void initSafepoint();

    
    
    virtual bool recoversInput() const {
        return false;
    }

    virtual void print(FILE *fp);
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

    LInstruction *instruction() {
        return ins_;
    }

  public:
    void setInstruction(LInstruction *ins) {
        ins_ = ins;
        if (ins->mirRaw())
            lastPC_ = ins->mirRaw()->trackedPc();
    }

    LInstructionVisitor()
      : ins_(NULL),
        lastPC_(NULL)
    {}

  public:
#define VISIT_INS(op) virtual bool visit##op(L##op *) { JS_NOT_REACHED("NYI: " #op); return false; }
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
    Vector<LPhi *, 4, IonAllocPolicy> phis_;
    InlineList<LInstruction> instructions_;
    LMoveGroup *entryMoveGroup_;
    LMoveGroup *exitMoveGroup_;

    LBlock(MBasicBlock *block)
      : block_(block),
        entryMoveGroup_(NULL),
        exitMoveGroup_(NULL)
    { }

  public:
    static LBlock *New(MBasicBlock *from) {
        return new LBlock(from);
    }
    void add(LInstruction *ins) {
        instructions_.pushBack(ins);
    }
    bool addPhi(LPhi *phi) {
        return phis_.append(phi);
    }
    size_t numPhis() const {
        return phis_.length();
    }
    LPhi *getPhi(size_t index) const {
        return phis_[index];
    }
    void removePhi(size_t index) {
        phis_.erase(&phis_[index]);
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
    uint32_t firstId();
    uint32_t lastId();
    Label *label();
    LMoveGroup *getEntryMoveGroup();
    LMoveGroup *getExitMoveGroup();
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






class LSnapshot : public TempObject
{
  private:
    uint32_t numSlots_;
    LAllocation *slots_;
    MResumePoint *mir_;
    SnapshotOffset snapshotOffset_;
    BailoutId bailoutId_;
    BailoutKind bailoutKind_;

    LSnapshot(MResumePoint *mir, BailoutKind kind);
    bool init(MIRGenerator *gen);

  public:
    static LSnapshot *New(MIRGenerator *gen, MResumePoint *snapshot, BailoutKind kind);

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
    MResumePoint *mir() const {
        return mir_;
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

  public:
    LSafepoint()
      : safepointOffset_(INVALID_SAFEPOINT_OFFSET)
      , osiCallPointOffset_(0)
#ifdef JS_NUNBOX32
      , partialNunboxes_(0)
#endif
    { }
    void addLiveRegister(AnyRegister reg) {
        liveRegs_.addUnchecked(reg);
    }
    const RegisterSet &liveRegs() const {
        return liveRegs_;
    }
    void addGcRegister(Register reg) {
        gcRegs_.addUnchecked(reg);
    }
    GeneralRegisterSet gcRegs() const {
        return gcRegs_;
    }
    bool addGcSlot(uint32_t slot) {
        return gcSlots_.append(slot);
    }
    SlotList &gcSlots() {
        return gcSlots_;
    }

    bool addGcPointer(LAllocation alloc) {
        if (alloc.isStackSlot())
            return addGcSlot(alloc.toStackSlot()->slot());
        if (alloc.isRegister())
            addGcRegister(alloc.toRegister().gpr());
        return true;
    }

    bool hasGcPointer(LAllocation alloc) {
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
        return valueSlots_.append(slot);
    }
    SlotList &valueSlots() {
        return valueSlots_;
    }

    bool hasValueSlot(uint32_t slot) {
        for (size_t i = 0; i < valueSlots_.length(); i++) {
            if (valueSlots_[i] == slot)
                return true;
        }
        return false;
    }

#ifdef JS_NUNBOX32

    bool addNunboxParts(LAllocation type, LAllocation payload) {
        return nunboxParts_.append(NunboxEntry(type, payload));
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
        return nunboxParts_.append(NunboxEntry(type, LUse(payloadVreg, LUse::ANY)));
    }

    bool hasNunboxType(LAllocation type) {
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
        return nunboxParts_.append(NunboxEntry(LUse(typeVreg, LUse::ANY), payload));
    }

    bool hasNunboxPayload(LAllocation payload) {
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
    }
    GeneralRegisterSet valueRegs() {
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

    bool hasBoxedValue(LAllocation alloc) {
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
        
        
        
        return osiCallPointOffset_ + Assembler::patchWrite_NearCallSize();
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
        safepointOffset_ = masm->actualOffset(safepointOffset_);
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
    InputIterator(LInstruction &ins) :
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
    Vector<LBlock *, 16, IonAllocPolicy> blocks_;
    Vector<HeapValue, 0, IonAllocPolicy> constantPool_;
    Vector<LInstruction *, 0, IonAllocPolicy> safepoints_;
    Vector<LInstruction *, 0, IonAllocPolicy> nonCallSafepoints_;
    uint32_t numVirtualRegisters_;
    uint32_t numInstructions_;

    
    uint32_t localSlotCount_;
    
    uint32_t argumentSlotCount_;

    
    LSnapshot *entrySnapshot_;

    
    LBlock *osrBlock_;

    MIRGraph &mir_;

  public:
    LIRGraph(MIRGraph *mir);

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
    bool addBlock(LBlock *block) {
        return blocks_.append(block);
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
    void setArgumentSlotCount(uint32_t argumentSlotCount) {
        argumentSlotCount_ = argumentSlotCount;
    }
    uint32_t argumentSlotCount() const {
        return argumentSlotCount_;
    }
    uint32_t totalSlotCount() const {
        return localSlotCount() + (argumentSlotCount() * sizeof(Value) / STACK_SLOT_SIZE);
    }
    bool addConstantToPool(const Value &v, uint32_t *index);
    size_t numConstants() const {
        return constantPool_.length();
    }
    HeapValue *constantPool() {
        return &constantPool_[0];
    }
    const HeapValue &getConstant(size_t index) const {
        return constantPool_[index];
    }
    void setEntrySnapshot(LSnapshot *snapshot) {
        JS_ASSERT(!entrySnapshot_);
        JS_ASSERT(snapshot->bailoutKind() == Bailout_Normal);
        snapshot->setBailoutKind(Bailout_ArgumentCheck);
        entrySnapshot_ = snapshot;
    }
    LSnapshot *entrySnapshot() const {
        JS_ASSERT(entrySnapshot_);
        return entrySnapshot_;
    }
    void setOsrBlock(LBlock *block) {
        JS_ASSERT(!osrBlock_);
        osrBlock_ = block;
    }
    LBlock *osrBlock() const {
        return osrBlock_;
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
};

LAllocation::LAllocation(const AnyRegister &reg)
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

#if defined(JS_NUNBOX32)
# define BOX_OUTPUT_ACCESSORS()                                             \
    const LDefinition *outputType() {                                       \
        return getDef(TYPE_INDEX);                                          \
    }                                                                       \
    const LDefinition *outputPayload() {                                    \
        return getDef(PAYLOAD_INDEX);                                       \
    }
#elif defined(JS_PUNBOX64)
# define BOX_OUTPUT_ACCESSORS()                                             \
    const LDefinition *outputValue() {                                      \
        return getDef(0);                                                   \
    }
#endif

#include "LIR-Common.h"
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
# if defined(JS_CPU_X86)
#  include "x86/LIR-x86.h"
# elif defined(JS_CPU_X64)
#  include "x64/LIR-x64.h"
# endif
# include "shared/LIR-x86-shared.h"
#elif defined(JS_CPU_ARM)
# include "arm/LIR-arm.h"
#endif

#undef LIR_HEADER

#include "LIR-inl.h"

#endif 

