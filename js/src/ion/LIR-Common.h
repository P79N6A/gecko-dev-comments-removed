








































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
    gc::Cell *ptr_;

  public:
    LIR_HEADER(Pointer);

    LPointer(gc::Cell *ptr) : ptr_(ptr)
    { }

    gc::Cell *ptr() const {
        return ptr_;
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

class LNewArray : public LCallInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewArray);

    MNewArray *mir() const {
        return mir_->toNewArray();
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

class LDefVar : public LInstructionHelper<0, 1, 1>
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

    
    JSFunction *target_;

  public:
    LIR_HEADER(CallGeneric);

    LCallGeneric(JSFunction *target,
                 const LAllocation &func,
                 uint32 argslot,
                 const LDefinition &nargsreg,
                 const LDefinition &tmpobjreg)
      : argslot_(argslot),
        target_(target)
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

    uint32 nargs() const {
        JS_ASSERT(mir()->argc() >= 1);
        return mir()->argc() - 1; 
    }

    bool hasSingleTarget() const {
        return target_ != NULL;
    }
    JSFunction *getSingleTarget() const {
        JS_ASSERT(hasSingleTarget());
        return target_;
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
    JSFunction *function_;
    uint32 argslot_;

  public:
    LIR_HEADER(CallNative);

    LCallNative(JSFunction *function, uint32 argslot,
                const LDefinition &argJSContext, const LDefinition &argUintN,
                const LDefinition &argVp, const LDefinition &tmpreg)
      : function_(function), argslot_(argslot)
    {
        
        setTemp(0, argJSContext);
        setTemp(1, argUintN);
        setTemp(2, argVp);

        
        setTemp(3, tmpreg);
    }

    JSFunction *function() const {
        return function_;
    }
    uint32 argslot() const {
        return argslot_;
    }
    MCall *mir() const {
        return mir_->toCall();
    }

    
    uint32 nargs() const {
        JS_ASSERT(mir()->argc() >= 1);
        return mir()->argc() - 1; 
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
    JSOp jsop_;

  public:
    LIR_HEADER(CompareD);
    LCompareD(JSOp jsop, const LAllocation &left, const LAllocation &right)
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
};

class LCompareV : public LCallInstructionHelper<1, 2 * BOX_PIECES, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(CompareV);

    LCompareV(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;
};



class LCompareAndBranch : public LInstructionHelper<0, 2, 0>
{
    JSOp jsop_;
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareAndBranch);
    LCompareAndBranch(MCompare *mir, JSOp jsop, const LAllocation &left, const LAllocation &right,
                       MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : jsop_(jsop),
        ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    {
        mir_ = mir;
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
    JSOp jsop_;
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareDAndBranch);
    LCompareDAndBranch(JSOp jsop, const LAllocation &left, const LAllocation &right,
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


class LNotV : public LCallInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(NotV);

    static const size_t Input = 0;
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



class LBitOp : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
    LIR_HEADER(BitOp);

    LBitOp(JSOp op)
      : op_(op)
    { }

    JSOp bitop() {
        return op_;
    }
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
};





class LTruncateDToInt32 : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(TruncateDToInt32);

    LTruncateDToInt32(const LAllocation &in) {
        setOperand(0, in);
    }

    const LAllocation *input() {
        return getOperand(0);
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

class LLambda : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Lambda);

    LLambda(const LAllocation &scopeChain)
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

class LLambdaJoinableForCall : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(LambdaJoinableForCall);

    LLambdaJoinableForCall(const LAllocation &callee, const LAllocation &scopeChain)
    {
        setOperand(0, callee);
        setOperand(1, scopeChain);
    }
    const LAllocation *callee() {
        return getOperand(0);
    }
    const LAllocation *scopeChain() {
        return getOperand(1);
    }
    const MLambdaJoinableForCall *mir() const {
        return mir_->toLambdaJoinableForCall();
    }
};

class LLambdaJoinableForSet : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(LambdaJoinableForSet);

    LLambdaJoinableForSet(const LAllocation &target, const LAllocation &scopeChain)
    {
        setOperand(0, target);
        setOperand(1, scopeChain);
    }
    const LAllocation *target() {
        return getOperand(0);
    }
    const LAllocation *scopeChain() {
        return getOperand(1);
    }
    const MLambdaJoinableForSet *mir() const {
        return mir_->toLambdaJoinableForSet();
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

class LFlatClosureUpvars : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(FlatClosureUpvars);

    LFlatClosureUpvars(const LAllocation &callee) {
        setOperand(0, callee);
    }

    const LAllocation *callee() {
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


class LRound : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Round);

    LRound(const LAllocation &num) {
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

class LCallGetName : public LCallInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(CallGetName);

    MCallGetName *mir() const {
        return mir_->toCallGetName();
    }
};

class LCallGetNameTypeOf : public LCallInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(CallGetNameTypeOf);

    MCallGetNameTypeOf *mir() const {
        return mir_->toCallGetNameTypeOf();
    }
};


class LCallGetElement : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallGetElement);
    BOX_OUTPUT_ACCESSORS();

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;
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
    MIteratorStart *mir() const {
        return mir_->toIteratorStart();
    }
};

class LCallIteratorNext : public LCallInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(CallIteratorNext);
    BOX_OUTPUT_ACCESSORS();

    LCallIteratorNext(const LAllocation &iterator) {
        setOperand(0, iterator);
    }
    MIteratorNext *mir() const {
        return mir_->toIteratorNext();
    }
};

class LCallIteratorMore : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(CallIteratorMore);

    LCallIteratorMore(const LAllocation &iterator) {
        setOperand(0, iterator);
    }
    MIteratorMore *mir() const {
        return mir_->toIteratorMore();
    }
};

class LCallIteratorEnd : public LCallInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(CallIteratorEnd);

    LCallIteratorEnd(const LAllocation &iterator) {
        setOperand(0, iterator);
    }
    MIteratorEnd *mir() const {
        return mir_->toIteratorEnd();
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

} 
} 

#endif 

