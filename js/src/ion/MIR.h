








































#ifndef jsion_mir_h__
#define jsion_mir_h__





#include "jscntxt.h"
#include "jslibmath.h"
#include "jsinfer.h"
#include "jsinferinlines.h"
#include "TypeOracle.h"
#include "TypePolicy.h"
#include "IonAllocPolicy.h"
#include "InlineList.h"
#include "MOpcodes.h"
#include "FixedArityList.h"
#include "IonMacroAssembler.h"
#include "Bailouts.h"

namespace js {
namespace ion {

static const inline
MIRType MIRTypeFromValue(const js::Value &vp)
{
    if (vp.isDouble())
        return MIRType_Double;
    return MIRTypeFromValueType(vp.extractNonDoubleType());
}

#define MIR_FLAG_LIST(_)                                                        \
    _(InWorklist)                                                               \
    _(EmittedAtUses)                                                            \
    _(LoopInvariant)                                                            \
    _(Commutative)                                                              \
    _(Movable)       /* Allow LICM and GVN to move this instruction */          \
    _(Lowered)       /* (Debug only) has a virtual register */                  \
    _(Guard)         /* Not removable if uses == 0 */                           \
                                                                                \
    /* The instruction has been marked dead for lazy removal from resume
     * points.
     */                                                                         \
    _(Unused)

class MDefinition;
class MInstruction;
class MBasicBlock;
class MNode;
class MUse;
class MIRGraph;
class MResumePoint;


class MUse : public TempObject, public InlineForwardListNode<MUse>
{
    friend class MDefinition;

    MNode *node_;           
    uint32 index_;          

    MUse(MNode *owner, uint32 index)
      : node_(owner),
        index_(index)
    { }

  public:
    static inline MUse *New(MNode *owner, uint32 index) {
        return new MUse(owner, index);
    }

    MNode *node() const {
        return node_;
    }
    uint32 index() const {
        return index_;
    }
};

typedef InlineForwardList<MUse>::iterator MUseIterator;








class MNode : public TempObject
{
    friend class MDefinition;

  protected:
    MBasicBlock *block_;    

  public:
    enum Kind {
        Definition,
        ResumePoint
    };

    MNode() : block_(NULL)
    { }
    MNode(MBasicBlock *block) : block_(block)
    { }

    virtual Kind kind() const = 0;

    
    virtual MDefinition *getOperand(size_t index) const = 0;
    virtual size_t numOperands() const = 0;

    bool isDefinition() const {
        return kind() == Definition;
    }
    bool isResumePoint() const {
        return kind() == ResumePoint;
    }
    MBasicBlock *block() const {
        return block_;
    }

    
    
    virtual TypePolicy *typePolicy() {
        return NULL;
    }

    
    
    MUseIterator replaceOperand(MUseIterator use, MDefinition *ins);
    void replaceOperand(size_t index, MDefinition *ins);

    inline MDefinition *toDefinition();
    inline MResumePoint *toResumePoint();

  protected:
    
    virtual void setOperand(size_t index, MDefinition *operand) = 0;

    
    inline void initOperand(size_t index, MDefinition *ins);
};

class AliasSet {
  private:
    uint32 flags_;

  public:
    enum Flag {
        None_             = 0,
        ObjectFields      = 1 << 0, 
        Element           = 1 << 1, 
        Slot              = 1 << 2, 
        Last              = Slot,
        Any               = Last | (Last - 1),

        
        Store_            = 1 << 31
    };
    AliasSet(uint32 flags)
      : flags_(flags)
    { }

  public:
    inline bool isNone() const {
        return flags_ == None_;
    }
    uint32 flags() const {
        return flags_ & Any;
    }
    inline bool isStore() const {
        return !!(flags_ & Store_);
    }
    inline bool isLoad() const {
        return !isStore() && !isNone();
    }
    inline AliasSet operator |(const AliasSet &other) const {
        return AliasSet(flags_ | other.flags_);
    }
    inline AliasSet operator &(const AliasSet &other) const {
        return AliasSet(flags_ & other.flags_);
    }
    static AliasSet None() {
        return AliasSet(None_);
    }
    static AliasSet Load(uint32 flags) {
        JS_ASSERT(flags && !(flags & Store_));
        return AliasSet(flags);
    }
    static AliasSet Store(uint32 flags) {
        JS_ASSERT(flags && !(flags & Store_));
        return AliasSet(flags | Store_);
    }
};

static const unsigned NUM_ALIAS_SETS = sizeof(AliasSet) * 8;


class MDefinition : public MNode
{
    friend class MBasicBlock;
    friend class Loop;

  public:
    enum Opcode {
#   define DEFINE_OPCODES(op) Op_##op,
        MIR_OPCODE_LIST(DEFINE_OPCODES)
#   undef DEFINE_OPCODES
        Op_Invalid
    };

  private:
    InlineForwardList<MUse> uses_; 
    uint32 id_;                    
                                   
    uint32 valueNumber_;           
    MIRType resultType_;           
    uint32 flags_;                 
    MDefinition *dependency_;      

  private:
    enum Flag {
        None = 0,
#   define DEFINE_FLAG(flag) flag,
        MIR_FLAG_LIST(DEFINE_FLAG)
#   undef DEFINE_FLAG
        Total
    };

    void setBlock(MBasicBlock *block) {
        block_ = block;
    }

    bool hasFlags(uint32 flags) const {
        return (flags_ & flags) == flags;
    }
    void removeFlags(uint32 flags) {
        flags_ &= ~flags;
    }
    void setFlags(uint32 flags) {
        flags_ |= flags;
    }

#ifdef TRACK_SNAPSHOTS
    
    jsbytecode *trackedPc_;

  public:
    void setTrackedPc(jsbytecode *pc) {
        if (!trackedPc_)
            trackedPc_ = pc;
    }

    jsbytecode *trackedPc() {
        return trackedPc_;
    }
#endif

  public:
    MDefinition()
      : id_(0),
        valueNumber_(0),
        resultType_(MIRType_None),
        flags_(0),
        dependency_(NULL)
#ifdef TRACK_SNAPSHOTS
      , trackedPc_(NULL)
#endif
    { }

    virtual Opcode op() const = 0;
    void printName(FILE *fp);
    static void PrintOpcodeName(FILE *fp, Opcode op);
    virtual void printOpcode(FILE *fp);

    virtual HashNumber valueHash() const;
    virtual bool congruentTo(MDefinition* const &ins) const {
        return false;
    }
    bool congruentIfOperandsEqual(MDefinition * const &ins) const;
    virtual MDefinition *foldsTo(bool useValueNumbers);

    MNode::Kind kind() const {
        return MNode::Definition;
    }

    uint32 id() const {
        JS_ASSERT(block_);
        return id_;
    }
    void setId(uint32 id) {
        id_ = id;
    }

    uint32 valueNumber() const {
        JS_ASSERT(block_);
        return valueNumber_;
    }

    void setValueNumber(uint32 vn) {
        valueNumber_ = vn;
    }
#define FLAG_ACCESSOR(flag) \
    bool is##flag() const {\
        return hasFlags(1 << flag);\
    }\
    void set##flag() {\
        JS_ASSERT(!hasFlags(1 << flag));\
        setFlags(1 << flag);\
    }\
    void setNot##flag() {\
        JS_ASSERT(hasFlags(1 << flag));\
        removeFlags(1 << flag);\
    }\
    void set##flag##Unchecked() {\
        setFlags(1 << flag);\
    }

    MIR_FLAG_LIST(FLAG_ACCESSOR)
#undef FLAG_ACCESSOR

    MIRType type() const {
        return resultType_;
    }

    
    MUseIterator usesBegin() const {
        return uses_.begin();
    }

    
    MUseIterator usesEnd() const {
        return uses_.end();
    }

    bool canEmitAtUses() const {
        return !isEmittedAtUses();
    }

    
    MUseIterator removeUse(MUseIterator use);

    
    size_t useCount() const;

    bool hasUses() const {
        return !uses_.empty();
    }

    virtual bool isControlInstruction() const {
        return false;
    }

    void addUse(MNode *node, size_t index) {
        uses_.pushFront(MUse::New(node, index));
    }
    void replaceAllUsesWith(MDefinition *dom);

    
    
    
    virtual bool updateForReplacement(MDefinition *ins) {
        return true;
    }

    
    
    void linkUse(MUse *use) {
        JS_ASSERT(use->node()->getOperand(use->index()) == this);
        uses_.pushFront(use);
    }

    void setVirtualRegister(uint32 vreg) {
        id_ = vreg;
#ifdef DEBUG
        setLoweredUnchecked();
#endif
    }
    uint32 virtualRegister() const {
        JS_ASSERT(isLowered());
        return id_;
    }

  public:
    
