








































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

class MInstruction;
class MBasicBlock;
class MUse;
class MIRGraph;


class MUse : public TempObject
{
    friend class MInstruction;

    MUse *next_;            
    MInstruction *ins_;     
    uint32 index_;          

    MUse(MUse *next, MInstruction *owner, uint32 index)
      : next_(next), ins_(owner), index_(index)
    { }

    void setNext(MUse *next) {
        next_ = next;
    }

  public:
    static inline MUse *New(MUse *next, MInstruction *owner, uint32 index)
    {
        return new MUse(next, owner, index);
    }

    MInstruction *ins() const {
        return ins_;
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
    MInstruction *def;
    MUse *use;
    MUse *prev_;

  public:
    inline MUseIterator(MInstruction *def);

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

class MOperand : public TempObject
{
    friend class MInstruction;

    MInstruction *ins_;

    MOperand(MInstruction *ins)
      : ins_(ins)
    { }

    void setInstruction(MInstruction *ins) {
        ins_ = ins;
    }

  public:
    static MOperand *New(MInstruction *ins)
    {
        return new MOperand(ins);
    }

    MInstruction *ins() const {
        return ins_;
    }
};



class MInstruction
  : public TempObject,
    public InlineListNode<MInstruction>
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
    MInstruction()
      : block_(NULL),
        uses_(NULL),
        id_(0),
        resultType_(MIRType_None),
        usedTypes_(0),
        flags_(0)
    { }

    virtual Opcode op() const = 0;
    void printName(FILE *fp);
    virtual void printOpcode(FILE *fp);

    uint32 id() const {
        JS_ASSERT(block_);
        return id_;
    }
    void setId(uint32 id) {
        id_ = id;
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

    
    virtual MOperand *getOperand(size_t index) const = 0;
    virtual size_t numOperands() const = 0;
    inline MInstruction *getInput(size_t index) const {
        return getOperand(index)->ins();
    }

    
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
    virtual MInstruction *rewrittenDef() const {
        JS_NOT_REACHED("Opcodes which can rewrite defs must implement this.");
        return NULL;
    }
    bool emitAtUses() const {
        return hasFlags(EMIT_AT_USES);
    }
    void setEmitAtUses() {
        setFlags(EMIT_AT_USES);
    }

    
    
    void replaceOperand(MUse *prev, MUse *use, MInstruction *ins);
    void replaceOperand(MUseIterator &use, MInstruction *ins);
    void replaceOperand(size_t index, MInstruction *ins);

    void addUse(MInstruction *ins, size_t index) {
        uses_ = MUse::New(uses_, ins, index);
    }

    
    
    void addUse(MUse *use) {
        JS_ASSERT(use->ins()->getInput(use->index()) == this);
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

  public:
    
#   define OPCODE_CASTS(opcode)                                             \
    bool is##opcode() const {                                               \
        return op() == Op_##opcode;                                         \
    }                                                                       \
    inline M##opcode *to##opcode();
    MIR_OPCODE_LIST(OPCODE_CASTS)
#   undef OPCODE_CASTS

    virtual bool accept(MInstructionVisitor *visitor) = 0;

  protected:
    
    virtual void setOperand(size_t index, MOperand *operand) = 0;
    void setOperand(size_t index, MInstruction *ins);

    
    void initOperand(size_t index, MInstruction *ins) {
        setOperand(index, MOperand::New(ins));
        ins->addUse(this, index);
    }

    void setResultType(MIRType type) {
        resultType_ = type;
    }
};

#define INSTRUCTION_HEADER(opcode)                                          \
    Opcode op() const {                                                     \
        return MInstruction::Op_##opcode;                                   \
    }                                                                       \
    bool accept(MInstructionVisitor *visitor) {                             \
        return visitor->visit##opcode(this);                                \
    }

template <size_t Arity>
class MAryInstruction : public MInstruction
{
  protected:
    FixedArityList<MOperand *, Arity> operands_;

    void setOperand(size_t index, MOperand *operand) {
        operands_[index] = operand;
    }

  public:
    MOperand *getOperand(size_t index) const {
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
    FixedArityList<MOperand *, Arity> operands_;

  protected:
    void setOperand(size_t index, MOperand *operand) {
        operands_[index] = operand;
    }

  public:
    MOperand *getOperand(size_t index) const {
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
    MTest(MInstruction *ins, MBasicBlock *if_true, MBasicBlock *if_false)
    {
        successors[0] = if_true;
        successors[1] = if_false;
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Test);
    static MTest *New(MInstruction *ins,
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
    MReturn(MInstruction *ins)
    {
        initOperand(0, ins);
    }

  public:
    INSTRUCTION_HEADER(Return);
    static MReturn *New(MInstruction *ins);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }
};

class MUnaryInstruction : public MAryInstruction<1>
{
  protected:
    MUnaryInstruction(MInstruction *ins)
    {
        initOperand(0, ins);
    }
};

class MBinaryInstruction : public MAryInstruction<2>
{
    
    
    
    
    MIRType specialization_;

  protected:
    MBinaryInstruction(MInstruction *left, MInstruction *right)
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
    MCopy(MInstruction *ins)
      : MUnaryInstruction(ins)
    {
        setResultType(ins->type());
    }

  public:
    INSTRUCTION_HEADER(Copy);
    static MCopy *New(MInstruction *ins);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
};


class MBox : public MUnaryInstruction
{
    MBox(MInstruction *ins)
      : MUnaryInstruction(ins)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Box);
    static MBox *New(MInstruction *ins)
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
    MUnbox(MInstruction *ins, MIRType type)
      : MUnaryInstruction(ins)
    {
        JS_ASSERT(ins->type() == MIRType_Value);
        setResultType(type);
    }

  public:
    INSTRUCTION_HEADER(Unbox);
    static MUnbox *New(MInstruction *ins, MIRType type)
    {
        return new MUnbox(ins, type);
    }

    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }

    MInstruction *rewrittenDef() const {
        return getInput(0);
    }
};

class MBitAnd : public MBinaryInstruction
{
    MBitAnd(MInstruction *left, MInstruction *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BitAnd);
    static MBitAnd *New(MInstruction *left, MInstruction *right);

    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
};

class MAdd : public MBinaryInstruction
{
    MAdd(MInstruction *left, MInstruction *right)
      : MBinaryInstruction(left, right)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Add);
    static MAdd *New(MInstruction *left, MInstruction *right) {
        return new MAdd(left, right);
    }

    MIRType requiredInputType(size_t index) const {
        return specialization();
    }
};

class MPhi : public MInstruction
{
    js::Vector<MOperand *, 2, IonAllocPolicy> inputs_;
    uint32 slot_;

