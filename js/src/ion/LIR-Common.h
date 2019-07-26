






#ifndef jsion_lir_common_h__
#define jsion_lir_common_h__

#include "ion/shared/Assembler-shared.h"



namespace js {
namespace ion {

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
};



class LLabel : public LInstructionHelper<0, 0, 0>
{
    Label label_;

  public:
    LIR_HEADER(Label)

    Label *label() {
        return &label_;
    }
};

class LNop : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(Nop)
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

    LIR_HEADER(OsiPoint)
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
    LIR_HEADER(MoveGroup)

    void printOperands(FILE *fp);

    
    bool add(LAllocation *from, LAllocation *to);

    
    bool addAfter(LAllocation *from, LAllocation *to);

    size_t numMoves() const {
        return moves_.length();
    }
    const LMove &getMove(size_t i) const {
        return moves_[i];
    }
};


class LInteger : public LInstructionHelper<1, 0, 0>
{
    int32_t i32_;

  public:
    LIR_HEADER(Integer)

    LInteger(int32_t i32)
      : i32_(i32)
    { }

    int32_t getValue() const {
        return i32_;
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
    LIR_HEADER(Pointer)

    LPointer(gc::Cell *ptr)
      : ptr_(ptr), kind_(GC_THING)
    { }

    LPointer(void *ptr, Kind kind)
      : ptr_(ptr), kind_(kind)
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
};


class LValue : public LInstructionHelper<BOX_PIECES, 0, 0>
{
    Value v_;

  public:
    LIR_HEADER(Value)

    LValue(const Value &v)
      : v_(v)
    { }

    Value value() const {
        return v_;
    }
};



class LParameter : public LInstructionHelper<BOX_PIECES, 0, 0>
{
  public:
    LIR_HEADER(Parameter)
};


class LCallee : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(Callee)
};


class LGoto : public LInstructionHelper<0, 0, 0>
{
    MBasicBlock *block_;

  public:
    LIR_HEADER(Goto)

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
    LIR_HEADER(NewSlots)

    LNewSlots(const LDefinition &temp1, const LDefinition &temp2, const LDefinition &temp3) {
        setTemp(0, temp1);
        setTemp(1, temp2);
        setTemp(2, temp3);
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

class LNewParallelArray : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewParallelArray);

    MNewParallelArray *mir() const {
        return mir_->toNewParallelArray();
    }
};

class LNewArray : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewArray)

    MNewArray *mir() const {
        return mir_->toNewArray();
    }
};

class LNewObject : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewObject)

    MNewObject *mir() const {
        return mir_->toNewObject();
    }
};

class LParNew : public LInstructionHelper<1, 1, 2>
{
  public:
    LIR_HEADER(ParNew);

    LParNew(const LAllocation &parSlice,
            const LDefinition &temp1,
            const LDefinition &temp2)
    {
        setOperand(0, parSlice);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }

    MParNew *mir() const {
        return mir_->toParNew();
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LAllocation *getTemp0() {
        return getTemp(0)->output();
    }

    const LAllocation *getTemp1() {
        return getTemp(1)->output();
    }
};

class LParNewDenseArray : public LCallInstructionHelper<1, 2, 3>
{
  public:
    LIR_HEADER(ParNewDenseArray);

    LParNewDenseArray(const LAllocation &parSlice,
                      const LAllocation &length,
                      const LDefinition &temp1,
                      const LDefinition &temp2,
                      const LDefinition &temp3) {
        setOperand(0, parSlice);
        setOperand(1, length);
        setTemp(0, temp1);
        setTemp(1, temp2);
        setTemp(2, temp3);
    }

    MParNewDenseArray *mir() const {
        return mir_->toParNewDenseArray();
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LAllocation *length() {
        return getOperand(1);
    }

    const LAllocation *getTemp0() {
        return getTemp(0)->output();
    }

    const LAllocation *getTemp1() {
        return getTemp(1)->output();
    }

    const LAllocation *getTemp2() {
        return getTemp(2)->output();
    }
};







class LNewDeclEnvObject : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(NewDeclEnvObject);

    MNewDeclEnvObject *mir() const {
        return mir_->toNewDeclEnvObject();
    }
};










class LNewCallObject : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NewCallObject)

    LNewCallObject(const LAllocation &slots) {
        setOperand(0, slots);
    }

    const LAllocation *slots() {
        return getOperand(0);
    }
    MNewCallObject *mir() const {
        return mir_->toNewCallObject();
    }
};

class LParNewCallObject : public LInstructionHelper<1, 2, 2>
{
    LParNewCallObject(const LAllocation &parSlice,
                      const LAllocation &slots,
                      const LDefinition &temp1,
                      const LDefinition &temp2) {
        setOperand(0, parSlice);
        setOperand(1, slots);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }

public:
    LIR_HEADER(ParNewCallObject);

    static LParNewCallObject *NewWithSlots(const LAllocation &parSlice,
                                           const LAllocation &slots,
                                           const LDefinition &temp1,
                                           const LDefinition &temp2) {
        return new LParNewCallObject(parSlice, slots, temp1, temp2);
    }

    static LParNewCallObject *NewSansSlots(const LAllocation &parSlice,
                                           const LDefinition &temp1,
                                           const LDefinition &temp2) {
        LAllocation slots = LConstantIndex::Bogus();
        return new LParNewCallObject(parSlice, slots, temp1, temp2);
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LAllocation *slots() {
        return getOperand(1);
    }

    const bool hasDynamicSlots() {
        
        
        
        
        return slots() && ! slots()->isConstant();
    }

    const MParNewCallObject *mir() const {
        return mir_->toParNewCallObject();
    }

    const LAllocation *getTemp0() {
        return getTemp(0)->output();
    }

    const LAllocation *getTemp1() {
        return getTemp(1)->output();
    }
};

class LNewStringObject : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(NewStringObject)

    LNewStringObject(const LAllocation &input, const LDefinition &temp) {
        setOperand(0, input);
        setTemp(0, temp);
    }

