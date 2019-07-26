








































#ifndef jsion_lir_common_h__
#define jsion_lir_common_h__

#include "ion/shared/Assembler-shared.h"



namespace js {
namespace ion {



class LLabel : public LInstructionHelper<0, 0, 0>
{
    Label label_;

  public:
    LIR_HEADER(Label);

    Label *label() {
        return &label_;
    }
};

class LNop : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(Nop);
};







class LOsiPoint : public LInstructionHelper<0, 0, 0>
{
    LSafepoint *safepoint_;

  public:
    LOsiPoint(LSafepoint *safepoint, LSnapshot *snapshot)
      : safepoint_(safepoint)
    {
        JS_ASSERT(safepoint && snapshot);
        assignSnapshot(snapshot);
    }

    LSafepoint *associatedSafepoint() {
        return safepoint_;
    }

    LIR_HEADER(OsiPoint);
};

class LMove
{
    LAllocation *from_;
    LAllocation *to_;

  public:
    LMove(LAllocation *from, LAllocation *to)
      : from_(from),
        to_(to)
    { }

    LAllocation *from() {
        return from_;
    }
    const LAllocation *from() const {
        return from_;
    }
    LAllocation *to() {
        return to_;
    }
    const LAllocation *to() const {
        return to_;
    }
};

class LMoveGroup : public LInstructionHelper<0, 0, 0>
{
    js::Vector<LMove, 2, IonAllocPolicy> moves_;

  public:
    LIR_HEADER(MoveGroup);

    void printOperands(FILE *fp);
    bool add(LAllocation *from, LAllocation *to) {
        JS_ASSERT(*from != *to);
        return moves_.append(LMove(from, to));
    }
    size_t numMoves() const {
        return moves_.length();
    }
    const LMove &getMove(size_t i) const {
        return moves_[i];
    }
};


class LInteger : public LInstructionHelper<1, 0, 0>
{
    int32 i32_;

  public:
    LIR_HEADER(Integer);

    LInteger(int32 i32) : i32_(i32)
    { }

    int32 getValue() const {
        return i32_;
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LPointer : public LInstructionHelper<1, 0, 0>
{
  public:
    enum Kind {
        GC_THING,
        NON_GC_THING
    };

  private:
    void *ptr_;
    Kind kind_;

  public:
    LIR_HEADER(Pointer);

    LPointer(gc::Cell *ptr) : ptr_(ptr), kind_(GC_THING)
    { }

    LPointer(void *ptr, Kind kind) : ptr_(ptr), kind_(kind)
    { }

    void *ptr() const {
        return ptr_;
    }
    Kind kind() const {
        return kind_;
    }

    gc::Cell *gcptr() const {
        JS_ASSERT(kind() == GC_THING);
        return (gc::Cell *) ptr_;
    }

    const LDefinition *output() {
        return getDef(0);
    }
};


class LValue : public LInstructionHelper<BOX_PIECES, 0, 0>
{
    Value v_;

  public:
    LIR_HEADER(Value);

    LValue(const Value &v) : v_(v)
    { }

    Value value() const {
        return v_;
    }
};



class LParameter : public LInstructionHelper<BOX_PIECES, 0, 0>
{
  public:
    LIR_HEADER(Parameter);
};


class LCallee : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(Callee);
};


class LGoto : public LInstructionHelper<0, 0, 0>
{
    MBasicBlock *block_;

  public:
    LIR_HEADER(Goto);

    LGoto(MBasicBlock *block)
      : block_(block)
    { }

    MBasicBlock *target() const {
        return block_;
    }
};

class LNewSlots : public LCallInstructionHelper<1, 0, 3>
{
  public:
    LIR_HEADER(NewSlots);

    LNewSlots(const LDefinition &temp1, const LDefinition &temp2,
              const LDefinition &temp3) {
        setTemp(0, temp1);
        setTemp(1, temp2);
        setTemp(2, temp3);
    }

    const LDefinition *output() {
        return getDef(0);
    }
    const LDefinition *temp1() {
        return getTemp(0);
    }
    const LDefinition *temp2() {
        return getTemp(1);
    }
    const LDefinition *temp3() {
        return getTemp(2);
    }

    MNewSlots *mir() const {
        return mir_->toNewSlots();
    }
};

class LNewArray : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewArray);

    const LDefinition *output() {
        return getDef(0);
    }

    MNewArray *mir() const {
        return mir_->toNewArray();
    }
};

class LNewObject : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewObject);

    const LDefinition *output() {
        return getDef(0);
    }

    MNewObject *mir() const {
        return mir_->toNewObject();
    }
};










class LNewCallObject : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NewCallObject);

    LNewCallObject(const LAllocation &slots)
    {
        setOperand(0, slots);
    }

    bool isCall() const;

    const LDefinition *output() {
        return getDef(0);
    }
    const LAllocation *slots() {
        return getOperand(0);
    }
    MNewCallObject *mir() const {
        return mir_->toNewCallObject();
    }
};


class LInitProp : public LInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(InitProp);

    LInitProp(const LAllocation &object)
    {
        setOperand(0, object);
    }

    static const size_t ValueIndex = 1;

    const LAllocation *getObject() {
        return getOperand(0);
    }
    const LAllocation *getValue() {
        return getOperand(1);
    }

    bool isCall() const {
        return true;
    }
    MInitProp *mir() const {
        return mir_->toInitProp();
    }
};

class LCheckOverRecursed : public LInstructionHelper<0, 0, 1>
{
  public:
    LIR_HEADER(CheckOverRecursed);

    LCheckOverRecursed(const LDefinition &limitreg)
    {
        setTemp(0, limitreg);
    }

    const LAllocation *limitTemp() {
        return getTemp(0)->output();
    }
};