#   define OPCODE_CASTS(opcode)                                             \
    bool is##opcode() const {                                               \
        return op() == Op_##opcode;                                         \
    }                                                                       \
    inline M##opcode *to##opcode();
    MIR_OPCODE_LIST(OPCODE_CASTS)
#   undef OPCODE_CASTS

    inline MInstruction *toInstruction();
    bool isInstruction() const {
        return !isPhi();
    }

    void setResultType(MIRType type) {
        resultType_ = type;
    }

    MDefinition *dependency() const {
        return dependency_;
    }
    void setDependency(MDefinition *dependency) {
        dependency_ = dependency;
    }
    virtual AliasSet getAliasSet() const {
        
        return AliasSet::Store(AliasSet::Any);
    }
    bool isEffectful() const {
        return getAliasSet().isStore();
    }
};




class MUseDefIterator
{
    MDefinition *def_;
    MUseIterator current_;

    MUseIterator search(MUseIterator start) {
        MUseIterator i(start);
        for (; i != def_->usesEnd(); i++) {
            if (i->node()->isDefinition())
                return i;
        }
        return def_->usesEnd();
    }

  public:
    MUseDefIterator(MDefinition *def)
      : def_(def),
        current_(search(def->usesBegin()))
    {
    }

    operator bool() const {
        return current_ != def_->usesEnd();
    }
    MUseDefIterator operator ++(int) {
        MUseDefIterator old(*this);
        if (current_ != def_->usesEnd())
            current_++;
        current_ = search(current_);
        return old;
    }
    MUse *use() const {
        return *current_;
    }
    MDefinition *def() const {
        return current_->node()->toDefinition();
    }
    size_t index() const {
        return current_->index();
    }
};



class MInstruction
  : public MDefinition,
    public InlineListNode<MInstruction>
{
    MResumePoint *resumePoint_;

  public:
    MInstruction()
      : resumePoint_(NULL)
    { }

    virtual bool accept(MInstructionVisitor *visitor) = 0;

    void setResumePoint(MResumePoint *resumePoint) {
        JS_ASSERT(!resumePoint_);
        resumePoint_ = resumePoint;
    }
    MResumePoint *resumePoint() const {
        return resumePoint_;
    }
};

#define INSTRUCTION_HEADER(opcode)                                          \
    Opcode op() const {                                                     \
        return MDefinition::Op_##opcode;                                    \
    }                                                                       \
    bool accept(MInstructionVisitor *visitor) {                             \
        return visitor->visit##opcode(this);                                \
    }

template <size_t Arity>
class MAryInstruction : public MInstruction
{
  protected:
    FixedArityList<MDefinition*, Arity> operands_;

    void setOperand(size_t index, MDefinition *operand) {
        operands_[index] = operand;
    }

  public:
    MDefinition *getOperand(size_t index) const {
        return operands_[index];
    }
    size_t numOperands() const {
        return Arity;
    }
};

class MNullaryInstruction : public MAryInstruction<0>
{ };


class MStart : public MNullaryInstruction
{
  public:
    enum StartType {
        StartType_Default,
        StartType_Osr
    };

  private:
    StartType startType_;

  private:
    MStart(StartType startType)
      : startType_(startType)
    { }

  public:
    INSTRUCTION_HEADER(Start);
    static MStart *New(StartType startType) {
        return new MStart(startType);
    }

    StartType startType() {
        return startType_;
    }
};




class MOsrEntry : public MNullaryInstruction
{
  protected:
    MOsrEntry() {
        setResultType(MIRType_StackFrame);
    }

  public:
    INSTRUCTION_HEADER(OsrEntry);
    static MOsrEntry *New() {
        return new MOsrEntry;
    }
};


class MConstant : public MNullaryInstruction
{
    js::Value value_;
    uint32 constantPoolIndex_;

    MConstant(const Value &v);

  public:
    INSTRUCTION_HEADER(Constant);
    static MConstant *New(const Value &v);

    const js::Value &value() const {
        return value_;
    }
    const js::Value *vp() const {
        return &value_;
    }
    void setConstantPoolIndex(uint32 index) {
        constantPoolIndex_ = index;
    }
    uint32 constantPoolIndex() const {
        JS_ASSERT(hasConstantPoolIndex());
        return constantPoolIndex_;
    }
    bool hasConstantPoolIndex() const {
        return !!constantPoolIndex_;
    }

    void printOpcode(FILE *fp);

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MParameter : public MNullaryInstruction
{
    int32 index_;
    types::TypeSet *typeSet_;

  public:
    static const int32 THIS_SLOT = -1;

    MParameter(int32 index, types::TypeSet *types)
      : index_(index),
        typeSet_(types)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Parameter);
    static MParameter *New(int32 index, types::TypeSet *types);

    int32 index() const {
        return index_;
    }
    types::TypeSet *typeSet() const {
        return typeSet_;
    }
    void printOpcode(FILE *fp);

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;
};