    const LAllocation *input() {
        return getOperand(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    MNewStringObject *mir() const {
        return mir_->toNewStringObject();
    }
};

class LParBailout : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(ParBailout);
};


class LInitProp : public LCallInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(InitProp)

    LInitProp(const LAllocation &object) {
        setOperand(0, object);
    }

    static const size_t ValueIndex = 1;

    const LAllocation *getObject() {
        return getOperand(0);
    }
    const LAllocation *getValue() {
        return getOperand(1);
    }

    MInitProp *mir() const {
        return mir_->toInitProp();
    }
};

class LCheckOverRecursed : public LInstructionHelper<0, 0, 1>
{
  public:
    LIR_HEADER(CheckOverRecursed)

    LCheckOverRecursed(const LDefinition &limitreg)
    {
        setTemp(0, limitreg);
    }

    const LAllocation *limitTemp() {
        return getTemp(0)->output();
    }
};

class LParCheckOverRecursed : public LInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(ParCheckOverRecursed);

    LParCheckOverRecursed(const LAllocation &parSlice,
                          const LDefinition &tempReg)
    {
        setOperand(0, parSlice);
        setTemp(0, tempReg);
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LDefinition *getTempReg() {
        return getTemp(0);
    }
};

class LParCheckInterrupt : public LInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(ParCheckInterrupt);

    LParCheckInterrupt(const LAllocation &parSlice,
                       const LDefinition &tempReg)
    {
        setOperand(0, parSlice);
        setTemp(0, tempReg);
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LDefinition *getTempReg() {
        return getTemp(0);
    }
};

class LDefVar : public LCallInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(DefVar)

    LDefVar(const LAllocation &scopeChain)
    {
        setOperand(0, scopeChain);
    }

    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    MDefVar *mir() const {
        return mir_->toDefVar();
    }
};

class LDefFun : public LCallInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(DefFun)

    LDefFun(const LAllocation &scopeChain)
    {
        setOperand(0, scopeChain);
    }

    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    MDefFun *mir() const {
        return mir_->toDefFun();
    }
};

class LTypeOfV : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(TypeOfV)

    static const size_t Input = 0;

    MTypeOf *mir() const {
        return mir_->toTypeOf();
    }
};

class LToIdV : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(ToIdV)

    static const size_t Object = 0;
    static const size_t Index = BOX_PIECES;

    MToId *mir() const {
        return mir_->toToId();
    }
};



class LCreateThis : public LCallInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(CreateThis)

    LCreateThis(const LAllocation &callee)
    {
        setOperand(0, callee);
    }

    const LAllocation *getCallee() {
        return getOperand(0);
    }

    MCreateThis *mir() const {
        return mir_->toCreateThis();
    }
};



class LCreateThisWithProto : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CreateThisWithProto)

    LCreateThisWithProto(const LAllocation &callee, const LAllocation &prototype)
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

    MCreateThis *mir() const {
        return mir_->toCreateThis();
    }
};



class LCreateThisWithTemplate : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(CreateThisWithTemplate)

    LCreateThisWithTemplate()
    { }

    MCreateThisWithTemplate *mir() const {
        return mir_->toCreateThisWithTemplate();
    }
};



class LReturnFromCtor : public LInstructionHelper<1, BOX_PIECES + 1, 0>
{
  public:
    LIR_HEADER(ReturnFromCtor)

    LReturnFromCtor(const LAllocation &object)
    {
        
        setOperand(LReturnFromCtor::ObjectIndex, object);
    }

    const LAllocation *getObject() {
        return getOperand(LReturnFromCtor::ObjectIndex);
    }

    static const size_t ValueIndex = 0;
    static const size_t ObjectIndex = BOX_PIECES;
};


class LStackArgT : public LInstructionHelper<0, 1, 0>
{
    uint32_t argslot_; 

  public:
    LIR_HEADER(StackArgT)

    LStackArgT(uint32_t argslot, const LAllocation &arg)
      : argslot_(argslot)
    {
        setOperand(0, arg);
    }

    MPassArg *mir() const {
        return this->mir_->toPassArg();
    }
    uint32_t argslot() const {
        return argslot_;
    }
    const LAllocation *getArgument() {
        return getOperand(0);
    }
};


class LStackArgV : public LInstructionHelper<0, BOX_PIECES, 0>
{
    uint32_t argslot_; 

  public:
    LIR_HEADER(StackArgV)

    LStackArgV(uint32_t argslot)
      : argslot_(argslot)
    { }

    uint32_t argslot() const {
        return argslot_;
    }
};


template <size_t Defs, size_t Operands, size_t Temps>
class LJSCallInstructionHelper : public LCallInstructionHelper<Defs, Operands, Temps>
{
    
    
    uint32_t argslot_;

  public:
    LJSCallInstructionHelper(uint32_t argslot)
      : argslot_(argslot)
    { }

    uint32_t argslot() const {
        return argslot_;
    }
    MCall *mir() const {
        return this->mir_->toCall();
    }

    bool hasSingleTarget() const {
        return getSingleTarget() != NULL;
    }
    JSFunction *getSingleTarget() const {
        return mir()->getSingleTarget();
    }

    
    
    
    
    uint32_t numStackArgs() const {
        JS_ASSERT(mir()->numStackArgs() >= 1);
        return mir()->numStackArgs() - 1; 
    }
    
    uint32_t numActualArgs() const {
        return mir()->numActualArgs();
    }

    typedef LJSCallInstructionHelper<Defs, Operands, Temps> JSCallHelper;
};



class LCallGeneric : public LJSCallInstructionHelper<BOX_PIECES, 1, 2>
{
  public:
    LIR_HEADER(CallGeneric)

