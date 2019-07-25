








































#ifndef jsion_mir_h__
#define jsion_mir_h__

#include "jscntxt.h"
#include "TempAllocPolicy.h"

namespace js {
namespace ion {

#define MIR_OPCODE_LIST(_)                                                  \
    _(Constant)                                                             \
    _(Parameter)                                                            \
    _(Goto)                                                                 \
    _(Test)                                                                 \
    _(Phi)                                                                  \
    _(BitAnd)                                                               \
    _(Return)                                                               \
    _(Copy)

enum MIRType
{
    MIRType_Double,
    MIRType_Int32,
    MIRType_Undefined,
    MIRType_String,
    MIRType_Boolean,
    MIRType_Magic,
    MIRType_Null,
    MIRType_Object,
    MIRType_Function,
    MIRType_Box
};

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
        return vp.toObject().isFunction()
               ? MIRType_Function
               : MIRType_Object;
      default:
        JS_NOT_REACHED("unexpected jsval type");
        return MIRType_Magic;
    }
}


#define FORWARD_DECLARE(op) class M##op;
 MIR_OPCODE_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class MInstruction;
class MBasicBlock;
class MUse;
class MIRGraph;

class MIRGenerator
{
  public:
    MIRGenerator(JSContext *cx, TempAllocator &temp, JSScript *script, JSFunction *fun,
                 MIRGraph &graph);

    TempAllocator &temp() {
        return temp_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    uint32 nslots() const {
        return nslots_;
    }
    uint32 nargs() const {
        return fun()->nargs;
    }
    uint32 nlocals() const {
        return script->nfixed;
    }
    uint32 calleeSlot() const {
        JS_ASSERT(fun());
        return 0;
    }
    uint32 thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32 firstArgSlot() const {
        JS_ASSERT(fun());
        return 2;
    }
    uint32 argSlot(uint32 i) const {
        return firstArgSlot() + i;
    }
    uint32 firstLocalSlot() const {
        return (fun() ? fun()->nargs + 2 : 0);
    }
    uint32 localSlot(uint32 i) const {
        return firstLocalSlot() + i;
    }
    uint32 firstStackSlot() const {
        return firstLocalSlot() + nlocals();
    }
    uint32 stackSlot(uint32 i) const {
        return firstStackSlot() + i;
    }
    MIRGraph &graph() {
        return graph_;
    }

    template <typename T>
    T * allocate(size_t count = 1)
    {
        return reinterpret_cast<T *>(temp().allocate(sizeof(T) * count));
    }

  public:
    JSContext *cx;
    JSScript *script;

  protected:
    jsbytecode *pc;
    TempAllocator &temp_;
    JSFunction *fun_;
    uint32 nslots_;
    MIRGraph &graph_;
};


class MUse : public TempObject
{
    friend class MInstruction;

    MUse *next_;            
    MInstruction *owner_;   
    uint32 index_;          

    MUse(MUse *next, MInstruction *owner, uint32 index)
      : next_(next), owner_(owner), index_(index)
    { }

    void set(MUse *next, MInstruction *owner) {
        next_ = next;
        owner_ = owner;
    }

  public:
    static inline MUse *New(MIRGenerator *gen, MUse *next, MInstruction *owner, uint32 index)
    {
        return new (gen->temp()) MUse(next, owner, index);
    }

    MInstruction *owner() const {
        return owner_;
    }
    uint32 index() const {
        return index_;
    }
    MUse *next() const {
        return next_;
    }
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
    static MOperand *New(MIRGenerator *gen, MInstruction *ins)
    {
        return new (gen->temp()) MOperand(ins);
    }

    MInstruction *ins() const {
        return ins_;
    }
};



class MInstruction : public TempObject
{
    friend class MBasicBlock;

  public:
    enum Opcode {
#   define DEFINE_OPCODES(op) Op_##op,
        MIR_OPCODE_LIST(DEFINE_OPCODES)
#   undef DEFINE_OPCODES
        Op_Invalid
    };

  private:
    
    MBasicBlock *block_;

    
    MIRType type_;

    
    uint32 usedAsType_;

    
    MUse *uses_;

    uint32 id_;

    
    bool inWorklist_;

  private:
    void setBlock(MBasicBlock *block) {
        block_ = block;
    }

  public:
    MInstruction()
      : block_(NULL),
        type_(MIRType_Box),
        uses_(NULL),
        id_(0),
        inWorklist_(0)
    { }

    virtual Opcode op() const = 0;
    void printName(FILE *fp);
    void printOpcode(FILE *fp);

    uint32 id() const {
        JS_ASSERT(block_);
        return id_;
    }
    void setId(uint32 id) {
        id_ = id;
    }
    MIRType type() const {
        return type_;
    }
    void setType(MIRType type) {
        type_ = type;
    }
    bool inWorklist() const {
        return inWorklist_;
    }
    void setInWorklist() {
        JS_ASSERT(!inWorklist());
        inWorklist_ = true;
    }
    void setNotInWorklist() {
        JS_ASSERT(inWorklist());
        inWorklist_ = false;
    }

    
    MBasicBlock *block() const {
        JS_ASSERT(block_);
        return block_;
    }

    
    MUse *uses() const {
        return uses_;
    }
    void removeUse(MUse *prev, MUse *use);

    
    size_t useCount() const;

    
    virtual MOperand *getOperand(size_t index) const = 0;
    virtual size_t numOperands() const = 0;

    
    
