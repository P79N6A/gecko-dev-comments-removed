








































#ifndef jsion_mir_h__
#define jsion_mir_h__





#include "jscntxt.h"
#include "TypeOracle.h"
#include "TypePolicy.h"
#include "IonAllocPolicy.h"
#include "InlineList.h"
#include "MOpcodes.h"
#include "FixedArityList.h"

namespace js {
namespace ion {

static const inline
MIRType MIRTypeFromValue(const js::Value &vp)
{
    if (vp.isDouble())
        return MIRType_Double;
    switch (vp.extractNonDoubleType()) {
      case JSVAL_TYPE_INT32:
        return MIRType_Int32;
      case JSVAL_TYPE_UNDEFINED:
        return MIRType_Undefined;
      case JSVAL_TYPE_STRING:
        return MIRType_String;
      case JSVAL_TYPE_BOOLEAN:
        return MIRType_Boolean;
      case JSVAL_TYPE_NULL:
        return MIRType_Null;
      case JSVAL_TYPE_OBJECT:
        return MIRType_Object;
      default:
        JS_NOT_REACHED("unexpected jsval type");
        return MIRType_None;
    }
}

#define MIR_FLAG_LIST(_)                                                    \
    _(InWorklist)                                                           \
    _(EmittedAtUses)                                                        \
    _(LoopInvariant)                                                        \
    _(Idempotent) /* The instruction has no side-effects. */                \
    _(NeverHoisted) /* Don't hoist, even if loop invariant */

class MDefinition;
class MInstruction;
class MBasicBlock;
class MNode;
class MUse;
class MIRGraph;
class MSnapshot;


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
        Snapshot
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
    bool isSnapshot() const {
        return kind() == Snapshot;
    }
    MBasicBlock *block() const {
        JS_ASSERT(block_);
        return block_;
    }

    
    
    virtual TypePolicy *typePolicy() {
        return NULL;
    }

    
    
    MUseIterator replaceOperand(MUseIterator use, MDefinition *ins);
    void replaceOperand(size_t index, MDefinition *ins);

    inline MDefinition *toDefinition();
    inline MSnapshot *toSnapshot();

  protected:
    
    virtual void setOperand(size_t index, MDefinition *operand) = 0;

    
    inline void initOperand(size_t index, MDefinition *ins);
};


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
    uint32 usedTypes_;             
    uint32 flags_;                 

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

  public:
    MDefinition()
      : id_(0),
        valueNumber_(0),
        resultType_(MIRType_None),
        usedTypes_(0),
        flags_(0)
    { }

    virtual Opcode op() const = 0;
    void printName(FILE *fp);
    virtual void printOpcode(FILE *fp);

    virtual HashNumber valueHash() const;
    virtual bool congruentTo(MDefinition* const &ins) const;

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

    
    
    
    MIRType usedAsType() const;

    
    MUseIterator usesBegin() const {
        return uses_.begin();
    }

    
    MUseIterator usesEnd() const {
        return uses_.end();
    }

    
    MUseIterator removeUse(MUseIterator use);

    
    size_t useCount() const;

    virtual bool isControlInstruction() {
        return false;
    }

    void addUse(MNode *node, size_t index) {
        uses_.pushFront(MUse::New(node, index));
    }
    void replaceAllUsesWith(MDefinition *dom);

    
    
    void linkUse(MUse *use) {
        JS_ASSERT(use->node()->getOperand(use->index()) == this);
        uses_.pushFront(use);
    }

  public:   
    
    
    void addUsedTypes(uint32 types) {
        usedTypes_ = types | usedTypes();
    }
    void useAsType(MIRType type) {
        JS_ASSERT(type < MIRType_Value);
        addUsedTypes(1 << uint32(type));
    }
    uint32 usedTypes() const {
        return usedTypes_;
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
};




class MUseDefIterator
{
    MDefinition *def_;
    MUseIterator current_;