    LCallGeneric(const LAllocation &func, uint32_t argslot,
                 const LDefinition &nargsreg, const LDefinition &tmpobjreg)
      : JSCallHelper(argslot)
    {
        setOperand(0, func);
        setTemp(0, nargsreg);
        setTemp(1, tmpobjreg);
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


class LCallKnown : public LJSCallInstructionHelper<BOX_PIECES, 1, 1>
{
  public:
    LIR_HEADER(CallKnown)

    LCallKnown(const LAllocation &func, uint32_t argslot, const LDefinition &tmpobjreg)
      : JSCallHelper(argslot)
    {
        setOperand(0, func);
        setTemp(0, tmpobjreg);
    }

    const LAllocation *getFunction() {
        return getOperand(0);
    }
    const LAllocation *getTempObject() {
        return getTemp(0)->output();
    }
};


class LCallNative : public LJSCallInstructionHelper<BOX_PIECES, 0, 4>
{
  public:
    LIR_HEADER(CallNative)

    LCallNative(uint32_t argslot,
                const LDefinition &argJSContext, const LDefinition &argUintN,
                const LDefinition &argVp, const LDefinition &tmpreg)
      : JSCallHelper(argslot)
    {
        
        setTemp(0, argJSContext);
        setTemp(1, argUintN);
        setTemp(2, argVp);

        
        setTemp(3, tmpreg);
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


class LCallDOMNative : public LJSCallInstructionHelper<BOX_PIECES, 0, 5>
{
  public:
    LIR_HEADER(CallDOMNative)

    LCallDOMNative(uint32_t argslot,
                   const LDefinition &argJSContext, const LDefinition &argObj,
                   const LDefinition &argPrivate, const LDefinition &argArgc,
                   const LDefinition &argVp)
      : JSCallHelper(argslot)
    {
        setTemp(0, argJSContext);
        setTemp(1, argObj);
        setTemp(2, argPrivate);
        setTemp(3, argArgc);
        setTemp(4, argVp);
    }

    const LAllocation *getArgJSContext() {
        return getTemp(0)->output();
    }
    const LAllocation *getArgObj() {
        return getTemp(1)->output();
    }
    const LAllocation *getArgPrivate() {
        return getTemp(2)->output();
    }
    const LAllocation *getArgArgc() {
        return getTemp(3)->output();
    }
    const LAllocation *getArgVp() {
        return getTemp(4)->output();
    }
};

template <size_t defs, size_t ops>
class LDOMPropertyInstructionHelper : public LCallInstructionHelper<defs, 1 + ops, 3>
{
  protected:
    LDOMPropertyInstructionHelper(const LDefinition &JSContextReg, const LAllocation &ObjectReg,
                                  const LDefinition &PrivReg, const LDefinition &ValueReg)
    {
        this->setOperand(0, ObjectReg);
        this->setTemp(0, JSContextReg);
        this->setTemp(1, PrivReg);
        this->setTemp(2, ValueReg);
    }

  public:
    const LAllocation *getJSContextReg() {
        return this->getTemp(0)->output();
    }
    const LAllocation *getObjectReg() {
        return this->getOperand(0);
    }
    const LAllocation *getPrivReg() {
        return this->getTemp(1)->output();
    }
    const LAllocation *getValueReg() {
        return this->getTemp(2)->output();
    }
};


class LGetDOMProperty : public LDOMPropertyInstructionHelper<BOX_PIECES, 0>
{
  public:
    LIR_HEADER(GetDOMProperty)

    LGetDOMProperty(const LDefinition &JSContextReg, const LAllocation &ObjectReg,
                    const LDefinition &PrivReg, const LDefinition &ValueReg)
      : LDOMPropertyInstructionHelper<BOX_PIECES, 0>(JSContextReg, ObjectReg,
                                                     PrivReg, ValueReg)
    { }

    MGetDOMProperty *mir() const {
        return mir_->toGetDOMProperty();
    }
};

class LSetDOMProperty : public LDOMPropertyInstructionHelper<0, BOX_PIECES>
{
  public:
    LIR_HEADER(SetDOMProperty)

    LSetDOMProperty(const LDefinition &JSContextReg, const LAllocation &ObjectReg,
                    const LDefinition &PrivReg, const LDefinition &ValueReg)
      : LDOMPropertyInstructionHelper<0, BOX_PIECES>(JSContextReg, ObjectReg,
                                                     PrivReg, ValueReg)
    { }

    static const size_t Value = 1;

    MSetDOMProperty *mir() const {
        return mir_->toSetDOMProperty();
    }
};



class LApplyArgsGeneric : public LCallInstructionHelper<BOX_PIECES, BOX_PIECES + 2, 2>
{
  public:
    LIR_HEADER(ApplyArgsGeneric)

    LApplyArgsGeneric(const LAllocation &func, const LAllocation &argc,
                      const LDefinition &tmpobjreg, const LDefinition &tmpcopy)
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
    LIR_HEADER(TestIAndBranch)

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
};


class LTestDAndBranch : public LInstructionHelper<0, 1, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(TestDAndBranch)

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
};



class LTestOAndBranch : public LInstructionHelper<0, 1, 1>
{
    MBasicBlock *ifTruthy_;
    MBasicBlock *ifFalsy_;

  public:
    LIR_HEADER(TestOAndBranch)

    LTestOAndBranch(const LAllocation &input, MBasicBlock *ifTruthy, MBasicBlock *ifFalsy,
                    const LDefinition &temp)
      : ifTruthy_(ifTruthy),
        ifFalsy_(ifFalsy)
    {
        setOperand(0, input);
        setTemp(0, temp);
    }

    const LDefinition *temp() {
        return getTemp(0);
    }

    Label *ifTruthy() {
        return ifTruthy_->lir()->label();
    }
    Label *ifFalsy() {
        return ifFalsy_->lir()->label();
    }

    MTest *mir() {
        return mir_->toTest();
    }
};


class LTestVAndBranch : public LInstructionHelper<0, BOX_PIECES, 3>
{
    MBasicBlock *ifTruthy_;
    MBasicBlock *ifFalsy_;

  public:
    LIR_HEADER(TestVAndBranch)

    LTestVAndBranch(MBasicBlock *ifTruthy, MBasicBlock *ifFalsy, const LDefinition &temp0,
                    const LDefinition &temp1, const LDefinition &temp2)
      : ifTruthy_(ifTruthy),
        ifFalsy_(ifFalsy)
    {
        setTemp(0, temp0);
        setTemp(1, temp1);
        setTemp(2, temp2);
    }

    static const size_t Input = 0;

    const LAllocation *tempFloat() {
        return getTemp(0)->output();
    }

    const LDefinition *temp1() {
        return getTemp(1);
    }

    const LDefinition *temp2() {
        return getTemp(2);
    }

    Label *ifTruthy() {
        return ifTruthy_->lir()->label();
    }
    Label *ifFalsy() {
        return ifFalsy_->lir()->label();
    }

    MTest *mir() {
        return mir_->toTest();
    }
};

class LPolyInlineDispatch : public LInstructionHelper<0, 1, 1>
{
  
  public:
    LIR_HEADER(PolyInlineDispatch)

    LPolyInlineDispatch(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
 
    const LDefinition *temp() {
        return getTemp(0);
    }

    MPolyInlineDispatch *mir() {
        return mir_->toPolyInlineDispatch();
    }
};



class LCompare : public LInstructionHelper<1, 2, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(Compare)
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
    MCompare *mir() {
        return mir_->toCompare();
    }
};



class LCompareAndBranch : public LInstructionHelper<0, 2, 0>
{
    JSOp jsop_;
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareAndBranch)
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

class LCompareD : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CompareD)
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
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LCompareDAndBranch : public LInstructionHelper<0, 2, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareDAndBranch)
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

class LCompareS : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(CompareS)
    LCompareS(const LAllocation &left, const LAllocation &right,
              const LDefinition &temp) {
        setOperand(0, left);
        setOperand(1, right);
        setTemp(0, temp);
    }

