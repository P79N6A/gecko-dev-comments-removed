








































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
#include "FixedList.h"
#include "CompilerRoot.h"

namespace js {
namespace ion {
class ValueNumberData;
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
        TypedArrayElement = 1 << 3, 
        Last              = TypedArrayElement,
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
                                   
    ValueNumberData *valueNumber_; 
    MIRType resultType_;           
    uint32 flags_;                 
    union {
        MDefinition *dependency_;  
                                   
        uint32 virtualRegister_;   
    };

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
        valueNumber_(NULL),
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
    virtual void analyzeRangeForward();
    virtual void analyzeRangeBackward();
    virtual void analyzeTruncateBackward();

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

    uint32 valueNumber() const;
    void setValueNumber(uint32 vn);
    ValueNumberData *valueNumberData() {
        return valueNumber_;
    }
    void setValueNumberData(ValueNumberData *vn) {
        JS_ASSERT(valueNumber_ == NULL);
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

    
    virtual bool updateForFolding(MDefinition *ins) {
        return true;
    }

    
    
    void linkUse(MUse *use) {
        JS_ASSERT(use->node()->getOperand(use->index()) == this);
        uses_.pushFront(use);
    }

    void setVirtualRegister(uint32 vreg) {
        virtualRegister_ = vreg;
#ifdef DEBUG
        setLoweredUnchecked();
#endif
    }
    uint32 virtualRegister() const {
        JS_ASSERT(isLowered());
        return virtualRegister_;
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

    
    Vector<MBasicBlock*, 0, IonAllocPolicy> blocks_;

    MDefinition *operand_;
    int32 low_;
    int32 high_;

    MTableSwitch(MDefinition *ins, int32 low, int32 high)
      : successors_(),
        blocks_(),
        low_(low),
        high_(high)
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
        successors_[i] = successor;
    }

    MBasicBlock** blocks() {
        return &blocks_[0];
    }

    size_t numBlocks() const {
        return blocks_.length();
    }

    int32 low() const {
        return low_;
    }

    int32 high() const {
        return high_;
    }

    MBasicBlock *getDefault() const {
        return getSuccessor(0);
    }

    MBasicBlock *getCase(size_t i) const {
        return getSuccessor(i+1);
    }

    size_t numCases() const {
        return high() - low() + 1;
    }

    void addDefault(MBasicBlock *block) {
        JS_ASSERT(successors_.length() == 0);
        successors_.append(block);
    }

    void addCase(MBasicBlock *block) {
        JS_ASSERT(successors_.length() < (size_t)(high_ - low_ + 2));
        JS_ASSERT(successors_.length() != 0);
        successors_.append(block);
    }

    MBasicBlock *getBlock(size_t i) const {
        JS_ASSERT(i < numBlocks());
        return blocks_[i];
    }

    void addBlock(MBasicBlock *block) {
        blocks_.append(block);
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



class MTest
  : public MAryControlInstruction<1, 2>,
    public TestPolicy
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
    TypePolicy *typePolicy() {
        return this;
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
    MDefinition *foldsTo(bool useValueNumbers);
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
  public:
    enum AllocatingBehaviour {
        NewArray_Allocating,
        NewArray_Unallocating
    };

  private:
    
    uint32 count_;
    
    HeapPtr<types::TypeObject> type_;
    
    AllocatingBehaviour allocating_;

  public:
    INSTRUCTION_HEADER(NewArray);

    MNewArray(uint32 count, types::TypeObject *type, AllocatingBehaviour allocating)
        : count_(count), type_(type), allocating_(allocating)
    {
        setResultType(MIRType_Object);
    }

    uint32 count() const {
        return count_;
    }

    types::TypeObject *type() const {
        return type_;
    }
    
    bool isAllocating() const {
        return allocating_ == NewArray_Allocating;
    }
};

class MNewObject : public MNullaryInstruction
{
    CompilerRootObject baseObj_;
    HeapPtr<types::TypeObject> type_;

    MNewObject(HandleObject baseObj, types::TypeObject *type)
      : baseObj_(baseObj),
        type_(type)
    {
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(NewObject);

    static MNewObject *New(HandleObject baseObj, types::TypeObject *type) {
        return new MNewObject(baseObj, type);
    }

    JSObject *baseObj() const {
        return baseObj_;
    }
    types::TypeObject *type() const {
        return type_;
    }
};


class MInitProp
  : public MAryInstruction<2>,
    public MixPolicy<ObjectPolicy<0>, BoxPolicy<1> >
{
  public:
    CompilerRootPropertyName name_;

  protected:
    MInitProp(MDefinition *obj, HandlePropertyName name, MDefinition *value)
      : name_(name)
    {
        initOperand(0, obj);
        initOperand(1, value);
        setResultType(MIRType_None);
    }

  public:
    INSTRUCTION_HEADER(InitProp);

    static MInitProp *New(MDefinition *obj, HandlePropertyName name, MDefinition *value) {
        return new MInitProp(obj, name, value);
    }

    MDefinition *getObject() const {
        return getOperand(0);
    }
    MDefinition *getValue() const {
        return getOperand(1);
    }

    PropertyName *propertyName() const {
        return name_;
    }
    TypePolicy *typePolicy() {
        return this;
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
    
    CompilerRootFunction target_;
    
    uint32 bytecodeArgc_;

    MCall(JSFunction *target, uint32 bytecodeArgc, bool construct)
      : construct_(construct),
        target_(target),
        bytecodeArgc_(bytecodeArgc)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Call);
    static MCall *New(JSFunction *target, size_t argc, size_t bytecodeArgc, bool construct);

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

    
    JSFunction *getSingleTarget() const {
        return target_;
    }

    bool isConstructing() const {
        return construct_;
    }

    
    uint32 argc() const {
        return numOperands() - NumNonArgumentOperands;
    }

    
    uint32 bytecodeArgc() const {
        return bytecodeArgc_;
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
    void swapOperands() {
        MDefinition *temp = getOperand(0);
        replaceOperand(0, getOperand(1));
        replaceOperand(1, temp);
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

  protected:
    bool congruentTo(MDefinition *const &ins) const {
        if (!MBinaryInstruction::congruentTo(ins))
            return false;
        return jsop() == ins->toCompare()->jsop();
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
        TypeBarrier,    
        TypeGuard       
    };

  private:
    Mode mode_;

    MUnbox(MDefinition *ins, MIRType type, Mode mode)
      : MUnaryInstruction(ins),
        mode_(mode)
    {
        JS_ASSERT(ins->type() == MIRType_Value);
        JS_ASSERT(type == MIRType_Boolean ||
                  type == MIRType_Int32   ||
                  type == MIRType_Double  || 
                  type == MIRType_String  ||
                  type == MIRType_Object);

        setResultType(type);
        setMovable();

        if (mode_ == TypeBarrier || mode_ == TypeGuard)
            setGuard();
        if (mode_ == TypeGuard)
            mode_ = Fallible;
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




class MCreateThis
  : public MAryInstruction<2>,
    public MixPolicy<ObjectPolicy<0>, ObjectPolicy<1> >
{
    
    CompilerRootObject templateObject_;

    MCreateThis(MDefinition *callee, MDefinition *prototype, JSObject *templateObject)
      : templateObject_(templateObject)
    {
        initOperand(0, callee);
        initOperand(1, prototype);
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(CreateThis);
    static MCreateThis *New(MDefinition *callee, MDefinition *prototype, JSObject *templateObject)
    {
        return new MCreateThis(callee, prototype, templateObject);
    }

    MDefinition *getCallee() const {
        return getOperand(0);
    }
    MDefinition *getPrototype() const {
        return getOperand(1);
    }
    bool hasTemplateObject() const {
        return !!templateObject_;
    }
    JSObject *getTemplateObject() const {
        JS_ASSERT(hasTemplateObject());
        return templateObject_;
    }

    
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
    TypePolicy *typePolicy() {
        return this;
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

    
    void printOpcode(FILE *fp);
};



class MToDouble
  : public MUnaryInstruction
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
    bool canBeNegativeZero_;

    MToInt32(MDefinition *def)
      : MUnaryInstruction(def),
        canBeNegativeZero_(true)
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

    
    void analyzeRangeBackward();

    bool canBeNegativeZero() {
        return canBeNegativeZero_;
    }

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

    MDefinition *foldsTo(bool useValueNumbers);

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
    virtual void infer(const TypeOracle::Binary &b);

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

    MDefinition *foldIfNegOne(size_t operand) {
        return getOperand(operand);
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

class MShiftInstruction
  : public MBinaryBitwiseInstruction
{
  protected:
    MShiftInstruction(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    MDefinition *foldIfEqual() {
        return this;
    }
    virtual void infer(const TypeOracle::Binary &b);
};

class MLsh : public MShiftInstruction
{
    MLsh(MDefinition *left, MDefinition *right)
      : MShiftInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(Lsh);
    static MLsh *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        
        
        return getOperand(0);
    }
};

class MRsh : public MShiftInstruction
{
    MRsh(MDefinition *left, MDefinition *right)
      : MShiftInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(Rsh);
    static MRsh *New(MDefinition *left, MDefinition *right);

    MDefinition *foldIfZero(size_t operand) {
        
        
        return getOperand(0);
    }
};

class MUrsh : public MShiftInstruction
{
    bool canOverflow_;

    MUrsh(MDefinition *left, MDefinition *right)
      : MShiftInstruction(left, right),
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

    void infer(const TypeOracle::Binary &b);

    bool canOverflow() {
        
        MDefinition *lhs = getOperand(0);
        MDefinition *rhs = getOperand(1);

        if (lhs->isConstant()) {
            Value lhsv = lhs->toConstant()->value();
            if (lhsv.isInt32() && lhsv.toInt32() >= 0)
                return false;
        }

        if (rhs->isConstant()) {
            Value rhsv = rhs->toConstant()->value();
            if (rhsv.isInt32() && rhsv.toInt32() % 32 != 0)
                return false;
        }

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

    void infer(JSContext *cx, const TypeOracle::BinaryTypes &b);

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
        specialization_ = type;
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

class MSqrt
  : public MUnaryInstruction,
    public ArithPolicy
{
    MSqrt(MDefinition *num)
      : MUnaryInstruction(num)
    {
        specialization_ = MIRType_Double;
        setResultType(MIRType_Double);
    }

  public:
    INSTRUCTION_HEADER(Sqrt);
    static MSqrt *New(MDefinition *num) {
        return new MSqrt(num);
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
    bool implicitTruncate_;

    MAdd(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right),
        implicitTruncate_(false)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Add);
    static MAdd *New(MDefinition *left, MDefinition *right) {
        return new MAdd(left, right);
    }
    void analyzeTruncateBackward();

    bool isTruncated() const {
        return implicitTruncate_;
    }
    void setTruncated(bool val) {
        implicitTruncate_ = val;
    }
    bool updateForReplacement(MDefinition *ins);
    double getIdentity() {
        return 0;
    }
};

class MSub : public MBinaryArithInstruction
{
    bool implicitTruncate_;
    MSub(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right),
        implicitTruncate_(false)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Sub);
    static MSub *New(MDefinition *left, MDefinition *right) {
        return new MSub(left, right);
    }

    void analyzeTruncateBackward();
    bool isTruncated() const {
        return implicitTruncate_;
    }
    void setTruncated(bool val) {
        implicitTruncate_ = val;
    }
    bool updateForReplacement(MDefinition *ins);

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
    void analyzeRangeForward();
    void analyzeRangeBackward();

    double getIdentity() {
        return 1;
    }

    bool canOverflow() {
        return canOverflow_;
    }

    bool canBeNegativeZero() {
        return canBeNegativeZero_;
    }
    bool updateForReplacement(MDefinition *ins);

    bool fallible() {
        return canBeNegativeZero_ || canOverflow_;
    }
};

class MDiv : public MBinaryArithInstruction
{
    bool canBeNegativeZero_;
    bool canBeNegativeOverflow_;
    bool canBeDivideByZero_;
    bool implicitTruncate_;

    MDiv(MDefinition *left, MDefinition *right)
      : MBinaryArithInstruction(left, right),
        canBeNegativeZero_(true),
        canBeNegativeOverflow_(true),
        canBeDivideByZero_(true),
        implicitTruncate_(false)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Div);
    static MDiv *New(MDefinition *left, MDefinition *right) {
        return new MDiv(left, right);
    }

    MDefinition *foldsTo(bool useValueNumbers);
    void analyzeRangeForward();
    void analyzeRangeBackward();
    void analyzeTruncateBackward();

    double getIdentity() {
        JS_NOT_REACHED("not used");
        return 1;
    }

    bool isTruncated() const {
        return implicitTruncate_;
    }
    void setTruncated(bool val) {
        implicitTruncate_ = val;
    }

    bool canBeNegativeZero() {
        return canBeNegativeZero_;
    }

    bool canBeNegativeOverflow() {
        return canBeNegativeOverflow_;
    }

    bool canBeDivideByZero() {
        return canBeDivideByZero_;
    }
    bool updateForReplacement(MDefinition *ins);

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

class MCharCodeAt
  : public MBinaryInstruction,
    public MixPolicy<StringPolicy, IntPolicy<1> >
{
  public:
    MCharCodeAt(MDefinition *str, MDefinition *index)
        : MBinaryInstruction(str, index)
    {
        setMovable();
        setResultType(MIRType_Int32);
    }

    INSTRUCTION_HEADER(CharCodeAt);

    TypePolicy *typePolicy() {
        return this;
    }

    virtual AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::ObjectFields);
    }
};

class MFromCharCode
  : public MUnaryInstruction,
    public IntPolicy<0>
{
  public:
    MFromCharCode(MDefinition *code)
      : MUnaryInstruction(code)
    {
        setMovable();
        setResultType(MIRType_String);
    }

    INSTRUCTION_HEADER(FromCharCode);

    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};

class MPhi : public MDefinition, public InlineForwardListNode<MPhi>
{
    js::Vector<MDefinition *, 2, IonAllocPolicy> inputs_;
    uint32 slot_;
    bool triedToSpecialize_;
    bool hasBytecodeUses_;
    bool isIterator_;

    MPhi(uint32 slot)
      : slot_(slot),
        triedToSpecialize_(false),
        hasBytecodeUses_(false),
        isIterator_(false)
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

    bool hasBytecodeUses() const {
        return hasBytecodeUses_;
    }
    void setHasBytecodeUses() {
        hasBytecodeUses_ = true;
    }
    bool isIterator() const {
        return isIterator_;
    }
    void setIterator() {
        isIterator_ = true;
    }

    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};



class MOsrValue : public MUnaryInstruction
{
  private:
    ptrdiff_t frameOffset_;

    MOsrValue(MOsrEntry *entry, ptrdiff_t frameOffset)
      : MUnaryInstruction(entry),
        frameOffset_(frameOffset)
    {
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



class MOsrScopeChain : public MUnaryInstruction
{
  private:
    MOsrScopeChain(MOsrEntry *entry)
      : MUnaryInstruction(entry)
    {
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


class MInterruptCheck : public MNullaryInstruction
{
    MInterruptCheck() {
        setGuard();
    }

  public:
    INSTRUCTION_HEADER(InterruptCheck);

    static MInterruptCheck *New() {
        return new MInterruptCheck();
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
};


class MDefVar : public MUnaryInstruction
{
  PropertyName *name_; 
  unsigned attrs_; 

  private:
    MDefVar(PropertyName *name, unsigned attrs, MDefinition *scopeChain)
      : MUnaryInstruction(scopeChain),
        name_(name),
        attrs_(attrs)
    {
    }

  public:
    INSTRUCTION_HEADER(DefVar);

    static MDefVar *New(PropertyName *name, unsigned attrs, MDefinition *scopeChain) {
        return new MDefVar(name, attrs, scopeChain);
    }

    PropertyName *name() const {
        return name_;
    }
    unsigned attrs() const {
        return attrs_;
    }
    MDefinition *scopeChain() const {
        return getOperand(0);
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


class MSetInitializedLength
  : public MAryInstruction<2>
{
    MSetInitializedLength(MDefinition *elements, MDefinition *index)
    {
        initOperand(0, elements);
        initOperand(1, index);
    }

  public:
    INSTRUCTION_HEADER(SetInitializedLength);

    static MSetInitializedLength *New(MDefinition *elements, MDefinition *index) {
        return new MSetInitializedLength(elements, index);
    }

    MDefinition *elements() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::ObjectFields);
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


class MTypedArrayLength
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MTypedArrayLength(MDefinition *obj)
      : MUnaryInstruction(obj)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(TypedArrayLength);

    static MTypedArrayLength *New(MDefinition *obj) {
        return new MTypedArrayLength(obj);
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
        
        
        return AliasSet::None();
    }
};


class MTypedArrayElements
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    MTypedArrayElements(MDefinition *object)
      : MUnaryInstruction(object)
    {
        setResultType(MIRType_Elements);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(TypedArrayElements);

    static MTypedArrayElements *New(MDefinition *object) {
        return new MTypedArrayElements(object);
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


class MNot
  : public MUnaryInstruction,
    public TestPolicy
{
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

    virtual AliasSet getAliasSet() const {
        return AliasSet::None();
    }
    TypePolicy *typePolicy() {
        return this;
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
        elementType_(MIRType_Value)
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


class MArrayPopShift
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
  public:
    enum Mode {
        Pop,
        Shift
    };

  private:
    Mode mode_;
    bool needsHoleCheck_;
    bool maybeUndefined_;

    MArrayPopShift(MDefinition *object, Mode mode, bool needsHoleCheck, bool maybeUndefined)
      : MUnaryInstruction(object), mode_(mode), needsHoleCheck_(needsHoleCheck),
        maybeUndefined_(maybeUndefined)
    { }

  public:
    INSTRUCTION_HEADER(ArrayPopShift);

    static MArrayPopShift *New(MDefinition *object, Mode mode, bool needsHoleCheck,
                               bool maybeUndefined) {
        return new MArrayPopShift(object, mode, needsHoleCheck, maybeUndefined);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    bool needsHoleCheck() const {
        return needsHoleCheck_;
    }
    bool maybeUndefined() const {
        return maybeUndefined_;
    }
    bool mode() const {
        return mode_;
    }
    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Element | AliasSet::ObjectFields);
    }
};


class MArrayPush
  : public MBinaryInstruction,
    public SingleObjectPolicy
{
    MArrayPush(MDefinition *object, MDefinition *value)
      : MBinaryInstruction(object, value)
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(ArrayPush);

    static MArrayPush *New(MDefinition *object, MDefinition *value) {
        return new MArrayPush(object, value);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    MDefinition *value() const {
        return getOperand(1);
    }
    TypePolicy *typePolicy() {
        return this;
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::Element | AliasSet::ObjectFields);
    }
};

class MLoadTypedArrayElement
  : public MBinaryInstruction
{
    int arrayType_;

    MLoadTypedArrayElement(MDefinition *elements, MDefinition *index, int arrayType)
      : MBinaryInstruction(elements, index), arrayType_(arrayType)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
        JS_ASSERT(arrayType >= 0 && arrayType < TypedArray::TYPE_MAX);
    }

  public:
    INSTRUCTION_HEADER(LoadTypedArrayElement);

    static MLoadTypedArrayElement *New(MDefinition *elements, MDefinition *index, int arrayType) {
        return new MLoadTypedArrayElement(elements, index, arrayType);
    }

    int arrayType() const {
        return arrayType_;
    }
    bool fallible() const {
        
        return arrayType_ == TypedArray::TYPE_UINT32 && type() == MIRType_Int32;
    }
    MDefinition *elements() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Load(AliasSet::TypedArrayElement);
    }
};



class MLoadTypedArrayElementHole
  : public MBinaryInstruction,
    public SingleObjectPolicy
{
    int arrayType_;
    bool allowDouble_;

    MLoadTypedArrayElementHole(MDefinition *object, MDefinition *index, int arrayType, bool allowDouble)
      : MBinaryInstruction(object, index), arrayType_(arrayType), allowDouble_(allowDouble)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(index->type() == MIRType_Int32);
        JS_ASSERT(arrayType >= 0 && arrayType < TypedArray::TYPE_MAX);
    }

  public:
    INSTRUCTION_HEADER(LoadTypedArrayElementHole);

    static MLoadTypedArrayElementHole *New(MDefinition *object, MDefinition *index, int arrayType, bool allowDouble) {
        return new MLoadTypedArrayElementHole(object, index, arrayType, allowDouble);
    }

    int arrayType() const {
        return arrayType_;
    }
    bool allowDouble() const {
        return allowDouble_;
    }
    bool fallible() const {
        return arrayType_ == TypedArray::TYPE_UINT32 && !allowDouble_;
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
    AliasSet getAliasSet() const {
        
        
        return AliasSet::Store(AliasSet::Any);
    }
};

class MStoreTypedArrayElement
  : public MTernaryInstruction,
    public StoreTypedArrayPolicy
{
    int arrayType_;

    MStoreTypedArrayElement(MDefinition *elements, MDefinition *index, MDefinition *value,
                            int arrayType)
      : MTernaryInstruction(elements, index, value), arrayType_(arrayType)
    {
        setResultType(MIRType_Value);
        setMovable();
        JS_ASSERT(elements->type() == MIRType_Elements);
        JS_ASSERT(index->type() == MIRType_Int32);
        JS_ASSERT(arrayType >= 0 && arrayType < TypedArray::TYPE_MAX);
    }

  public:
    INSTRUCTION_HEADER(StoreTypedArrayElement);

    static MStoreTypedArrayElement *New(MDefinition *elements, MDefinition *index, MDefinition *value,
                                        int arrayType) {
        return new MStoreTypedArrayElement(elements, index, value, arrayType);
    }

    int arrayType() const {
        return arrayType_;
    }
    bool isByteArray() const {
        return (arrayType_ == TypedArray::TYPE_INT8 ||
                arrayType_ == TypedArray::TYPE_UINT8 ||
                arrayType_ == TypedArray::TYPE_UINT8_CLAMPED);
    }
    bool isFloatArray() const {
        return (arrayType_ == TypedArray::TYPE_FLOAT32 ||
                arrayType_ == TypedArray::TYPE_FLOAT64);
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
    MDefinition *value() const {
        return getOperand(2);
    }
    AliasSet getAliasSet() const {
        return AliasSet::Store(AliasSet::TypedArrayElement);
    }
};


class MClampToUint8
  : public MUnaryInstruction,
    public ClampPolicy
{
    MClampToUint8(MDefinition *input)
      : MUnaryInstruction(input)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

  public:
    INSTRUCTION_HEADER(ClampToUint8);

    static MClampToUint8 *New(MDefinition *input) {
        return new MClampToUint8(input);
    }

    MDefinition *foldsTo(bool useValueNumbers);

    MDefinition *input() const {
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
    bool needsBarrier_;
    size_t slot_;

    MStoreFixedSlot(MDefinition *obj, MDefinition *rval, size_t slot)
      : MBinaryInstruction(obj, rval), needsBarrier_(false), slot_(slot)
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
    bool needsBarrier() const {
        return needsBarrier_;
    }
    void setNeedsBarrier() {
        needsBarrier_ = true;
    }
};

class MGetPropertyCache
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    CompilerRootPropertyName name_;

    MGetPropertyCache(MDefinition *obj, HandlePropertyName name)
      : MUnaryInstruction(obj),
        name_(name)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(GetPropertyCache);

    static MGetPropertyCache *New(MDefinition *obj, HandlePropertyName name) {
        return new MGetPropertyCache(obj, name);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    PropertyName *name() const {
        return name_;
    }
    TypePolicy *typePolicy() { return this; }
};

class MGetElementCache
  : public MBinaryInstruction,
    public MixPolicy<ObjectPolicy<0>, BoxPolicy<1> >
{
    
    bool monitoredResult_;

    MGetElementCache(MDefinition *obj, MDefinition *value, bool monitoredResult)
      : MBinaryInstruction(obj, value), monitoredResult_(monitoredResult)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(GetElementCache);

    static MGetElementCache *New(MDefinition *obj, MDefinition *value, bool monitoredResult) {
        return new MGetElementCache(obj, value, monitoredResult);
    }

    MDefinition *object() const {
        return getOperand(0);
    }
    MDefinition *index() const {
        return getOperand(1);
    }
    bool monitoredResult() const {
        return monitoredResult_;
    }
    TypePolicy *typePolicy() {
        return this;
    }
};

class MBindNameCache
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
    PropertyName *name_;
    JSScript *script_;
    jsbytecode *pc_;

    MBindNameCache(MDefinition *scopeChain, PropertyName *name, JSScript *script, jsbytecode *pc)
      : MUnaryInstruction(scopeChain), name_(name), script_(script), pc_(pc)
    {
        setResultType(MIRType_Object);
    }

  public:
    INSTRUCTION_HEADER(BindNameCache);

    static MBindNameCache *New(MDefinition *scopeChain, PropertyName *name, JSScript *script,
                               jsbytecode *pc) {
        return new MBindNameCache(scopeChain, name, script, pc);
    }

    TypePolicy *typePolicy() {
        return this;
    }
    MDefinition *scopeChain() const {
        return getOperand(0);
    }
    PropertyName *name() const {
        return name_;
    }
    JSScript *script() const {
        return script_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
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
          slotType_(MIRType_Value),
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



class MCallGetNameInstruction
  : public MUnaryInstruction,
    public SingleObjectPolicy
{
  public:
    enum AccessKind {
        NAMETYPEOF,
        NAME
    };

  private:
    CompilerRootPropertyName name_;
    AccessKind kind_;

  protected:
    MCallGetNameInstruction(MDefinition *obj, HandlePropertyName name, AccessKind kind)
      : MUnaryInstruction(obj),
        name_(name),
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
    PropertyName *name() const {
        return name_;
    }
    AccessKind accessKind() const {
        return kind_;
    }
};

class MSetPropertyInstruction : public MBinaryInstruction
{
    CompilerRootPropertyName name_;
    bool strict_;
    bool needsBarrier_;

  protected:
    MSetPropertyInstruction(MDefinition *obj, MDefinition *value, HandlePropertyName name,
                            bool strict)
      : MBinaryInstruction(obj, value),
        name_(name), strict_(strict), needsBarrier_(true)
    {}

  public:
    MDefinition *obj() const {
        return getOperand(0);
    }
    MDefinition *value() const {
        return getOperand(1);
    }
    PropertyName *name() const {
        return name_;
    }
    bool strict() const {
        return strict_;
    }
    bool needsBarrier() const {
        return needsBarrier_;
    }
    void setNeedsBarrier() {
        needsBarrier_ = true;
    }
};

class MDeleteProperty
  : public MUnaryInstruction,
    public BoxInputsPolicy
{
    JSAtom *atom_;
    bool needsBarrier_;

  protected:
    MDeleteProperty(MDefinition *val, JSAtom *atom)
      : MUnaryInstruction(val),
        atom_(atom)
    {
        setResultType(MIRType_Boolean);
    }

  public:
    INSTRUCTION_HEADER(DeleteProperty);

    static MDeleteProperty *New(MDefinition *obj, JSAtom *atom) {
        return new MDeleteProperty(obj, atom);
    }
    MDefinition *value() const {
        return getOperand(0);
    }
    JSAtom *atom() const {
        return atom_;
    }
    virtual TypePolicy *typePolicy() {
        return this;
    }
};



class MCallSetProperty
  : public MSetPropertyInstruction,
    public CallSetElementPolicy
{
    MCallSetProperty(MDefinition *obj, MDefinition *value, HandlePropertyName name, bool strict)
      : MSetPropertyInstruction(obj, value, name, strict)
    {
    }

  public:
    INSTRUCTION_HEADER(CallSetProperty);

    static MCallSetProperty *New(MDefinition *obj, MDefinition *value, HandlePropertyName name, bool strict) {
        return new MCallSetProperty(obj, value, name, strict);
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

class MSetPropertyCache
  : public MSetPropertyInstruction,
    public SingleObjectPolicy
{
    MSetPropertyCache(MDefinition *obj, MDefinition *value, HandlePropertyName name, bool strict)
      : MSetPropertyInstruction(obj, value, name, strict)
    {
    }

  public:
    INSTRUCTION_HEADER(SetPropertyCache);

    static MSetPropertyCache *New(MDefinition *obj, MDefinition *value, HandlePropertyName name, bool strict) {
        return new MSetPropertyCache(obj, value, name, strict);
    }

    TypePolicy *typePolicy() {
        return this;
    }
};

class MCallGetProperty
  : public MUnaryInstruction,
    public BoxInputsPolicy
{
    CompilerRootPropertyName name_;
    bool markEffectful_;

    MCallGetProperty(MDefinition *value, HandlePropertyName name)
      : MUnaryInstruction(value), name_(name),
        markEffectful_(true)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(CallGetProperty);

    static MCallGetProperty *New(MDefinition *value, HandlePropertyName name) {
        return new MCallGetProperty(value, name);
    }
    MDefinition *value() const {
        return getOperand(0);
    }
    PropertyName *name() const {
        return name_;
    }
    TypePolicy *typePolicy() {
        return this;
    }

    
    
    
    void markUneffectful() {
        markEffectful_ = false;
    }
    AliasSet getAliasSet() const {
        if (markEffectful_)
            return AliasSet::Store(AliasSet::Any);
        return AliasSet::None();
    }
};

class MCallGetName : public MCallGetNameInstruction
{
    MCallGetName(MDefinition *obj, HandlePropertyName name)
        : MCallGetNameInstruction(obj, name, MCallGetNameInstruction::NAME)
    {}

  public:
    INSTRUCTION_HEADER(CallGetName);

    static MCallGetName *New(MDefinition *obj, HandlePropertyName name) {
        return new MCallGetName(obj, name);
    }
};

class MCallGetNameTypeOf : public MCallGetNameInstruction
{
    MCallGetNameTypeOf(MDefinition *obj, HandlePropertyName name)
        : MCallGetNameInstruction(obj, name, MCallGetNameInstruction::NAMETYPEOF)
    {}

  public:
    INSTRUCTION_HEADER(CallGetNameTypeOf);

    static MCallGetNameTypeOf *New(MDefinition *obj, HandlePropertyName name) {
        return new MCallGetNameTypeOf(obj, name);
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
    MStringLength(MDefinition *string)
      : MUnaryInstruction(string)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }
  public:
    INSTRUCTION_HEADER(StringLength);

    static MStringLength *New(MDefinition *string) {
        return new MStringLength(string);
    }

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


class MFloor
  : public MUnaryInstruction,
    public DoublePolicy<0>
{
  public:
    MFloor(MDefinition *num)
      : MUnaryInstruction(num)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

    INSTRUCTION_HEADER(Floor);

    MDefinition *num() const {
        return getOperand(0);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
    TypePolicy *typePolicy() {
        return this;
    }
};


class MRound
  : public MUnaryInstruction,
    public DoublePolicy<0>
{
  public:
    MRound(MDefinition *num)
      : MUnaryInstruction(num)
    {
        setResultType(MIRType_Int32);
        setMovable();
    }

    INSTRUCTION_HEADER(Round);

    MDefinition *num() const {
        return getOperand(0);
    }
    AliasSet getAliasSet() const {
        return AliasSet::None();
    }
    TypePolicy *typePolicy() {
        return this;
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




class MMonitorTypes : public MUnaryInstruction
{
    types::TypeSet *typeSet_;

    MMonitorTypes(MDefinition *def, types::TypeSet *types)
      : MUnaryInstruction(def),
        typeSet_(types)
    {
        setResultType(MIRType_Value);
        setGuard();
        JS_ASSERT(!types->unknown());
    }

  public:
    INSTRUCTION_HEADER(MonitorTypes);

    static MMonitorTypes *New(MDefinition *def, types::TypeSet *types) {
        return new MMonitorTypes(def, types);
    }
    MDefinition *input() const {
        return getOperand(0);
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