class MCallee : public MNullaryInstruction
{
  public:
    MCallee()
    {
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(Callee);

    bool congruentTo(MDefinition * const &ins) const {
        return congruentIfOperandsEqual(ins);
    }

    static MCallee *New() {
        return new MCallee();
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MControlInstruction : public MInstruction
{
  public:
    MControlInstruction()
    { }

    virtual size_t numSuccessors() const = 0;
    virtual MBasicBlock *getSuccessor(size_t i) const = 0;
    virtual void replaceSuccessor(size_t i, MBasicBlock *successor) = 0;

    bool isControlInstruction() const {
        return true;
    }
};

class MTableSwitch
  : public MControlInstruction,
    public TableSwitchPolicy
{
    
    Vector<MBasicBlock*, 0, IonAllocPolicy> successors_;

    
    
    Vector<MBasicBlock*, 0, IonAllocPolicy> cases_;

    MDefinition *operand_;
    int32 low_;
    int32 high_;
    MBasicBlock *defaultCase_;

    MTableSwitch(MDefinition *ins, int32 low, int32 high)
      : successors_(),
        cases_(),
        low_(low),
        high_(high),
        defaultCase_(NULL)
    {
        initOperand(0, ins);
    }

  protected:
    void setOperand(size_t index, MDefinition *operand) {
        JS_ASSERT(index == 0);
        operand_ = operand;
    }

  public:
    INSTRUCTION_HEADER(TableSwitch);
    static MTableSwitch *New(MDefinition *ins,
                             int32 low, int32 high);

    size_t numSuccessors() const {
        return successors_.length();
    }

    MBasicBlock *getSuccessor(size_t i) const {
        JS_ASSERT(i < numSuccessors());
        return successors_[i];
    }

    void replaceSuccessor(size_t i, MBasicBlock *successor) {
        JS_ASSERT(i < numSuccessors());
        if (successors_[i] == defaultCase_)
            defaultCase_ = successor;
        successors_[i] = successor;
    }

    MBasicBlock** successors() {
        return &successors_[0];
    }

    int32 low() const {
        return low_;
    }

    int32 high() const {
        return high_;
    }

    MBasicBlock *getDefault() const {
        JS_ASSERT(defaultCase_);
        return defaultCase_;
    }

    MBasicBlock *getCase(size_t i) const {
        JS_ASSERT(i < cases_.length());
        return cases_[i];
    }

    size_t numCases() const {
        return high() - low() + 1;
    }

    void addDefault(MBasicBlock *block) {
        JS_ASSERT(!defaultCase_);
        defaultCase_ = block;
        successors_.append(block);
    }

    void addCase(MBasicBlock *block, bool isDefault = false) {
        JS_ASSERT(cases_.length() < (size_t)(high_ - low_ + 1));
        cases_.append(block);
        if (!isDefault) {
            JS_ASSERT(block != defaultCase_);
            successors_.append(block);
        }
    }

    MDefinition *getOperand(size_t index) const {
        JS_ASSERT(index == 0);
        return operand_;
    }

    size_t numOperands() const {
        return 1;
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

template <size_t Arity, size_t Successors>
class MAryControlInstruction : public MControlInstruction
{
    FixedArityList<MDefinition *, Arity> operands_;
    FixedArityList<MBasicBlock *, Successors> successors_;

  protected:
    void setOperand(size_t index, MDefinition *operand) {
        operands_[index] = operand;
    }
    void setSuccessor(size_t index, MBasicBlock *successor) {
        successors_[index] = successor;
    }

  public:
    MDefinition *getOperand(size_t index) const {
        return operands_[index];
    }
    size_t numOperands() const {
        return Arity;
    }
    size_t numSuccessors() const {
        return Successors;
    }
    MBasicBlock *getSuccessor(size_t i) const {
        return successors_[i];
    }
    void replaceSuccessor(size_t i, MBasicBlock *succ) {
        successors_[i] = succ;
    }
};


class MGoto : public MAryControlInstruction<0, 1>
{
    MGoto(MBasicBlock *target) {
        setSuccessor(0, target);
    }

  public:
    INSTRUCTION_HEADER(Goto);
    static MGoto *New(MBasicBlock *target);

    MBasicBlock *target() {
        return getSuccessor(0);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MTest : public MAryControlInstruction<1, 2>
{
    MTest(MDefinition *ins, MBasicBlock *if_true, MBasicBlock *if_false) {
        initOperand(0, ins);
        setSuccessor(0, if_true);
        setSuccessor(1, if_false);
    }

  public:
    INSTRUCTION_HEADER(Test);
    static MTest *New(MDefinition *ins,
                      MBasicBlock *ifTrue, MBasicBlock *ifFalse);

    MBasicBlock *ifTrue() const {
        return getSuccessor(0);
    }
    MBasicBlock *ifFalse() const {
        return getSuccessor(1);
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};


class MReturn
  : public MAryControlInstruction<1, 0>,
    public BoxInputsPolicy
{
    MReturn(MDefinition *ins) {
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Return);
    static MReturn *New(MDefinition *ins) {
        return new MReturn(ins);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MThrow
  : public MAryControlInstruction<1, 0>,
    public BoxInputsPolicy
{
    MThrow(MDefinition *ins) {
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Throw);
    static MThrow *New(MDefinition *ins) {
        return new MThrow(ins);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MNewArray : public MNullaryInstruction
{
    
    uint32 count_;
    
    HeapPtr<types::TypeObject> type_;

  public:
    INSTRUCTION_HEADER(NewArray);

    MNewArray(uint32 count, types::TypeObject *type)
        : count_(count), type_(type)
    {
        setResultType(MIRType_Object);
    }

    uint32 count() const {
        return count_;
    }

    types::TypeObject *type() const {
        return type_;
    }
};




class MPrepareCall : public MNullaryInstruction
{
  public:
    INSTRUCTION_HEADER(PrepareCall);

    MPrepareCall()
    { }

    
    uint32 argc() const;

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MVariadicInstruction : public MInstruction
{
    FixedList<MDefinition *> operands_;

  protected:
    bool init(size_t length) {
        return operands_.init(length);
    }

  public:
    
    MDefinition *getOperand(size_t index) const {
        return operands_[index];
    }
    size_t numOperands() const {
        return operands_.length();
    }
    void setOperand(size_t index, MDefinition *operand) {
        operands_[index] = operand;
    }
};

class MCall
  : public MVariadicInstruction,
    public CallPolicy
{
  private:
    
    
    static const size_t PrepareCallOperandIndex  = 0;
    static const size_t FunctionOperandIndex   = 1;
    static const size_t NumNonArgumentOperands = 2;

  protected:
    
    bool construct_;

    MCall(bool construct)
      : construct_(construct)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Call);
    static MCall *New(size_t argc, bool construct);

    void initPrepareCall(MDefinition *start) {
        JS_ASSERT(start->isPrepareCall());
        return initOperand(PrepareCallOperandIndex, start);
    }
    void initFunction(MDefinition *func) {
        JS_ASSERT(!func->isPassArg());
        return initOperand(FunctionOperandIndex, func);
    }

    MDefinition *getFunction() const {
        return getOperand(FunctionOperandIndex);
    }
    void replaceFunction(MInstruction *newfunc) {
        replaceOperand(FunctionOperandIndex, newfunc);
    }

    void addArg(size_t argnum, MPassArg *arg);

    MDefinition *getArg(uint32 index) const {
        return getOperand(NumNonArgumentOperands + index);
    }

    bool isConstruct() const {
        return construct_;
    }

    
    uint32 argc() const {
        return numOperands() - NumNonArgumentOperands;
    }

    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Any);
    }
};

class MUnaryInstruction : public MAryInstruction<1>
{
  protected:
    MUnaryInstruction(MDefinition *ins)
    {
        initOperand(0, ins);
    }
};

class MBinaryInstruction : public MAryInstruction<2>
{
  protected:
    MBinaryInstruction(MDefinition *left, MDefinition *right)
    {
        initOperand(0, left);
        initOperand(1, right);
    }

  public:
    MDefinition *lhs() const {
        return getOperand(0);
    }
    MDefinition *rhs() const {
        return getOperand(1);
    }

  protected:
    HashNumber valueHash() const
    {
        MDefinition *lhs = getOperand(0);
        MDefinition *rhs = getOperand(1);

        return op() ^ lhs->valueNumber() ^ rhs->valueNumber();
    }

    bool congruentTo(MDefinition *const &ins) const
    {
        if (op() != ins->op())
            return false;

        if (type() != ins->type())
            return false;

        if (isEffectful() || ins->isEffectful())
            return false;

        MDefinition *left = getOperand(0);
        MDefinition *right = getOperand(1);
        MDefinition *tmp;

        if (isCommutative() && left->valueNumber() > right->valueNumber()) {
            tmp = right;
            right = left;
            left = tmp;
        }

        MDefinition *insLeft = ins->getOperand(0);
        MDefinition *insRight = ins->getOperand(1);
        if (isCommutative() && insLeft->valueNumber() > insRight->valueNumber()) {
            tmp = insRight;
            insRight = insLeft;
            insLeft = tmp;
        }

        return (left->valueNumber() == insLeft->valueNumber()) &&
               (right->valueNumber() == insRight->valueNumber());
    }
};

class MTernaryInstruction : public MAryInstruction<3>
{
  protected:
    MTernaryInstruction(MDefinition *first, MDefinition *second, MDefinition *third)
    {
        initOperand(0, first);
        initOperand(1, second);
        initOperand(2, third);
    }

  protected:
    HashNumber valueHash() const
    {
        MDefinition *first = getOperand(0);
        MDefinition *second = getOperand(1);
        MDefinition *third = getOperand(2);

        return op() ^ first->valueNumber() ^ second->valueNumber() ^ third->valueNumber();
    }

    bool congruentTo(MDefinition *const &ins) const
    {
        if (op() != ins->op())
            return false;

        if (type() != ins->type())
            return false;

        if (isEffectful() || ins->isEffectful())
            return false;

        MDefinition *first = getOperand(0);
        MDefinition *second = getOperand(1);
        MDefinition *third = getOperand(2);
        MDefinition *insFirst = ins->getOperand(0);
        MDefinition *insSecond = ins->getOperand(1);
        MDefinition *insThird = ins->getOperand(2);

        return first->valueNumber() == insFirst->valueNumber() &&
               second->valueNumber() == insSecond->valueNumber() &&
               third->valueNumber() == insThird->valueNumber();
    }
};

class MCompare
  : public MBinaryInstruction,
    public ComparePolicy
{
    JSOp jsop_;

    MCompare(MDefinition *left, MDefinition *right, JSOp jsop)
      : MBinaryInstruction(left, right),
        jsop_(jsop)
    {
        setResultType(MIRType_Boolean);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(Compare);
    static MCompare *New(MDefinition *left, MDefinition *right, JSOp op);

    void infer(JSContext *cx, const TypeOracle::BinaryTypes &b);
    MIRType specialization() const {
        return specialization_;
    }

    JSOp jsop() const {
        return jsop_;
    }
    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        if (specialization_ == MIRType_None)
            return AliasSet::Store(AliasSet::Any);
        JS_ASSERT(specialization_ <= MIRType_Object);
        return AliasSet::None();
    }
};



class MCopy : public MUnaryInstruction
{
    MCopy(MDefinition *ins)
      : MUnaryInstruction(ins)
    {
        setResultType(ins->type());
    }

  public:
    INSTRUCTION_HEADER(Copy);
    static MCopy *New(MDefinition *ins);

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;

    AliasSet getAliasSet() const {
        JS_NOT_REACHED("Unexpected MCopy after building SSA");
        return AliasSet::None();
    }
};


class MBox : public MUnaryInstruction
{
    MBox(MDefinition *ins)
      : MUnaryInstruction(ins)
    {
        setResultType(MIRType_Value);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(Box);
    static MBox *New(MDefinition *ins)
    {
        
        JS_ASSERT(ins->type() != MIRType_Value);

        return new MBox(ins);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};




class MUnbox : public MUnaryInstruction
{
  public:
    enum Mode {
        Fallible,       
        Infallible,     
        TypeBarrier     
    };

  private:
    Mode mode_;

    MUnbox(MDefinition *ins, MIRType type, Mode mode)
      : MUnaryInstruction(ins),
        mode_(mode)
    {
        JS_ASSERT(ins->type() == MIRType_Value);
        setResultType(type);
        setMovable();
        if (mode_ == TypeBarrier && ins->isEffectful())
            setGuard();
    }

  public:
    INSTRUCTION_HEADER(Unbox);
    static MUnbox *New(MDefinition *ins, MIRType type, Mode mode)
    {
        return new MUnbox(ins, type, mode);
    }

    Mode mode() const {
        return mode_;
    }
    MDefinition *input() const {
        return getOperand(0);
    }
    BailoutKind bailoutKind() const {
        
        JS_ASSERT(fallible());
        return mode() == Fallible
               ? Bailout_Normal
               : Bailout_TypeBarrier;
    }
    bool fallible() const {
        return mode() != Infallible;
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MGuardObject : public MUnaryInstruction, public SingleObjectPolicy
{
    MGuardObject(MDefinition *ins)
      : MUnaryInstruction(ins)
    {
        setGuard();
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(GuardObject);

    static MGuardObject *New(MDefinition *ins) {
        return new MGuardObject(ins);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};






class MPassArg
  : public MUnaryInstruction,
    public BoxInputsPolicy
{
    int32 argnum_;

  private:
    MPassArg(MDefinition *def)
      : MUnaryInstruction(def), argnum_(-1)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(PassArg);
    static MPassArg *New(MDefinition *def)
    {
        return new MPassArg(def);
    }

    MDefinition *getArgument() const {
        return getOperand(0);
    }

    
    void setArgnum(uint32 argnum) {
        argnum_ = argnum;
    }
    uint32 getArgnum() const {
        JS_ASSERT(argnum_ >= 0);
        return (uint32)argnum_;
    }

    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MToDouble : public MUnaryInstruction
{
    MToDouble(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Double);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(ToDouble);
    static MToDouble *New(MDefinition *def)
    {
        return new MToDouble(def);
    }

    MDefinition *foldsTo(bool useValueNumbers);
    MDefinition *input() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};




class MToInt32 : public MUnaryInstruction
{
    MToInt32(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(ToInt32);
    static MToInt32 *New(MDefinition *def)
    {
        return new MToInt32(def);
    }

    MDefinition *input() const {
        return getOperand(0);
    }

    MDefinition *foldsTo(bool useValueNumbers);

    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MTruncateToInt32 : public MUnaryInstruction
{
    MTruncateToInt32(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(TruncateToInt32);
    static MTruncateToInt32 *New(MDefinition *def)
    {
        return new MTruncateToInt32(def);
    }

    MDefinition *input() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};


class MToString : public MUnaryInstruction
{
    MToString(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_String);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(ToString);
    static MToString *New(MDefinition *def)
    {
        return new MToString(def);
    }

    MDefinition *input() const {
        return getOperand(0);
    }

    MDefinition *foldsTo(bool useValueNumbers);

    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        JS_ASSERT(input()->type() < MIRType_Object);
        return AliasSet::None();
    }
};

class MBitNot
  : public MUnaryInstruction,
    public BitwisePolicy
{
  protected:
    MBitNot(MDefinition *input)
      : MUnaryInstruction(input)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(BitNot);
    static MBitNot *New(MDefinition *input);

    TypePolicy *typePolicy() {
        return this;
    }

    MDefinition *foldsTo(bool useValueNumbers);
    void infer(const TypeOracle::Unary &u);

    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        if (specialization_ == MIRType_None)
            return AliasSet::Store(AliasSet::Any);
        return AliasSet::None();
    }
};

class MTypeOf
  : public MUnaryInstruction,
    public BoxInputsPolicy
{
    MIRType inputType_;

    MTypeOf(MDefinition *def, MIRType inputType)
      : MUnaryInstruction(def), inputType_(inputType)
    {
        setResultType(MIRType_String);
    }

  public:
    INSTRUCTION_HEADER(TypeOf);

    static MTypeOf *New(MDefinition *def, MIRType inputType) {
        return new MTypeOf(def, inputType);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MIRType inputType() const {
        return inputType_;
    }
    MDefinition *input() const {
        return getOperand(0);
    }
    MDefinition *foldsTo(bool useValueNumbers);

    AliasSet getAliasSet() const {
        if (inputType_ <= MIRType_String)
            return AliasSet::None();

        
        return AliasSet::Store(AliasSet::Any);
    }
};

class MToId
  : public MBinaryInstruction,
    public BoxInputsPolicy
{
    MToId(MDefinition *object, MDefinition *index)
      : MBinaryInstruction(object, index)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(ToId);

    static MToId *New(MDefinition *object, MDefinition *index) {
        return new MToId(object, index);
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

class MBinaryBitwiseInstruction
  : public MBinaryInstruction,
    public BitwisePolicy
{
  protected:
    MBinaryBitwiseInstruction(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    TypePolicy *typePolicy() {
        return this;
    }

    MDefinition *foldsTo(bool useValueNumbers);
    virtual MDefinition *foldIfZero(size_t operand) = 0;
    virtual MDefinition *foldIfEqual()  = 0;
    void infer(const TypeOracle::Binary &b);

    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        if (specialization_ >= MIRType_Object)
            return AliasSet::Store(AliasSet::Any);
        return AliasSet::None();
    }
};

class MBitAnd : public MBinaryBitwiseInstruction
{
    MBitAnd(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitAnd);
    static MBitAnd *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        return getOperand(operand); 
    }

    MDefinition *foldIfEqual() {
        return getOperand(0); 
    }
};

class MBitOr : public MBinaryBitwiseInstruction
{
    MBitOr(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitOr);
    static MBitOr *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        return getOperand(1 - operand); 
    }

    MDefinition *foldIfEqual() {
        return getOperand(0); 
    }

};

class MBitXor : public MBinaryBitwiseInstruction
{
    MBitXor(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitXor);
    static MBitXor *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        return getOperand(1 - operand); 
    }

    MDefinition *foldIfEqual() {
        return MConstant::New(Int32Value(0));
    }
};

class MLsh : public MBinaryBitwiseInstruction
{
    MLsh(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(Lsh);
    static MLsh *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        
        
        return getOperand(0);
    }

    MDefinition *foldIfEqual() {
        return this;
    }
};

class MRsh : public MBinaryBitwiseInstruction
{
    MRsh(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(Rsh);
    static MRsh *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        
        
        return getOperand(0);
    }

    MDefinition *foldIfEqual() {
        return this;
    }
};

class MUrsh : public MBinaryBitwiseInstruction
{
    bool canOverflow_;

    MUrsh(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right),
        canOverflow_(true)
    { }

  public:
    INSTRUCTION_HEADER(Ursh);
    static MUrsh *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        
        if (operand == 0)
            return getOperand(0);

        return this;
    }

    MDefinition *foldIfEqual() {
        return this;
    }

    bool canOverflow() {
        
        MDefinition *lhs = getOperand(0);
        MDefinition *rhs = getOperand(1);

        if (lhs->isConstant() && lhs->toConstant()->value().toInt32() >= 0)
            return false;

        if (rhs->isConstant() && (rhs->toConstant()->value().toInt32() & 0x1F) != 0)
            return false;

        return canOverflow_;
    }

    bool fallible() {
        return canOverflow();
    }
};

class MBinaryArithInstruction
  : public MBinaryInstruction,
    public ArithPolicy
{
  public:
    MBinaryArithInstruction(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setMovable();
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MIRType specialization() const {
        return specialization_;
    }

    MDefinition *foldsTo(bool useValueNumbers);

    virtual double getIdentity() = 0;

    void infer(const TypeOracle::Binary &b);

    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        if (specialization_ >= MIRType_Object)
            return AliasSet::Store(AliasSet::Any);
        return AliasSet::None();
    }
};

class MAbs
  : public MUnaryInstruction,
    public ArithPolicy
{
    MAbs(MDefinition *num, MIRType type)
      : MUnaryInstruction(num)
    {
        JS_ASSERT(type == MIRType_Double || type == MIRType_Int32);
        setResultType(type);
    }

  public:
    INSTRUCTION_HEADER(Abs);
    static MAbs *New(MDefinition *num, MIRType type) {
        return new MAbs(num, type);
    }
    MDefinition *num() const {
        return getOperand(0);
    }
    TypePolicy *typePolicy() {
        return this;
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};


class MAdd : public MBinaryArithInstruction
{
    MAdd(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Add);
    static MAdd *New(MDefinition *left, MDefinition *right) {
        return new MAdd(left, right);
    }

    double getIdentity() {
        return 0;
    }
};

class MSub : public MBinaryArithInstruction
{
    MSub(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Sub);
    static MSub *New(MDefinition *left, MDefinition *right) {
        return new MSub(left, right);
    }

    double getIdentity() {
        return 0;
    }
};

class MMul : public MBinaryArithInstruction
{
    bool canOverflow_;
    bool canBeNegativeZero_;

    MMul(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right),
        canOverflow_(true),
        canBeNegativeZero_(true)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Mul);
    static MMul *New(MDefinition *left, MDefinition *right) {
        return new MMul(left, right);
    }

    MDefinition *foldsTo(bool useValueNumbers);

    double getIdentity() {
        return 1;
    }

    bool canOverflow() {
        return canOverflow_;
    }

    bool canBeNegativeZero() {
        return canBeNegativeZero_;
    }

    bool fallible() {
        return canBeNegativeZero_ || canOverflow_;
    }
};

class MDiv : public MBinaryArithInstruction
{
    MDiv(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Div);
    static MDiv *New(MDefinition *left, MDefinition *right) {
        return new MDiv(left, right);
    }

    MDefinition *foldsTo(bool useValueNumbers);
    double getIdentity() {
        JS_NOT_REACHED("not used");
        return 1;
    }
};

class MMod : public MBinaryArithInstruction
{
    MMod(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Mod);
    static MMod *New(MDefinition *left, MDefinition *right) {
        return new MMod(left, right);
    }

    MDefinition *foldsTo(bool useValueNumbers);
    double getIdentity() {
        JS_NOT_REACHED("not used");
        return 1;
    }
};

class MConcat
  : public MBinaryInstruction,
    public BinaryStringPolicy
{
    MConcat(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setMovable();
        setResultType(MIRType_String);
    }

  public:
    INSTRUCTION_HEADER(Concat);
    static MConcat *New(MDefinition *left, MDefinition *right) {
        return new MConcat(left, right);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MPhi : public MDefinition, public InlineForwardListNode<MPhi>
{
    js::Vector<MDefinition *, 2, IonAllocPolicy> inputs_;
    uint32 slot_;
    bool triedToSpecialize_;

    MPhi(uint32 slot)
      : slot_(slot),
        triedToSpecialize_(false)
    {
        setResultType(MIRType_Value);
    }

  protected:
    void setOperand(size_t index, MDefinition *operand) {
        inputs_[index] = operand;
    }

  public:
    INSTRUCTION_HEADER(Phi);
    static MPhi *New(uint32 slot);

    MDefinition *getOperand(size_t index) const {
        return inputs_[index];
    }
    size_t numOperands() const {
        return inputs_.length();
    }
    uint32 slot() const {
        return slot_;
    }
    bool triedToSpecialize() const {
        return triedToSpecialize_;
    }
    void specialize(MIRType type) {
        triedToSpecialize_ = true;
        setResultType(type);
    }
    bool addInput(MDefinition *ins);

    MDefinition *foldsTo(bool useValueNumbers);

    bool congruentTo(MDefinition * const &ins) const;

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MOsrValue : public MAryInstruction<1>
{
  private:
    ptrdiff_t frameOffset_;

    MOsrValue(MOsrEntry *entry, ptrdiff_t frameOffset)
      : frameOffset_(frameOffset)
    {
        setOperand(0, entry);
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(OsrValue);
    static MOsrValue *New(MOsrEntry *entry, ptrdiff_t frameOffset) {
        return new MOsrValue(entry, frameOffset);
    }

    ptrdiff_t frameOffset() const {
        return frameOffset_;
    }

    MOsrEntry *entry() {
        return getOperand(0)->toOsrEntry();
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MOsrScopeChain : public MAryInstruction<1>
{
  private:
    MOsrScopeChain(MOsrEntry *entry)
    {
        setOperand(0, entry);
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(OsrScopeChain);
    static MOsrScopeChain *New(MOsrEntry *entry) {
        return new MOsrScopeChain(entry);
    }

    MOsrEntry *entry() {
        return getOperand(0)->toOsrEntry();
    }
};


class MCheckOverRecursed : public MNullaryInstruction
{
  public:
    INSTRUCTION_HEADER(CheckOverRecursed);
};



class MRecompileCheck : public MNullaryInstruction
{
    MRecompileCheck() {
        setGuard();
    }

  public:
    INSTRUCTION_HEADER(RecompileCheck);

    static MRecompileCheck *New() {
        return new MRecompileCheck();
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MRegExp : public MNullaryInstruction
{
  public:
    
    
    
    
    enum CloneBehavior {
        UseSource,
        MustClone
    };

  private:
    HeapPtr<RegExpObject> source_;
    CloneBehavior shouldClone_;

    MRegExp(RegExpObject *source, CloneBehavior shouldClone)
      : source_(source),
        shouldClone_(shouldClone)
    {
        setResultType(MIRType_Object);

        
        
        if (shouldClone == UseSource)
            setMovable();
    }

  public:
    INSTRUCTION_HEADER(RegExp)

    static MRegExp *New(RegExpObject *source, CloneBehavior shouldClone) {
        return new MRegExp(source, shouldClone);
    }

    const HeapPtr<RegExpObject> &source() const {
        return source_;
    }
    CloneBehavior shouldClone() const {
        return shouldClone_;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MLambda
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    HeapPtr<JSFunction> fun_;

    MLambda(MDefinition *scopeChain, JSFunction *fun)
      : MUnaryInstruction(scopeChain), fun_(fun)
    {
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(Lambda);

    static MLambda *New(MDefinition *scopeChain, JSFunction *fun) {
        return new MLambda(scopeChain, fun);
    }
    MDefinition *scopeChain() const {
        return getOperand(0);
    }
    const HeapPtr<JSFunction> &fun() const {
        return fun_;
    }
    TypePolicy *typePolicy() {
        return this;
    }
};

class MLambdaJoinableForCallOrSet
  : public MBinaryInstruction,
    public ObjectPolicy
{
    HeapPtr<JSFunction> fun_;

  protected:
    MLambdaJoinableForCallOrSet(MDefinition *target, MDefinition *scopeChain, JSFunction *fun)
      : MBinaryInstruction(target, scopeChain), fun_(fun)
    {
        setResultType(MIRType_Object);
    }

  public:
    MDefinition *target() const {
        return getOperand(0);
    }
    MDefinition *scopeChain() const {
        return getOperand(1);
    }
    const HeapPtr<JSFunction> &fun() const {
        return fun_;
    }
    TypePolicy *typePolicy() {
        return this;
    }
};

class MLambdaJoinableForCall
  : public MLambdaJoinableForCallOrSet
{
    uint32_t argc_;

    MLambdaJoinableForCall(MDefinition *callee, uint32_t argc, MDefinition *scopeChain,
                           JSFunction *fun)
      : MLambdaJoinableForCallOrSet(callee, scopeChain, fun), argc_(argc)
    { }

  public:
    INSTRUCTION_HEADER(LambdaJoinableForCall);

    static MLambdaJoinableForCall *New(MDefinition *callee, uint32_t argc, MDefinition *scopeChain,
                                       JSFunction *fun) {
        return new MLambdaJoinableForCall(callee, argc, scopeChain, fun);
    }
    uint32_t argc() const {
        return argc_;
    }
};

class MLambdaJoinableForSet
  : public MLambdaJoinableForCallOrSet
{
    MLambdaJoinableForSet(MDefinition *target, MDefinition *scopeChain, JSFunction *fun)
      : MLambdaJoinableForCallOrSet(target, scopeChain, fun)
    { }

  public:
    INSTRUCTION_HEADER(LambdaJoinableForSet);

    static MLambdaJoinableForSet *New(MDefinition *target, MDefinition *scopeChain,
                                      JSFunction *fun) {
        return new MLambdaJoinableForSet(target, scopeChain, fun);
    }
};


class MImplicitThis
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MImplicitThis(MDefinition *callee)
      : MUnaryInstruction(callee)
    {
        setResultType(MIRType_Value);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(ImplicitThis);

    static MImplicitThis *New(MDefinition *callee) {
        return new MImplicitThis(callee);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *callee() const {
        return getOperand(0);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};


class MSlots
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MSlots(MDefinition *object)
      : MUnaryInstruction(object)
    {
        setResultType(MIRType_Slots);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(Slots);

    static MSlots *New(MDefinition *object) {
        return new MSlots(object);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *object() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MElements
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MElements(MDefinition *object)
      : MUnaryInstruction(object)
    {
        setResultType(MIRType_Elements);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(Elements);

    static MElements *New(MDefinition *object) {
        return new MElements(object);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *object() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};

class MFlatClosureUpvars
  : public MUnaryInstruction
{
    MFlatClosureUpvars(MDefinition *callee)
      : MUnaryInstruction(callee)
    {
        JS_ASSERT(callee->type() == MIRType_Object);
        setResultType(MIRType_UpvarSlots);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(FlatClosureUpvars);

    static MFlatClosureUpvars *New(MDefinition *callee) {
        return new MFlatClosureUpvars(callee);
    }

    MDefinition *callee() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition * const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MInitializedLength
  : public MUnaryInstruction
{
    MInitializedLength(MDefinition *elements)
      : MUnaryInstruction(elements)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(InitializedLength);

    static MInitializedLength *New(MDefinition *elements) {
        return new MInitializedLength(elements);
    }

    MDefinition *elements() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MArrayLength
  : public MUnaryInstruction
{
  public:
    MArrayLength(MDefinition *elements)
      : MUnaryInstruction(elements)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

    INSTRUCTION_HEADER(ArrayLength);

    MDefinition *elements() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MNot : public MUnaryInstruction
{
    MIRType specialization_;

  public:
    MNot(MDefinition *elements)
      : MUnaryInstruction(elements)
    {
        setResultType(MIRType_Boolean);
        setMovable();
    }

    INSTRUCTION_HEADER(Not);

    MDefinition *foldsTo(bool useValueNumbers);

    MDefinition *operand() const {
        return getOperand(0);
    }

    void infer(MIRType type);
    MIRType specialization() const {
        return specialization_;
    }

    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};




class MBoundsCheck
  : public MBinaryInstruction
{
    
    int32 minimum_;
    int32 maximum_;

    MBoundsCheck(MDefinition *index, MDefinition *length)
      : MBinaryInstruction(index, length), minimum_(0), maximum_(0)
    {
        setGuard();
        setMovable();
        JS_ASSERT(index->type() == MIRType_Int32);
        JS_ASSERT(length->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BoundsCheck);

    static MBoundsCheck *New(MDefinition *index, MDefinition *length) {
        return new MBoundsCheck(index, length);
    }

    MDefinition *index() const {
        return getOperand(0);
    }
    MDefinition *length() const {
        return getOperand(1);
    }
    int32 minimum() const {
        return minimum_;
    }
    void setMinimum(int32 n) {
        minimum_ = n;
    }
    int32 maximum() const {
        return maximum_;
    }
    void setMaximum(int32 n) {
        maximum_ = n;
    }
    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;
    bool updateForReplacement(MDefinition *ins);
};


class MBoundsCheckLower
  : public MUnaryInstruction
{
    int32 minimum_;

    MBoundsCheckLower(MDefinition *index)
      : MUnaryInstruction(index), minimum_(0)
    {
        setGuard();
        setMovable();
        JS_ASSERT(index->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BoundsCheckLower);

    static MBoundsCheckLower *New(MDefinition *index) {
        return new MBoundsCheckLower(index);
    }

    MDefinition *index() const {
        return getOperand(0);
    }
    int32 minimum() const {
        return minimum_;
    }
    void setMinimum(int32 n) {
        minimum_ = n;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MLoadElement
  : public MBinaryInstruction,
    public SingleObjectPolicy
{
    bool needsHoleCheck_;

    MLoadElement(MDefinition *elements, MDefinition *index, bool needsHoleCheck)
      : MBinaryInstruction(elements, index),
        needsHoleCheck_(needsHoleCheck)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(LoadElement);

    static MLoadElement *New(MDefinition *elements, MDefinition *index, bool needsHoleCheck) {
        return new MLoadElement(elements, index, needsHoleCheck);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *elements() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    bool needsHoleCheck() const {
        return needsHoleCheck_;
    }
    bool fallible() const {
        return needsHoleCheck();
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::Element);
    }
};




class MLoadElementHole
  : public MTernaryInstruction,
    public SingleObjectPolicy
{
    bool needsHoleCheck_;

    MLoadElementHole(MDefinition *elements, MDefinition *index, MDefinition *initLength, bool needsHoleCheck)
      : MTernaryInstruction(elements, index, initLength),
        needsHoleCheck_(needsHoleCheck)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
        JS_ASSERT(initLength->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(LoadElementHole);

    static MLoadElementHole *New(MDefinition *elements, MDefinition *index,
                                 MDefinition *initLength, bool needsHoleCheck) {
        return new MLoadElementHole(elements, index, initLength, needsHoleCheck);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *elements() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    MDefinition *initLength() const {
        return getOperand(2);
    }
    bool needsHoleCheck() const {
        return needsHoleCheck_;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::Element);
    }
};

class MStoreElementCommon
{
    bool needsBarrier_;
    MIRType elementType_;

  protected:
    MStoreElementCommon()
      : needsBarrier_(false),
        elementType_(MIRType_None)
    { }

  public:
    MIRType elementType() const {
        return elementType_;
    }
    void setElementType(MIRType elementType) {
        JS_ASSERT(elementType != MIRType_None);
        elementType_ = elementType;
    }
    bool needsBarrier() const {
        return needsBarrier_;
    }
    void setNeedsBarrier(bool needsBarrier) {
        needsBarrier_ = needsBarrier;
    }
};


class MStoreElement
  : public MAryInstruction<3>,
    public MStoreElementCommon,
    public SingleObjectPolicy
{
    MStoreElement(MDefinition *elements, MDefinition *index, MDefinition *value) {
        initOperand(0, elements);
        initOperand(1, index);
        initOperand(2, value);
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(StoreElement);

    static MStoreElement *New(MDefinition *elements, MDefinition *index, MDefinition *value) {
        return new MStoreElement(elements, index, value);
    }
    MDefinition *elements() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    MDefinition *value() const {
        return getOperand(2);
    }
    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Element);
    }
};





class MStoreElementHole
  : public MAryInstruction<4>,
    public MStoreElementCommon,
    public SingleObjectPolicy
{
    MStoreElementHole(MDefinition *object, MDefinition *elements,
                      MDefinition *index, MDefinition *value) {
        initOperand(0, object);
        initOperand(1, elements);
        initOperand(2, index);
        initOperand(3, value);
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(StoreElementHole);

    static MStoreElementHole *New(MDefinition *object, MDefinition *elements,
                                  MDefinition *index, MDefinition *value) {
        return new MStoreElementHole(object, elements, index, value);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    MDefinition *elements() const {
        return getOperand(1);
    }
    MDefinition *index() const {
        return getOperand(2);
    }
    MDefinition *value() const {
        return getOperand(3);
    }
    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        
        
        return AliasSet::Store(AliasSet::Element | AliasSet::ObjectFields);
    }
};

class MLoadFixedSlot : public MUnaryInstruction, public SingleObjectPolicy
{
    size_t slot_;

    MLoadFixedSlot(MDefinition *obj, size_t slot)
      : MUnaryInstruction(obj), slot_(slot)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(LoadFixedSlot);

    static MLoadFixedSlot *New(MDefinition *obj, size_t slot) {
        return new MLoadFixedSlot(obj, slot);
    }

    TypePolicy *typePolicy() {
        return this;
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    size_t slot() const {
        return slot_;
    }
    bool congruentTo(MDefinition * const &ins) const {
        if (!ins->isLoadFixedSlot())
            return false;
        if (slot() != ins->toLoadFixedSlot()->slot())
            return false;
        return congruentIfOperandsEqual(ins);
    }

    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::Slot);
    }
};

class MStoreFixedSlot : public MBinaryInstruction, public SingleObjectPolicy
{
    size_t slot_;

    MStoreFixedSlot(MDefinition *obj, MDefinition *rval, size_t slot)
      : MBinaryInstruction(obj, rval), slot_(slot)
    {}

  public:
    INSTRUCTION_HEADER(StoreFixedSlot);

    static MStoreFixedSlot *New(MDefinition *obj, MDefinition *rval, size_t slot) {
        return new MStoreFixedSlot(obj, rval, slot);
    }

    TypePolicy *typePolicy() {
        return this;
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    MDefinition *value() const {
        return getOperand(1);
    }
    size_t slot() const {
        return slot_;
    }

    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Slot);
    }
};

class MGetPropertyCache
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    JSAtom *atom_;
    JSScript *script_;
    jsbytecode *pc_;

    MGetPropertyCache(MDefinition *obj, JSAtom *atom, JSScript *script, jsbytecode *pc)
      : MUnaryInstruction(obj),
        atom_(atom),
        script_(script),
        pc_(pc)
    {
        
        JS_ASSERT(script);

        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(GetPropertyCache);

    static MGetPropertyCache *New(MDefinition *obj, JSAtom *atom,
                                  JSScript *script, jsbytecode *pc) {
        return new MGetPropertyCache(obj, atom, script, pc);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    JSAtom *atom() const {
        return atom_;
    }
    JSScript *script() const {
        return script_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
    TypePolicy *typePolicy() { return this; }
};


class MGuardShape
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    const Shape *shape_;

    MGuardShape(MDefinition *obj, const Shape *shape)
      : MUnaryInstruction(obj),
        shape_(shape)
    {
        setGuard();
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(GuardShape);

    static MGuardShape *New(MDefinition *obj, const Shape *shape) {
        return new MGuardShape(obj, shape);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *obj() const {
        return getOperand(0);
    }
    const Shape *shape() const {
        return shape_;
    }
    bool congruentTo(MDefinition * const &ins) const {
        if (!ins->isGuardShape())
            return false;
        if (shape() != ins->toGuardShape()->shape())
            return false;
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MGuardClass
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    const Class *class_;

    MGuardClass(MDefinition *obj, const Class *clasp)
      : MUnaryInstruction(obj),
        class_(clasp)
    {
        setGuard();
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(GuardClass);

    static MGuardClass *New(MDefinition *obj, const Class *clasp) {
        return new MGuardClass(obj, clasp);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *obj() const {
        return getOperand(0);
    }
    const Class *getClass() const {
        return class_;
    }
    bool congruentTo(MDefinition * const &ins) const {
        if (!ins->isGuardClass())
            return false;
        if (getClass() != ins->toGuardClass()->getClass())
            return false;
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MLoadSlot
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    uint32 slot_;

    MLoadSlot(MDefinition *slots, uint32 slot)
      : MUnaryInstruction(slots),
        slot_(slot)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(slots->type() == MIRType_Slots || slots->type() == MIRType_UpvarSlots);
    }

  public:
    INSTRUCTION_HEADER(LoadSlot);

    static MLoadSlot *New(MDefinition *slots, uint32 slot) {
        return new MLoadSlot(slots, slot);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *slots() const {
        return getOperand(0);
    }
    uint32 slot() const {
        return slot_;
    }
    bool congruentTo(MDefinition * const &ins) const {
        if (!ins->isLoadSlot())
            return false;
        if (slot() != ins->toLoadSlot()->slot())
            return false;
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        if (slots()->type() == MIRType_Slots)
            return AliasSet::Load(AliasSet::Slot);

        JS_ASSERT(slots()->type() == MIRType_UpvarSlots);
        return AliasSet::None();
    }
};


class MFunctionEnvironment
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
  public:
    MFunctionEnvironment(MDefinition *function)
        : MUnaryInstruction(function)
    {
        setResultType(MIRType_Object);
    }

    INSTRUCTION_HEADER(FunctionEnvironment);

    static MFunctionEnvironment *New(MDefinition *function) {
        return new MFunctionEnvironment(function);
    }

    MDefinition *function() const {
        return getOperand(0);
    }
};


class MStoreSlot
  : public MBinaryInstruction,
    public SingleObjectPolicy
{
    uint32 slot_;
    MIRType slotType_;
    bool needsBarrier_;

    MStoreSlot(MDefinition *slots, uint32 slot, MDefinition *value)
        : MBinaryInstruction(slots, value),
          slot_(slot),
          slotType_(MIRType_None),
          needsBarrier_(false)
    {
        JS_ASSERT(slots->type() == MIRType_Slots);
    }

  public:
    INSTRUCTION_HEADER(StoreSlot);

    static MStoreSlot *New(MDefinition *slots, uint32 slot, MDefinition *value) {
        return new MStoreSlot(slots, slot, value);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *slots() const {
        return getOperand(0);
    }
    MDefinition *value() const {
        return getOperand(1);
    }
    uint32 slot() const {
        return slot_;
    }
    MIRType slotType() const {
        return slotType_;
    }
    void setSlotType(MIRType slotType) {
        JS_ASSERT(slotType != MIRType_None);
        slotType_ = slotType;
    }
    bool needsBarrier() const {
        return needsBarrier_;
    }
    void setNeedsBarrier(bool needsBarrier) {
        needsBarrier_ = needsBarrier;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Slot);
    }
};




class MCallGetPropertyOrName
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
  public:
    enum AccessKind {
        PROPERTY,
        NAMETYPEOF,
        NAME
    };

  private:
    JSAtom *atom_;
    AccessKind kind_;

  protected:
    MCallGetPropertyOrName(MDefinition *obj, JSAtom *atom, AccessKind kind)
      : MUnaryInstruction(obj),
        atom_(atom),
        kind_(kind)
    {
        setResultType(MIRType_Value);
    }

  public:
    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *obj() const {
        return getOperand(0);
    }
    JSAtom *atom() const {
        return atom_;
    }
    AccessKind accessKind() const {
        return kind_;
    }
    AliasSet getAliasSet() const {
        
        
        
        return AliasSet::Load(AliasSet::Any) |
               AliasSet::Store(AliasSet::Any);
    }
};

class MSetPropertyInstruction : public MBinaryInstruction
{
    JSAtom *atom_;
    bool strict_;

  protected:
    MSetPropertyInstruction(MDefinition *obj, MDefinition *value, JSAtom *atom,
                        bool strict)
      : MBinaryInstruction(obj, value),
        atom_(atom), strict_(strict)
    {}

  public:
    MDefinition *obj() const {
        return getOperand(0);
    }
    MDefinition *value() const {
        return getOperand(1);
    }
    JSAtom *atom() const {
        return atom_;
    }
    bool strict() const {
        return strict_;
    }
};



class MCallSetProperty
  : public MSetPropertyInstruction,
    public CallSetElementPolicy
{
    MCallSetProperty(MDefinition *obj, MDefinition *value, JSAtom *atom, bool strict)
      : MSetPropertyInstruction(obj, value, atom, strict)
    {
    }

  public:
    INSTRUCTION_HEADER(CallSetProperty);

    static MCallSetProperty *New(MDefinition *obj, MDefinition *value, JSAtom *atom, bool strict) {
        return new MCallSetProperty(obj, value, atom, strict);
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

class MSetPropertyCache
  : public MSetPropertyInstruction,
    public SingleObjectPolicy
{
    MSetPropertyCache(MDefinition *obj, MDefinition *value, JSAtom *atom, bool strict)
      : MSetPropertyInstruction(obj, value, atom, strict)
    {
    }

  public:
    INSTRUCTION_HEADER(SetPropertyCache);

    static MSetPropertyCache *New(MDefinition *obj, MDefinition *value, JSAtom *atom, bool strict) {
        return new MSetPropertyCache(obj, value, atom, strict);
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

class MCallGetProperty : public MCallGetPropertyOrName
{
    MCallGetProperty(MDefinition *obj, JSAtom *atom)
        : MCallGetPropertyOrName(obj, atom, MCallGetPropertyOrName::PROPERTY)
    {}

  public:
    INSTRUCTION_HEADER(CallGetProperty);

    static MCallGetProperty *New(MDefinition *obj, JSAtom *atom) {
        return new MCallGetProperty(obj, atom);
    }
};

class MCallGetName : public MCallGetPropertyOrName
{
    MCallGetName(MDefinition *obj, JSAtom *atom)
        : MCallGetPropertyOrName(obj, atom, MCallGetPropertyOrName::NAME)
    {}

  public:
    INSTRUCTION_HEADER(CallGetName);

    static MCallGetName *New(MDefinition *obj, JSAtom *atom) {
        return new MCallGetName(obj, atom);
    }
};

class MCallGetNameTypeOf : public MCallGetPropertyOrName
{
    MCallGetNameTypeOf(MDefinition *obj, JSAtom *atom)
        : MCallGetPropertyOrName(obj, atom, MCallGetPropertyOrName::NAMETYPEOF)
    {}

  public:
    INSTRUCTION_HEADER(CallGetNameTypeOf);

    static MCallGetNameTypeOf *New(MDefinition *obj, JSAtom *atom) {
        return new MCallGetNameTypeOf(obj, atom);
    }
};



class MCallGetElement
  : public MBinaryInstruction,
    public BoxInputsPolicy
{
    MCallGetElement(MDefinition *lhs, MDefinition *rhs)
      : MBinaryInstruction(lhs, rhs)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(CallGetElement);

    static MCallGetElement *New(MDefinition *lhs, MDefinition *rhs) {
        return new MCallGetElement(lhs, rhs);
    }
    TypePolicy *typePolicy() {
        return this;
    }
};

class MCallSetElement
  : public MAryInstruction<3>,
    public CallSetElementPolicy
{
    MCallSetElement(MDefinition *object, MDefinition *index, MDefinition *value) {
        initOperand(0, object);
        initOperand(1, index);
        initOperand(2, value);
    }

  public:
    INSTRUCTION_HEADER(CallSetElement);

    static MCallSetElement *New(MDefinition *object, MDefinition *index, MDefinition *value) {
        return new MCallSetElement(object, index, value);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *object() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    MDefinition *value() const {
        return getOperand(2);
    }
};

class MStringLength
  : public MUnaryInstruction,
    public StringPolicy
{
  public:
    MStringLength(MDefinition *string)
      : MUnaryInstruction(string)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

    INSTRUCTION_HEADER(StringLength);

    TypePolicy *typePolicy() {
        return this;
    }

    MDefinition *string() const {
        return getOperand(0);
    }
    bool congruentTo(MDefinition *const &ins) const {
        return congruentIfOperandsEqual(ins);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};


class MRound
  : public MUnaryInstruction
{
  public:
    enum RoundingMode {
        
        RoundingMode_Round,
        
        RoundingMode_Floor
        
    };

  private:
    RoundingMode mode_;

  public:

    MRound(MDefinition *num, RoundingMode mode)
      : MUnaryInstruction(num),
        mode_(mode)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

    INSTRUCTION_HEADER(Round);

    MDefinition *num() const {
        return getOperand(0);
    }
    RoundingMode mode() const {
        return mode_;
    }

    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MIteratorStart
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    uint8 flags_;

    MIteratorStart(MDefinition *obj, uint8 flags)
      : MUnaryInstruction(obj), flags_(flags)
    {
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(IteratorStart);

    static MIteratorStart *New(MDefinition *obj, uint8 flags) {
        return new MIteratorStart(obj, flags);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *object() const {
        return getOperand(0);
    }
    uint8 flags() const {
        return flags_;
    }
};

class MIteratorNext
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MIteratorNext(MDefinition *iter)
      : MUnaryInstruction(iter)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(IteratorNext);

    static MIteratorNext *New(MDefinition *iter) {
        return new MIteratorNext(iter);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *iterator() const {
        return getOperand(0);
    }
};

class MIteratorMore
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MIteratorMore(MDefinition *iter)
      : MUnaryInstruction(iter)
    {
        setResultType(MIRType_Boolean);
    }

  public:
    INSTRUCTION_HEADER(IteratorMore);

    static MIteratorMore *New(MDefinition *iter) {
        return new MIteratorMore(iter);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *iterator() const {
        return getOperand(0);
    }
};

class MIteratorEnd
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MIteratorEnd(MDefinition *iter)
      : MUnaryInstruction(iter)
    {}

  public:
    INSTRUCTION_HEADER(IteratorEnd);

    static MIteratorEnd *New(MDefinition *iter) {
        return new MIteratorEnd(iter);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *iterator() const {
        return getOperand(0);
    }
};



class MTypeBarrier : public MUnaryInstruction
{
    BailoutKind bailoutKind_;
    types::TypeSet *typeSet_;

    MTypeBarrier(MDefinition *def, types::TypeSet *types)
      : MUnaryInstruction(def),
        typeSet_(types)
    {
        setResultType(MIRType_Value);
        setGuard();
        setMovable();
        bailoutKind_ = def->isEffectful()
                       ? Bailout_TypeBarrier
                       : Bailout_Normal;
    }

  public:
    INSTRUCTION_HEADER(TypeBarrier);

    static MTypeBarrier *New(MDefinition *def, types::TypeSet *types) {
        return new MTypeBarrier(def, types);
    }
    bool congruentTo(MDefinition * const &def) const {
        return false;
    }
    MDefinition *input() const {
        return getOperand(0);
    }
    BailoutKind bailoutKind() const {
        return bailoutKind_;
    }
    types::TypeSet *typeSet() const {
        return typeSet_;
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};




class MResumePoint : public MNode
{
  public:
    enum Mode {
        ResumeAt,
        ResumeAfter,
        Outer
    };

  private:
    friend class MBasicBlock;

    MDefinition **operands_;
    uint32 stackDepth_;
    jsbytecode *pc_;
    MResumePoint *caller_;
    Mode mode_;

    MResumePoint(MBasicBlock *block, jsbytecode *pc, MResumePoint *parent, Mode mode);
    bool init(MBasicBlock *state);
    void inherit(MBasicBlock *state);

  protected:
    void setOperand(size_t index, MDefinition *operand) {
        JS_ASSERT(index < stackDepth_);
        operands_[index] = operand;
    }

  public:
    static MResumePoint *New(MBasicBlock *block, jsbytecode *pc, MResumePoint *parent, Mode mode);

    MNode::Kind kind() const {
        return MNode::ResumePoint;
    }
    size_t numOperands() const {
        return stackDepth_;
    }
    MDefinition *getOperand(size_t index) const {
        JS_ASSERT(index < stackDepth_);
        return operands_[index];
    }
    jsbytecode *pc() const {
        return pc_;
    }
    uint32 stackDepth() const {
        return stackDepth_;
    }
    MResumePoint *caller() {
        return caller_;
    }
    void setCaller(MResumePoint *caller) {
        caller_ = caller;
    }
    uint32 frameCount() const {
        uint32 count = 1;
        for (MResumePoint *it = caller_; it; it = it->caller_)
            count++;
        return count;
    }
    Mode mode() const {
        return mode_;
    }
};





class FlattenedMResumePointIter
{
    Vector<MResumePoint *, 8, SystemAllocPolicy> resumePoints;
    MResumePoint *newest;
    size_t numOperands_;

    size_t resumePointIndex;
    size_t operand;

  public:
    explicit FlattenedMResumePointIter(MResumePoint *newest)
      : newest(newest), numOperands_(0), resumePointIndex(0), operand(0)
    {}

    bool init() {
        MResumePoint *it = newest;
        do {
            if (!resumePoints.append(it))
                return false;
            it = it->caller();
        } while (it);
        Reverse(resumePoints.begin(), resumePoints.end());
        return true;
    }

    MResumePoint **begin() {
        return resumePoints.begin();
    }
    MResumePoint **end() {
        return resumePoints.end();
    }

    size_t numOperands() const {
        return numOperands_;
    }
};

#undef INSTRUCTION_HEADER


#define OPCODE_CASTS(opcode)                                                \
    M##opcode *MDefinition::to##opcode()                                    \
    {                                                                       \
        JS_ASSERT(is##opcode());                                            \
        return static_cast<M##opcode *>(this);                              \
    }
MIR_OPCODE_LIST(OPCODE_CASTS)
#undef OPCODE_CASTS

MDefinition *MNode::toDefinition()
{
    JS_ASSERT(isDefinition());
    return (MDefinition *)this;
}

MResumePoint *MNode::toResumePoint()
{
    JS_ASSERT(isResumePoint());
    return (MResumePoint *)this;
}

MInstruction *MDefinition::toInstruction()
{
    JS_ASSERT(!isPhi());
    return (MInstruction *)this;
}

void MNode::initOperand(size_t index, MDefinition *ins)
{
    setOperand(index, ins);
    ins->addUse(this, index);
}

typedef Vector<MDefinition *, 8, IonAllocPolicy> MDefinitionVector;

} 
} 

#endif