    const LAllocation *left() {
        return getOperand(0);
    }
    const LAllocation *right() {
        return getOperand(1);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};


class LCompareStrictS : public LInstructionHelper<1, BOX_PIECES + 1, 2>
{
  public:
    LIR_HEADER(CompareStrictS)
    LCompareStrictS(const LAllocation &rhs, const LDefinition &temp0,
                    const LDefinition &temp1) {
        setOperand(BOX_PIECES, rhs);
        setTemp(0, temp0);
        setTemp(1, temp1);
    }

    static const size_t Lhs = 0;

    const LAllocation *right() {
        return getOperand(BOX_PIECES);
    }
    const LDefinition *temp0() {
        return getTemp(0);
    }
    const LDefinition *temp1() {
        return getTemp(1);
    }
    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LParCompareS : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(ParCompareS);

    LParCompareS(const LAllocation &left, const LAllocation &right) {
        setOperand(0, left);
        setOperand(1, right);
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
    LIR_HEADER(CompareB)

    LCompareB(const LAllocation &rhs) {
        setOperand(BOX_PIECES, rhs);
    }

    static const size_t Lhs = 0;

    const LAllocation *rhs() {
        return getOperand(BOX_PIECES);
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
    LIR_HEADER(CompareBAndBranch)

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

class LCompareV : public LInstructionHelper<1, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CompareV)

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    MCompare *mir() const {
        return mir_->toCompare();
    }
};

class LCompareVAndBranch : public LInstructionHelper<0, 2 * BOX_PIECES, 0>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(CompareVAndBranch)

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    LCompareVAndBranch(MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue_(ifTrue),
        ifFalse_(ifFalse)
    { }

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

class LCompareVM : public LCallInstructionHelper<1, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CompareVM)

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    MCompare *mir() const {
        return mir_->toCompare();
    }
};

class LIsNullOrLikeUndefined : public LInstructionHelper<1, BOX_PIECES, 2>
{
  public:
    LIR_HEADER(IsNullOrLikeUndefined)

    LIsNullOrLikeUndefined(const LDefinition &temp0, const LDefinition &temp1)
    {
        setTemp(0, temp0);
        setTemp(1, temp1);
    }

    static const size_t Value = 0;

    MCompare *mir() {
        return mir_->toCompare();
    }

    const LDefinition *temp0() {
        return getTemp(0);
    }

    const LDefinition *temp1() {
        return getTemp(1);
    }
};

class LIsNullOrLikeUndefinedAndBranch : public LInstructionHelper<0, BOX_PIECES, 2>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(IsNullOrLikeUndefinedAndBranch)

    LIsNullOrLikeUndefinedAndBranch(MBasicBlock *ifTrue, MBasicBlock *ifFalse, const LDefinition &temp0, const LDefinition &temp1)
      : ifTrue_(ifTrue), ifFalse_(ifFalse)
    {
        setTemp(0, temp0);
        setTemp(1, temp1);
    }

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
    const LDefinition *temp0() {
        return getTemp(0);
    }
    const LDefinition *temp1() {
        return getTemp(1);
    }
};




class LEmulatesUndefined : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(EmulatesUndefined)

    LEmulatesUndefined(const LAllocation &input)
    {
        setOperand(0, input);
    }

    MCompare *mir() {
        return mir_->toCompare();
    }
};

class LEmulatesUndefinedAndBranch : public LInstructionHelper<0, 1, 1>
{
    MBasicBlock *ifTrue_;
    MBasicBlock *ifFalse_;

  public:
    LIR_HEADER(EmulatesUndefinedAndBranch)

    LEmulatesUndefinedAndBranch(const LAllocation &input, MBasicBlock *ifTrue, MBasicBlock *ifFalse, const LDefinition &temp)
      : ifTrue_(ifTrue), ifFalse_(ifFalse)
    {
        setOperand(0, input);
        setTemp(0, temp);
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
    const LDefinition *temp() {
        return getTemp(0);
    }
};


class LNotI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NotI)

    LNotI(const LAllocation &input) {
        setOperand(0, input);
    }
};


