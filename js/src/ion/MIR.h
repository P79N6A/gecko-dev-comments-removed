








































#ifndef jsion_mir_h__
#define jsion_mir_h__





#include "jscntxt.h"
#include "TypeOracle.h"
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

class MDefinition;
class MInstruction;
class MBasicBlock;
class MUse;
class MIRGraph;
class MUseIterator;








class MNode : public TempObject
{
  public:
    enum Kind {
        Definition,
        Snapshot
    };

    virtual Kind kind() const = 0;

    
    virtual MDefinition *getOperand(size_t index) const = 0;
    virtual size_t numOperands() const = 0;

    bool isDefinition() const {
        return kind() == Definition;
    }
    bool isSnapshot() const {
        return kind() == Snapshot;
    }

    
    virtual MBasicBlock *block() const = 0;

    
    
    void replaceOperand(MUse *prev, MUse *use, MDefinition *ins);
    void replaceOperand(MUseIterator &use, MDefinition *ins);
    void replaceOperand(size_t index, MDefinition *ins);

    inline MDefinition *toDefinition();

  protected:
    
    virtual void setOperand(size_t index, MDefinition *operand) = 0;
};


class MUse : public TempObject
{
    friend class MDefinition;

    MUse *next_;            
    MNode *node_;           
    uint32 index_;          

    MUse(MUse *next, MNode *owner, uint32 index)
      : next_(next), node_(owner), index_(index)
    { }

    void setNext(MUse *next) {
        next_ = next;
    }

  public:
    static inline MUse *New(MUse *next, MNode *owner, uint32 index)
    {
        return new MUse(next, owner, index);
    }

    MNode *node() const {
        return node_;
    }
    uint32 index() const {
        return index_;
    }
    MUse *next() const {
        return next_;
    }
};

class MUseIterator
{
    MDefinition *def;
    MUse *use;
    MUse *prev_;

  public:
    inline MUseIterator(MDefinition *def);

    bool more() const {
        return !!use;
    }
    void next() {
        if (use) {
            prev_ = use;
            use = use->next();
        }
    }
    MUse * operator ->() const {
        return use;
    }
    MUse * operator *() const {
        return use;
    }
    MUse *prev() const {
        return prev_;
    }

    
    
    inline MUse *unlink();
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
    MBasicBlock *block_;    
    MUse *uses_;            
    uint32 id_;             
                            
    uint32 valueNumber_;    
    MIRType resultType_;    
    uint32 usedTypes_;      
    uint32 flags_;          

  private:
    static const uint32 IN_WORKLIST =  0x01;
    static const uint32 REWRITES_DEF = 0x02;
    static const uint32 EMIT_AT_USES = 0x04;
    static const uint32 LOOP_INVARIANT = 0x08;

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
      : block_(NULL),
        uses_(NULL),
        id_(0),
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
    bool inWorklist() const {
        return hasFlags(IN_WORKLIST);
    }
    void setInWorklist() {
        JS_ASSERT(!inWorklist());
        setFlags(IN_WORKLIST);
    }
    void setInWorklistUnchecked() {
        setFlags(IN_WORKLIST);
    }
    void setNotInWorklist() {
        JS_ASSERT(inWorklist());
        removeFlags(IN_WORKLIST);
    }

    void setLoopInvariant() {
        setFlags(LOOP_INVARIANT);
    }

    void setNotLoopInvariant() {
        removeFlags(LOOP_INVARIANT);
    }

    bool isLoopInvariant() {
        return hasFlags(LOOP_INVARIANT);
    }

    MBasicBlock *block() const {
        JS_ASSERT(block_);
        return block_;
    }
    MIRType type() const {
        return resultType_;
    }

    
    
    
    MIRType usedAsType() const;

    
    MUse *uses() const {
        return uses_;
    }

    
    MUse *removeUse(MUse *prev, MUse *use);

    
    size_t useCount() const;

    
    virtual MIRType requiredInputType(size_t index) const {
        return MIRType_None;
    }

    
    
    
    virtual bool adjustForInputs() {
        return false;
    }

    virtual bool isControlInstruction() {
        return false;
    }

    
    
    
    
    
    
    
    
    
    
    
    bool rewritesDef() const {
        return hasFlags(REWRITES_DEF);
    }
    void setRewritesDef() {
        setFlags(REWRITES_DEF);
    }
    virtual MDefinition *rewrittenDef() const {
        JS_NOT_REACHED("Opcodes which can rewrite defs must implement this.");
        return NULL;
    }
    bool emitAtUses() const {
        return hasFlags(EMIT_AT_USES);
    }
    void setEmitAtUses() {
        setFlags(EMIT_AT_USES);
    }