    MUseIterator search(MUseIterator start) {
        MUseIterator i(start);
        if (i != def_->usesEnd())
            i++;
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
    MSnapshot *snapshot_;

  public:
    MInstruction() : snapshot_(NULL)
    { }

    virtual bool accept(MInstructionVisitor *visitor) = 0;

    void setSnapshot(MSnapshot *snapshot) {
        JS_ASSERT(!snapshot_);
        snapshot_ = snapshot;
    }
    MSnapshot *snapshot() const {
        return snapshot_;
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


class MStart : public MAryInstruction<0>
{
  public:
    INSTRUCTION_HEADER(Start);
    static MStart *New() {
        return new MStart;
    }
};


class MConstant : public MAryInstruction<0>
{
    js::Value value_;

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

    void printOpcode(FILE *fp);

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;
};


class MParameter : public MAryInstruction<0>
{
    int32 index_;

  public:
    static const int32 CALLEE_SLOT = -2;
    static const int32 THIS_SLOT = -1;

    MParameter(int32 index)
      : index_(index)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Parameter);
    static MParameter *New(int32 index);

    int32 index() const {
        return index_;
    }
    void printOpcode(FILE *fp);

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;
};

class MControlInstruction : public MInstruction
{
  protected:
    MBasicBlock *successors[2];

  public:
    MControlInstruction()
      : successors()
    { }

    uint32 numSuccessors() const {
        if (successors[1])
            return 2;
        if (successors[0])
            return 1;
        return 0;
    }

    MBasicBlock *getSuccessor(uint32 i) const {
        JS_ASSERT(i < numSuccessors());
        return successors[i];
    }

    void replaceSuccessor(size_t i, MBasicBlock *split) {
        JS_ASSERT(successors[i]);
        successors[i] = split;
    }
};

template <size_t Arity>
class MAryControlInstruction : public MControlInstruction
{
    FixedArityList<MDefinition *, Arity> operands_;

  protected:
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


class MGoto : public MAryControlInstruction<0>
{
    MGoto(MBasicBlock *target) {
        successors[0] = target;
    }

  public:
    INSTRUCTION_HEADER(Goto);
    static MGoto *New(MBasicBlock *target);

    MBasicBlock *target() {
        return successors[0];
    }
};



class MTest : public MAryControlInstruction<1>
{
    MTest(MDefinition *ins, MBasicBlock *if_true, MBasicBlock *if_false)
    {
        successors[0] = if_true;
        successors[1] = if_false;
        initOperand(0, ins);
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
};


class MReturn
  : public MAryControlInstruction<1>,
    public BoxInputsPolicy
{
    MReturn(MDefinition *ins)
    {
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Return);
    static MReturn *New(MDefinition *ins);

    TypePolicy *typePolicy() {
        return this;
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
};


class MBox : public MUnaryInstruction
{
    MBox(MDefinition *ins)
      : MUnaryInstruction(ins)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Box);
    static MBox *New(MDefinition *ins)
    {
        
        JS_ASSERT(ins->type() != MIRType_Value);

        return new MBox(ins);
    }
};




class MUnbox : public MUnaryInstruction
{
    MUnbox(MDefinition *ins, MIRType type)
      : MUnaryInstruction(ins)
    {
        JS_ASSERT(ins->type() == MIRType_Value);
        setResultType(type);
    }

  public:
    INSTRUCTION_HEADER(Unbox);
    static MUnbox *New(MDefinition *ins, MIRType type)
    {
        return new MUnbox(ins, type);
    }
};



class MToDouble : public MUnaryInstruction
{
    MToDouble(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Double);
        setIdempotent();
    }

  public:
    INSTRUCTION_HEADER(ToDouble);
    static MToDouble *New(MDefinition *def)
    {
        return new MToDouble(def);
    }
};




class MToInt32 : public MUnaryInstruction
{
    MToInt32(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Int32);
        setIdempotent();
    }

  public:
    INSTRUCTION_HEADER(ToInt32);
    static MToInt32 *New(MDefinition *def)
    {
        return new MToInt32(def);
    }
};



class MTruncateToInt32 : public MUnaryInstruction
{
    MTruncateToInt32(MDefinition *def)
      : MUnaryInstruction(def)
    {
        setResultType(MIRType_Int32);
        setIdempotent();
    }

  public:
    INSTRUCTION_HEADER(TruncateToInt32);
    static MTruncateToInt32 *New(MDefinition *def)
    {
        return new MTruncateToInt32(def);
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
    }

  public:
    TypePolicy *typePolicy() {
        return this;
    }

    void infer(const TypeOracle::Binary &b);
};

class MBitAnd : public MBinaryBitwiseInstruction
{
    MBitAnd(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitAnd);
    static MBitAnd *New(MDefinition *left, MDefinition *right);
};

class MBitOr : public MBinaryBitwiseInstruction
{
    MBitOr(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitOr);
    static MBitOr *New(MDefinition *left, MDefinition *right);
};

class MBitXor : public MBinaryBitwiseInstruction
{
    MBitXor(MDefinition *left, MDefinition *right)
      : MBinaryBitwiseInstruction(left, right)
    { }

  public:
    INSTRUCTION_HEADER(BitXor);
    static MBitXor *New(MDefinition *left, MDefinition *right);
};

class MBinaryArithInstruction
  : public MBinaryInstruction,
    public BinaryArithPolicy
{
  public:
    MBinaryArithInstruction(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    { }

    TypePolicy *typePolicy() {
        return this;
    }
    void infer(const TypeOracle::Binary &b);
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

    bool congruentTo(MDefinition * const &ins) const;
};




class MSnapshot : public MNode
{
    friend class MBasicBlock;

    MDefinition **operands_;
    uint32 stackDepth_;
    jsbytecode *pc_;

    MSnapshot(MBasicBlock *block, jsbytecode *pc);
    bool init(MBasicBlock *state);
    void inherit(MBasicBlock *state);

  protected:
    void setOperand(size_t index, MDefinition *operand) {
        JS_ASSERT(index < stackDepth_);
        operands_[index] = operand;
    }

  public:
    static MSnapshot *New(MBasicBlock *block, jsbytecode *pc);

    MNode::Kind kind() const {
        return MNode::Snapshot;
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

MSnapshot *MNode::toSnapshot()
{
    JS_ASSERT(isSnapshot());
    return (MSnapshot *)this;
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

} 
} 

#endif 