class LNotD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NotD)

    LNotD(const LAllocation &input) {
        setOperand(0, input);
    }
};


class LNotO : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NotO)

    LNotO(const LAllocation &input)
    {
        setOperand(0, input);
    }

    MNot *mir() {
        return mir_->toNot();
    }
};


class LNotV : public LInstructionHelper<1, BOX_PIECES, 3>
{
  public:
    LIR_HEADER(NotV)

    static const size_t Input = 0;
    LNotV(const LDefinition &temp0, const LDefinition &temp1, const LDefinition &temp2)
    {
        setTemp(0, temp0);
        setTemp(1, temp1);
        setTemp(2, temp2);
    }

    const LAllocation *tempFloat() {
        return getTemp(0)->output();
    }

    const LDefinition *temp1() {
        return getTemp(1);
    }

    const LDefinition *temp2() {
        return getTemp(2);
    }

    MNot *mir() {
        return mir_->toNot();
    }
};



class LBitNotI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(BitNotI)
};


class LBitNotV : public LCallInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(BitNotV)

    static const size_t Input = 0;
};



class LBitOpI : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
    LIR_HEADER(BitOpI)

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
    LIR_HEADER(BitOpV)

    LBitOpV(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;
};



class LShiftI : public LBinaryMath<0>
{
    JSOp op_;

  public:
    LIR_HEADER(ShiftI)

    LShiftI(JSOp op)
      : op_(op)
    { }

    JSOp bitop() {
        return op_;
    }

    MInstruction *mir() {
        return mir_->toInstruction();
    }
};

class LUrshD : public LBinaryMath<1>
{
  public:
    LIR_HEADER(UrshD)

    LUrshD(const LAllocation &lhs, const LAllocation &rhs, const LDefinition &temp) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};



class LReturn : public LInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(Return)
};

class LThrow : public LCallInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(Throw)

    static const size_t Value = 0;
};

class LMinMaxI : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(MinMaxI)
    LMinMaxI(const LAllocation &first, const LAllocation &second)
    {
        setOperand(0, first);
        setOperand(1, second);
    }

    const LAllocation *first() {
        return this->getOperand(0);
    }
    const LAllocation *second() {
        return this->getOperand(1);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
    MMinMax *mir() const {
        return mir_->toMinMax();
    }
};

class LMinMaxD : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(MinMaxD)
    LMinMaxD(const LAllocation &first, const LAllocation &second) 
    {
        setOperand(0, first);
        setOperand(1, second);
    }

    const LAllocation *first() {
        return this->getOperand(0);
    }
    const LAllocation *second() {
        return this->getOperand(1);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
    MMinMax *mir() const {
        return mir_->toMinMax();
    }
};


class LNegD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(NegD)
    LNegD(const LAllocation &num) {
        setOperand(0, num);
    }
};


class LAbsI : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(AbsI)
    LAbsI(const LAllocation &num) {
        setOperand(0, num);
    }
};


class LAbsD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(AbsD)
    LAbsD(const LAllocation &num) {
        setOperand(0, num);
    }
};


class LSqrtD : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(SqrtD)
    LSqrtD(const LAllocation &num) {
        setOperand(0, num);
    }
};


class LPowI : public LCallInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(PowI)
    LPowI(const LAllocation &value, const LAllocation &power, const LDefinition &temp) {
        setOperand(0, value);
        setOperand(1, power);
        setTemp(0, temp);
    }

    const LAllocation *value() {
        return getOperand(0);
    }
    const LAllocation *power() {
        return getOperand(1);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};


class LPowD : public LCallInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(PowD)
    LPowD(const LAllocation &value, const LAllocation &power, const LDefinition &temp) {
        setOperand(0, value);
        setOperand(1, power);
        setTemp(0, temp);
    }

    const LAllocation *value() {
        return getOperand(0);
    }
    const LAllocation *power() {
        return getOperand(1);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};


class LRandom : public LCallInstructionHelper<1, 0, 2>
{
  public:
    LIR_HEADER(Random)
    LRandom(const LDefinition &temp, const LDefinition &temp2) {
        setTemp(0, temp);
        setTemp(1, temp2);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    const LDefinition *temp2() {
        return getTemp(1);
    }
};

class LMathFunctionD : public LCallInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(MathFunctionD)
    LMathFunctionD(const LAllocation &input, const LDefinition &temp) {
        setOperand(0, input);
        setTemp(0, temp);
    }

    const LDefinition *temp() {
        return getTemp(0);
    }
    MMathFunction *mir() const {
        return mir_->toMathFunction();
    }
};


class LAddI : public LBinaryMath<0>
{
    bool recoversInput_;

  public:
    LIR_HEADER(AddI)

    LAddI()
      : recoversInput_(false)
    { }

    virtual bool recoversInput() const {
        return recoversInput_;
    }
    void setRecoversInput() {
        recoversInput_ = true;
    }
};


class LSubI : public LBinaryMath<0>
{
    bool recoversInput_;

  public:
    LIR_HEADER(SubI)

    LSubI()
      : recoversInput_(false)
    { }

    virtual bool recoversInput() const {
        return recoversInput_;
    }
    void setRecoversInput() {
        recoversInput_ = true;
    }
};


class LMathD : public LBinaryMath<0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(MathD)

    LMathD(JSOp jsop)
      : jsop_(jsop)
    { }

    JSOp jsop() const {
        return jsop_;
    }
};

class LModD : public LBinaryMath<1>
{
  public:
    LIR_HEADER(ModD)

    LModD(const LAllocation &lhs, const LAllocation &rhs, const LDefinition &temp) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
    bool isCall() const {
        return true;
    }
};


class LBinaryV : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
    JSOp jsop_;

  public:
    LIR_HEADER(BinaryV)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(Concat)

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
};


class LCharCodeAt : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(CharCodeAt)

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
};


class LFromCharCode : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(FromCharCode)

    LFromCharCode(const LAllocation &code) {
        setOperand(0, code);
    }

    const LAllocation *code() {
        return this->getOperand(0);
    }
};