class LDefVar : public LCallInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(DefVar);

    LDefVar(const LAllocation &scopeChain, const LDefinition &namereg)
    {
        setOperand(0, scopeChain);
        setTemp(0, namereg);
    }

    const LAllocation *getScopeChain() {
        return getOperand(0);
    }
    const LAllocation *nameTemp() {
        return getTemp(0)->output();
    }
    MDefVar *mir() const {
        return mir_->toDefVar();
    }
};

class LTypeOfV : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(TypeOfV);

    static const size_t Input = 0;

    MTypeOf *mir() const {
        return mir_->toTypeOf();
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LToIdV : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(ToIdV);

    static const size_t Object = 0;
    static const size_t Index = BOX_PIECES;
};


class LCreateThis : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CreateThis);

    LCreateThis(const LAllocation &callee, const LAllocation &prototype)
    {
        setOperand(0, callee);
        setOperand(1, prototype);
    }

    const LAllocation *getCallee() {
        return getOperand(0);
    }
    const LAllocation *getPrototype() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }

    MCreateThis *mir() const {
        return mir_->toCreateThis();
    }
};



class LReturnFromCtor : public LInstructionHelper<1, BOX_PIECES + 1, 0>
{
  public:
    LIR_HEADER(ReturnFromCtor);

    LReturnFromCtor(const LAllocation &object)
    {
        
        setOperand(LReturnFromCtor::ObjectIndex, object);
    }

    const LAllocation *getObject() {
        return getOperand(LReturnFromCtor::ObjectIndex);
    }
    const LDefinition *output() {
        return getDef(0);
    }

    static const size_t ValueIndex = 0;
    static const size_t ObjectIndex = BOX_PIECES;
};


class LStackArg : public LInstructionHelper<0, BOX_PIECES, 0>
{
    uint32 argslot_; 

  public:
    LIR_HEADER(StackArg);

    LStackArg(uint32 argslot)
      : argslot_(argslot)
    { }

    uint32 argslot() const {
        return argslot_;
    }
};



class LCallGeneric : public LCallInstructionHelper<BOX_PIECES, 1, 2>
{
    
    
    uint32 argslot_;

  public:
    LIR_HEADER(CallGeneric);

    LCallGeneric(const LAllocation &func,
                 uint32 argslot,
                 const LDefinition &nargsreg,
                 const LDefinition &tmpobjreg)
      : argslot_(argslot)
    {
        setOperand(0, func);
        setTemp(0, nargsreg);
        setTemp(1, tmpobjreg);
    }

    uint32 argslot() const {
        return argslot_;
    }
    MCall *mir() const {
        return mir_->toCall();
    }

    
    
    
    
    uint32 numStackArgs() const {
        JS_ASSERT(mir()->numStackArgs() >= 1);
        return mir()->numStackArgs() - 1; 
    }
    
    uint32 numActualArgs() const {
        return mir()->numActualArgs();
    }

    bool hasSingleTarget() const {
        return getSingleTarget() != NULL;
    }
    JSFunction *getSingleTarget() const {
        return mir()->getSingleTarget();
    }

    const LAllocation *getFunction() {
        return getOperand(0);
    }
    const LAllocation *getNargsReg() {
        return getTemp(0)->output();
    }
    const LAllocation *getTempObject() {
        return getTemp(1)->output();
    }
};


class LCallNative : public LCallInstructionHelper<BOX_PIECES, 0, 4>
{
    uint32 argslot_;

  public:
    LIR_HEADER(CallNative);

    LCallNative(uint32 argslot,
                const LDefinition &argJSContext, const LDefinition &argUintN,
                const LDefinition &argVp, const LDefinition &tmpreg)
      : argslot_(argslot)
    {
        
        setTemp(0, argJSContext);
        setTemp(1, argUintN);
        setTemp(2, argVp);

        
        setTemp(3, tmpreg);
    }

    JSFunction *function() const {
        return mir()->getSingleTarget();
    }
    uint32 argslot() const {
        return argslot_;
    }
    MCall *mir() const {
        return mir_->toCall();
    }

    
    uint32 numStackArgs() const {
        JS_ASSERT(mir()->numStackArgs() >= 1);
        return mir()->numStackArgs() - 1; 
    }
    uint32 numActualArgs() const {
        return mir()->numActualArgs();
    }

    const LAllocation *getArgJSContextReg() {
        return getTemp(0)->output();
    }
    const LAllocation *getArgUintNReg() {
        return getTemp(1)->output();
    }
    const LAllocation *getArgVpReg() {
        return getTemp(2)->output();
    }

    const LAllocation *getTempReg() {
        return getTemp(3)->output();
    }
};



class LCallConstructor : public LInstructionHelper<BOX_PIECES, 1, 0>
{
    uint32 argslot_;

  public:
    LIR_HEADER(CallConstructor);

    LCallConstructor(const LAllocation &func, uint32 argslot)
      : argslot_(argslot)
    {
        setOperand(0, func);
    }

    uint32 argslot() const {
        return argslot_;
    }
    MCall *mir() const {
        return mir_->toCall();
    }

    uint32 numStackArgs() const {
        JS_ASSERT(mir()->numStackArgs() >= 1);
        return mir()->numStackArgs() - 1; 
    }
    uint32 numActualArgs() const {
        return mir()->numActualArgs();
    }
    bool isCall() const {
        return true;
    }

    const LAllocation *getFunction() {
        return getOperand(0);
    }
};



class LApplyArgsGeneric : public LCallInstructionHelper<BOX_PIECES, BOX_PIECES + 2, 2>
{
  public:
    LIR_HEADER(ApplyArgsGeneric);