    MPhi(uint32 slot)
      : slot_(slot)
    {
        setResultType(MIRType_Value);
    }

  protected:
    void setOperand(size_t index, MOperand *operand) {
        inputs_[index] = operand;
    }

  public:
    INSTRUCTION_HEADER(Phi);
    static MPhi *New(uint32 slot);

    MOperand *getOperand(size_t index) const {
        return inputs_[index];
    }
    size_t numOperands() const {
        return inputs_.length();
    }
    uint32 slot() const {
        return slot_;
    }
    bool addInput(MInstruction *ins);
    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }
};




class MSnapshot : public MInstruction
{
    friend class MBasicBlock;

    MOperand **operands_;
    uint32 stackDepth_;
    jsbytecode *pc_;

    MSnapshot(MBasicBlock *block, jsbytecode *pc);
    bool init(MBasicBlock *state);
    void inherit(MBasicBlock *state);

  protected:
    void setOperand(size_t index, MOperand *operand) {
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
    MOperand *getOperand(size_t index) const {
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

inline
MUseIterator::MUseIterator(MInstruction *def)
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
    M##opcode *MInstruction::to##opcode()                                   \
    {                                                                       \
        JS_ASSERT(is##opcode());                                            \
        return static_cast<M##opcode *>(this);                              \
    }
MIR_OPCODE_LIST(OPCODE_CASTS)
#undef OPCODE_CASTS

} 
} 

#endif 