class LInt32ToDouble : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Int32ToDouble)

    LInt32ToDouble(const LAllocation &input) {
        setOperand(0, input);
    }
};


class LValueToDouble : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(ValueToDouble)
    static const size_t Input = 0;
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
    LIR_HEADER(ValueToInt32)

    LValueToInt32(const LDefinition &temp, Mode mode)
      : mode_(mode)
    {
        setTemp(0, temp);
    }

    static const size_t Input = 0;

    Mode mode() const {
        return mode_;
    }
    const LDefinition *tempFloat() {
        return getTemp(0);
    }
    MToInt32 *mir() const {
        return mir_->toToInt32();
    }
};





class LDoubleToInt32 : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(DoubleToInt32)

    LDoubleToInt32(const LAllocation &in) {
        setOperand(0, in);
    }

    MToInt32 *mir() const {
        return mir_->toToInt32();
    }
};





class LTruncateDToInt32 : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(TruncateDToInt32)

    LTruncateDToInt32(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }

    const LDefinition *tempFloat() {
        return getTemp(0);
    }
};



class LIntToString : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(IntToString)

    LIntToString(const LAllocation &input) {
        setOperand(0, input);
    }

    const MToString *mir() {
        return mir_->toToString();
    }
};




class LStart : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(Start)
};



class LOsrEntry : public LInstructionHelper<1, 0, 0>
{
  protected:
    Label label_;
    uint32_t frameDepth_;

  public:
    LIR_HEADER(OsrEntry)

    LOsrEntry()
      : frameDepth_(0)
    { }

    void setFrameDepth(uint32_t depth) {
        frameDepth_ = depth;
    }
    uint32_t getFrameDepth() {
        return frameDepth_;
    }
    Label *label() {
        return &label_;
    }

};


class LOsrValue : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(OsrValue)

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
    LIR_HEADER(OsrScopeChain)

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
    LIR_HEADER(RegExp)

    const MRegExp *mir() const {
        return mir_->toRegExp();
    }
};

class LRegExpTest : public LCallInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(RegExpTest)

    LRegExpTest(const LAllocation &regexp, const LAllocation &string)
    {
        setOperand(0, regexp);
        setOperand(1, string);
    }

    const LAllocation *regexp() {
        return getOperand(0);
    }
    const LAllocation *string() {
        return getOperand(1);
    }

    const MRegExpTest *mir() const {
        return mir_->toRegExpTest();
    }
};

class LLambdaForSingleton : public LCallInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(LambdaForSingleton)

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
    LIR_HEADER(Lambda)

    LLambda(const LAllocation &scopeChain) {
        setOperand(0, scopeChain);
    }
    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    const MLambda *mir() const {
        return mir_->toLambda();
    }
};

class LParLambda : public LInstructionHelper<1, 2, 2>
{
  public:
    LIR_HEADER(ParLambda);

    LParLambda(const LAllocation &parSlice,
               const LAllocation &scopeChain,
               const LDefinition &temp1,
               const LDefinition &temp2) {
        setOperand(0, parSlice);
        setOperand(1, scopeChain);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }
    const LAllocation *parSlice() {
        return getOperand(0);
    }
    const LAllocation *scopeChain() {
        return getOperand(1);
    }
    const MParLambda *mir() const {
        return mir_->toParLambda();
    }
    const LAllocation *getTemp0() {
        return getTemp(0)->output();
    }
    const LAllocation *getTemp1() {
        return getTemp(1)->output();
    }
};


class LImplicitThis : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(ImplicitThis)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(Slots)

    LSlots(const LAllocation &object) {
        setOperand(0, object);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
};




class LElements : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Elements)

    LElements(const LAllocation &object) {
        setOperand(0, object);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
};


class LConvertElementsToDoubles : public LInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(ConvertElementsToDoubles)

    LConvertElementsToDoubles(const LAllocation &elements) {
        setOperand(0, elements);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
};


class LInitializedLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(InitializedLength)

    LInitializedLength(const LAllocation &elements) {
        setOperand(0, elements);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
};


class LSetInitializedLength : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(SetInitializedLength)

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
    LIR_HEADER(ArrayLength)

    LArrayLength(const LAllocation &elements) {
        setOperand(0, elements);
    }

    const LAllocation *elements() {
        return getOperand(0);
    }
};


class LTypedArrayLength : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(TypedArrayLength)

    LTypedArrayLength(const LAllocation &obj) {
        setOperand(0, obj);
    }

    const LAllocation *object() {
        return getOperand(0);
    }
};


class LTypedArrayElements : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(TypedArrayElements)

    LTypedArrayElements(const LAllocation &object) {
        setOperand(0, object);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
};


class LBoundsCheck : public LInstructionHelper<0, 2, 0>
{
  public:
    LIR_HEADER(BoundsCheck)

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
    LIR_HEADER(BoundsCheckRange)

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
    LIR_HEADER(BoundsCheckLower)

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
    LIR_HEADER(LoadElementV)
    BOX_OUTPUT_ACCESSORS()

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

class LInArray : public LInstructionHelper<1, 3, 0>
{
  public:
    LIR_HEADER(InArray)

    LInArray(const LAllocation &elements, const LAllocation &index, const LAllocation &initLength) {
        setOperand(0, elements);
        setOperand(1, index);
        setOperand(2, initLength);
    }
    const MInArray *mir() const {
        return mir_->toInArray();
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



class LLoadElementHole : public LInstructionHelper<BOX_PIECES, 3, 0>
{
  public:
    LIR_HEADER(LoadElementHole)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(LoadElementT)

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
};


class LStoreElementV : public LInstructionHelper<0, 2 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreElementV)

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
    LIR_HEADER(StoreElementT)

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
    LIR_HEADER(StoreElementHoleV)

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
    LIR_HEADER(StoreElementHoleT)

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
    LIR_HEADER(ArrayPopShiftV)

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
    LIR_HEADER(ArrayPopShiftT)

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
};

class LArrayPushV : public LInstructionHelper<1, 1 + BOX_PIECES, 1>
{
  public:
    LIR_HEADER(ArrayPushV)

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
};

class LArrayPushT : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(ArrayPushT)

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
};