    LApplyArgsGeneric(const LAllocation &func,
                      const LAllocation &argc,
                      const LDefinition &tmpobjreg,
                      const LDefinition &tmpcopy)
    {
        setOperand(0, func);
        setOperand(1, argc);
        setTemp(0, tmpobjreg);
        setTemp(1, tmpcopy);
    }

    MApplyArgs *mir() const {
        return mir_->toApplyArgs();
    }

    bool hasSingleTarget() const {
        return getSingleTarget() != NULL;
    }
    JSFunction *getSingleTarget() const {
        return mir()->getSingleTarget();
    }

    const LAllocation *getFunction() {
        return getOperand(0);
    }
    const LAllocation *getArgc() {
        return getOperand(1);
    }
    static const size_t ThisIndex = 2;

    const LAllocation *getTempObject() {
        return getTemp(0)->output();
    }
    const LAllocation *getTempCopy() {
        return getTemp(1)->output();
    }
};


class LTestIAndBranch : public LInstructionHelper<0, 1, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(TestIAndBranch);

    LTestIAndBranch(const LAllocation &in, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        setOperand(0, in);
    }

    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    const LAllocation *input() {
        return getOperand(0);
    }
};


class LTestDAndBranch : public LInstructionHelper<0, 1, 1>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(TestDAndBranch);

    LTestDAndBranch(const LAllocation &in, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        setOperand(0, in);
    }

    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    const LAllocation *input() {
        return getOperand(0);
    }
};


class LTestVAndBranch : public LInstructionHelper<0, BOX_PIECES, 1>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(TestVAndBranch);

    LTestVAndBranch(MBasicBlock *ifTrue, MBasicBlock *ifFalse, const LDefinition &temp)
      : ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        setTemp(0, temp);
    }

    static const size_t Input = 0;

    const LAllocation *tempFloat() {
        return getTemp(0)->output();
    }

    Label *ifTrue();
    Label *ifFalse();
};

class LPolyInlineDispatch : public LInstructionHelper<0, 1, 0>
{
  
  public:
    LIR_HEADER(PolyInlineDispatch);

    LPolyInlineDispatch(const LAllocation &in) {
        setOperand(0, in);
    }

    const LAllocation *input() {
        return getOperand(0);
    }

    MPolyInlineDispatch *mir() {
        return mir_->toPolyInlineDispatch();
    }
};




class LCompare : public LInstructionHelper<1, 2, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(Compare);
    LCompare(JSOp jsop, const LAllocation &left, const LAllocation &right)
      : jsop_(jsop)
    {
        setOperand(0, left);
        setOperand(1, right);
    }

    JSOp jsop() const {
        return jsop_;
    }
    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareD : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CompareD);
    LCompareD(const LAllocation &left, const LAllocation &right) {
        setOperand(0, left);
        setOperand(1, right);
    }

    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareS : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CompareS);
    LCompareS(const LAllocation &left, const LAllocation &right) {
        setOperand(0, left);
        setOperand(1, right);
    }

    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareV : public LCallInstructionHelper<1, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CompareV);

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    MCompare *mir() const {
        return mir_->toCompare();
    }
};



class LCompareAndBranch : public LInstructionHelper<0, 2, 0>
{
    JSOp jsop_;
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareAndBranch);
    LCompareAndBranch(JSOp jsop, const LAllocation &left, const LAllocation &right,
                      MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : jsop_(jsop),
        ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        setOperand(0, left);
        setOperand(1, right);
    }

    JSOp jsop() const {
        return jsop_;
    }
    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareDAndBranch : public LInstructionHelper<0, 2, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareDAndBranch);
    LCompareDAndBranch(const LAllocation &left, const LAllocation &right,
                       MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        setOperand(0, left);
        setOperand(1, right);
    }

    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};




class LCompareB : public LInstructionHelper<1, BOX_PIECES + 1, 0>
{
  public:
    LIR_HEADER(CompareB);

    LCompareB(const LAllocation &rhs) {
        setOperand(BOX_PIECES, rhs);
    }

    static const size_t Lhs = 0;

    const LAllocation *rhs() {
        return getOperand(BOX_PIECES);
    }

    const LDefinition *output() {
        return getDef(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareBAndBranch : public LInstructionHelper<0, BOX_PIECES + 1, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareBAndBranch);

    LCompareBAndBranch(const LAllocation &rhs, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue), ifFalse_(ifFalse)
    {
        setOperand(BOX_PIECES, rhs);
    }

    static const size_t Lhs = 0;

    const LAllocation *rhs() {
        return getOperand(BOX_PIECES);
    }

    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LIsNullOrUndefined : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(IsNullOrUndefined);

    static const size_t Value = 0;

    const LDefinition *output() {
        return getDef(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LIsNullOrUndefinedAndBranch : public LInstructionHelper<0, BOX_PIECES, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(IsNullOrUndefinedAndBranch);

    LIsNullOrUndefinedAndBranch(MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue), ifFalse_(ifFalse)
    { }

    static const size_t Value = 0;

    MBasicBlock *ifTrue() const {
        return ifTrue_;
    }
    MBasicBlock *ifFalse() const {
        return ifFalse_;
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};


class LNotI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NotI);

    LNotI(const LAllocation &input) {
        setOperand(0, input);
    }
    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LNotD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NotD);

    LNotD(const LAllocation &input) {
        setOperand(0, input);
    }
    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LNotV : public LInstructionHelper<1, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(NotV);

    static const size_t Input = 0;
    LNotV(const LDefinition &temp)
    {
        setTemp(0, temp);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }

    const LAllocation *tempFloat() {
        return getTemp(0)->output();
    }
};



class LBitNotI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(BitNotI);
};


class LBitNotV : public LCallInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(BitNotV);

    static const size_t Input = 0;
};