    void addUse(MDefinition *ins, size_t index) {
        uses_ = MUse::New(uses_, ins, index);
    }

    
    
    void addUse(MUse *use) {
        JS_ASSERT(use->node()->getOperand(use->index()) == this);
        use->setNext(uses_);
        uses_ = use;
    }

  public:   
    
    
    bool addUsedTypes(uint32 types) {
        uint32 newTypes = types | usedTypes();
        if (newTypes == usedTypes())
            return false;
        usedTypes_ = newTypes;
        return true;
    }
    bool useAsType(MIRType type) {
        JS_ASSERT(type < MIRType_Value);
        return addUsedTypes(1 << uint32(type));
    }
    uint32 usedTypes() const {
        return usedTypes_;
    }

    
    
    
    virtual bool specializeTo(MIRType type) {
        return false;
    }

    
    
    
    enum HoistWin {
        NO_WIN,
        POTENTIAL_WIN,
        BIG_WIN
    };
    virtual HoistWin estimateHoistWin() {
        return NO_WIN;
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
        return isPhi();
    }

    
    void initOperand(size_t index, MDefinition *ins) {
        setOperand(index, ins);
        ins->addUse(this, index);
    }

    void setResultType(MIRType type) {
        resultType_ = type;
    }
};



class MInstruction
  : public MDefinition,
    public InlineListNode<MInstruction>
{
  public:
    virtual bool accept(MInstructionVisitor *visitor) = 0;
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
    HoistWin estimateHoistWin() {
        return POTENTIAL_WIN;
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

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }

    MBasicBlock *ifTrue() const {
        return getSuccessor(0);
    }
    MBasicBlock *ifFalse() const {
        return getSuccessor(1);
    }
};


class MReturn : public MAryControlInstruction<1>
{
    MReturn(MDefinition *ins)
    {
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Return);
    static MReturn *New(MDefinition *ins);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
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
    
    
    
    
    MIRType specialization_;

  protected:
    MBinaryInstruction(MDefinition *left, MDefinition *right)
      : specialization_(MIRType_None)
    {
        initOperand(0, left);
        initOperand(1, right);
    }

    MIRType specialization() const {
        return specialization_;
    }

  public:
    void infer(const TypeOracle::Binary &b);
    bool adjustForInputs();
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

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
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

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
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

    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }

    MDefinition *rewrittenDef() const {
        return getOperand(0);
    }
};

class MBitAnd : public MBinaryInstruction
{
    MBitAnd(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BitAnd);
    static MBitAnd *New(MDefinition *left, MDefinition *right);

    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
    HoistWin estimateHoistWin() {
        return BIG_WIN;
    }
};

class MBitOr : public MBinaryInstruction
{
    MBitOr(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BitOr);
    static MBitOr *New(MDefinition *left, MDefinition *right);

    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
    HoistWin estimateHoistWin() {
        return BIG_WIN;
    }
};

class MBitXOr : public MBinaryInstruction
{
    MBitXOr(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BitXOr);
    static MBitXOr *New(MDefinition *left, MDefinition *right);

    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
    HoistWin estimateHoistWin() {
        return BIG_WIN;
    }
};

class MAdd : public MBinaryInstruction
{
    MAdd(MDefinition *left, MDefinition *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Add);
    static MAdd *New(MDefinition *left, MDefinition *right) {
        return new MAdd(left, right);
    }
    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
    HoistWin estimateHoistWin() {
        return BIG_WIN;
    }
};

class MPhi : public MDefinition
{
    js::Vector<MDefinition *, 2, IonAllocPolicy> inputs_;
    uint32 slot_;

    MPhi(uint32 slot)
      : slot_(slot)
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
    bool addInput(MDefinition *ins);
    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }
};




class MSnapshot : public MInstruction
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
    INSTRUCTION_HEADER(Snapshot);
    static MSnapshot *New(MBasicBlock *block, jsbytecode *pc);

    void printOpcode(FILE *fp);
    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
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

    HashNumber valueHash() const;
    bool congruentTo(MDefinition * const &ins) const;
};

#undef INSTRUCTION_HEADER

inline
MUseIterator::MUseIterator(MDefinition *def)
  : def(def),
    use(def->uses()),
    prev_(NULL)
{
}

inline MUse *
MUseIterator::unlink()
{
    MUse *old = use;
    use = def->removeUse(prev(), use);
    return old;
}


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

MInstruction *MDefinition::toInstruction()
{
    JS_ASSERT(!isPhi());
    return (MInstruction *)this;
}

} 
} 

#endif 