class LArrayConcat : public LCallInstructionHelper<1, 2, 2>
{
  public:
    LIR_HEADER(ArrayConcat)

    LArrayConcat(const LAllocation &lhs, const LAllocation &rhs,
                 const LDefinition &temp1, const LDefinition &temp2) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }
    const MArrayConcat *mir() const {
        return mir_->toArrayConcat();
    }
    const LAllocation *lhs() {
        return getOperand(0);
    }
    const LAllocation *rhs() {
        return getOperand(1);
    }
    const LDefinition *temp1() {
        return getTemp(0);
    }
    const LDefinition *temp2() {
        return getTemp(1);
    }
};


class LLoadTypedArrayElement : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(LoadTypedArrayElement)

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
};

class LLoadTypedArrayElementHole : public LInstructionHelper<BOX_PIECES, 2, 0>
{
  public:
    LIR_HEADER(LoadTypedArrayElementHole)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(StoreTypedArrayElement)

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
    LIR_HEADER(ClampIToUint8)

    LClampIToUint8(const LAllocation &in) {
        setOperand(0, in);
    }
};

class LClampDToUint8 : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(ClampDToUint8)

    LClampDToUint8(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
};

class LClampVToUint8 : public LInstructionHelper<1, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(ClampVToUint8)

    LClampVToUint8(const LDefinition &tempFloat) {
        setTemp(0, tempFloat);
    }

    static const size_t Input = 0;

    const LDefinition *tempFloat() {
        return getTemp(0);
    }
};


class LLoadFixedSlotV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(LoadFixedSlotV)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(LoadFixedSlotT)

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
    LIR_HEADER(StoreFixedSlotV)

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
    LIR_HEADER(StoreFixedSlotT)

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
    LIR_HEADER(GetNameCache)
    BOX_OUTPUT_ACCESSORS()

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

class LCallGetIntrinsicValue : public LCallInstructionHelper<BOX_PIECES, 0, 0>
{
  public:
    LIR_HEADER(CallGetIntrinsicValue)
    BOX_OUTPUT_ACCESSORS()

    const MCallGetIntrinsicValue *mir() const {
        return mir_->toCallGetIntrinsicValue();
    }
};

class LCallsiteCloneCache : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(CallsiteCloneCache);

    LCallsiteCloneCache(const LAllocation &callee) {
        setOperand(0, callee);
    }
    const LAllocation *callee() {
        return getOperand(0);
    }
    const MCallsiteCloneCache *mir() const {
        return mir_->toCallsiteCloneCache();
    }
};



class LGetPropertyCacheV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(GetPropertyCacheV)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(GetPropertyCacheT)

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
    LIR_HEADER(GetElementCacheV)
    BOX_OUTPUT_ACCESSORS()

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

class LGetElementCacheT : public LInstructionHelper<1, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(GetElementCacheT)

    static const size_t Index = 1;

    LGetElementCacheT(const LAllocation &object) {
        setOperand(0, object);
    }
    const LAllocation *object() {
        return getOperand(0);
    }
    const LDefinition *output() {
        return getDef(0);
    }
    const MGetElementCache *mir() const {
        return mir_->toGetElementCache();
    }
};

class LBindNameCache : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(BindNameCache)

    LBindNameCache(const LAllocation &scopeChain) {
        setOperand(0, scopeChain);
    }
    const LAllocation *scopeChain() {
        return getOperand(0);
    }
    const MBindNameCache *mir() const {
        return mir_->toBindNameCache();
    }
};


class LLoadSlotV : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(LoadSlotV)
    BOX_OUTPUT_ACCESSORS()

    LLoadSlotV(const LAllocation &in) {
        setOperand(0, in);
    }
    const MLoadSlot *mir() const {
        return mir_->toLoadSlot();
    }
};




class LLoadSlotT : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(LoadSlotT)

    LLoadSlotT(const LAllocation &in) {
        setOperand(0, in);
    }
    const MLoadSlot *mir() const {
        return mir_->toLoadSlot();
    }
};


class LStoreSlotV : public LInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(StoreSlotV)

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
    LIR_HEADER(StoreSlotT)

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
    LIR_HEADER(StringLength)

    LStringLength(const LAllocation &string) {
        setOperand(0, string);
    }

    const LAllocation *string() {
        return getOperand(0);
    }
};


class LFloor : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Floor)

    LFloor(const LAllocation &num) {
        setOperand(0, num);
    }

    MRound *mir() const {
        return mir_->toRound();
    }
};


class LRound : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(Round)

    LRound(const LAllocation &num, const LDefinition &temp) {
        setOperand(0, num);
        setTemp(0, temp);
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
    LIR_HEADER(FunctionEnvironment)

    LFunctionEnvironment(const LAllocation &function) {
        setOperand(0, function);
    }
    const LAllocation *function() {
        return getOperand(0);
    }
};

class LParSlice : public LCallInstructionHelper<1, 0, 1>
{
  public:
    LIR_HEADER(ParSlice);

    LParSlice(const LDefinition &temp1) {
        setTemp(0, temp1);
    }

    const LAllocation *getTempReg() {
        return getTemp(0)->output();
    }
};

class LCallGetProperty : public LCallInstructionHelper<BOX_PIECES, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallGetProperty)

    static const size_t Value = 0;

    MCallGetProperty *mir() const {
        return mir_->toCallGetProperty();
    }
};


class LCallGetElement : public LCallInstructionHelper<BOX_PIECES, 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallGetElement)
    BOX_OUTPUT_ACCESSORS()

    static const size_t LhsInput = 0;
    static const size_t RhsInput = BOX_PIECES;

    MCallGetElement *mir() const {
        return mir_->toCallGetElement();
    }
};