class LBitOpI : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
    LIR_HEADER(BitOpI);

    LBitOpI(JSOp op)
      : op_(op)
    { }

    JSOp bitop() {
        return op_;
    }
};


class LBitOpV : public LCallInstructionHelper<1, 2 * BOX_PIECES, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(BitOpV);

    LBitOpV(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;
};



class LShiftOp : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
    LIR_HEADER(ShiftOp);

    LShiftOp(JSOp op)
      : op_(op)
    { }

    JSOp bitop() {
        return op_;
    }

    MInstruction *mir() {
        return mir_->toInstruction();
    }
};



class LReturn : public LInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(Return);
};

class LThrow : public LCallInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(Throw);

    static const size_t Value = 0;
};

template <size_t Temps, size_t ExtraUses = 0>
class LBinaryMath : public LInstructionHelper<1, 2 + ExtraUses, Temps>
{
  public:
    const LAllocation *lhs() {
        return this->getOperand(0);
    }
    const LAllocation *rhs() {
        return this->getOperand(1);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LAbsI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(AbsI);
    LAbsI(const LAllocation &num) {
        setOperand(0, num);
    }

    const LAllocation *input() {
        return this->getOperand(0);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LAbsD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(AbsD);
    LAbsD(const LAllocation &num) {
        setOperand(0, num);
    }

    const LAllocation *input() {
        return this->getOperand(0);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LSqrtD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(SqrtD);
    LSqrtD(const LAllocation &num) {
        setOperand(0, num);
    }

    const LAllocation *input() {
        return this->getOperand(0);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};

class LMathFunctionD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(MathFunctionD);
    LMathFunctionD(const LAllocation &input) {
        setOperand(0, input);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MMathFunction *mir() const {
        return mir_->toMathFunction();
    }
};


class LAddI : public LBinaryMath<0>
{
  public:
    LIR_HEADER(AddI);
};


class LSubI : public LBinaryMath<0>
{
  public:
    LIR_HEADER(SubI);
};


class LMathD : public LBinaryMath<0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(MathD);

    LMathD(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }
};


class LBinaryV : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(BinaryV);
    BOX_OUTPUT_ACCESSORS();

    LBinaryV(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;
};


class LConcat : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(Concat);

    LConcat(const LAllocation &lhs, const LAllocation &rhs) {
        setOperand(0, lhs);
        setOperand(1, rhs);
    }

    const LAllocation *lhs() {
        return this->getOperand(0);
    }
    const LAllocation *rhs() {
        return this->getOperand(1);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LCharCodeAt : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CharCodeAt);

    LCharCodeAt(const LAllocation &str, const LAllocation &index) {
        setOperand(0, str);
        setOperand(1, index);
    }

    const LAllocation *str() {
        return this->getOperand(0);
    }
    const LAllocation *index() {
        return this->getOperand(1);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LFromCharCode : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(FromCharCode);

    LFromCharCode(const LAllocation &code) {
        setOperand(0, code);
    }

    const LAllocation *code() {
        return this->getOperand(0);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};


class LInt32ToDouble : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Int32ToDouble);

    LInt32ToDouble(const LAllocation &input) {
        setOperand(0, input);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LValueToDouble : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(ValueToDouble);

    static const size_t Input = 0;

    const LDefinition *output() {
        return getDef(0);
    }
};








class LValueToInt32 : public LInstructionHelper<1, BOX_PIECES, 1>
{
  public:
    enum Mode {
        NORMAL,
        TRUNCATE
    };

  private:
    Mode mode_;

  public:
    LIR_HEADER(ValueToInt32);

    LValueToInt32(const LDefinition &temp, Mode mode) : mode_(mode) {
        setTemp(0, temp);
    }

    static const size_t Input = 0;

    Mode mode() const {
        return mode_;
    }
    const LDefinition *tempFloat() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MToInt32 *mir() const {
        return mir_->toToInt32();
    }
};





class LDoubleToInt32 : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(DoubleToInt32);

    LDoubleToInt32(const LAllocation &in) {
        setOperand(0, in);
    }

    const LAllocation *input() {
        return getOperand(0);
    }

    const LDefinition *output() {
        return getDef(0);
    }

    MToInt32 *mir() const {
        return mir_->toToInt32();
    }
};





class LTruncateDToInt32 : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(TruncateDToInt32);

    LTruncateDToInt32(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *tempFloat() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};



class LIntToString : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(IntToString);

    LIntToString(const LAllocation &input) {
        setOperand(0, input);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    const MToString *mir() {
        return mir_->toToString();
    }
};




class LStart : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(Start);
};



class LOsrEntry : public LInstructionHelper<1, 0, 0>
{
  protected:
    Label label_;
    uint32 frameDepth_;

  public:
    LIR_HEADER(OsrEntry);

    LOsrEntry()
      : frameDepth_(0)
    { }

    void setFrameDepth(uint32 depth) {
        frameDepth_ = depth;
    }
    uint32 getFrameDepth() {
        return frameDepth_;
    }
    Label *label() {
        return &label_;
    }

};


class LOsrValue : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(OsrValue);

    LOsrValue(const LAllocation &entry)
    {
        setOperand(0, entry);
    }

    const MOsrValue *mir() {
        return mir_->toOsrValue();
    }
};


class LOsrScopeChain : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(OsrScopeChain);

    LOsrScopeChain(const LAllocation &entry)
    {
        setOperand(0, entry);
    }

    const MOsrScopeChain *mir() {
        return mir_->toOsrScopeChain();
    }
};

class LRegExp : public LCallInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(RegExp);

    const MRegExp *mir() const {
        return mir_->toRegExp();
    }
};

class LLambdaForSingleton : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(LambdaForSingleton);

    LLambdaForSingleton(const LAllocation &scopeChain)
    {
        setOperand(0, scopeChain);
    }
    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    const MLambda *mir() const {
        return mir_->toLambda();
    }
};