    void replaceOperand(MUse *prev, MUse *use, MInstruction *ins);

    bool addUse(MIRGenerator *gen, MInstruction *ins, size_t index) {
        uses_ = MUse::New(gen, uses_, ins, index);
        return !!uses_;
    }
    void addUse(MUse *use, MInstruction *ins) {
        use->set(uses_, ins);
        uses_ = use;
    }

    
#   define OPCODE_CASTS(opcode)                                             \
    bool is##opcode() const {                                               \
        return op() == Op_##opcode;                                         \
    }                                                                       \
    inline M##opcode *to##opcode();
    MIR_OPCODE_LIST(OPCODE_CASTS)
#   undef OPCODE_CASTS

  protected:
    
    virtual void setOperand(size_t index, MOperand *operand) = 0;
    void setOperand(size_t index, MInstruction *ins);

    
    bool initOperand(MIRGenerator *gen, size_t index, MInstruction *ins) {
        MOperand *operand = MOperand::New(gen, ins);
        if (!operand)
            return false;
        setOperand(index, operand);
        return ins->addUse(gen, this, index);
    }
};

template <typename T, size_t Arity>
class MFixedArityList
{
    T list_[Arity];

  public:
    T &operator [](size_t index) {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
};

class MUseIterator
{
    MInstruction *def;
    MUse *use;

  public:
    MUseIterator(MInstruction *def)
      : use(def->uses())
    { }

    bool more() const {
        return !!use;
    }
    void next() {
        use = use->next();
    }
    MUse * operator *() const {
        return use;
    }
};


template <typename T>
class MFixedArityList<T, 0>
{
  public:
    T &operator [](size_t index) {
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
    const T &operator [](size_t index) const {
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
};

template <size_t Arity>
class MAryInstruction : public MInstruction
{
  protected:
    MFixedArityList<MOperand *, Arity> operands_;

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

class MConstant : public MAryInstruction<0>
{
    js::Value value_;

    MConstant(const Value &v);

  public:
    static MConstant *New(MIRGenerator *gen, const Value &v);

    virtual Opcode op() const {
        return MInstruction::Op_Constant;
    }
    const js::Value &value() const {
        return value_;
    }
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
    }

  public:
    static MParameter *New(MIRGenerator *gen, int32 index);

    virtual Opcode op() const {
        return MInstruction::Op_Parameter;
    }

    int32 index() const {
        return index_;
    }
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
};

template <size_t Arity>
class MAryControlInstruction : public MControlInstruction
{
    MFixedArityList<MOperand *, Arity> operands_;

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
    static MGoto *New(MIRGenerator *gen, MBasicBlock *target);

    virtual Opcode op() const {
        return MInstruction::Op_Goto;
    }
};

class MTest : public MAryControlInstruction<1>
{
    MTest(MBasicBlock *if_true, MBasicBlock *if_false)
    {
        successors[0] = if_true;
        successors[1] = if_false;
    }

  public:
    static MTest *New(MIRGenerator *gen, MInstruction *ins,
                      MBasicBlock *ifTrue, MBasicBlock *ifFalse);

    virtual Opcode op() const {
        return MInstruction::Op_Test;
    }
};

class MReturn : public MAryControlInstruction<1>
{
  public:
    static MReturn *New(MIRGenerator *gen, MInstruction *ins);

    virtual Opcode op() const {
        return MInstruction::Op_Return;
    }
};

class MUnaryInstruction : public MAryInstruction<1>
{
  protected:
    inline bool init(MIRGenerator *gen, MInstruction *ins)
    {
        if (!initOperand(gen, 0, ins))
            return false;
        return true;
    }
};

class MBinaryInstruction : public MAryInstruction<2>
{
  protected:
    inline bool init(MIRGenerator *gen, MInstruction *left, MInstruction *right)
    {
        if (!initOperand(gen, 0, left) ||
            !initOperand(gen, 1, right)) {
            return false;
        }
        return true;
    }
};

class MCopy : public MUnaryInstruction
{
  public:
    static MCopy *New(MIRGenerator  *gen, MInstruction *ins);

    virtual Opcode op() const {
        return MInstruction::Op_Copy;
    }
};

class MBitAnd : public MBinaryInstruction
{
  public:
    static MBitAnd *New(MIRGenerator *gen, MInstruction *left, MInstruction *right);

    virtual Opcode op() const {
        return MInstruction::Op_BitAnd;
    }
};

class MPhi : public MInstruction
{
    js::Vector<MOperand *, 2, TempAllocPolicy> inputs_;
    uint32 slot_;

    MPhi(JSContext *cx, uint32 slot)
      : inputs_(TempAllocPolicy(cx)),
        slot_(slot)
    { }

  protected:
    void setOperand(size_t index, MOperand *operand) {
        inputs_[index] = operand;
    }

  public:
    static MPhi *New(MIRGenerator *gen, uint32 slot);

    virtual Opcode op() const {
        return MInstruction::Op_Phi;
    }

    MOperand *getOperand(size_t index) const {
        return inputs_[index];
    }
    size_t numOperands() const {
        return inputs_.length();
    }
    uint32 slot() const {
        return slot_;
    }
    bool addInput(MIRGenerator *gen, MInstruction *ins);
};


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