class LCallSetElement : public LCallInstructionHelper<0, 1 + 2 * BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallSetElement)
    BOX_OUTPUT_ACCESSORS()

    static const size_t Index = 1;
    static const size_t Value = 1 + BOX_PIECES;
};


class LCallSetProperty : public LCallInstructionHelper<0, 1 + BOX_PIECES, 0>
{
  public:
    LIR_HEADER(CallSetProperty)

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
    LIR_HEADER(CallDeleteProperty)

    static const size_t Value = 0;

    MDeleteProperty *mir() const {
        return mir_->toDeleteProperty();
    }
};



class LSetPropertyCacheV : public LInstructionHelper<0, 1 + BOX_PIECES, 1>
{
  public:
    LIR_HEADER(SetPropertyCacheV)

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
    LIR_HEADER(SetPropertyCacheT)

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
    LIR_HEADER(CallIteratorStart)

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
    LIR_HEADER(IteratorStart)

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
    MIteratorStart *mir() const {
        return mir_->toIteratorStart();
    }
};

class LIteratorNext : public LInstructionHelper<BOX_PIECES, 1, 1>
{
  public:
    LIR_HEADER(IteratorNext)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(IteratorMore)

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
    MIteratorMore *mir() const {
        return mir_->toIteratorMore();
    }
};

class LIteratorEnd : public LInstructionHelper<0, 1, 3>
{
  public:
    LIR_HEADER(IteratorEnd)

    LIteratorEnd(const LAllocation &iterator, const LDefinition &temp1,
                 const LDefinition &temp2, const LDefinition &temp3) {
        setOperand(0, iterator);
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
    MIteratorEnd *mir() const {
        return mir_->toIteratorEnd();
    }
};


class LArgumentsLength : public LInstructionHelper<1, 0, 0>
{
  public:
    LIR_HEADER(ArgumentsLength)
};


class LGetArgument : public LInstructionHelper<BOX_PIECES, 1, 0>
{
  public:
    LIR_HEADER(GetArgument)
    BOX_OUTPUT_ACCESSORS()

    LGetArgument(const LAllocation &index) {
        setOperand(0, index);
    }
    const LAllocation *index() {
        return getOperand(0);
    }
};

class LParWriteGuard : public LCallInstructionHelper<0, 2, 1>
{
  public:
    LIR_HEADER(ParWriteGuard);

    LParWriteGuard(const LAllocation &parSlice,
                   const LAllocation &object,
                   const LDefinition &temp1) {
        setOperand(0, parSlice);
        setOperand(1, object);
        setTemp(0, temp1);
    }

    bool isCall() const {
        return true;
    }

    const LAllocation *parSlice() {
        return getOperand(0);
    }

    const LAllocation *object() {
        return getOperand(1);
    }

    const LAllocation *getTempReg() {
        return getTemp(0)->output();
    }
};

class LParDump : public LCallInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(ParDump);

    static const size_t Value = 0;

    const LAllocation *value() {
        return getOperand(0);
    }
};


class LTypeBarrier : public LInstructionHelper<BOX_PIECES, BOX_PIECES, 1>
{
  public:
    LIR_HEADER(TypeBarrier)
    BOX_OUTPUT_ACCESSORS()

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
    LIR_HEADER(MonitorTypes)

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
    LIR_HEADER(GuardClass)

    LGuardClass(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
    const MGuardClass *mir() const {
        return mir_->toGuardClass();
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
};

class MPhi;





class LPhi : public LInstruction
{
    uint32_t numInputs_;
    LAllocation *inputs_;
    LDefinition def_;

    bool init(MIRGenerator *gen);

    LPhi(MPhi *mir);

  public:
    LIR_HEADER(Phi)

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

class LIn : public LCallInstructionHelper<1, BOX_PIECES+1, 0>
{
  public:
    LIR_HEADER(In)
    LIn(const LAllocation &rhs) {
        setOperand(RHS, rhs);
    }

    const LAllocation *lhs() {
        return getOperand(LHS);
    }
    const LAllocation *rhs() {
        return getOperand(RHS);
    }

    static const size_t LHS = 0;
    static const size_t RHS = BOX_PIECES;
};

class LInstanceOfO : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(InstanceOfO)
    LInstanceOfO(const LAllocation &lhs) {
        setOperand(0, lhs);
    }

    MInstanceOf *mir() const {
        return mir_->toInstanceOf();
    }

    const LAllocation *lhs() {
        return getOperand(0);
    }
};

class LInstanceOfV : public LInstructionHelper<1, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(InstanceOfV)
    LInstanceOfV() {
    }

    MInstanceOf *mir() const {
        return mir_->toInstanceOf();
    }

    const LAllocation *lhs() {
        return getOperand(LHS);
    }

    static const size_t LHS = 0;
};

class LCallInstanceOf : public LCallInstructionHelper<1, BOX_PIECES+1, 0>
{
  public:
    LIR_HEADER(CallInstanceOf)
    LCallInstanceOf(const LAllocation &rhs) {
        setOperand(RHS, rhs);
    }

    const LDefinition *output() {
        return this->getDef(0);
    }
    const LAllocation *lhs() {
        return getOperand(LHS);
    }
    const LAllocation *rhs() {
        return getOperand(RHS);
    }

    static const size_t LHS = 0;
    static const size_t RHS = BOX_PIECES;
};

class LFunctionBoundary : public LInstructionHelper<0, 0, 1>
{
  public:
    LIR_HEADER(FunctionBoundary)

    LFunctionBoundary(const LDefinition &temp) {
        setTemp(0, temp);
    }

    const LDefinition *temp() {
        return getTemp(0);
    }

    UnrootedScript script() {
        return mir_->toFunctionBoundary()->script();
    }

    MFunctionBoundary::Type type() {
        return mir_->toFunctionBoundary()->type();
    }

    unsigned inlineLevel() {
        return mir_->toFunctionBoundary()->inlineLevel();
    }
};


} 
} 

#endif 