class LLambda : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Lambda);

    LLambda(const LAllocation &scopeChain) {
        setOperand(0, scopeChain);
    }
    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    const MLambda *mir() const {
        return mir_->toLambda();
    }
};


class LImplicitThis : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(ImplicitThis);
    BOX_OUTPUT_ACCESSORS();

    LImplicitThis(const LAllocation &callee) {
        setOperand(0, callee);
    }

    const MImplicitThis *mir() const {
        return mir_->toImplicitThis();
    }
    const LAllocation *callee() {
        return getOperand(0);
    }
};




class LSlots : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Slots);

    LSlots(const LAllocation &object) {
        setOperand(0, object);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};




class LElements : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Elements);

    LElements(const LAllocation &object) {
        setOperand(0, object);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LInitializedLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(InitializedLength);

    LInitializedLength(const LAllocation &elements) {
        setOperand(0, elements);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LSetInitializedLength : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(SetInitializedLength);

    LSetInitializedLength(const LAllocation &elements, const LAllocation &index) {
        setOperand(0, elements);
        setOperand(1, index);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
};


class LArrayLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(ArrayLength);

    LArrayLength(const LAllocation &elements) {
        setOperand(0, elements);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LTypedArrayLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(TypedArrayLength);

    LTypedArrayLength(const LAllocation &obj) {
        setOperand(0, obj);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LTypedArrayElements : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(TypedArrayElements);

    LTypedArrayElements(const LAllocation &object) {
        setOperand(0, object);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LBoundsCheck : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(BoundsCheck);

    LBoundsCheck(const LAllocation &index, const LAllocation &length) {
        setOperand(0, index);
        setOperand(1, length);
    }
    const MBoundsCheck *mir() const {
        return mir_->toBoundsCheck();
    }
    const LAllocation *index() {
        return getOperand(0);
    }
    const LAllocation *length() {
        return getOperand(1);
    }
};


class LBoundsCheckRange : public LInstructionHelper<0, 2, 1>
{
  public:
    LIR_HEADER(BoundsCheckRange);

    LBoundsCheckRange(const LAllocation &index, const LAllocation &length,
                      const LDefinition &temp)
    {
        setOperand(0, index);
        setOperand(1, length);
        setTemp(0, temp);
    }
    const MBoundsCheck *mir() const {
        return mir_->toBoundsCheck();
    }
    const LAllocation *index() {
        return getOperand(0);
    }
    const LAllocation *length() {
        return getOperand(1);
    }
};


class LBoundsCheckLower : public LInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(BoundsCheckLower);

    LBoundsCheckLower(const LAllocation &index)
    {
        setOperand(0, index);
    }
    MBoundsCheckLower *mir() const {
        return mir_->toBoundsCheckLower();
    }
    const LAllocation *index() {
        return getOperand(0);
    }
};


class LLoadElementV : public LInstructionHelper<BOX_PIECES, 2, 0>
{
  public:
    LIR_HEADER(LoadElementV);
    BOX_OUTPUT_ACCESSORS();

    LLoadElementV(const LAllocation &elements, const LAllocation &index) {
        setOperand(0, elements);
        setOperand(1, index);
    }
    const MLoadElement *mir() const {
        return mir_->toLoadElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
};


class LLoadElementHole : public LInstructionHelper<BOX_PIECES, 3, 0>
{
  public:
    LIR_HEADER(LoadElementHole);
    BOX_OUTPUT_ACCESSORS();

    LLoadElementHole(const LAllocation &elements, const LAllocation &index, const LAllocation &initLength) {
        setOperand(0, elements);
        setOperand(1, index);
        setOperand(2, initLength);
    }
    const MLoadElementHole *mir() const {
        return mir_->toLoadElementHole();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
    const LAllocation *initLength() {
        return getOperand(2);
    }
};





class LLoadElementT : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(LoadElementT);

    LLoadElementT(const LAllocation &elements, const LAllocation &index) {
        setOperand(0, elements);
        setOperand(1, index);
    }
    const MLoadElement *mir() const {
        return mir_->toLoadElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LStoreElementV : public LInstructionHelper<0, 2 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreElementV);

    LStoreElementV(const LAllocation &elements, const LAllocation &index) {
        setOperand(0, elements);
        setOperand(1, index);
    }

    static const size_t Value = 2;

    const MStoreElement *mir() const {
        return mir_->toStoreElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
};





class LStoreElementT : public LInstructionHelper<0, 3, 0>
{
  public:
    LIR_HEADER(StoreElementT);

    LStoreElementT(const LAllocation &elements, const LAllocation &index, const LAllocation &value) {
        setOperand(0, elements);
        setOperand(1, index);
        setOperand(2, value);
    }

    const MStoreElement *mir() const {
        return mir_->toStoreElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
    const LAllocation *value() {
        return getOperand(2);
    }
};


class LStoreElementHoleV : public LInstructionHelper<0, 3 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreElementHoleV);

    LStoreElementHoleV(const LAllocation &object, const LAllocation &elements,
                       const LAllocation &index) {
        setOperand(0, object);
        setOperand(1, elements);
        setOperand(2, index);
    }

    static const size_t Value = 3;

    const MStoreElementHole *mir() const {
        return mir_->toStoreElementHole();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LAllocation *elements() {
        return getOperand(1);
    }
    const LAllocation *index() {
        return getOperand(2);
    }
};


class LStoreElementHoleT : public LInstructionHelper<0, 4, 0>
{
  public:
    LIR_HEADER(StoreElementHoleT);

    LStoreElementHoleT(const LAllocation &object, const LAllocation &elements,
                       const LAllocation &index, const LAllocation &value) {
        setOperand(0, object);
        setOperand(1, elements);
        setOperand(2, index);
        setOperand(3, value);
    }

    const MStoreElementHole *mir() const {
        return mir_->toStoreElementHole();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LAllocation *elements() {
        return getOperand(1);
    }
    const LAllocation *index() {
        return getOperand(2);
    }
    const LAllocation *value() {
        return getOperand(3);
    }
};

class LArrayPopShiftV : public LInstructionHelper<BOX_PIECES, 1, 2>
{
  public:
    LIR_HEADER(ArrayPopShiftV);

    LArrayPopShiftV(const LAllocation &object, const LDefinition &temp0, const LDefinition &temp1) {
        setOperand(0, object);
        setTemp(0, temp0);
        setTemp(1, temp1);
    }

    const MArrayPopShift *mir() const {
        return mir_->toArrayPopShift();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp0() {
        return getTemp(0);
    }
    const LDefinition *temp1() {
        return getTemp(1);
    }
};

class LArrayPopShiftT : public LInstructionHelper<1, 1, 2>
{
  public:
    LIR_HEADER(ArrayPopShiftT);

    LArrayPopShiftT(const LAllocation &object, const LDefinition &temp0, const LDefinition &temp1) {
        setOperand(0, object);
        setTemp(0, temp0);
        setTemp(1, temp1);
    }

    const MArrayPopShift *mir() const {
        return mir_->toArrayPopShift();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp0() {
        return getTemp(0);
    }
    const LDefinition *temp1() {
        return getTemp(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LArrayPushV : public LInstructionHelper<1, 1 + BOX_PIECES, 1>
{
  public:
    LIR_HEADER(ArrayPushV);

    LArrayPushV(const LAllocation &object, const LDefinition &temp) {
        setOperand(0, object);
        setTemp(0, temp);
    }

    static const size_t Value = 1;

    const MArrayPush *mir() const {
        return mir_->toArrayPush();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LArrayPushT : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(ArrayPushT);

    LArrayPushT(const LAllocation &object, const LAllocation &value, const LDefinition &temp) {
        setOperand(0, object);
        setOperand(1, value);
        setTemp(0, temp);
    }

    const MArrayPush *mir() const {
        return mir_->toArrayPush();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LAllocation *value() {
        return getOperand(1);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LLoadTypedArrayElement : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(LoadTypedArrayElement);

    LLoadTypedArrayElement(const LAllocation &elements, const LAllocation &index,
                           const LDefinition &temp) {
        setOperand(0, elements);
        setOperand(1, index);
        setTemp(0, temp);
    }
    const MLoadTypedArrayElement *mir() const {
        return mir_->toLoadTypedArrayElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LLoadTypedArrayElementHole : public LInstructionHelper<BOX_PIECES, 2, 0>
{
  public:
    LIR_HEADER(LoadTypedArrayElementHole);
    BOX_OUTPUT_ACCESSORS();

    LLoadTypedArrayElementHole(const LAllocation &object, const LAllocation &index) {
        setOperand(0, object);
        setOperand(1, index);
    }
    const MLoadTypedArrayElementHole *mir() const {
        return mir_->toLoadTypedArrayElementHole();
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
};

class LStoreTypedArrayElement : public LInstructionHelper<0, 3, 0>
{
  public:
    LIR_HEADER(StoreTypedArrayElement);

    LStoreTypedArrayElement(const LAllocation &elements, const LAllocation &index,
                            const LAllocation &value) {
        setOperand(0, elements);
        setOperand(1, index);
        setOperand(2, value);
    }

    const MStoreTypedArrayElement *mir() const {
        return mir_->toStoreTypedArrayElement();
    }
    const LAllocation *elements() {
        return getOperand(0);
    }
    const LAllocation *index() {
        return getOperand(1);
    }
    const LAllocation *value() {
        return getOperand(2);
    }
};

class LClampIToUint8 : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(ClampIToUint8);

    LClampIToUint8(const LAllocation &in) {
        setOperand(0, in);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LClampDToUint8 : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(ClampDToUint8);

    LClampDToUint8(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LClampVToUint8 : public LInstructionHelper<1, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(ClampVToUint8);

    LClampVToUint8(const LDefinition &tempFloat) {
        setTemp(0, tempFloat);
    }

    static const size_t Input = 0;

    const LDefinition *tempFloat() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LLoadFixedSlotV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(LoadFixedSlotV);
    BOX_OUTPUT_ACCESSORS();

    LLoadFixedSlotV(const LAllocation &object) {
        setOperand(0, object);
    }
    const MLoadFixedSlot *mir() const {
        return mir_->toLoadFixedSlot();
    }
};


class LLoadFixedSlotT : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(LoadFixedSlotT);

    LLoadFixedSlotT(const LAllocation &object) {
        setOperand(0, object);
    }
    const MLoadFixedSlot *mir() const {
        return mir_->toLoadFixedSlot();
    }
};


class LStoreFixedSlotV : public LInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreFixedSlotV);

    LStoreFixedSlotV(const LAllocation &obj) {
        setOperand(0, obj);
    }

    static const size_t Value = 1;

    const MStoreFixedSlot *mir() const {
        return mir_->toStoreFixedSlot();
    }
    const LAllocation *obj() {
        return getOperand(0);
    }
};


class LStoreFixedSlotT : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(StoreFixedSlotT);

    LStoreFixedSlotT(const LAllocation &obj, const LAllocation &value)
    {
        setOperand(0, obj);
        setOperand(1, value);
    }
    const MStoreFixedSlot *mir() const {
        return mir_->toStoreFixedSlot();
    }
    const LAllocation *obj() {
        return getOperand(0);
    }
    const LAllocation *value() {
        return getOperand(1);
    }
};


class LGetNameCache : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(GetNameCache);
    BOX_OUTPUT_ACCESSORS();

    LGetNameCache(const LAllocation &scopeObj) {
        setOperand(0, scopeObj);
    }
    const LAllocation *scopeObj() {
        return getOperand(0);
    }
    const MGetNameCache *mir() const {
        return mir_->toGetNameCache();
    }
};



class LGetPropertyCacheV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(GetPropertyCacheV);
    BOX_OUTPUT_ACCESSORS();

    LGetPropertyCacheV(const LAllocation &object) {
        setOperand(0, object);
    }
    const MGetPropertyCache *mir() const {
        return mir_->toGetPropertyCache();
    }
};



class LGetPropertyCacheT : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(GetPropertyCacheT);

    LGetPropertyCacheT(const LAllocation &object) {
        setOperand(0, object);
    }
    const MGetPropertyCache *mir() const {
        return mir_->toGetPropertyCache();
    }
};

class LGetElementCacheV : public LInstructionHelper<BOX_PIECES, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(GetElementCacheV);
    BOX_OUTPUT_ACCESSORS();

    static const size_t Index = 1;

    LGetElementCacheV(const LAllocation &object) {
        setOperand(0, object);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const MGetElementCache *mir() const {
        return mir_->toGetElementCache();
    }
};

class LBindNameCache : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(BindNameCache);

    LBindNameCache(const LAllocation &scopeChain) {
        setOperand(0, scopeChain);
    }
    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    const MBindNameCache *mir() const {
        return mir_->toBindNameCache();
    }
};


class LLoadSlotV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(LoadSlotV);
    BOX_OUTPUT_ACCESSORS();

    LLoadSlotV(const LAllocation &in) {
        setOperand(0, in);
    }
    const MLoadSlot *mir() const {
        return mir_->toLoadSlot();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
};




class LLoadSlotT : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(LoadSlotT);

    LLoadSlotT(const LAllocation &in) {
        setOperand(0, in);
    }
    const MLoadSlot *mir() const {
        return mir_->toLoadSlot();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LStoreSlotV : public LInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreSlotV);

    LStoreSlotV(const LAllocation &slots) {
        setOperand(0, slots);
    }

    static const size_t Value = 1;

    const MStoreSlot *mir() const {
        return mir_->toStoreSlot();
    }
    const LAllocation *slots() {
        return getOperand(0);
    }
};







class LStoreSlotT : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(StoreSlotT);

    LStoreSlotT(const LAllocation &slots, const LAllocation &value) {
        setOperand(0, slots);
        setOperand(1, value);
    }
    const MStoreSlot *mir() const {
        return mir_->toStoreSlot();
    }
    const LAllocation *slots() {
        return getOperand(0);
    }
    const LAllocation *value() {
        return getOperand(1);
    }
};


class LStringLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(StringLength);

    LStringLength(const LAllocation &string) {
        setOperand(0, string);
    }

    const LAllocation *string() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};


class LFloor : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Floor);

    LFloor(const LAllocation &num) {
        setOperand(0, num);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MRound *mir() const {
        return mir_->toRound();
    }
};


class LRound : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(Round);

    LRound(const LAllocation &num, const LDefinition &temp) {
        setOperand(0, num);
        setTemp(0, temp);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    MRound *mir() const {
        return mir_->toRound();
    }
};


class LFunctionEnvironment : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(FunctionEnvironment);

    LFunctionEnvironment(const LAllocation &function) {
        setOperand(0, function);
    }
    const LAllocation *function() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LCallGetProperty : public LCallInstructionHelper<BOX_PIECES, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallGetProperty);

    static const size_t Value = 0;

    MCallGetProperty *mir() const {
        return mir_->toCallGetProperty();
    }
};


class LCallGetElement : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallGetElement);
    BOX_OUTPUT_ACCESSORS();

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    MCallGetElement *mir() const {
        return mir_->toCallGetElement();
    }
};


class LCallSetElement : public LCallInstructionHelper<0, 1 + 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallSetElement);
    BOX_OUTPUT_ACCESSORS();

    static const size_t Index = 1;
    static const size_t Value = 1 + BOX_PIECES;
};


class LCallSetProperty : public LCallInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallSetProperty);

    LCallSetProperty(const LAllocation &obj) {
        setOperand(0, obj);
    }

    static const size_t Value = 1;

    const MCallSetProperty *mir() const {
        return mir_->toCallSetProperty();
    }
};

class LCallDeleteProperty : public LCallInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallDeleteProperty);

    static const size_t Value = 0;

    MDeleteProperty *mir() const {
        return mir_->toDeleteProperty();
    }
};



class LSetPropertyCacheV : public LInstructionHelper<0, 1 + BOX_PIECES, 1>
{
  public:
    LIR_HEADER(SetPropertyCacheV);

    LSetPropertyCacheV(const LAllocation &object, const LDefinition &slots) {
        setOperand(0, object);
        setTemp(0, slots);
    }

    static const size_t Value = 1;

    const MSetPropertyCache *mir() const {
        return mir_->toSetPropertyCache();
    }
};



class LSetPropertyCacheT : public LInstructionHelper<0, 2, 1>
{
    MIRType valueType_;

  public:
    LIR_HEADER(SetPropertyCacheT);

    LSetPropertyCacheT(const LAllocation &object, const LDefinition &slots,
                       const LAllocation &value, MIRType valueType)
        : valueType_(valueType)
    {
        setOperand(0, object);
        setOperand(1, value);
        setTemp(0, slots);
    }

    const MSetPropertyCache *mir() const {
        return mir_->toSetPropertyCache();
    }
    MIRType valueType() {
        return valueType_;
    }
};

class LCallIteratorStart : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(CallIteratorStart);

    LCallIteratorStart(const LAllocation &object) {
        setOperand(0, object);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    MIteratorStart *mir() const {
        return mir_->toIteratorStart();
    }
};

class LIteratorStart : public LInstructionHelper<1, 1, 3>
{
  public:
    LIR_HEADER(IteratorStart);

    LIteratorStart(const LAllocation &object, const LDefinition &temp1,
                   const LDefinition &temp2, const LDefinition &temp3) {
        setOperand(0, object);
        setTemp(0, temp1);
        setTemp(1, temp2);
        setTemp(2, temp3);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp1() {
        return getTemp(0);
    }
    const LDefinition *temp2() {
        return getTemp(1);
    }
    const LDefinition *temp3() {
        return getTemp(2);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MIteratorStart *mir() const {
        return mir_->toIteratorStart();
    }
};

class LIteratorNext : public LInstructionHelper<BOX_PIECES, 1, 1>
{
  public:
    LIR_HEADER(IteratorNext);
    BOX_OUTPUT_ACCESSORS();

    LIteratorNext(const LAllocation &iterator, const LDefinition &temp) {
        setOperand(0, iterator);
        setTemp(0, temp);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    MIteratorNext *mir() const {
        return mir_->toIteratorNext();
    }
};

class LIteratorMore : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(IteratorMore);

    LIteratorMore(const LAllocation &iterator, const LDefinition &temp) {
        setOperand(0, iterator);
        setTemp(0, temp);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    MIteratorMore *mir() const {
        return mir_->toIteratorMore();
    }
};

class LIteratorEnd : public LInstructionHelper<0, 1, 2>
{
  public:
    LIR_HEADER(IteratorEnd);

    LIteratorEnd(const LAllocation &iterator, const LDefinition &temp1,
                 const LDefinition &temp2) {
        setOperand(0, iterator);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *temp1() {
        return getTemp(0);
    }
    const LDefinition *temp2() {
        return getTemp(1);
    }
    MIteratorEnd *mir() const {
        return mir_->toIteratorEnd();
    }
};


class LArgumentsLength : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(ArgumentsLength);

    const LDefinition *output() {
        return getDef(0);
    }
};


class LGetArgument : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(GetArgument);
    BOX_OUTPUT_ACCESSORS();

    LGetArgument(const LAllocation &index) {
        setOperand(0, index);
    }
    const LAllocation *index() {
        return getOperand(0);
    }
};


class LTypeBarrier : public LInstructionHelper<BOX_PIECES, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(TypeBarrier);
    BOX_OUTPUT_ACCESSORS();

    LTypeBarrier(const LDefinition &temp) {
        setTemp(0, temp);
    }

    static const size_t Input = 0;

    const MTypeBarrier *mir() const {
        return mir_->toTypeBarrier();
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};


class LMonitorTypes : public LInstructionHelper<0, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(MonitorTypes);

    LMonitorTypes(const LDefinition &temp) {
        setTemp(0, temp);
    }

    static const size_t Input = 0;

    const MMonitorTypes *mir() const {
        return mir_->toMonitorTypes();
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};


class LGuardClass : public LInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(GuardClass);

    LGuardClass(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
    const MGuardClass *mir() const {
        return mir_->toGuardClass();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
};

class MPhi;





class LPhi : public LInstruction
{
    uint32 numInputs_;
    LAllocation *inputs_;
    LDefinition def_;

    bool init(MIRGenerator *gen);

    LPhi(MPhi *mir);

  public:
    LIR_HEADER(Phi);

    static LPhi *New(MIRGenerator *gen, MPhi *phi);

    size_t numDefs() const {
        return 1;
    }
    LDefinition *getDef(size_t index) {
        JS_ASSERT(index == 0);
        return &def_;
    }
    void setDef(size_t index, const LDefinition &def) {
        JS_ASSERT(index == 0);
        def_ = def;
    }
    size_t numOperands() const {
        return numInputs_;
    }
    LAllocation *getOperand(size_t index) {
        JS_ASSERT(index < numOperands());
        return &inputs_[index];
    }
    void setOperand(size_t index, const LAllocation &a) {
        JS_ASSERT(index < numOperands());
        inputs_[index] = a;
    }
    size_t numTemps() const {
        return 0;
    }
    LDefinition *getTemp(size_t index) {
        JS_NOT_REACHED("no temps");
        return NULL;
    }
    void setTemp(size_t index, const LDefinition &temp) {
        JS_NOT_REACHED("no temps");
    }

    virtual void printInfo(FILE *fp) {
        printOperands(fp);
    }
};

class LInstanceOfO : public LInstructionHelper<1, 2, 2>
{
  public:
    LIR_HEADER(InstanceOfO);
    LInstanceOfO(const LAllocation &lhs, const LAllocation &rhs, const LDefinition &temp, const LDefinition &temp2) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp);
        setTemp(1, temp2);
    }

    const LAllocation *lhs() {
        return getOperand(0);
    }
    const LAllocation *rhs() {
        return getOperand(1);
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

class LInstanceOfV : public LInstructionHelper<1, BOX_PIECES+1, 2>
{
  public:
    LIR_HEADER(InstanceOfV);
    LInstanceOfV(const LAllocation &rhs, const LDefinition &temp, const LDefinition &temp2) {
        setOperand(RHS, rhs);
        setTemp(0, temp);
        setTemp(1, temp2);
    }

    const LAllocation *lhs() {
        return getOperand(LHS);
    }
    const LAllocation *rhs() {
        return getOperand(RHS);
    }
    const LDefinition *output() {
        return getDef(0);
    }

    static const size_t LHS = 0;
    static const size_t RHS = BOX_PIECES;
};

} 
} 

#endif 

