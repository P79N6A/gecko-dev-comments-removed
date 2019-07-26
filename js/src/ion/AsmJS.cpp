





#include "ion/AsmJS.h"

#include "mozilla/Move.h"

#include "jsmath.h"
#include "jsworkers.h"
#include "jswrapper.h"
#include "prmjtime.h"

#ifdef MOZ_VTUNE
# include "jitprofiling.h"
#endif

#include "frontend/Parser.h"
#include "ion/AsmJSModule.h"
#include "ion/PerfSpewer.h"
#include "ion/CodeGenerator.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"

#include "jsfuninlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"

#ifdef MOZ_VTUNE
# include "jitprofiling.h"
#endif

using namespace js;
using namespace js::frontend;
using namespace js::ion;

using mozilla::AddToHash;
using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::HashGeneric;
using mozilla::IsNegativeZero;
using mozilla::Maybe;
using mozilla::Move;
using mozilla::MoveRef;

static const size_t LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12;




static inline ParseNode *
NextNode(ParseNode *pn)
{
    return pn->pn_next;
}

static inline ParseNode *
UnaryKid(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_UNARY));
    return pn->pn_kid;
}

static inline ParseNode *
ReturnExpr(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_RETURN));
    return UnaryKid(pn);
}

static inline ParseNode *
BinaryRight(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_BINARY));
    return pn->pn_right;
}

static inline ParseNode *
BinaryLeft(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_BINARY));
    return pn->pn_left;
}

static inline ParseNode *
TernaryKid1(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_TERNARY));
    return pn->pn_kid1;
}

static inline ParseNode *
TernaryKid2(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_TERNARY));
    return pn->pn_kid2;
}

static inline ParseNode *
TernaryKid3(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_TERNARY));
    return pn->pn_kid3;
}

static inline ParseNode *
ListHead(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    return pn->pn_head;
}

static inline unsigned
ListLength(ParseNode *pn)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    return pn->pn_count;
}

static inline ParseNode *
CallCallee(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_CALL));
    return ListHead(pn);
}

static inline unsigned
CallArgListLength(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_CALL));
    JS_ASSERT(ListLength(pn) >= 1);
    return ListLength(pn) - 1;
}

static inline ParseNode *
CallArgList(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_CALL));
    return NextNode(ListHead(pn));
}

static inline ParseNode *
VarListHead(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_VAR));
    return ListHead(pn);
}

static inline ParseNode *
CaseExpr(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_CASE) || pn->isKind(PNK_DEFAULT));
    return BinaryLeft(pn);
}

static inline ParseNode *
CaseBody(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_CASE) || pn->isKind(PNK_DEFAULT));
    return BinaryRight(pn);
}

static inline JSAtom *
StringAtom(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_STRING));
    return pn->pn_atom;
}

static inline bool
IsExpressionStatement(ParseNode *pn)
{
    return pn->isKind(PNK_SEMI);
}

static inline ParseNode *
ExpressionStatementExpr(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_SEMI));
    return UnaryKid(pn);
}

static inline PropertyName *
LoopControlMaybeLabel(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_BREAK) || pn->isKind(PNK_CONTINUE));
    JS_ASSERT(pn->isArity(PN_NULLARY));
    return pn->as<LoopControlStatement>().label();
}

static inline PropertyName *
LabeledStatementLabel(ParseNode *pn)
{
    return pn->as<LabeledStatement>().label();
}

static inline ParseNode *
LabeledStatementStatement(ParseNode *pn)
{
    return pn->as<LabeledStatement>().statement();
}

static double
NumberNodeValue(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_NUMBER));
    return pn->pn_dval;
}

static bool
NumberNodeHasFrac(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_NUMBER));
    return pn->pn_u.number.decimalPoint == HasDecimal;
}

static ParseNode *
DotBase(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_DOT));
    JS_ASSERT(pn->isArity(PN_NAME));
    return pn->expr();
}

static PropertyName *
DotMember(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_DOT));
    JS_ASSERT(pn->isArity(PN_NAME));
    return pn->pn_atom->asPropertyName();
}

static ParseNode *
ElemBase(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_ELEM));
    return BinaryLeft(pn);
}

static ParseNode *
ElemIndex(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_ELEM));
    return BinaryRight(pn);
}

static inline JSFunction *
FunctionObject(ParseNode *fn)
{
    JS_ASSERT(fn->isKind(PNK_FUNCTION));
    JS_ASSERT(fn->isArity(PN_CODE));
    return fn->pn_funbox->function();
}

static inline PropertyName *
FunctionName(ParseNode *fn)
{
    if (JSAtom *atom = FunctionObject(fn)->atom())
        return atom->asPropertyName();
    return NULL;
}

static inline ParseNode *
FunctionArgsList(ParseNode *fn, unsigned *numFormals)
{
    JS_ASSERT(fn->isKind(PNK_FUNCTION));
    ParseNode *argsBody = fn->pn_body;
    JS_ASSERT(argsBody->isKind(PNK_ARGSBODY));
    *numFormals = argsBody->pn_count;
    if (*numFormals > 0 && argsBody->last()->isKind(PNK_STATEMENTLIST))
        (*numFormals)--;
    return ListHead(argsBody);
}

static inline ParseNode *
FunctionStatementList(ParseNode *fn)
{
    JS_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
    ParseNode *last = fn->pn_body->last();
    JS_ASSERT(last->isKind(PNK_STATEMENTLIST));
    return last;
}

static inline ParseNode *
FunctionLastReturnStatementOrNull(ParseNode *fn)
{
    ParseNode *listIter = ListHead(FunctionStatementList(fn));
    ParseNode *lastReturn = NULL;
    while (listIter) {
        if (listIter->isKind(PNK_RETURN))
            lastReturn = listIter;
        listIter = listIter->pn_next;
    }
    return lastReturn;
}

static inline bool
IsNormalObjectField(JSContext *cx, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_COLON));
    return pn->getOp() == JSOP_INITPROP &&
           BinaryLeft(pn)->isKind(PNK_NAME) &&
           BinaryLeft(pn)->name() != cx->names().proto;
}

static inline PropertyName *
ObjectNormalFieldName(JSContext *cx, ParseNode *pn)
{
    JS_ASSERT(IsNormalObjectField(cx, pn));
    return BinaryLeft(pn)->name();
}

static inline ParseNode *
ObjectFieldInitializer(ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_COLON));
    return BinaryRight(pn);
}

static inline bool
IsDefinition(ParseNode *pn)
{
    return pn->isKind(PNK_NAME) && pn->isDefn();
}

static inline ParseNode *
MaybeDefinitionInitializer(ParseNode *pn)
{
    JS_ASSERT(IsDefinition(pn));
    return pn->expr();
}

static inline bool
IsUseOfName(ParseNode *pn, PropertyName *name)
{
    return pn->isKind(PNK_NAME) && pn->name() == name;
}

static inline bool
IsEmptyStatement(ParseNode *pn)
{
    return pn->isKind(PNK_SEMI) && !UnaryKid(pn);
}

static inline ParseNode *
SkipEmptyStatements(ParseNode *pn)
{
    while (pn && IsEmptyStatement(pn))
        pn = pn->pn_next;
    return pn;
}

static inline ParseNode *
NextNonEmptyStatement(ParseNode *pn)
{
    return SkipEmptyStatements(pn->pn_next);
}

static TokenKind
PeekToken(AsmJSParser &parser)
{
    TokenStream &ts = parser.tokenStream;
    while (ts.peekToken(TSF_OPERAND) == TOK_SEMI)
        ts.getToken(TSF_OPERAND);
    return ts.peekToken(TSF_OPERAND);
}

static bool
ParseVarStatement(AsmJSParser &parser, ParseNode **var)
{
    if (PeekToken(parser) != TOK_VAR) {
        *var = NULL;
        return true;
    }

    *var = parser.statement();
    if (!*var)
        return false;

    JS_ASSERT((*var)->isKind(PNK_VAR));
    return true;
}




class Type
{
  public:
    enum Which {
        Double,
        Doublish,
        Fixnum,
        Int,
        Signed,
        Unsigned,
        Intish,
        Void,
        Unknown
    };

  private:
    Which which_;

  public:
    Type() : which_(Which(-1)) {}
    Type(Which w) : which_(w) {}

    bool operator==(Type rhs) const { return which_ == rhs.which_; }
    bool operator!=(Type rhs) const { return which_ != rhs.which_; }

    bool isSigned() const {
        return which_ == Signed || which_ == Fixnum;
    }

    bool isUnsigned() const {
        return which_ == Unsigned || which_ == Fixnum;
    }

    bool isInt() const {
        return isSigned() || isUnsigned() || which_ == Int;
    }

    bool isIntish() const {
        return isInt() || which_ == Intish || which_ == Unknown;
    }

    bool isDouble() const {
        return which_ == Double;
    }

    bool isDoublish() const {
        return isDouble() || which_ == Doublish || which_ == Unknown;
    }

    bool isVoid() const {
        return which_ == Void;
    }

    bool isExtern() const {
        return isDouble() || isSigned();
    }

    bool isVarType() const {
        return isInt() || isDouble();
    }

    MIRType toMIRType() const {
        switch (which_) {
          case Double:
          case Doublish:
            return MIRType_Double;
          case Fixnum:
          case Int:
          case Signed:
          case Unsigned:
          case Intish:
            return MIRType_Int32;
          case Void:
          case Unknown:
            return MIRType_None;
        }
        MOZ_ASSUME_UNREACHABLE("Invalid Type");
    }

    const char *toChars() const {
        switch (which_) {
          case Double:    return "double";
          case Doublish:  return "doublish";
          case Fixnum:    return "fixnum";
          case Int:       return "int";
          case Signed:    return "signed";
          case Unsigned:  return "unsigned";
          case Intish:    return "intish";
          case Void:      return "void";
          case Unknown:   return "unknown";
        }
        MOZ_ASSUME_UNREACHABLE("Invalid Type");
    }
};



class RetType
{
  public:
    enum Which {
        Void = Type::Void,
        Signed = Type::Signed,
        Double = Type::Double
    };

  private:
    Which which_;

  public:
    RetType() {}
    RetType(Which w) : which_(w) {}
    RetType(AsmJSCoercion coercion) {
        switch (coercion) {
          case AsmJS_ToInt32: which_ = Signed; break;
          case AsmJS_ToNumber: which_ = Double; break;
        }
    }
    Which which() const {
        return which_;
    }
    Type toType() const {
        return Type::Which(which_);
    }
    AsmJSModule::ReturnType toModuleReturnType() const {
        switch (which_) {
          case Void: return AsmJSModule::Return_Void;
          case Signed: return AsmJSModule::Return_Int32;
          case Double: return AsmJSModule::Return_Double;
        }
        MOZ_ASSUME_UNREACHABLE("Unexpected return type");
    }
    MIRType toMIRType() const {
        switch (which_) {
          case Void: return MIRType_None;
          case Signed: return MIRType_Int32;
          case Double: return MIRType_Double;
        }
        MOZ_ASSUME_UNREACHABLE("Unexpected return type");
    }
    bool operator==(RetType rhs) const { return which_ == rhs.which_; }
    bool operator!=(RetType rhs) const { return which_ != rhs.which_; }
};





















class VarType
{
  public:
    enum Which {
        Int = Type::Int,
        Double = Type::Double
    };

  private:
    Which which_;

  public:
    VarType()
      : which_(Which(-1)) {}
    VarType(Which w)
      : which_(w) {}
    VarType(AsmJSCoercion coercion) {
        switch (coercion) {
          case AsmJS_ToInt32: which_ = Int; break;
          case AsmJS_ToNumber: which_ = Double; break;
        }
    }
    Which which() const {
        return which_;
    }
    Type toType() const {
        return Type::Which(which_);
    }
    MIRType toMIRType() const {
        return which_ == Int ? MIRType_Int32 : MIRType_Double;
    }
    AsmJSCoercion toCoercion() const {
        return which_ == Int ? AsmJS_ToInt32 : AsmJS_ToNumber;
    }
    static VarType FromMIRType(MIRType type) {
        JS_ASSERT(type == MIRType_Int32 || type == MIRType_Double);
        return type == MIRType_Int32 ? Int : Double;
    }
    static VarType FromCheckedType(Type type) {
        JS_ASSERT(type.isInt() || type.isDoublish());
        return type.isDoublish() ? Double : Int;
    }
    bool operator==(VarType rhs) const { return which_ == rhs.which_; }
    bool operator!=(VarType rhs) const { return which_ != rhs.which_; }
};


static inline bool
operator<=(Type lhs, VarType rhs)
{
    switch (rhs.which()) {
      case VarType::Int:    return lhs.isInt();
      case VarType::Double: return lhs.isDouble();
    }
    MOZ_ASSUME_UNREACHABLE("Unexpected rhs type");
}



static inline MIRType ToMIRType(MIRType t) { return t; }
static inline MIRType ToMIRType(VarType t) { return t.toMIRType(); }

template <class VecT>
class ABIArgIter
{
    ABIArgGenerator gen_;
    const VecT &types_;
    unsigned i_;

    void settle() { if (!done()) gen_.next(ToMIRType(types_[i_])); }

  public:
    ABIArgIter(const VecT &types) : types_(types), i_(0) { settle(); }
    void operator++(int) { JS_ASSERT(!done()); i_++; settle(); }
    bool done() const { return i_ == types_.length(); }

    ABIArg *operator->() { JS_ASSERT(!done()); return &gen_.current(); }
    ABIArg &operator*() { JS_ASSERT(!done()); return gen_.current(); }

    unsigned index() const { JS_ASSERT(!done()); return i_; }
    MIRType mirType() const { JS_ASSERT(!done()); return ToMIRType(types_[i_]); }
    uint32_t stackBytesConsumedSoFar() const { return gen_.stackBytesConsumedSoFar(); }
};

typedef Vector<MIRType, 8> MIRTypeVector;
typedef ABIArgIter<MIRTypeVector> ABIArgMIRTypeIter;

typedef Vector<VarType, 8> VarTypeVector;
typedef ABIArgIter<VarTypeVector> ABIArgTypeIter;

class Signature
{
    VarTypeVector argTypes_;
    RetType retType_;

  public:
    Signature(JSContext *cx)
      : argTypes_(cx) {}
    Signature(JSContext *cx, RetType retType)
      : argTypes_(cx), retType_(retType) {}
    Signature(MoveRef<VarTypeVector> argTypes, RetType retType)
      : argTypes_(argTypes), retType_(retType) {}
    Signature(MoveRef<Signature> rhs)
      : argTypes_(Move(rhs->argTypes_)), retType_(rhs->retType_) {}

    bool copy(const Signature &rhs) {
        if (!argTypes_.resize(rhs.argTypes_.length()))
            return false;
        for (unsigned i = 0; i < argTypes_.length(); i++)
            argTypes_[i] = rhs.argTypes_[i];
        retType_ = rhs.retType_;
        return true;
    }

    bool appendArg(VarType type) { return argTypes_.append(type); }
    VarType arg(unsigned i) const { return argTypes_[i]; }
    const VarTypeVector &args() const { return argTypes_; }
    MoveRef<VarTypeVector> extractArgs() { return Move(argTypes_); }

    RetType retType() const { return retType_; }
};

static
bool operator==(const Signature &lhs, const Signature &rhs)
{
    if (lhs.retType() != rhs.retType())
        return false;
    if (lhs.args().length() != rhs.args().length())
        return false;
    for (unsigned i = 0; i < lhs.args().length(); i++) {
        if (lhs.arg(i) != rhs.arg(i))
            return false;
    }
    return true;
}

static inline
bool operator!=(const Signature &lhs, const Signature &rhs)
{
    return !(lhs == rhs);
}












class NumLit
{
  public:
    enum Which {
        Fixnum = Type::Fixnum,
        NegativeInt = Type::Signed,
        BigUnsigned = Type::Unsigned,
        Double = Type::Double,
        OutOfRangeInt = -1
    };

  private:
    Which which_;
    Value v_;

  public:
    NumLit(Which w, Value v)
      : which_(w), v_(v)
    {}

    Which which() const {
        return which_;
    }

    int32_t toInt32() const {
        JS_ASSERT(which_ == Fixnum || which_ == NegativeInt || which_ == BigUnsigned);
        return v_.toInt32();
    }

    double toDouble() const {
        return v_.toDouble();
    }

    Type type() const {
        JS_ASSERT(which_ != OutOfRangeInt);
        return Type::Which(which_);
    }

    Value value() const {
        JS_ASSERT(which_ != OutOfRangeInt);
        return v_;
    }
};



static bool
IsNumericLiteral(ParseNode *pn)
{
    return pn->isKind(PNK_NUMBER) ||
           (pn->isKind(PNK_NEG) && UnaryKid(pn)->isKind(PNK_NUMBER));
}

static NumLit
ExtractNumericLiteral(ParseNode *pn)
{
    JS_ASSERT(IsNumericLiteral(pn));
    ParseNode *numberNode;
    double d;
    if (pn->isKind(PNK_NEG)) {
        numberNode = UnaryKid(pn);
        d = -NumberNodeValue(numberNode);
    } else {
        numberNode = pn;
        d = NumberNodeValue(numberNode);
    }

    if (NumberNodeHasFrac(numberNode) || IsNegativeZero(d))
        return NumLit(NumLit::Double, DoubleValue(d));

    int64_t i64 = int64_t(d);

    if (i64 >= 0) {
        if (i64 <= INT32_MAX)
            return NumLit(NumLit::Fixnum, Int32Value(i64));
        if (i64 <= UINT32_MAX)
            return NumLit(NumLit::BigUnsigned, Int32Value(uint32_t(i64)));
        return NumLit(NumLit::OutOfRangeInt, UndefinedValue());
    }
    if (i64 >= INT32_MIN)
        return NumLit(NumLit::NegativeInt, Int32Value(i64));
    return NumLit(NumLit::OutOfRangeInt, UndefinedValue());
}

static inline bool
IsLiteralUint32(ParseNode *pn, uint32_t *u32)
{
    if (!IsNumericLiteral(pn))
        return false;

    NumLit literal = ExtractNumericLiteral(pn);
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::BigUnsigned:
        *u32 = uint32_t(literal.toInt32());
        return true;
      case NumLit::NegativeInt:
      case NumLit::Double:
      case NumLit::OutOfRangeInt:
        return false;
    }

    MOZ_ASSUME_UNREACHABLE("Bad literal type");
}

static inline bool
IsBits32(ParseNode *pn, int32_t i)
{
    if (!IsNumericLiteral(pn))
        return false;

    NumLit literal = ExtractNumericLiteral(pn);
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::BigUnsigned:
      case NumLit::NegativeInt:
        return literal.toInt32() == i;
      case NumLit::Double:
      case NumLit::OutOfRangeInt:
        return false;
    }

    MOZ_ASSUME_UNREACHABLE("Bad literal type");
}




static Type
TypedArrayLoadType(ArrayBufferView::ViewType viewType)
{
    switch (viewType) {
      case ArrayBufferView::TYPE_INT8:
      case ArrayBufferView::TYPE_INT16:
      case ArrayBufferView::TYPE_INT32:
      case ArrayBufferView::TYPE_UINT8:
      case ArrayBufferView::TYPE_UINT16:
      case ArrayBufferView::TYPE_UINT32:
        return Type::Intish;
      case ArrayBufferView::TYPE_FLOAT32:
      case ArrayBufferView::TYPE_FLOAT64:
        return Type::Doublish;
      default:;
    }
    MOZ_ASSUME_UNREACHABLE("Unexpected array type");
}

enum ArrayStoreEnum {
    ArrayStore_Intish,
    ArrayStore_Doublish
};

static ArrayStoreEnum
TypedArrayStoreType(ArrayBufferView::ViewType viewType)
{
    switch (viewType) {
      case ArrayBufferView::TYPE_INT8:
      case ArrayBufferView::TYPE_INT16:
      case ArrayBufferView::TYPE_INT32:
      case ArrayBufferView::TYPE_UINT8:
      case ArrayBufferView::TYPE_UINT16:
      case ArrayBufferView::TYPE_UINT32:
        return ArrayStore_Intish;
      case ArrayBufferView::TYPE_FLOAT32:
      case ArrayBufferView::TYPE_FLOAT64:
        return ArrayStore_Doublish;
      default:;
    }
    MOZ_ASSUME_UNREACHABLE("Unexpected array type");
}



typedef Vector<PropertyName*,1> LabelVector;
typedef Vector<MBasicBlock*,8> BlockVector;



























































class MOZ_STACK_CLASS ModuleCompiler
{
  public:
    class Func
    {
        PropertyName *name_;
        uint32_t srcOffset_;
        Signature sig_;
        Label *code_;
        unsigned compileTime_;

      public:
        Func(JSContext *cx, PropertyName *name, MoveRef<Signature> sig, Label *code)
          : name_(name), srcOffset_(UINT32_MAX), sig_(sig), code_(code), compileTime_(0) {}

        Func(MoveRef<Func> rhs)
          : name_(rhs->name_),
            sig_(Move(rhs->sig_)),
            code_(rhs->code_),
            compileTime_(rhs->compileTime_)
        {}

        PropertyName *name() const { return name_; }
        void initSrcOffset(uint32_t srcOffset) { srcOffset_ = srcOffset; }
        uint32_t srcOffset() const { JS_ASSERT(srcOffset_ != UINT32_MAX); return srcOffset_; }
        Signature &sig() { return sig_; }
        const Signature &sig() const { return sig_; }
        Label *code() const { return code_; }
        unsigned compileTime() const { return compileTime_; }
        void accumulateCompileTime(unsigned ms) { compileTime_ += ms; }
    };

    class Global
    {
      public:
        enum Which { Variable, Function, FuncPtrTable, FFI, ArrayView, MathBuiltin, Constant };

      private:
        Which which_;
        union {
            struct {
                uint32_t index_;
                VarType::Which type_;
            } var;
            uint32_t funcIndex_;
            uint32_t funcPtrTableIndex_;
            uint32_t ffiIndex_;
            ArrayBufferView::ViewType viewType_;
            AsmJSMathBuiltin mathBuiltin_;
            double constant_;
        } u;

        friend class ModuleCompiler;
        friend class js::LifoAlloc;

        Global(Which which) : which_(which) {}

      public:
        Which which() const {
            return which_;
        }
        VarType varType() const {
            JS_ASSERT(which_ == Variable);
            return VarType(u.var.type_);
        }
        uint32_t varIndex() const {
            JS_ASSERT(which_ == Variable);
            return u.var.index_;
        }
        uint32_t funcIndex() const {
            JS_ASSERT(which_ == Function);
            return u.funcIndex_;
        }
        uint32_t funcPtrTableIndex() const {
            JS_ASSERT(which_ == FuncPtrTable);
            return u.funcPtrTableIndex_;
        }
        unsigned ffiIndex() const {
            JS_ASSERT(which_ == FFI);
            return u.ffiIndex_;
        }
        ArrayBufferView::ViewType viewType() const {
            JS_ASSERT(which_ == ArrayView);
            return u.viewType_;
        }
        AsmJSMathBuiltin mathBuiltin() const {
            JS_ASSERT(which_ == MathBuiltin);
            return u.mathBuiltin_;
        }
        double constant() const {
            JS_ASSERT(which_ == Constant);
            return u.constant_;
        }
    };

    typedef Vector<const Func*> FuncPtrVector;

    class FuncPtrTable
    {
        Signature sig_;
        uint32_t mask_;
        uint32_t globalDataOffset_;
        FuncPtrVector elems_;

      public:
        FuncPtrTable(JSContext *cx, MoveRef<Signature> sig, uint32_t mask, uint32_t globalDataOffset)
          : sig_(sig), mask_(mask), globalDataOffset_(globalDataOffset), elems_(cx) {}

        FuncPtrTable(MoveRef<FuncPtrTable> rhs)
          : sig_(Move(rhs->sig_)), mask_(rhs->mask_), globalDataOffset_(rhs->globalDataOffset_),
            elems_(Move(rhs->elems_))
        {}

        Signature &sig() { return sig_; }
        const Signature &sig() const { return sig_; }
        unsigned mask() const { return mask_; }
        unsigned globalDataOffset() const { return globalDataOffset_; }

        void initElems(MoveRef<FuncPtrVector> elems) { elems_ = elems; JS_ASSERT(!elems_.empty()); }
        unsigned numElems() const { JS_ASSERT(!elems_.empty()); return elems_.length(); }
        const Func &elem(unsigned i) const { return *elems_[i]; }
    };

    typedef Vector<FuncPtrTable> FuncPtrTableVector;

    class ExitDescriptor
    {
        PropertyName *name_;
        Signature sig_;

      public:
        ExitDescriptor(PropertyName *name, MoveRef<Signature> sig)
          : name_(name), sig_(sig) {}
        ExitDescriptor(MoveRef<ExitDescriptor> rhs)
          : name_(rhs->name_), sig_(Move(rhs->sig_))
        {}
        const Signature &sig() const {
            return sig_;
        }

        
        typedef ExitDescriptor Lookup;
        static HashNumber hash(const ExitDescriptor &d) {
            HashNumber hn = HashGeneric(d.name_, d.sig_.retType().which());
            const VarTypeVector &args = d.sig_.args();
            for (unsigned i = 0; i < args.length(); i++)
                hn = AddToHash(hn, args[i].which());
            return hn;
        }
        static bool match(const ExitDescriptor &lhs, const ExitDescriptor &rhs) {
            return lhs.name_ == rhs.name_ && lhs.sig_ == rhs.sig_;
        }
    };

    typedef HashMap<ExitDescriptor, unsigned, ExitDescriptor, ContextAllocPolicy> ExitMap;

  private:
    struct SlowFunction
    {
        PropertyName *name;
        unsigned ms;
        unsigned line;
        unsigned column;
    };

    typedef HashMap<PropertyName*, AsmJSMathBuiltin> MathNameMap;
    typedef HashMap<PropertyName*, Global*> GlobalMap;
    typedef Vector<Func*> FuncVector;
    typedef Vector<AsmJSGlobalAccess> GlobalAccessVector;
    typedef Vector<SlowFunction> SlowFunctionVector;

    JSContext *                    cx_;
    AsmJSParser &                  parser_;

    MacroAssembler                 masm_;

    ScopedJSDeletePtr<AsmJSModule> module_;
    LifoAlloc                      moduleLifo_;
    ParseNode *                    moduleFunctionNode_;
    PropertyName *                 moduleFunctionName_;

    GlobalMap                      globals_;
    FuncVector                     functions_;
    FuncPtrTableVector             funcPtrTables_;
    ExitMap                        exits_;
    MathNameMap                    standardLibraryMathNames_;
    GlobalAccessVector             globalAccesses_;
    Label                          stackOverflowLabel_;
    Label                          operationCallbackLabel_;

    char *                         errorString_;
    uint32_t                       errorOffset_;
    uint32_t                       bodyStart_;

    int64_t                        usecBefore_;
    SlowFunctionVector             slowFunctions_;

    DebugOnly<bool>                finishedFunctionBodies_;

    bool addStandardLibraryMathName(const char *name, AsmJSMathBuiltin builtin) {
        JSAtom *atom = Atomize(cx_, name, strlen(name));
        if (!atom)
            return false;
        return standardLibraryMathNames_.putNew(atom->asPropertyName(), builtin);
    }

  public:
    ModuleCompiler(JSContext *cx, AsmJSParser &parser)
      : cx_(cx),
        parser_(parser),
        masm_(MacroAssembler::AsmJSToken()),
        moduleLifo_(LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
        moduleFunctionNode_(parser.pc->maybeFunction),
        moduleFunctionName_(NULL),
        globals_(cx),
        functions_(cx),
        funcPtrTables_(cx),
        exits_(cx),
        standardLibraryMathNames_(cx),
        globalAccesses_(cx),
        errorString_(NULL),
        errorOffset_(UINT32_MAX),
        bodyStart_(parser.tokenStream.currentToken().pos.end),
        usecBefore_(PRMJ_Now()),
        slowFunctions_(cx),
        finishedFunctionBodies_(false)
    {
        JS_ASSERT(moduleFunctionNode_->pn_funbox == parser.pc->sc->asFunctionBox());
    }

    ~ModuleCompiler() {
        if (errorString_) {
            JS_ASSERT(errorOffset_ != UINT32_MAX);
            parser_.tokenStream.reportAsmJSError(errorOffset_,
                                                 JSMSG_USE_ASM_TYPE_FAIL,
                                                 errorString_);
            js_free(errorString_);
        }

        
        if (!stackOverflowLabel_.bound())
            stackOverflowLabel_.bind(0);
        if (!operationCallbackLabel_.bound())
            operationCallbackLabel_.bind(0);
    }

    bool init() {
        if (!cx_->compartment()->ensureIonCompartmentExists(cx_))
            return false;

        if (!globals_.init() || !exits_.init())
            return false;

        if (!standardLibraryMathNames_.init() ||
            !addStandardLibraryMathName("sin", AsmJSMathBuiltin_sin) ||
            !addStandardLibraryMathName("cos", AsmJSMathBuiltin_cos) ||
            !addStandardLibraryMathName("tan", AsmJSMathBuiltin_tan) ||
            !addStandardLibraryMathName("asin", AsmJSMathBuiltin_asin) ||
            !addStandardLibraryMathName("acos", AsmJSMathBuiltin_acos) ||
            !addStandardLibraryMathName("atan", AsmJSMathBuiltin_atan) ||
            !addStandardLibraryMathName("ceil", AsmJSMathBuiltin_ceil) ||
            !addStandardLibraryMathName("floor", AsmJSMathBuiltin_floor) ||
            !addStandardLibraryMathName("exp", AsmJSMathBuiltin_exp) ||
            !addStandardLibraryMathName("log", AsmJSMathBuiltin_log) ||
            !addStandardLibraryMathName("pow", AsmJSMathBuiltin_pow) ||
            !addStandardLibraryMathName("sqrt", AsmJSMathBuiltin_sqrt) ||
            !addStandardLibraryMathName("abs", AsmJSMathBuiltin_abs) ||
            !addStandardLibraryMathName("atan2", AsmJSMathBuiltin_atan2) ||
            !addStandardLibraryMathName("imul", AsmJSMathBuiltin_imul))
        {
            return false;
        }

        module_ = cx_->new_<AsmJSModule>();
        if (!module_)
            return false;

        return true;
    }

    bool failOffset(uint32_t offset, const char *str) {
        JS_ASSERT(!errorString_);
        JS_ASSERT(errorOffset_ == UINT32_MAX);
        JS_ASSERT(str);
        errorOffset_ = offset;
        errorString_ = strdup(str);
        return false;
    }

    bool fail(ParseNode *pn, const char *str) {
        return failOffset(pn ? pn->pn_pos.begin : parser_.tokenStream.currentToken().pos.end, str);
    }

    bool failfVA(ParseNode *pn, const char *fmt, va_list ap) {
        JS_ASSERT(!errorString_);
        JS_ASSERT(errorOffset_ == UINT32_MAX);
        JS_ASSERT(fmt);
        errorOffset_ = pn ? pn->pn_pos.begin : parser_.tokenStream.currentToken().pos.end;
        errorString_ = JS_vsmprintf(fmt, ap);
        return false;
    }

    bool failf(ParseNode *pn, const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        failfVA(pn, fmt, ap);
        va_end(ap);
        return false;
    }

    bool failName(ParseNode *pn, const char *fmt, PropertyName *name) {
        JSAutoByteString bytes(cx_, name);
        if (bytes.ptr())
            failf(pn, fmt, bytes.ptr());
        return false;
    }

    static const unsigned SLOW_FUNCTION_THRESHOLD_MS = 250;

    bool maybeReportCompileTime(const Func &func) {
        if (func.compileTime() < SLOW_FUNCTION_THRESHOLD_MS)
            return true;
        SlowFunction sf;
        sf.name = func.name();
        sf.ms = func.compileTime();
        parser_.tokenStream.srcCoords.lineNumAndColumnIndex(func.srcOffset(), &sf.line, &sf.column);
        return slowFunctions_.append(sf);
    }

    

    JSContext *cx() const { return cx_; }
    AsmJSParser &parser() const { return parser_; }
    MacroAssembler &masm() { return masm_; }
    Label &stackOverflowLabel() { return stackOverflowLabel_; }
    Label &operationCallbackLabel() { return operationCallbackLabel_; }
    bool hasError() const { return errorString_ != NULL; }
    const AsmJSModule &module() const { return *module_.get(); }

    ParseNode *moduleFunctionNode() const { return moduleFunctionNode_; }
    PropertyName *moduleFunctionName() const { return moduleFunctionName_; }

    const Global *lookupGlobal(PropertyName *name) const {
        if (GlobalMap::Ptr p = globals_.lookup(name))
            return p->value;
        return NULL;
    }
    Func *lookupFunction(PropertyName *name) {
        if (GlobalMap::Ptr p = globals_.lookup(name)) {
            if (p->value->which() == Global::Function)
                return functions_[p->value->funcIndex()];
        }
        return NULL;
    }
    unsigned numFunctions() const {
        return functions_.length();
    }
    Func &function(unsigned i) {
        return *functions_[i];
    }
    unsigned numFuncPtrTables() const {
        return funcPtrTables_.length();
    }
    FuncPtrTable &funcPtrTable(unsigned i) {
        return funcPtrTables_[i];
    }
    bool lookupStandardLibraryMathName(PropertyName *name, AsmJSMathBuiltin *mathBuiltin) const {
        if (MathNameMap::Ptr p = standardLibraryMathNames_.lookup(name)) {
            *mathBuiltin = p->value;
            return true;
        }
        return false;
    }
    ExitMap::Range allExits() const {
        return exits_.all();
    }

    

    void initModuleFunctionName(PropertyName *name) { moduleFunctionName_ = name; }

    void initGlobalArgumentName(PropertyName *n) { module_->initGlobalArgumentName(n); }
    void initImportArgumentName(PropertyName *n) { module_->initImportArgumentName(n); }
    void initBufferArgumentName(PropertyName *n) { module_->initBufferArgumentName(n); }

    bool addGlobalVarInitConstant(PropertyName *varName, VarType type, const Value &v) {
        uint32_t index;
        if (!module_->addGlobalVarInitConstant(v, &index))
            return false;
        Global *global = moduleLifo_.new_<Global>(Global::Variable);
        if (!global)
            return false;
        global->u.var.index_ = index;
        global->u.var.type_ = type.which();
        return globals_.putNew(varName, global);
    }
    bool addGlobalVarImport(PropertyName *varName, PropertyName *fieldName, AsmJSCoercion coercion) {
        uint32_t index;
        if (!module_->addGlobalVarImport(fieldName, coercion, &index))
            return false;
        Global *global = moduleLifo_.new_<Global>(Global::Variable);
        if (!global)
            return false;
        global->u.var.index_ = index;
        global->u.var.type_ = VarType(coercion).which();
        return globals_.putNew(varName, global);
    }
    bool addFunction(PropertyName *name, MoveRef<Signature> sig, Func **func) {
        JS_ASSERT(!finishedFunctionBodies_);
        Global *global = moduleLifo_.new_<Global>(Global::Function);
        if (!global)
            return false;
        global->u.funcIndex_ = functions_.length();
        if (!globals_.putNew(name, global))
            return false;
        Label *code = moduleLifo_.new_<Label>();
        if (!code)
            return false;
        *func = moduleLifo_.new_<Func>(cx_, name, sig, code);
        if (!*func)
            return false;
        return functions_.append(*func);
    }
    bool addFuncPtrTable(PropertyName *name, MoveRef<Signature> sig, uint32_t mask, FuncPtrTable **table) {
        Global *global = moduleLifo_.new_<Global>(Global::FuncPtrTable);
        if (!global)
            return false;
        global->u.funcPtrTableIndex_ = funcPtrTables_.length();
        if (!globals_.putNew(name, global))
            return false;
        uint32_t globalDataOffset;
        if (!module_->addFuncPtrTable( mask + 1, &globalDataOffset))
            return false;
        FuncPtrTable tmpTable(cx_, sig, mask, globalDataOffset);
        if (!funcPtrTables_.append(Move(tmpTable)))
            return false;
        *table = &funcPtrTables_.back();
        return true;
    }
    bool addFFI(PropertyName *varName, PropertyName *field) {
        Global *global = moduleLifo_.new_<Global>(Global::FFI);
        if (!global)
            return false;
        uint32_t index;
        if (!module_->addFFI(field, &index))
            return false;
        global->u.ffiIndex_ = index;
        return globals_.putNew(varName, global);
    }
    bool addArrayView(PropertyName *varName, ArrayBufferView::ViewType vt, PropertyName *fieldName) {
        Global *global = moduleLifo_.new_<Global>(Global::ArrayView);
        if (!global)
            return false;
        if (!module_->addArrayView(vt, fieldName))
            return false;
        global->u.viewType_ = vt;
        return globals_.putNew(varName, global);
    }
    bool addMathBuiltin(PropertyName *varName, AsmJSMathBuiltin mathBuiltin, PropertyName *fieldName) {
        if (!module_->addMathBuiltin(mathBuiltin, fieldName))
            return false;
        Global *global = moduleLifo_.new_<Global>(Global::MathBuiltin);
        if (!global)
            return false;
        global->u.mathBuiltin_ = mathBuiltin;
        return globals_.putNew(varName, global);
    }
    bool addGlobalConstant(PropertyName *varName, double constant, PropertyName *fieldName) {
        if (!module_->addGlobalConstant(constant, fieldName))
            return false;
        Global *global = moduleLifo_.new_<Global>(Global::Constant);
        if (!global)
            return false;
        global->u.constant_ = constant;
        return globals_.putNew(varName, global);
    }
    bool addExportedFunction(const Func *func, PropertyName *maybeFieldName) {
        AsmJSModule::ArgCoercionVector argCoercions;
        const VarTypeVector &args = func->sig().args();
        if (!argCoercions.resize(args.length()))
            return false;
        for (unsigned i = 0; i < args.length(); i++)
            argCoercions[i] = args[i].toCoercion();
        AsmJSModule::ReturnType retType = func->sig().retType().toModuleReturnType();
        return module_->addExportedFunction(func->name(), maybeFieldName,
                                            Move(argCoercions), retType);
    }
    bool addExit(unsigned ffiIndex, PropertyName *name, MoveRef<Signature> sig, unsigned *exitIndex) {
        ExitDescriptor exitDescriptor(name, sig);
        ExitMap::AddPtr p = exits_.lookupForAdd(exitDescriptor);
        if (p) {
            *exitIndex = p->value;
            return true;
        }
        if (!module_->addExit(ffiIndex, exitIndex))
            return false;
        return exits_.add(p, Move(exitDescriptor), *exitIndex);
    }
    bool addGlobalAccess(AsmJSGlobalAccess access) {
        return globalAccesses_.append(access);
    }

    bool collectAccesses(MIRGenerator &gen) {
#ifdef JS_CPU_ARM
        if (!module_->addBoundsChecks(gen.asmBoundsChecks()))
            return false;
#else
        if (!module_->addHeapAccesses(gen.heapAccesses()))
            return false;
#endif
        if (!globalAccesses_.appendAll(gen.globalAccesses()))
            return false;
        return true;
    }

#ifdef MOZ_VTUNE
    bool trackProfiledFunction(const Func &func, unsigned endCodeOffset) {
        unsigned startCodeOffset = func.code()->offset();
        return module_->trackProfiledFunction(func.name(), startCodeOffset, endCodeOffset);
    }
#endif
#ifdef JS_ION_PERF
    bool trackPerfProfiledFunction(const Func &func, unsigned endCodeOffset) {
        unsigned lineno = 0U, columnIndex = 0U;
        parser().tokenStream.srcCoords.lineNumAndColumnIndex(func.srcOffset(), &lineno, &columnIndex);

        unsigned startCodeOffset = func.code()->offset();
        return module_->trackPerfProfiledFunction(func.name(), startCodeOffset, endCodeOffset, lineno, columnIndex);
    }

    bool trackPerfProfiledBlocks(AsmJSPerfSpewer &perfSpewer, const Func &func, unsigned endCodeOffset) {
        unsigned startCodeOffset = func.code()->offset();
        perfSpewer.noteBlocksOffsets(masm_);
        return module_->trackPerfProfiledBlocks(func.name(), startCodeOffset, endCodeOffset, perfSpewer.basicBlocks());
    }
#endif
    bool addFunctionCounts(IonScriptCounts *counts) {
        return module_->addFunctionCounts(counts);
    }

    void finishFunctionBodies() {
        JS_ASSERT(!finishedFunctionBodies_);
        masm_.align(AsmJSPageSize);
        finishedFunctionBodies_ = true;
        module_->initFunctionBytes(masm_.size());
    }

    void setInterpExitOffset(unsigned exitIndex) {
#if defined(JS_CPU_ARM)
        masm_.flush();
#endif
        module_->exit(exitIndex).initInterpOffset(masm_.size());
    }
    void setIonExitOffset(unsigned exitIndex) {
#if defined(JS_CPU_ARM)
        masm_.flush();
#endif
        module_->exit(exitIndex).initIonOffset(masm_.size());
    }
    void setEntryOffset(unsigned exportIndex) {
#if defined(JS_CPU_ARM)
        masm_.flush();
#endif
        module_->exportedFunction(exportIndex).initCodeOffset(masm_.size());
    }

    void buildCompilationTimeReport(ScopedJSFreePtr<char> *out) {
        int msTotal = 0;
        ScopedJSFreePtr<char> slowFuns;
#ifndef JS_MORE_DETERMINISTIC
        int64_t usecAfter = PRMJ_Now();
        msTotal = (usecAfter - usecBefore_) / PRMJ_USEC_PER_MSEC;
        if (!slowFunctions_.empty()) {
            slowFuns.reset(JS_smprintf("; %d functions compiled slowly: ", slowFunctions_.length()));
            if (!slowFuns)
                return;
            for (unsigned i = 0; i < slowFunctions_.length(); i++) {
                SlowFunction &func = slowFunctions_[i];
                JSAutoByteString name;
                if (!AtomToPrintableString(cx_, func.name, &name))
                    return;
                slowFuns.reset(JS_smprintf("%s%s:%u:%u (%ums)%s", slowFuns.get(),
                                           name.ptr(), func.line, func.column, func.ms,
                                           i+1 < slowFunctions_.length() ? ", " : ""));
                if (!slowFuns)
                    return;
            }
        }
#endif
        out->reset(JS_smprintf("total compilation time %dms%s",
                               msTotal, slowFuns ? slowFuns.get() : ""));
    }

    bool staticallyLink(ScopedJSDeletePtr<AsmJSModule> *module, ScopedJSFreePtr<char> *report) {
        module_->initPostLinkFailureInfo(cx_->runtime(),
                                         parser_.tokenStream.getOriginPrincipals(),
                                         parser_.ss,
                                         bodyStart_,
                                         parser_.tokenStream.currentToken().pos.end);

        
        masm_.finish();
        if (masm_.oom())
            return false;

        
        
        
        size_t codeBytes = AlignBytes(masm_.bytesNeeded(), sizeof(double));
        size_t totalBytes = codeBytes + module_->globalDataBytes();

        
        
        size_t allocedBytes = totalBytes + AsmJSPageSize;

        
        JSC::ExecutableAllocator *execAlloc = cx_->compartment()->ionCompartment()->execAlloc();
        JSC::ExecutablePool *pool;
        uint8_t *unalignedBytes = (uint8_t*)execAlloc->alloc(allocedBytes, &pool, JSC::ASMJS_CODE);
        if (!unalignedBytes)
            return false;
        uint8_t *code = (uint8_t*)AlignBytes((uintptr_t)unalignedBytes, AsmJSPageSize);

        
        module_->takeOwnership(pool, code, codeBytes, totalBytes);

        
        masm_.executableCopy(code);
        masm_.processCodeLabels(code);
        JS_ASSERT(masm_.jumpRelocationTableBytes() == 0);
        JS_ASSERT(masm_.dataRelocationTableBytes() == 0);
        JS_ASSERT(masm_.preBarrierTableBytes() == 0);
        JS_ASSERT(!masm_.hasEnteredExitFrame());

        

        
        for (unsigned i = 0; i < module_->numExportedFunctions(); i++)
            module_->exportedFunction(i).patch(code);

        
        for (unsigned i = 0; i < module_->numExits(); i++) {
            module_->exit(i).patch(code);
            module_->exitIndexToGlobalDatum(i).exit = module_->exit(i).interpCode();
            module_->exitIndexToGlobalDatum(i).fun = NULL;
        }
        module_->setOperationCallbackExit(code + masm_.actualOffset(operationCallbackLabel_.offset()));

        
        for (unsigned i = 0; i < funcPtrTables_.length(); i++) {
            FuncPtrTable &table = funcPtrTables_[i];
            uint8_t **data = module_->globalDataOffsetToFuncPtrTable(table.globalDataOffset());
            for (unsigned j = 0; j < table.numElems(); j++)
                data[j] = code + masm_.actualOffset(table.elem(j).code()->offset());
        }

        
#ifdef JS_CPU_ARM
        JS_ASSERT(globalAccesses_.length() == 0);
        
        
        module_->convertBoundsChecksToActualOffset(masm_);

#else

        for (unsigned i = 0; i < globalAccesses_.length(); i++) {
            AsmJSGlobalAccess access = globalAccesses_[i];
            masm_.patchAsmJSGlobalAccess(access.offset, code, codeBytes, access.globalDataOffset);
        }
#endif
        
        
        for (unsigned i = 0; i < module_->numHeapAccesses(); i++) {
            AsmJSHeapAccess &access = module_->heapAccess(i);
            access.updateOffset(masm_.actualOffset(access.offset()));
        }

        *module = module_.forget();

        buildCompilationTimeReport(report);
        return true;
    }
};






class FunctionCompiler
{
  public:
    struct Local
    {
        VarType type;
        unsigned slot;
        Local(VarType t, unsigned slot) : type(t), slot(slot) {}
    };

  private:
    typedef HashMap<PropertyName*, Local> LocalMap;
    typedef Vector<Value> VarInitializerVector;
    typedef HashMap<PropertyName*, BlockVector> LabeledBlockMap;
    typedef HashMap<ParseNode*, BlockVector> UnlabeledBlockMap;
    typedef Vector<ParseNode*, 4> NodeStack;

    ModuleCompiler &       m_;
    LifoAlloc &            lifo_;
    ParseNode *            fn_;

    LocalMap               locals_;
    VarInitializerVector   varInitializers_;
    Maybe<RetType>         alreadyReturned_;

    TempAllocator *        alloc_;
    MIRGraph *             graph_;
    CompileInfo *          info_;
    MIRGenerator *         mirGen_;
    Maybe<IonContext>      ionContext_;

    MBasicBlock *          curBlock_;

    NodeStack              loopStack_;
    NodeStack              breakableStack_;
    UnlabeledBlockMap      unlabeledBreaks_;
    UnlabeledBlockMap      unlabeledContinues_;
    LabeledBlockMap        labeledBreaks_;
    LabeledBlockMap        labeledContinues_;

    AutoFlushCache         autoFlushCache_;

  public:
    FunctionCompiler(ModuleCompiler &m, ParseNode *fn, LifoAlloc &lifo)
      : m_(m),
        lifo_(lifo),
        fn_(fn),
        locals_(m.cx()),
        varInitializers_(m.cx()),
        alloc_(NULL),
        graph_(NULL),
        info_(NULL),
        mirGen_(NULL),
        curBlock_(NULL),
        loopStack_(m.cx()),
        breakableStack_(m.cx()),
        unlabeledBreaks_(m.cx()),
        unlabeledContinues_(m.cx()),
        labeledBreaks_(m.cx()),
        labeledContinues_(m.cx()),
        autoFlushCache_("asm.js")
    {}

    ModuleCompiler &    m() const      { return m_; }
    LifoAlloc &         lifo() const   { return lifo_; }
    ParseNode *         fn() const     { return fn_; }
    JSContext *         cx() const     { return m_.cx(); }
    const AsmJSModule & module() const { return m_.module(); }

    bool init()
    {
        return locals_.init() &&
               unlabeledBreaks_.init() &&
               unlabeledContinues_.init() &&
               labeledBreaks_.init() &&
               labeledContinues_.init();
    }

    bool fail(ParseNode *pn, const char *str)
    {
        return m_.fail(pn, str);
    }

    bool failf(ParseNode *pn, const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        m_.failfVA(pn, fmt, ap);
        va_end(ap);
        return false;
    }

    bool failName(ParseNode *pn, const char *fmt, PropertyName *name)
    {
        return m_.failName(pn, fmt, name);
    }

    ~FunctionCompiler()
    {
        if (!m().hasError() && !cx()->isExceptionPending()) {
            JS_ASSERT(loopStack_.empty());
            JS_ASSERT(unlabeledBreaks_.empty());
            JS_ASSERT(unlabeledContinues_.empty());
            JS_ASSERT(labeledBreaks_.empty());
            JS_ASSERT(labeledContinues_.empty());
            JS_ASSERT(curBlock_ == NULL);
        }
    }

    

    bool addFormal(ParseNode *pn, PropertyName *name, VarType type)
    {
        LocalMap::AddPtr p = locals_.lookupForAdd(name);
        if (p)
            return failName(pn, "duplicate local name '%s' not allowed", name);
        return locals_.add(p, name, Local(type, locals_.count()));
    }

    bool addVariable(ParseNode *pn, PropertyName *name, VarType type, const Value &init)
    {
        LocalMap::AddPtr p = locals_.lookupForAdd(name);
        if (p)
            return failName(pn, "duplicate local name '%s' not allowed", name);
        if (!locals_.add(p, name, Local(type, locals_.count())))
            return false;
        return varInitializers_.append(init);
    }

    bool prepareToEmitMIR(const VarTypeVector &argTypes)
    {
        JS_ASSERT(locals_.count() == argTypes.length() + varInitializers_.length());

        alloc_  = lifo_.new_<TempAllocator>(&lifo_);
        ionContext_.construct(m_.cx()->compartment(), alloc_);

        graph_  = lifo_.new_<MIRGraph>(alloc_);
        info_   = lifo_.new_<CompileInfo>(locals_.count(), SequentialExecution);
        mirGen_ = lifo_.new_<MIRGenerator>(cx()->compartment(), alloc_, graph_, info_);

        if (!newBlock( NULL, &curBlock_, fn_))
            return false;

        curBlock_->add(MAsmJSCheckOverRecursed::New(&m_.stackOverflowLabel()));

        for (ABIArgTypeIter i = argTypes; !i.done(); i++) {
            MAsmJSParameter *ins = MAsmJSParameter::New(*i, i.mirType());
            curBlock_->add(ins);
            curBlock_->initSlot(info().localSlot(i.index()), ins);
        }
        unsigned firstLocalSlot = argTypes.length();
        for (unsigned i = 0; i < varInitializers_.length(); i++) {
            MConstant *ins = MConstant::New(varInitializers_[i]);
            curBlock_->add(ins);
            curBlock_->initSlot(info().localSlot(firstLocalSlot + i), ins);
        }
        return true;
    }

    

    bool hasAlreadyReturned() const {
        return !alreadyReturned_.empty();
    }

    RetType returnedType() const {
        return alreadyReturned_.ref();
    }

    void setReturnedType(RetType retType) {
        alreadyReturned_.construct(retType);
    }

    

    MIRGenerator & mirGen() const     { JS_ASSERT(mirGen_); return *mirGen_; }
    MIRGraph &     mirGraph() const   { JS_ASSERT(graph_); return *graph_; }
    CompileInfo &  info() const       { JS_ASSERT(info_); return *info_; }

    const Local *lookupLocal(PropertyName *name) const
    {
        if (LocalMap::Ptr p = locals_.lookup(name))
            return &p->value;
        return NULL;
    }

    MDefinition *getLocalDef(const Local &local)
    {
        if (!curBlock_)
            return NULL;
        return curBlock_->getSlot(info().localSlot(local.slot));
    }

    const ModuleCompiler::Global *lookupGlobal(PropertyName *name) const
    {
        if (locals_.has(name))
            return NULL;
        return m_.lookupGlobal(name);
    }

    

    MDefinition *constant(const Value &v)
    {
        if (!curBlock_)
            return NULL;
        JS_ASSERT(v.isNumber());
        MConstant *constant = MConstant::New(v);
        curBlock_->add(constant);
        return constant;
    }

    template <class T>
    MDefinition *unary(MDefinition *op)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::NewAsmJS(op);
        curBlock_->add(ins);
        return ins;
    }

    template <class T>
    MDefinition *unary(MDefinition *op, MIRType type)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::NewAsmJS(op, type);
        curBlock_->add(ins);
        return ins;
    }

    template <class T>
    MDefinition *binary(MDefinition *lhs, MDefinition *rhs)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::New(lhs, rhs);
        curBlock_->add(ins);
        return ins;
    }

    template <class T>
    MDefinition *binary(MDefinition *lhs, MDefinition *rhs, MIRType type)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::NewAsmJS(lhs, rhs, type);
        curBlock_->add(ins);
        return ins;
    }

    MDefinition *mul(MDefinition *lhs, MDefinition *rhs, MIRType type, MMul::Mode mode)
    {
        if (!curBlock_)
            return NULL;
        MMul *ins = MMul::New(lhs, rhs, type, mode);
        curBlock_->add(ins);
        return ins;
    }

    template <class T>
    MDefinition *bitwise(MDefinition *lhs, MDefinition *rhs)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::NewAsmJS(lhs, rhs);
        curBlock_->add(ins);
        return ins;
    }

    template <class T>
    MDefinition *bitwise(MDefinition *op)
    {
        if (!curBlock_)
            return NULL;
        T *ins = T::NewAsmJS(op);
        curBlock_->add(ins);
        return ins;
    }

    MDefinition *compare(MDefinition *lhs, MDefinition *rhs, JSOp op, MCompare::CompareType type)
    {
        if (!curBlock_)
            return NULL;
        MCompare *ins = MCompare::NewAsmJS(lhs, rhs, op, type);
        curBlock_->add(ins);
        return ins;
    }

    void assign(const Local &local, MDefinition *def)
    {
        if (!curBlock_)
            return;
        curBlock_->setSlot(info().localSlot(local.slot), def);
    }

    MDefinition *loadHeap(ArrayBufferView::ViewType vt, MDefinition *ptr)
    {
        if (!curBlock_)
            return NULL;
        MAsmJSLoadHeap *load = MAsmJSLoadHeap::New(vt, ptr);
        curBlock_->add(load);
        return load;
    }

    void storeHeap(ArrayBufferView::ViewType vt, MDefinition *ptr, MDefinition *v)
    {
        if (!curBlock_)
            return;
        curBlock_->add(MAsmJSStoreHeap::New(vt, ptr, v));
    }

    MDefinition *loadGlobalVar(const ModuleCompiler::Global &global)
    {
        if (!curBlock_)
            return NULL;
        MIRType type = global.varType().toMIRType();
        unsigned globalDataOffset = module().globalVarIndexToGlobalDataOffset(global.varIndex());
        MAsmJSLoadGlobalVar *load = MAsmJSLoadGlobalVar::New(type, globalDataOffset);
        curBlock_->add(load);
        return load;
    }

    void storeGlobalVar(const ModuleCompiler::Global &global, MDefinition *v)
    {
        if (!curBlock_)
            return;
        unsigned globalDataOffset = module().globalVarIndexToGlobalDataOffset(global.varIndex());
        curBlock_->add(MAsmJSStoreGlobalVar::New(globalDataOffset, v));
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    class Call
    {
        ABIArgGenerator abi_;
        uint32_t prevMaxStackBytes_;
        uint32_t maxChildStackBytes_;
        uint32_t spIncrement_;
        Signature sig_;
        MAsmJSCall::Args regArgs_;
        Vector<MAsmJSPassStackArg*> stackArgs_;
        bool childClobbers_;

        friend class FunctionCompiler;

      public:
        Call(FunctionCompiler &f, RetType retType)
          : prevMaxStackBytes_(0),
            maxChildStackBytes_(0),
            spIncrement_(0),
            sig_(f.cx(), retType),
            regArgs_(f.cx()),
            stackArgs_(f.cx()),
            childClobbers_(false)
        {}
        Signature &sig() { return sig_; }
        const Signature &sig() const { return sig_; }
    };

    void startCallArgs(Call *call)
    {
        if (!curBlock_)
            return;
        call->prevMaxStackBytes_ = mirGen().resetAsmJSMaxStackArgBytes();
    }

    bool passArg(MDefinition *argDef, VarType type, Call *call)
    {
        if (!call->sig().appendArg(type))
            return false;

        if (!curBlock_)
            return true;

        uint32_t childStackBytes = mirGen().resetAsmJSMaxStackArgBytes();
        call->maxChildStackBytes_ = Max(call->maxChildStackBytes_, childStackBytes);
        if (childStackBytes > 0 && !call->stackArgs_.empty())
            call->childClobbers_ = true;

        ABIArg arg = call->abi_.next(type.toMIRType());
        if (arg.kind() == ABIArg::Stack) {
            MAsmJSPassStackArg *mir = MAsmJSPassStackArg::New(arg.offsetFromArgBase(), argDef);
            curBlock_->add(mir);
            if (!call->stackArgs_.append(mir))
                return false;
        } else {
            if (!call->regArgs_.append(MAsmJSCall::Arg(arg.reg(), argDef)))
                return false;
        }
        return true;
    }

    void finishCallArgs(Call *call)
    {
        if (!curBlock_)
            return;
        uint32_t parentStackBytes = call->abi_.stackBytesConsumedSoFar();
        uint32_t newStackBytes;
        if (call->childClobbers_) {
            call->spIncrement_ = AlignBytes(call->maxChildStackBytes_, StackAlignment);
            for (unsigned i = 0; i < call->stackArgs_.length(); i++)
                call->stackArgs_[i]->incrementOffset(call->spIncrement_);
            newStackBytes = Max(call->prevMaxStackBytes_,
                                call->spIncrement_ + parentStackBytes);
        } else {
            call->spIncrement_ = 0;
            newStackBytes = Max(call->prevMaxStackBytes_,
                                Max(call->maxChildStackBytes_, parentStackBytes));
        }
        mirGen_->setAsmJSMaxStackArgBytes(newStackBytes);
    }

  private:
    bool callPrivate(MAsmJSCall::Callee callee, const Call &call, MIRType returnType, MDefinition **def)
    {
        if (!curBlock_) {
            *def = NULL;
            return true;
        }
        MAsmJSCall *ins = MAsmJSCall::New(callee, call.regArgs_, returnType, call.spIncrement_);
        if (!ins)
            return false;
        curBlock_->add(ins);
        *def = ins;
        return true;
    }

  public:
    bool internalCall(const ModuleCompiler::Func &func, const Call &call, MDefinition **def)
    {
        MIRType returnType = func.sig().retType().toMIRType();
        return callPrivate(MAsmJSCall::Callee(func.code()), call, returnType, def);
    }

    bool funcPtrCall(const ModuleCompiler::FuncPtrTable &table, MDefinition *index,
                     const Call &call, MDefinition **def)
    {
        if (!curBlock_) {
            *def = NULL;
            return true;
        }

        MConstant *mask = MConstant::New(Int32Value(table.mask()));
        curBlock_->add(mask);
        MBitAnd *maskedIndex = MBitAnd::NewAsmJS(index, mask);
        curBlock_->add(maskedIndex);
        MAsmJSLoadFuncPtr *ptrFun = MAsmJSLoadFuncPtr::New(table.globalDataOffset(), maskedIndex);
        curBlock_->add(ptrFun);

        MIRType returnType = table.sig().retType().toMIRType();
        return callPrivate(MAsmJSCall::Callee(ptrFun), call, returnType, def);
    }

    bool ffiCall(unsigned exitIndex, const Call &call, MIRType returnType, MDefinition **def)
    {
        if (!curBlock_) {
            *def = NULL;
            return true;
        }

        JS_STATIC_ASSERT(offsetof(AsmJSModule::ExitDatum, exit) == 0);
        unsigned globalDataOffset = module().exitIndexToGlobalDataOffset(exitIndex);

        MAsmJSLoadFFIFunc *ptrFun = MAsmJSLoadFFIFunc::New(globalDataOffset);
        curBlock_->add(ptrFun);

        return callPrivate(MAsmJSCall::Callee(ptrFun), call, returnType, def);
    }

    bool builtinCall(void *builtin, const Call &call, MIRType returnType, MDefinition **def)
    {
        return callPrivate(MAsmJSCall::Callee(builtin), call, returnType, def);
    }

    

    void returnExpr(MDefinition *expr)
    {
        if (!curBlock_)
            return;
        MAsmJSReturn *ins = MAsmJSReturn::New(expr);
        curBlock_->end(ins);
        curBlock_ = NULL;
    }

    void returnVoid()
    {
        if (!curBlock_)
            return;
        MAsmJSVoidReturn *ins = MAsmJSVoidReturn::New();
        curBlock_->end(ins);
        curBlock_ = NULL;
    }

    bool branchAndStartThen(MDefinition *cond, MBasicBlock **thenBlock, MBasicBlock **elseBlock, ParseNode *thenPn, ParseNode* elsePn)
    {
        if (!curBlock_) {
            *thenBlock = NULL;
            *elseBlock = NULL;
            return true;
        }
        if (!newBlock(curBlock_, thenBlock, thenPn) || !newBlock(curBlock_, elseBlock, elsePn))
            return false;
        curBlock_->end(MTest::New(cond, *thenBlock, *elseBlock));
        curBlock_ = *thenBlock;
        return true;
    }

    bool appendThenBlock(BlockVector *thenBlocks) {
        if (!curBlock_)
            return true;
        return thenBlocks->append(curBlock_);
    }

    void joinIf(const BlockVector &thenBlocks, MBasicBlock *joinBlock)
    {
        if (!joinBlock)
            return;
        JS_ASSERT_IF(curBlock_, thenBlocks.back() == curBlock_);
        for (size_t i = 0; i < thenBlocks.length(); i++) {
            thenBlocks[i]->end(MGoto::New(joinBlock));
            joinBlock->addPredecessor(thenBlocks[i]);
        }
        curBlock_ = joinBlock;
        mirGraph().moveBlockToEnd(curBlock_);
    }

    void switchToElse(MBasicBlock *elseBlock)
    {
        if (!elseBlock)
            return;
        curBlock_ = elseBlock;
        mirGraph().moveBlockToEnd(curBlock_);
    }

    bool joinIfElse(const BlockVector &thenBlocks, ParseNode *pn)
    {
        if (!curBlock_ && thenBlocks.empty())
            return true;
        MBasicBlock *pred = curBlock_ ? curBlock_ : thenBlocks[0];
        MBasicBlock *join;
        if (!newBlock(pred, &join, pn))
            return false;
        if (curBlock_)
            curBlock_->end(MGoto::New(join));
        for (size_t i = 0; i < thenBlocks.length(); i++) {
            thenBlocks[i]->end(MGoto::New(join));
            if (pred == curBlock_ || i > 0)
                join->addPredecessor(thenBlocks[i]);
        }
        curBlock_ = join;
        return true;
    }

    void pushPhiInput(MDefinition *def)
    {
        if (!curBlock_)
            return;
        JS_ASSERT(curBlock_->stackDepth() == info().firstStackSlot());
        curBlock_->push(def);
    }

    MDefinition *popPhiOutput()
    {
        if (!curBlock_)
            return NULL;
        JS_ASSERT(curBlock_->stackDepth() == info().firstStackSlot() + 1);
        return curBlock_->pop();
    }

    bool startPendingLoop(ParseNode *pn, MBasicBlock **loopEntry, ParseNode *bodyStmt)
    {
        if (!loopStack_.append(pn) || !breakableStack_.append(pn))
            return false;
        JS_ASSERT_IF(curBlock_, curBlock_->loopDepth() == loopStack_.length() - 1);
        if (!curBlock_) {
            *loopEntry = NULL;
            return true;
        }
        *loopEntry = MBasicBlock::NewPendingLoopHeader(mirGraph(), info(), curBlock_, NULL);
        if (!*loopEntry)
            return false;
        mirGraph().addBlock(*loopEntry);
        noteBasicBlockPosition(*loopEntry, bodyStmt);
        (*loopEntry)->setLoopDepth(loopStack_.length());
        curBlock_->end(MGoto::New(*loopEntry));
        curBlock_ = *loopEntry;
        return true;
    }

    bool branchAndStartLoopBody(MDefinition *cond, MBasicBlock **afterLoop, ParseNode *bodyPn, ParseNode *afterPn)
    {
        if (!curBlock_) {
            *afterLoop = NULL;
            return true;
        }
        JS_ASSERT(curBlock_->loopDepth() > 0);
        MBasicBlock *body;
        if (!newBlock(curBlock_, &body, bodyPn))
            return false;
        if (cond->isConstant() && ToBoolean(cond->toConstant()->value())) {
            *afterLoop = NULL;
            curBlock_->end(MGoto::New(body));
        } else {
            if (!newBlockWithDepth(curBlock_, curBlock_->loopDepth() - 1, afterLoop, afterPn))
                return false;
            curBlock_->end(MTest::New(cond, body, *afterLoop));
        }
        curBlock_ = body;
        return true;
    }

  private:
    ParseNode *popLoop()
    {
        ParseNode *pn = loopStack_.back();
        JS_ASSERT(!unlabeledContinues_.has(pn));
        loopStack_.popBack();
        breakableStack_.popBack();
        return pn;
    }

  public:
    bool closeLoop(MBasicBlock *loopEntry, MBasicBlock *afterLoop)
    {
        ParseNode *pn = popLoop();
        if (!loopEntry) {
            JS_ASSERT(!afterLoop);
            JS_ASSERT(!curBlock_);
            JS_ASSERT(!unlabeledBreaks_.has(pn));
            return true;
        }
        JS_ASSERT(loopEntry->loopDepth() == loopStack_.length() + 1);
        JS_ASSERT_IF(afterLoop, afterLoop->loopDepth() == loopStack_.length());
        if (curBlock_) {
            JS_ASSERT(curBlock_->loopDepth() == loopStack_.length() + 1);
            curBlock_->end(MGoto::New(loopEntry));
            loopEntry->setBackedge(curBlock_);
        }
        curBlock_ = afterLoop;
        if (curBlock_)
            mirGraph().moveBlockToEnd(curBlock_);
        return bindUnlabeledBreaks(pn);
    }

    bool branchAndCloseDoWhileLoop(MDefinition *cond, MBasicBlock *loopEntry, ParseNode *afterLoopStmt)
    {
        ParseNode *pn = popLoop();
        if (!loopEntry) {
            JS_ASSERT(!curBlock_);
            JS_ASSERT(!unlabeledBreaks_.has(pn));
            return true;
        }
        JS_ASSERT(loopEntry->loopDepth() == loopStack_.length() + 1);
        if (curBlock_) {
            JS_ASSERT(curBlock_->loopDepth() == loopStack_.length() + 1);
            if (cond->isConstant()) {
                if (ToBoolean(cond->toConstant()->value())) {
                    curBlock_->end(MGoto::New(loopEntry));
                    loopEntry->setBackedge(curBlock_);
                    curBlock_ = NULL;
                } else {
                    MBasicBlock *afterLoop;
                    if (!newBlock(curBlock_, &afterLoop, afterLoopStmt))
                        return false;
                    curBlock_->end(MGoto::New(afterLoop));
                    curBlock_ = afterLoop;
                }
            } else {
                MBasicBlock *afterLoop;
                if (!newBlock(curBlock_, &afterLoop, afterLoopStmt))
                    return false;
                curBlock_->end(MTest::New(cond, loopEntry, afterLoop));
                loopEntry->setBackedge(curBlock_);
                curBlock_ = afterLoop;
            }
        }
        return bindUnlabeledBreaks(pn);
    }

    bool bindContinues(ParseNode *pn, const LabelVector *maybeLabels)
    {
        bool createdJoinBlock = false;
        if (UnlabeledBlockMap::Ptr p = unlabeledContinues_.lookup(pn)) {
            if (!bindBreaksOrContinues(&p->value, &createdJoinBlock, pn))
                return false;
            unlabeledContinues_.remove(p);
        }
        return bindLabeledBreaksOrContinues(maybeLabels, &labeledContinues_, &createdJoinBlock, pn);
    }

    bool bindLabeledBreaks(const LabelVector *maybeLabels, ParseNode *pn)
    {
        bool createdJoinBlock = false;
        return bindLabeledBreaksOrContinues(maybeLabels, &labeledBreaks_, &createdJoinBlock, pn);
    }

    bool addBreak(PropertyName *maybeLabel) {
        if (maybeLabel)
            return addBreakOrContinue(maybeLabel, &labeledBreaks_);
        return addBreakOrContinue(breakableStack_.back(), &unlabeledBreaks_);
    }

    bool addContinue(PropertyName *maybeLabel) {
        if (maybeLabel)
            return addBreakOrContinue(maybeLabel, &labeledContinues_);
        return addBreakOrContinue(loopStack_.back(), &unlabeledContinues_);
    }

    bool startSwitch(ParseNode *pn, MDefinition *expr, int32_t low, int32_t high,
                     MBasicBlock **switchBlock)
    {
        if (!breakableStack_.append(pn))
            return false;
        if (!curBlock_) {
            *switchBlock = NULL;
            return true;
        }
        curBlock_->end(MTableSwitch::New(expr, low, high));
        *switchBlock = curBlock_;
        curBlock_ = NULL;
        return true;
    }

    bool startSwitchCase(MBasicBlock *switchBlock, MBasicBlock **next, ParseNode *pn)
    {
        if (!switchBlock) {
            *next = NULL;
            return true;
        }
        if (!newBlock(switchBlock, next, pn))
            return false;
        if (curBlock_) {
            curBlock_->end(MGoto::New(*next));
            (*next)->addPredecessor(curBlock_);
        }
        curBlock_ = *next;
        return true;
    }

    bool startSwitchDefault(MBasicBlock *switchBlock, BlockVector *cases, MBasicBlock **defaultBlock, ParseNode *pn)
    {
        if (!startSwitchCase(switchBlock, defaultBlock, pn))
            return false;
        if (!*defaultBlock)
            return true;
        for (unsigned i = 0; i < cases->length(); i++) {
            if (!(*cases)[i]) {
                MBasicBlock *bb;
                if (!newBlock(switchBlock, &bb, NULL))
                    return false;
                bb->end(MGoto::New(*defaultBlock));
                (*defaultBlock)->addPredecessor(bb);
                (*cases)[i] = bb;
            }
        }
        mirGraph().moveBlockToEnd(*defaultBlock);
        return true;
    }

    bool joinSwitch(MBasicBlock *switchBlock, const BlockVector &cases, MBasicBlock *defaultBlock)
    {
        ParseNode *pn = breakableStack_.popCopy();
        if (!switchBlock)
            return true;
        MTableSwitch *mir = switchBlock->lastIns()->toTableSwitch();
        mir->addDefault(defaultBlock);
        for (unsigned i = 0; i < cases.length(); i++)
            mir->addCase(cases[i]);
        if (curBlock_) {
            MBasicBlock *next;
            if (!newBlock(curBlock_, &next, pn))
                return false;
            curBlock_->end(MGoto::New(next));
            curBlock_ = next;
        }
        return bindUnlabeledBreaks(pn);
    }

    

    MIRGenerator *extractMIR()
    {
        JS_ASSERT(mirGen_ != NULL);
        MIRGenerator *mirGen = mirGen_;
        mirGen_ = NULL;
        return mirGen;
    }

    
  private:
    void noteBasicBlockPosition(MBasicBlock *blk, ParseNode *pn)
    {
#if defined(JS_ION_PERF)
        if (pn) {
            unsigned line = 0U, column = 0U;
            m().parser().tokenStream.srcCoords.lineNumAndColumnIndex(pn->pn_pos.begin, &line, &column);
            blk->setLineno(line);
            blk->setColumnIndex(column);
        }
#endif
    }

    bool newBlockWithDepth(MBasicBlock *pred, unsigned loopDepth, MBasicBlock **block, ParseNode *pn)
    {
        *block = MBasicBlock::New(mirGraph(), info(), pred,  NULL, MBasicBlock::NORMAL);
        if (!*block)
            return false;
        noteBasicBlockPosition(*block, pn);
        mirGraph().addBlock(*block);
        (*block)->setLoopDepth(loopDepth);
        return true;
    }

    bool newBlock(MBasicBlock *pred, MBasicBlock **block, ParseNode *pn)
    {
        return newBlockWithDepth(pred, loopStack_.length(), block, pn);
    }

    bool bindBreaksOrContinues(BlockVector *preds, bool *createdJoinBlock, ParseNode *pn)
    {
        for (unsigned i = 0; i < preds->length(); i++) {
            MBasicBlock *pred = (*preds)[i];
            if (*createdJoinBlock) {
                pred->end(MGoto::New(curBlock_));
                curBlock_->addPredecessor(pred);
            } else {
                MBasicBlock *next;
                if (!newBlock(pred, &next, pn))
                    return false;
                pred->end(MGoto::New(next));
                if (curBlock_) {
                    curBlock_->end(MGoto::New(next));
                    next->addPredecessor(curBlock_);
                }
                curBlock_ = next;
                *createdJoinBlock = true;
            }
            JS_ASSERT(curBlock_->begin() == curBlock_->end());
        }
        preds->clear();
        return true;
    }

    bool bindLabeledBreaksOrContinues(const LabelVector *maybeLabels, LabeledBlockMap *map,
                                      bool *createdJoinBlock, ParseNode *pn)
    {
        if (!maybeLabels)
            return true;
        const LabelVector &labels = *maybeLabels;
        for (unsigned i = 0; i < labels.length(); i++) {
            if (LabeledBlockMap::Ptr p = map->lookup(labels[i])) {
                if (!bindBreaksOrContinues(&p->value, createdJoinBlock, pn))
                    return false;
                map->remove(p);
            }
        }
        return true;
    }

    template <class Key, class Map>
    bool addBreakOrContinue(Key key, Map *map)
    {
        if (!curBlock_)
            return true;
        typename Map::AddPtr p = map->lookupForAdd(key);
        if (!p) {
            BlockVector empty(m().cx());
            if (!map->add(p, key, Move(empty)))
                return false;
        }
        if (!p->value.append(curBlock_))
            return false;
        curBlock_ = NULL;
        return true;
    }

    bool bindUnlabeledBreaks(ParseNode *pn)
    {
        bool createdJoinBlock = false;
        if (UnlabeledBlockMap::Ptr p = unlabeledBreaks_.lookup(pn)) {
            if (!bindBreaksOrContinues(&p->value, &createdJoinBlock, pn))
                return false;
            unlabeledBreaks_.remove(p);
        }
        return true;
    }
};








static void AsmJSModuleObject_finalize(FreeOp *fop, JSObject *obj);
static void AsmJSModuleObject_trace(JSTracer *trc, JSObject *obj);

static const unsigned ASM_CODE_RESERVED_SLOT = 0;
static const unsigned ASM_CODE_NUM_RESERVED_SLOTS = 1;

static Class AsmJSModuleClass = {
    "AsmJSModuleObject",
    JSCLASS_IS_ANONYMOUS | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(ASM_CODE_NUM_RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    NULL,                    
    AsmJSModuleObject_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    AsmJSModuleObject_trace
};

AsmJSModule &
js::AsmJSModuleObjectToModule(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &AsmJSModuleClass);
    return *(AsmJSModule *)obj->getReservedSlot(ASM_CODE_RESERVED_SLOT).toPrivate();
}

bool
js::IsAsmJSModuleObject(JSObject *obj)
{
    return obj->getClass() == &AsmJSModuleClass;
}

static const unsigned ASM_MODULE_FUNCTION_MODULE_OBJECT_SLOT = 0;

JSObject &
js::AsmJSModuleObject(JSFunction *moduleFun)
{
    return moduleFun->getExtendedSlot(ASM_MODULE_FUNCTION_MODULE_OBJECT_SLOT).toObject();
}

void
js::SetAsmJSModuleObject(JSFunction *moduleFun, JSObject *moduleObj)
{
    moduleFun->setExtendedSlot(ASM_MODULE_FUNCTION_MODULE_OBJECT_SLOT, OBJECT_TO_JSVAL(moduleObj));
}

static void
AsmJSModuleObject_finalize(FreeOp *fop, JSObject *obj)
{
    fop->delete_(&AsmJSModuleObjectToModule(obj));
}

static void
AsmJSModuleObject_trace(JSTracer *trc, JSObject *obj)
{
    AsmJSModuleObjectToModule(obj).trace(trc);
}

static JSObject *
NewAsmJSModuleObject(JSContext *cx, ScopedJSDeletePtr<AsmJSModule> *module)
{
    JSObject *obj = NewObjectWithGivenProto(cx, &AsmJSModuleClass, NULL, NULL);
    if (!obj)
        return NULL;

    obj->setReservedSlot(ASM_CODE_RESERVED_SLOT, PrivateValue(module->forget()));
    return obj;
}




static bool
CheckIdentifier(ModuleCompiler &m, ParseNode *usepn, PropertyName *name)
{
    if (name == m.cx()->names().arguments || name == m.cx()->names().eval)
        return m.failName(usepn, "'%s' is not an allowed identifier", name);
    return true;
}

static bool
CheckModuleLevelName(ModuleCompiler &m, ParseNode *usepn, PropertyName *name)
{
    if (!CheckIdentifier(m, usepn, name))
        return false;

    if (name == m.moduleFunctionName() ||
        name == m.module().globalArgumentName() ||
        name == m.module().importArgumentName() ||
        name == m.module().bufferArgumentName() ||
        m.lookupGlobal(name))
    {
        return m.failName(usepn, "duplicate name '%s' not allowed", name);
    }

    return true;
}

static bool
CheckFunctionHead(ModuleCompiler &m, ParseNode *fn)
{
    JSFunction *fun = FunctionObject(fn);
    if (fun->hasRest())
        return m.fail(fn, "rest args not allowed");
    if (fun->hasDefaults())
        return m.fail(fn, "default args not allowed");
    if (fun->isExprClosure())
        return m.fail(fn, "expression closures not allowed");
    if (fn->pn_funbox->hasDestructuringArgs)
        return m.fail(fn, "destructuring args not allowed");
    return true;
}

static bool
CheckArgument(ModuleCompiler &m, ParseNode *arg, PropertyName **name)
{
    if (!IsDefinition(arg))
        return m.fail(arg, "duplicate argument name not allowed");

    if (MaybeDefinitionInitializer(arg))
        return m.fail(arg, "default arguments not allowed");

    if (!CheckIdentifier(m, arg, arg->name()))
        return false;

    *name = arg->name();
    return true;
}

static bool
CheckModuleArgument(ModuleCompiler &m, ParseNode *arg, PropertyName **name)
{
    if (!CheckArgument(m, arg, name))
        return false;

    if (!CheckModuleLevelName(m, arg, *name))
        return false;

    return true;
}

static bool
CheckModuleArguments(ModuleCompiler &m, ParseNode *fn)
{
    unsigned numFormals;
    ParseNode *arg1 = FunctionArgsList(fn, &numFormals);
    ParseNode *arg2 = arg1 ? NextNode(arg1) : NULL;
    ParseNode *arg3 = arg2 ? NextNode(arg2) : NULL;

    if (numFormals > 3)
        return m.fail(fn, "asm.js modules takes at most 3 argument");

    PropertyName *arg1Name = NULL;
    if (numFormals >= 1 && !CheckModuleArgument(m, arg1, &arg1Name))
        return false;
    m.initGlobalArgumentName(arg1Name);

    PropertyName *arg2Name = NULL;
    if (numFormals >= 2 && !CheckModuleArgument(m, arg2, &arg2Name))
        return false;
    m.initImportArgumentName(arg2Name);

    PropertyName *arg3Name = NULL;
    if (numFormals >= 3 && !CheckModuleArgument(m, arg3, &arg3Name))
        return false;
    m.initBufferArgumentName(arg3Name);

    return true;
}

static bool
CheckPrecedingStatements(ModuleCompiler &m, ParseNode *stmtList)
{
    JS_ASSERT(stmtList->isKind(PNK_STATEMENTLIST));

    if (ListLength(stmtList) != 0)
        return m.fail(ListHead(stmtList), "invalid asm.js statement");

    return true;
}

static bool
CheckGlobalVariableInitConstant(ModuleCompiler &m, PropertyName *varName, ParseNode *initNode)
{
    NumLit literal = ExtractNumericLiteral(initNode);
    VarType type;
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::NegativeInt:
      case NumLit::BigUnsigned:
        type = VarType::Int;
        break;
      case NumLit::Double:
        type = VarType::Double;
        break;
      case NumLit::OutOfRangeInt:
        return m.fail(initNode, "global initializer is out of representable integer range");
    }
    return m.addGlobalVarInitConstant(varName, type, literal.value());
}

static bool
CheckTypeAnnotation(ModuleCompiler &m, ParseNode *coercionNode, AsmJSCoercion *coercion,
                    ParseNode **coercedExpr = NULL)
{
    switch (coercionNode->getKind()) {
      case PNK_BITOR: {
        ParseNode *rhs = BinaryRight(coercionNode);

        if (!IsNumericLiteral(rhs))
            return m.fail(rhs, "must use |0 for argument/return coercion");

        NumLit rhsLiteral = ExtractNumericLiteral(rhs);
        if (rhsLiteral.which() != NumLit::Fixnum || rhsLiteral.toInt32() != 0)
            return m.fail(rhs, "must use |0 for argument/return coercion");

        *coercion = AsmJS_ToInt32;
        if (coercedExpr)
            *coercedExpr = BinaryLeft(coercionNode);
        return true;
      }
      case PNK_POS: {
        *coercion = AsmJS_ToNumber;
        if (coercedExpr)
            *coercedExpr = UnaryKid(coercionNode);
        return true;
      }
      default:;
    }

    return m.fail(coercionNode, "in coercion expression, the expression must be of the form +x or x|0");
}

static bool
CheckGlobalVariableInitImport(ModuleCompiler &m, PropertyName *varName, ParseNode *initNode)
{
    AsmJSCoercion coercion;
    ParseNode *coercedExpr;
    if (!CheckTypeAnnotation(m, initNode, &coercion, &coercedExpr))
        return false;

    if (!coercedExpr->isKind(PNK_DOT))
        return m.failName(coercedExpr, "invalid import expression for global '%s'", varName);

    ParseNode *base = DotBase(coercedExpr);
    PropertyName *field = DotMember(coercedExpr);

    PropertyName *importName = m.module().importArgumentName();
    if (!importName)
        return m.fail(coercedExpr, "cannot import without an asm.js foreign parameter");
    if (!IsUseOfName(base, importName))
        return m.failName(coercedExpr, "base of import expression must be '%s'", importName);

    return m.addGlobalVarImport(varName, field, coercion);
}

static bool
CheckNewArrayView(ModuleCompiler &m, PropertyName *varName, ParseNode *newExpr)
{
    ParseNode *ctorExpr = ListHead(newExpr);
    if (!ctorExpr->isKind(PNK_DOT))
        return m.fail(ctorExpr, "only valid 'new' import is 'new global.*Array(buf)'");

    ParseNode *base = DotBase(ctorExpr);
    PropertyName *field = DotMember(ctorExpr);

    PropertyName *globalName = m.module().globalArgumentName();
    if (!globalName)
        return m.fail(base, "cannot create array view without an asm.js global parameter");
    if (!IsUseOfName(base, globalName))
        return m.failName(base, "expecting '%s.*Array", globalName);

    ParseNode *bufArg = NextNode(ctorExpr);
    if (!bufArg || NextNode(bufArg) != NULL)
        return m.fail(ctorExpr, "array view constructor takes exactly one argument");

    PropertyName *bufferName = m.module().bufferArgumentName();
    if (!bufferName)
        return m.fail(bufArg, "cannot create array view without an asm.js heap parameter");
    if (!IsUseOfName(bufArg, bufferName))
        return m.failName(bufArg, "argument to array view constructor must be '%s'", bufferName);

    JSAtomState &names = m.cx()->names();
    ArrayBufferView::ViewType type;
    if (field == names.Int8Array)
        type = ArrayBufferView::TYPE_INT8;
    else if (field == names.Uint8Array)
        type = ArrayBufferView::TYPE_UINT8;
    else if (field == names.Int16Array)
        type = ArrayBufferView::TYPE_INT16;
    else if (field == names.Uint16Array)
        type = ArrayBufferView::TYPE_UINT16;
    else if (field == names.Int32Array)
        type = ArrayBufferView::TYPE_INT32;
    else if (field == names.Uint32Array)
        type = ArrayBufferView::TYPE_UINT32;
    else if (field == names.Float32Array)
        type = ArrayBufferView::TYPE_FLOAT32;
    else if (field == names.Float64Array)
        type = ArrayBufferView::TYPE_FLOAT64;
    else
        return m.fail(ctorExpr, "could not match typed array name");

    return m.addArrayView(varName, type, field);
}

static bool
CheckGlobalDotImport(ModuleCompiler &m, PropertyName *varName, ParseNode *initNode)
{
    ParseNode *base = DotBase(initNode);
    PropertyName *field = DotMember(initNode);

    if (base->isKind(PNK_DOT)) {
        ParseNode *global = DotBase(base);
        PropertyName *math = DotMember(base);
        if (!IsUseOfName(global, m.module().globalArgumentName()) || math != m.cx()->names().Math)
            return m.fail(base, "expecting global.Math");

        AsmJSMathBuiltin mathBuiltin;
        if (!m.lookupStandardLibraryMathName(field, &mathBuiltin))
            return m.failName(initNode, "'%s' is not a standard Math builtin", field);

        return m.addMathBuiltin(varName, mathBuiltin, field);
    }

    if (IsUseOfName(base, m.module().globalArgumentName())) {
        if (field == m.cx()->names().NaN)
            return m.addGlobalConstant(varName, js_NaN, field);
        if (field == m.cx()->names().Infinity)
            return m.addGlobalConstant(varName, js_PositiveInfinity, field);
        return m.failName(initNode, "'%s' is not a standard global constant", field);
    }

    if (IsUseOfName(base, m.module().importArgumentName()))
        return m.addFFI(varName, field);

    return m.fail(initNode, "expecting c.y where c is either the global or foreign parameter");
}

static bool
CheckModuleGlobal(ModuleCompiler &m, ParseNode *var)
{
    if (!IsDefinition(var))
        return m.fail(var, "import variable names must be unique");

    if (!CheckModuleLevelName(m, var, var->name()))
        return false;

    ParseNode *initNode = MaybeDefinitionInitializer(var);
    if (!initNode)
        return m.fail(var, "module import needs initializer");

    if (IsNumericLiteral(initNode))
        return CheckGlobalVariableInitConstant(m, var->name(), initNode);

    if (initNode->isKind(PNK_BITOR) || initNode->isKind(PNK_POS))
        return CheckGlobalVariableInitImport(m, var->name(), initNode);

    if (initNode->isKind(PNK_NEW))
        return CheckNewArrayView(m, var->name(), initNode);

    if (initNode->isKind(PNK_DOT))
        return CheckGlobalDotImport(m, var->name(), initNode);

    return m.fail(initNode, "unsupported import expression");
}

static bool
CheckModuleGlobals(ModuleCompiler &m)
{
    while (true) {
        ParseNode *varStmt;
        if (!ParseVarStatement(m.parser(), &varStmt))
            return false;
        if (!varStmt)
            break;
        for (ParseNode *var = VarListHead(varStmt); var; var = NextNode(var)) {
            if (!CheckModuleGlobal(m, var))
                return false;
        }
    }

    return true;
}

static bool
ArgFail(FunctionCompiler &f, PropertyName *argName, ParseNode *stmt)
{
    return f.failName(stmt, "expecting argument type declaration for '%s' of the "
                      "form 'arg = arg|0' or 'arg = +arg'", argName);
}

static bool
CheckArgumentType(FunctionCompiler &f, ParseNode *stmt, PropertyName *name, VarType *type)
{
    if (!stmt || !IsExpressionStatement(stmt))
        return ArgFail(f, name, stmt ? stmt : f.fn());

    ParseNode *initNode = ExpressionStatementExpr(stmt);
    if (!initNode || !initNode->isKind(PNK_ASSIGN))
        return ArgFail(f, name, stmt);

    ParseNode *argNode = BinaryLeft(initNode);
    ParseNode *coercionNode = BinaryRight(initNode);

    if (!IsUseOfName(argNode, name))
        return ArgFail(f, name, stmt);

    ParseNode *coercedExpr;
    AsmJSCoercion coercion;
    if (!CheckTypeAnnotation(f.m(), coercionNode, &coercion, &coercedExpr))
        return false;

    if (!IsUseOfName(coercedExpr, name))
        return ArgFail(f, name, stmt);

    *type = VarType(coercion);
    return true;
}

static bool
CheckArguments(FunctionCompiler &f, ParseNode **stmtIter, VarTypeVector *argTypes)
{
    ParseNode *stmt = *stmtIter;

    unsigned numFormals;
    ParseNode *argpn = FunctionArgsList(f.fn(), &numFormals);

    for (unsigned i = 0; i < numFormals; i++, argpn = NextNode(argpn), stmt = NextNode(stmt)) {
        PropertyName *name;
        if (!CheckArgument(f.m(), argpn, &name))
            return false;

        VarType type;
        if (!CheckArgumentType(f, stmt, name, &type))
            return false;

        if (!argTypes->append(type))
            return false;

        if (!f.addFormal(argpn, name, type))
            return false;
    }

    *stmtIter = stmt;
    return true;
}

static bool
CheckFinalReturn(FunctionCompiler &f, ParseNode *stmt, RetType *retType)
{
    if (stmt && stmt->isKind(PNK_RETURN)) {
        if (ParseNode *coercionNode = UnaryKid(stmt)) {
            if (IsNumericLiteral(coercionNode)) {
                switch (ExtractNumericLiteral(coercionNode).which()) {
                  case NumLit::BigUnsigned:
                  case NumLit::OutOfRangeInt:
                    return f.fail(coercionNode, "returned literal is out of integer range");
                  case NumLit::Fixnum:
                  case NumLit::NegativeInt:
                    *retType = RetType::Signed;
                    break;
                  case NumLit::Double:
                    *retType = RetType::Double;
                    break;
                }
                return true;
            }

            AsmJSCoercion coercion;
            if (!CheckTypeAnnotation(f.m(), coercionNode, &coercion))
                return false;

            *retType = RetType(coercion);
            return true;
        }

        *retType = RetType::Void;
        return true;
    }

    *retType = RetType::Void;
    f.returnVoid();
    return true;
}

static bool
CheckVariable(FunctionCompiler &f, ParseNode *var)
{
    if (!IsDefinition(var))
        return f.fail(var, "local variable names must not restate argument names");

    PropertyName *name = var->name();

    if (!CheckIdentifier(f.m(), var, name))
        return false;

    ParseNode *initNode = MaybeDefinitionInitializer(var);
    if (!initNode)
        return f.failName(var, "var '%s' needs explicit type declaration via an initial value", name);

    if (!IsNumericLiteral(initNode))
        return f.failName(initNode, "initializer for '%s' needs to be a numeric literal", name);

    NumLit literal = ExtractNumericLiteral(initNode);
    VarType type;
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::NegativeInt:
      case NumLit::BigUnsigned:
        type = VarType::Int;
        break;
      case NumLit::Double:
        type = VarType::Double;
        break;
      case NumLit::OutOfRangeInt:
        return f.failName(initNode, "initializer for '%s' is out of range", name);
    }

    return f.addVariable(var, name, type, literal.value());
}

static bool
CheckVariables(FunctionCompiler &f, ParseNode **stmtIter)
{
    ParseNode *stmt = *stmtIter;

    for (; stmt && stmt->isKind(PNK_VAR); stmt = NextNonEmptyStatement(stmt)) {
        for (ParseNode *var = VarListHead(stmt); var; var = NextNode(var)) {
            if (!CheckVariable(f, var))
                return false;
        }
    }

    *stmtIter = stmt;
    return true;
}

static bool
CheckExpr(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type);

static bool
CheckNumericLiteral(FunctionCompiler &f, ParseNode *num, MDefinition **def, Type *type)
{
    JS_ASSERT(IsNumericLiteral(num));
    NumLit literal = ExtractNumericLiteral(num);

    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::NegativeInt:
      case NumLit::BigUnsigned:
      case NumLit::Double:
        break;
      case NumLit::OutOfRangeInt:
        return f.fail(num, "numeric literal out of representable integer range");
    }

    *type = literal.type();
    *def = f.constant(literal.value());
    return true;
}

static bool
CheckVarRef(FunctionCompiler &f, ParseNode *varRef, MDefinition **def, Type *type)
{
    PropertyName *name = varRef->name();

    if (const FunctionCompiler::Local *local = f.lookupLocal(name)) {
        *def = f.getLocalDef(*local);
        *type = local->type.toType();
        return true;
    }

    if (const ModuleCompiler::Global *global = f.lookupGlobal(name)) {
        switch (global->which()) {
          case ModuleCompiler::Global::Constant:
            *def = f.constant(DoubleValue(global->constant()));
            *type = Type::Double;
            break;
          case ModuleCompiler::Global::Variable:
            *def = f.loadGlobalVar(*global);
            *type = global->varType().toType();
            break;
          case ModuleCompiler::Global::Function:
          case ModuleCompiler::Global::FFI:
          case ModuleCompiler::Global::MathBuiltin:
          case ModuleCompiler::Global::FuncPtrTable:
          case ModuleCompiler::Global::ArrayView:
            return f.failName(varRef, "'%s' may not be accessed by ordinary expressions", name);
        }
        return true;
    }

    return f.failName(varRef, "'%s' not found in local or asm.js module scope", name);
}

static bool
CheckArrayAccess(FunctionCompiler &f, ParseNode *elem, ArrayBufferView::ViewType *viewType,
                 MDefinition **def)
{
    ParseNode *viewName = ElemBase(elem);
    ParseNode *indexExpr = ElemIndex(elem);

    if (!viewName->isKind(PNK_NAME))
        return f.fail(viewName, "base of array access must be a typed array view name");

    const ModuleCompiler::Global *global = f.lookupGlobal(viewName->name());
    if (!global || global->which() != ModuleCompiler::Global::ArrayView)
        return f.fail(viewName, "base of array access must be a typed array view name");

    *viewType = global->viewType();

    uint32_t pointer;
    if (IsLiteralUint32(indexExpr, &pointer)) {
        pointer <<= TypedArrayShift(*viewType);
        *def = f.constant(Int32Value(pointer));
        return true;
    }

    MDefinition *pointerDef;
    if (indexExpr->isKind(PNK_RSH)) {
        ParseNode *shiftNode = BinaryRight(indexExpr);
        ParseNode *pointerNode = BinaryLeft(indexExpr);

        uint32_t shift;
        if (!IsLiteralUint32(shiftNode, &shift))
            return f.failf(shiftNode, "shift amount must be constant");

        unsigned requiredShift = TypedArrayShift(*viewType);
        if (shift != requiredShift)
            return f.failf(shiftNode, "shift amount must be %u", requiredShift);

        Type pointerType;
        if (!CheckExpr(f, pointerNode, &pointerDef, &pointerType))
            return false;

        if (!pointerType.isIntish())
            return f.failf(indexExpr, "%s is not a subtype of int", pointerType.toChars());
    } else {
        if (TypedArrayShift(*viewType) != 0)
            return f.fail(indexExpr, "index expression isn't shifted; must be an Int8/Uint8 access");

        Type pointerType;
        if (!CheckExpr(f, indexExpr, &pointerDef, &pointerType))
            return false;

        if (!pointerType.isInt())
            return f.failf(indexExpr, "%s is not a subtype of int", pointerType.toChars());
    }

    
    
    
    int32_t mask = ~((uint32_t(1) << TypedArrayShift(*viewType)) - 1);
    *def = f.bitwise<MBitAnd>(pointerDef, f.constant(Int32Value(mask)));
    return true;
}

static bool
CheckArrayLoad(FunctionCompiler &f, ParseNode *elem, MDefinition **def, Type *type)
{
    ArrayBufferView::ViewType viewType;
    MDefinition *pointerDef;
    if (!CheckArrayAccess(f, elem, &viewType, &pointerDef))
        return false;

    *def = f.loadHeap(viewType, pointerDef);
    *type = TypedArrayLoadType(viewType);
    return true;
}

static bool
CheckStoreArray(FunctionCompiler &f, ParseNode *lhs, ParseNode *rhs, MDefinition **def, Type *type)
{
    ArrayBufferView::ViewType viewType;
    MDefinition *pointerDef;
    if (!CheckArrayAccess(f, lhs, &viewType, &pointerDef))
        return false;

    MDefinition *rhsDef;
    Type rhsType;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    switch (TypedArrayStoreType(viewType)) {
      case ArrayStore_Intish:
        if (!rhsType.isIntish())
            return f.failf(lhs, "%s is not a subtype of intish", rhsType.toChars());
        break;
      case ArrayStore_Doublish:
        if (!rhsType.isDoublish())
            return f.failf(lhs, "%s is not a subtype of doublish", rhsType.toChars());
        break;
    }

    f.storeHeap(viewType, pointerDef, rhsDef);

    *def = rhsDef;
    *type = rhsType;
    return true;
}

static bool
CheckAssignName(FunctionCompiler &f, ParseNode *lhs, ParseNode *rhs, MDefinition **def, Type *type)
{
    Rooted<PropertyName *> name(f.cx(), lhs->name());

    MDefinition *rhsDef;
    Type rhsType;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if (const FunctionCompiler::Local *lhsVar = f.lookupLocal(name)) {
        if (!(rhsType <= lhsVar->type)) {
            return f.failf(lhs, "%s is not a subtype of %s",
                           rhsType.toChars(), lhsVar->type.toType().toChars());
        }
        f.assign(*lhsVar, rhsDef);
    } else if (const ModuleCompiler::Global *global = f.lookupGlobal(name)) {
        if (global->which() != ModuleCompiler::Global::Variable)
            return f.failName(lhs, "'%s' is not a mutable variable", name);
        if (!(rhsType <= global->varType())) {
            return f.failf(lhs, "%s is not a subtype of %s",
                           rhsType.toChars(), global->varType().toType().toChars());
        }
        f.storeGlobalVar(*global, rhsDef);
    } else {
        return f.failName(lhs, "'%s' not found in local or asm.js module scope", name);
    }

    *def = rhsDef;
    *type = rhsType;
    return true;
}

static bool
CheckAssign(FunctionCompiler &f, ParseNode *assign, MDefinition **def, Type *type)
{
    JS_ASSERT(assign->isKind(PNK_ASSIGN));
    ParseNode *lhs = BinaryLeft(assign);
    ParseNode *rhs = BinaryRight(assign);

    if (lhs->getKind() == PNK_ELEM)
        return CheckStoreArray(f, lhs, rhs, def, type);

    if (lhs->getKind() == PNK_NAME)
        return CheckAssignName(f, lhs, rhs, def, type);

    return f.fail(assign, "left-hand side of assignment must be a variable or array access");
}

static bool
CheckMathIMul(FunctionCompiler &f, ParseNode *call, RetType retType, MDefinition **def, Type *type)
{
    if (CallArgListLength(call) != 2)
        return f.fail(call, "Math.imul must be passed 2 arguments");

    ParseNode *lhs = CallArgList(call);
    ParseNode *rhs = NextNode(lhs);

    MDefinition *lhsDef;
    Type lhsType;
    if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
        return false;

    MDefinition *rhsDef;
    Type rhsType;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if (!lhsType.isIntish())
        return f.failf(lhs, "%s is not a subtype of intish", lhsType.toChars());
    if (!rhsType.isIntish())
        return f.failf(rhs, "%s is not a subtype of intish", rhsType.toChars());
    if (retType != RetType::Signed)
        return f.failf(call, "return type is signed, used as %s", retType.toType().toChars());

    *def = f.mul(lhsDef, rhsDef, MIRType_Int32, MMul::Integer);
    *type = Type::Signed;
    return true;
}

static bool
CheckMathAbs(FunctionCompiler &f, ParseNode *call, RetType retType, MDefinition **def, Type *type)
{
    if (CallArgListLength(call) != 1)
        return f.fail(call, "Math.abs must be passed 1 argument");

    ParseNode *arg = CallArgList(call);

    MDefinition *argDef;
    Type argType;
    if (!CheckExpr(f, arg, &argDef, &argType))
        return false;

    if (argType.isSigned()) {
        if (retType != RetType::Signed)
            return f.failf(call, "return type is signed, used as %s", retType.toType().toChars());
        *def = f.unary<MAbs>(argDef, MIRType_Int32);
        *type = Type::Signed;
        return true;
    }

    if (argType.isDoublish()) {
        if (retType != RetType::Double)
            return f.failf(call, "return type is double, used as %s", retType.toType().toChars());
        *def = f.unary<MAbs>(argDef, MIRType_Double);
        *type = Type::Double;
        return true;
    }

    return f.failf(call, "%s is not a subtype of signed or doublish", argType.toChars());
}

static bool
CheckMathSqrt(FunctionCompiler &f, ParseNode *call, RetType retType, MDefinition **def, Type *type)
{
    if (CallArgListLength(call) != 1)
        return f.fail(call, "Math.sqrt must be passed 1 argument");

    ParseNode *arg = CallArgList(call);

    MDefinition *argDef;
    Type argType;
    if (!CheckExpr(f, arg, &argDef, &argType))
        return false;

    if (argType.isDoublish()) {
        if (retType != RetType::Double)
            return f.failf(call, "return type is double, used as %s", retType.toType().toChars());
        *def = f.unary<MSqrt>(argDef, MIRType_Double);
        *type = Type::Double;
        return true;
    }

    return f.failf(call, "%s is not a subtype of doublish", argType.toChars());
}

typedef bool (*CheckArgType)(FunctionCompiler &f, ParseNode *argNode, Type type);

static bool
CheckCallArgs(FunctionCompiler &f, ParseNode *callNode, CheckArgType checkArg,
              FunctionCompiler::Call *call)
{
    f.startCallArgs(call);

    ParseNode *argNode = CallArgList(callNode);
    for (unsigned i = 0; i < CallArgListLength(callNode); i++, argNode = NextNode(argNode)) {
        MDefinition *def;
        Type type;
        if (!CheckExpr(f, argNode, &def, &type))
            return false;

        if (!checkArg(f, argNode, type))
            return false;

        if (!f.passArg(def, VarType::FromCheckedType(type), call))
            return false;
    }

    f.finishCallArgs(call);
    return true;
}

static bool
CheckSignatureAgainstExisting(ModuleCompiler &m, ParseNode *usepn, const Signature &sig,
                              const Signature &existing)
{
    if (sig.args().length() != existing.args().length()) {
        return m.failf(usepn, "incompatible number of arguments (%u here vs. %u before)",
                       sig.args().length(), existing.args().length());
    }

    for (unsigned i = 0; i < sig.args().length(); i++) {
        if (sig.arg(i) != existing.arg(i)) {
            return m.failf(usepn, "incompatible type for argument %u: (%s here vs. %s before)",
                           i, sig.arg(i).toType().toChars(), existing.arg(i).toType().toChars());
        }
    }

    if (sig.retType() != existing.retType()) {
        return m.failf(usepn, "%s incompatible with previous return of type %s",
                       sig.retType().toType().toChars(), existing.retType().toType().toChars());
    }

    JS_ASSERT(sig == existing);
    return true;
}

static bool
CheckFunctionSignature(ModuleCompiler &m, ParseNode *usepn, MoveRef<Signature> sig, PropertyName *name,
                       ModuleCompiler::Func **func)
{
    ModuleCompiler::Func *existing = m.lookupFunction(name);
    if (!existing) {
        if (!CheckModuleLevelName(m, usepn, name))
            return false;
        return m.addFunction(name, sig, func);
    }

    if (!CheckSignatureAgainstExisting(m, usepn, sig, existing->sig()))
        return false;

    *func = existing;
    return true;
}

static bool
CheckIsVarType(FunctionCompiler &f, ParseNode *argNode, Type type)
{
    if (!type.isVarType())
        return f.failf(argNode, "%s is not a subtype of int or double", type.toChars());
    return true;
}

static bool
CheckInternalCall(FunctionCompiler &f, ParseNode *callNode, PropertyName *calleeName,
                  RetType retType, MDefinition **def, Type *type)
{
    FunctionCompiler::Call call(f, retType);

    if (!CheckCallArgs(f, callNode, CheckIsVarType, &call))
        return false;

    ModuleCompiler::Func *callee;
    if (!CheckFunctionSignature(f.m(), callNode, Move(call.sig()), calleeName, &callee))
        return false;

    if (!f.internalCall(*callee, call, def))
        return false;

    *type = retType.toType();
    return true;
}

static bool
CheckFuncPtrTableAgainstExisting(ModuleCompiler &m, ParseNode *usepn,
                                 PropertyName *name, MoveRef<Signature> sig, unsigned mask,
                                 ModuleCompiler::FuncPtrTable **tableOut)
{
    if (const ModuleCompiler::Global *existing = m.lookupGlobal(name)) {
        if (existing->which() != ModuleCompiler::Global::FuncPtrTable)
            return m.failName(usepn, "'%s' is not a function-pointer table", name);

        ModuleCompiler::FuncPtrTable &table = m.funcPtrTable(existing->funcPtrTableIndex());
        if (mask != table.mask())
            return m.failf(usepn, "mask does not match previous value (%u)", table.mask());

        if (!CheckSignatureAgainstExisting(m, usepn, sig, table.sig()))
            return false;

        *tableOut = &table;
        return true;
    }

    if (!CheckModuleLevelName(m, usepn, name))
        return false;

    return m.addFuncPtrTable(name, sig, mask, tableOut);
}

static bool
CheckFuncPtrCall(FunctionCompiler &f, ParseNode *callNode, RetType retType, MDefinition **def, Type *type)
{
    ParseNode *callee = CallCallee(callNode);
    ParseNode *tableNode = ElemBase(callee);
    ParseNode *indexExpr = ElemIndex(callee);

    if (!tableNode->isKind(PNK_NAME))
        return f.fail(tableNode, "expecting name of function-pointer array");

    PropertyName *name = tableNode->name();
    if (const ModuleCompiler::Global *existing = f.lookupGlobal(name)) {
        if (existing->which() != ModuleCompiler::Global::FuncPtrTable)
            return f.failName(tableNode, "'%s' is not the name of a function-pointer array", name);
    }

    if (!indexExpr->isKind(PNK_BITAND))
        return f.fail(indexExpr, "function-pointer table index expression needs & mask");

    ParseNode *indexNode = BinaryLeft(indexExpr);
    ParseNode *maskNode = BinaryRight(indexExpr);

    uint32_t mask;
    if (!IsLiteralUint32(maskNode, &mask) || mask == UINT32_MAX || !IsPowerOfTwo(mask + 1))
        return f.fail(maskNode, "function-pointer table index mask value must be a power of two");

    MDefinition *indexDef;
    Type indexType;
    if (!CheckExpr(f, indexNode, &indexDef, &indexType))
        return false;

    if (!indexType.isIntish())
        return f.failf(indexNode, "%s is not a subtype of intish", indexType.toChars());

    FunctionCompiler::Call call(f, retType);

    if (!CheckCallArgs(f, callNode, CheckIsVarType, &call))
        return false;

    ModuleCompiler::FuncPtrTable *table;
    if (!CheckFuncPtrTableAgainstExisting(f.m(), tableNode, name, Move(call.sig()), mask, &table))
        return false;

    if (!f.funcPtrCall(*table, indexDef, call, def))
        return false;

    *type = retType.toType();
    return true;
}

static bool
CheckIsExternType(FunctionCompiler &f, ParseNode *argNode, Type type)
{
    if (!type.isExtern())
        return f.failf(argNode, "%s is not a subtype of extern", type.toChars());
    return true;
}

static bool
CheckFFICall(FunctionCompiler &f, ParseNode *callNode, unsigned ffiIndex, RetType retType,
             MDefinition **def, Type *type)
{
    PropertyName *calleeName = CallCallee(callNode)->name();

    FunctionCompiler::Call call(f, retType);
    if (!CheckCallArgs(f, callNode, CheckIsExternType, &call))
        return false;

    unsigned exitIndex;
    if (!f.m().addExit(ffiIndex, calleeName, Move(call.sig()), &exitIndex))
        return false;

    if (!f.ffiCall(exitIndex, call, retType.toMIRType(), def))
        return false;

    *type = retType.toType();
    return true;
}

static inline void *
UnaryMathFunCast(double (*pf)(double))
{
    return JS_FUNC_TO_DATA_PTR(void*, pf);
}

static inline void *
BinaryMathFunCast(double (*pf)(double, double))
{
    return JS_FUNC_TO_DATA_PTR(void*, pf);
}

static bool
CheckIsDoublish(FunctionCompiler &f, ParseNode *argNode, Type type)
{
    if (!type.isDoublish())
        return f.failf(argNode, "%s is not a subtype of doublish", type.toChars());
    return true;
}

static bool
CheckMathBuiltinCall(FunctionCompiler &f, ParseNode *callNode, AsmJSMathBuiltin mathBuiltin,
                     RetType retType, MDefinition **def, Type *type)
{
    unsigned arity = 0;
    void *callee = NULL;
    switch (mathBuiltin) {
      case AsmJSMathBuiltin_imul:  return CheckMathIMul(f, callNode, retType, def, type);
      case AsmJSMathBuiltin_abs:   return CheckMathAbs(f, callNode, retType, def, type);
      case AsmJSMathBuiltin_sin:   arity = 1; callee = UnaryMathFunCast(sin);        break;
      case AsmJSMathBuiltin_cos:   arity = 1; callee = UnaryMathFunCast(cos);        break;
      case AsmJSMathBuiltin_tan:   arity = 1; callee = UnaryMathFunCast(tan);        break;
      case AsmJSMathBuiltin_asin:  arity = 1; callee = UnaryMathFunCast(asin);       break;
      case AsmJSMathBuiltin_acos:  arity = 1; callee = UnaryMathFunCast(acos);       break;
      case AsmJSMathBuiltin_atan:  arity = 1; callee = UnaryMathFunCast(atan);       break;
      case AsmJSMathBuiltin_ceil:  arity = 1; callee = UnaryMathFunCast(ceil);       break;
      case AsmJSMathBuiltin_floor: arity = 1; callee = UnaryMathFunCast(floor);      break;
      case AsmJSMathBuiltin_exp:   arity = 1; callee = UnaryMathFunCast(exp);        break;
      case AsmJSMathBuiltin_log:   arity = 1; callee = UnaryMathFunCast(log);        break;
      case AsmJSMathBuiltin_sqrt:  return CheckMathSqrt(f, callNode, retType, def, type);
      case AsmJSMathBuiltin_pow:   arity = 2; callee = BinaryMathFunCast(ecmaPow);   break;
      case AsmJSMathBuiltin_atan2: arity = 2; callee = BinaryMathFunCast(ecmaAtan2); break;
    }

    FunctionCompiler::Call call(f, retType);
    if (!CheckCallArgs(f, callNode, CheckIsDoublish, &call))
        return false;

    if (call.sig().args().length() != arity)
        return f.failf(callNode, "call passed %u arguments, expected %u", call.sig().args().length(), arity);

    if (!f.builtinCall(callee, call, MIRType_Double, def))
        return false;

    if (retType != RetType::Double)
        return f.failf(callNode, "return type is double, used as %s", retType.toType().toChars());

    *type = Type::Double;
    return true;
}

static bool
CheckCall(FunctionCompiler &f, ParseNode *call, RetType retType, MDefinition **def, Type *type)
{
    JS_CHECK_RECURSION(f.cx(), return false);

    ParseNode *callee = CallCallee(call);

    if (callee->isKind(PNK_ELEM))
        return CheckFuncPtrCall(f, call, retType, def, type);

    if (!callee->isKind(PNK_NAME))
        return f.fail(callee, "unexpected callee expression type");

    PropertyName *calleeName = callee->name();

    if (const ModuleCompiler::Global *global = f.lookupGlobal(calleeName)) {
        switch (global->which()) {
          case ModuleCompiler::Global::FFI:
            return CheckFFICall(f, call, global->ffiIndex(), retType, def, type);
          case ModuleCompiler::Global::MathBuiltin:
            return CheckMathBuiltinCall(f, call, global->mathBuiltin(), retType, def, type);
          case ModuleCompiler::Global::Constant:
          case ModuleCompiler::Global::Variable:
          case ModuleCompiler::Global::FuncPtrTable:
          case ModuleCompiler::Global::ArrayView:
            return f.failName(callee, "'%s' is not callable function", callee->name());
          case ModuleCompiler::Global::Function:
            break;
        }
    }

    return CheckInternalCall(f, call, calleeName, retType, def, type);
}

static bool
CheckPos(FunctionCompiler &f, ParseNode *pos, MDefinition **def, Type *type)
{
    JS_ASSERT(pos->isKind(PNK_POS));
    ParseNode *operand = UnaryKid(pos);

    if (operand->isKind(PNK_CALL))
        return CheckCall(f, operand, RetType::Double, def, type);

    MDefinition *operandDef;
    Type operandType;
    if (!CheckExpr(f, operand, &operandDef, &operandType))
        return false;

    if (operandType.isSigned())
        *def = f.unary<MToDouble>(operandDef);
    else if (operandType.isUnsigned())
        *def = f.unary<MAsmJSUnsignedToDouble>(operandDef);
    else if (operandType.isDoublish())
        *def = operandDef;
    else
        return f.failf(operand, "%s is not a subtype of signed, unsigned or doublish", operandType.toChars());

    *type = Type::Double;
    return true;
}

static bool
CheckNot(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type)
{
    JS_ASSERT(expr->isKind(PNK_NOT));
    ParseNode *operand = UnaryKid(expr);

    MDefinition *operandDef;
    Type operandType;
    if (!CheckExpr(f, operand, &operandDef, &operandType))
        return false;

    if (!operandType.isInt())
        return f.failf(operand, "%s is not a subtype of int", operandType.toChars());

    *def = f.unary<MNot>(operandDef);
    *type = Type::Int;
    return true;
}

static bool
CheckNeg(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type)
{
    JS_ASSERT(expr->isKind(PNK_NEG));
    ParseNode *operand = UnaryKid(expr);

    MDefinition *operandDef;
    Type operandType;
    if (!CheckExpr(f, operand, &operandDef, &operandType))
        return false;

    if (operandType.isInt()) {
        *def = f.unary<MAsmJSNeg>(operandDef, MIRType_Int32);
        *type = Type::Intish;
        return true;
    }

    if (operandType.isDoublish()) {
        *def = f.unary<MAsmJSNeg>(operandDef, MIRType_Double);
        *type = Type::Double;
        return true;
    }

    return f.failf(operand, "%s is not a subtype of int or doublish", operandType.toChars());
}

static bool
CheckCoerceToInt(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type)
{
    JS_ASSERT(expr->isKind(PNK_BITNOT));
    ParseNode *operand = UnaryKid(expr);

    MDefinition *operandDef;
    Type operandType;
    if (!CheckExpr(f, operand, &operandDef, &operandType))
        return false;

    if (operandType.isDouble()) {
        *def = f.unary<MTruncateToInt32>(operandDef);
        *type = Type::Signed;
        return true;
    }

    if (!operandType.isIntish())
        return f.failf(operand, "%s is not a subtype of double or intish", operandType.toChars());

    *def = operandDef;
    *type = Type::Signed;
    return true;
}

static bool
CheckBitNot(FunctionCompiler &f, ParseNode *neg, MDefinition **def, Type *type)
{
    JS_ASSERT(neg->isKind(PNK_BITNOT));
    ParseNode *operand = UnaryKid(neg);

    if (operand->isKind(PNK_BITNOT))
        return CheckCoerceToInt(f, operand, def, type);

    MDefinition *operandDef;
    Type operandType;
    if (!CheckExpr(f, operand, &operandDef, &operandType))
        return false;

    if (!operandType.isIntish())
        return f.failf(operand, "%s is not a subtype of intish", operandType.toChars());

    *def = f.bitwise<MBitNot>(operandDef);
    *type = Type::Signed;
    return true;
}

static bool
CheckComma(FunctionCompiler &f, ParseNode *comma, MDefinition **def, Type *type)
{
    JS_ASSERT(comma->isKind(PNK_COMMA));
    ParseNode *operands = ListHead(comma);

    ParseNode *pn = operands;
    for (; NextNode(pn); pn = NextNode(pn)) {
        MDefinition *_1;
        Type _2;
        if (pn->isKind(PNK_CALL)) {
            if (!CheckCall(f, pn, RetType::Void, &_1, &_2))
                return false;
        } else {
            if (!CheckExpr(f, pn, &_1, &_2))
                return false;
        }
    }

    if (!CheckExpr(f, pn, def, type))
        return false;

    return true;
}

static bool
CheckConditional(FunctionCompiler &f, ParseNode *ternary, MDefinition **def, Type *type)
{
    JS_ASSERT(ternary->isKind(PNK_CONDITIONAL));
    ParseNode *cond = TernaryKid1(ternary);
    ParseNode *thenExpr = TernaryKid2(ternary);
    ParseNode *elseExpr = TernaryKid3(ternary);

    MDefinition *condDef;
    Type condType;
    if (!CheckExpr(f, cond, &condDef, &condType))
        return false;

    if (!condType.isInt())
        return f.failf(cond, "%s is not a subtype of int", condType.toChars());

    MBasicBlock *thenBlock, *elseBlock;
    if (!f.branchAndStartThen(condDef, &thenBlock, &elseBlock, thenExpr, elseExpr))
        return false;

    MDefinition *thenDef;
    Type thenType;
    if (!CheckExpr(f, thenExpr, &thenDef, &thenType))
        return false;

    BlockVector thenBlocks(f.cx());
    if (!f.appendThenBlock(&thenBlocks))
        return false;

    f.pushPhiInput(thenDef);
    f.switchToElse(elseBlock);

    MDefinition *elseDef;
    Type elseType;
    if (!CheckExpr(f, elseExpr, &elseDef, &elseType))
        return false;

    f.pushPhiInput(elseDef);

    
    
    if (!f.joinIfElse(thenBlocks, elseExpr))
        return false;
    *def = f.popPhiOutput();

    if (thenType.isInt() && elseType.isInt()) {
        *type = Type::Int;
    } else if (thenType.isDouble() && elseType.isDouble()) {
        *type = Type::Double;
    } else {
        return f.failf(ternary, "then/else branches of conditional must both produce int or double, "
                       "current types are %s and %s", thenType.toChars(), elseType.toChars());
    }

    return true;
}

static bool
IsValidIntMultiplyConstant(ParseNode *expr)
{
    if (!IsNumericLiteral(expr))
        return false;

    NumLit literal = ExtractNumericLiteral(expr);
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::NegativeInt:
        if (abs(literal.toInt32()) < (1<<20))
            return true;
        return false;
      case NumLit::BigUnsigned:
      case NumLit::Double:
      case NumLit::OutOfRangeInt:
        return false;
    }

    MOZ_ASSUME_UNREACHABLE("Bad literal");
}

static bool
CheckMultiply(FunctionCompiler &f, ParseNode *star, MDefinition **def, Type *type)
{
    JS_ASSERT(star->isKind(PNK_STAR));
    ParseNode *lhs = BinaryLeft(star);
    ParseNode *rhs = BinaryRight(star);

    MDefinition *lhsDef;
    Type lhsType;
    if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
        return false;

    MDefinition *rhsDef;
    Type rhsType;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if (lhsType.isInt() && rhsType.isInt()) {
        if (!IsValidIntMultiplyConstant(lhs) && !IsValidIntMultiplyConstant(rhs))
            return f.fail(star, "one arg to int multiply must be a small (-2^20, 2^20) int literal");
        *def = f.mul(lhsDef, rhsDef, MIRType_Int32, MMul::Integer);
        *type = Type::Intish;
        return true;
    }

    if (!lhsType.isDoublish())
        return f.failf(lhs, "%s is not a subtype of doublish", lhsType.toChars());
    if (!rhsType.isDoublish())
        return f.failf(rhs, "%s is not a subtype of doublish", rhsType.toChars());

    *def = f.mul(lhsDef, rhsDef, MIRType_Double, MMul::Normal);
    *type = Type::Double;
    return true;
}

static bool
CheckAddOrSub(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type,
              unsigned *numAddOrSubOut = NULL)
{
    JS_CHECK_RECURSION(f.cx(), return false);

    JS_ASSERT(expr->isKind(PNK_ADD) || expr->isKind(PNK_SUB));
    ParseNode *lhs = BinaryLeft(expr);
    ParseNode *rhs = BinaryRight(expr);

    MDefinition *lhsDef, *rhsDef;
    Type lhsType, rhsType;
    unsigned lhsNumAddOrSub, rhsNumAddOrSub;

    if (lhs->isKind(PNK_ADD) || lhs->isKind(PNK_SUB)) {
        if (!CheckAddOrSub(f, lhs, &lhsDef, &lhsType, &lhsNumAddOrSub))
            return false;
        if (lhsType == Type::Intish)
            lhsType = Type::Int;
    } else {
        if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
            return false;
        lhsNumAddOrSub = 0;
    }

    if (rhs->isKind(PNK_ADD) || rhs->isKind(PNK_SUB)) {
        if (!CheckAddOrSub(f, rhs, &rhsDef, &rhsType, &rhsNumAddOrSub))
            return false;
        if (rhsType == Type::Intish)
            rhsType = Type::Int;
    } else {
        if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
            return false;
        rhsNumAddOrSub = 0;
    }

    unsigned numAddOrSub = lhsNumAddOrSub + rhsNumAddOrSub + 1;
    if (numAddOrSub > (1<<20))
        return f.fail(expr, "too many + or - without intervening coercion");

    if (expr->isKind(PNK_ADD)) {
        if (lhsType.isInt() && rhsType.isInt()) {
            *def = f.binary<MAdd>(lhsDef, rhsDef, MIRType_Int32);
            *type = Type::Intish;
        } else if (lhsType.isDouble() && rhsType.isDouble()) {
            *def = f.binary<MAdd>(lhsDef, rhsDef, MIRType_Double);
            *type = Type::Double;
        } else {
            return f.failf(expr, "operands to + must both be int or double, got %s and %s",
                           lhsType.toChars(), rhsType.toChars());
        }
    } else {
        if (lhsType.isInt() && rhsType.isInt()) {
            *def = f.binary<MSub>(lhsDef, rhsDef, MIRType_Int32);
            *type = Type::Intish;
        } else if (lhsType.isDoublish() && rhsType.isDoublish()) {
            *def = f.binary<MSub>(lhsDef, rhsDef, MIRType_Double);
            *type = Type::Double;
        } else {
            return f.failf(expr, "operands to - must both be int or doublish, got %s and %s",
                           lhsType.toChars(), rhsType.toChars());
        }
    }

    if (numAddOrSubOut)
        *numAddOrSubOut = numAddOrSub;
    return true;
}

static bool
CheckDivOrMod(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type)
{
    JS_ASSERT(expr->isKind(PNK_DIV) || expr->isKind(PNK_MOD));
    ParseNode *lhs = BinaryLeft(expr);
    ParseNode *rhs = BinaryRight(expr);

    MDefinition *lhsDef, *rhsDef;
    Type lhsType, rhsType;
    if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
        return false;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if (lhsType.isDoublish() && rhsType.isDoublish()) {
        *def = expr->isKind(PNK_DIV)
               ? f.binary<MDiv>(lhsDef, rhsDef, MIRType_Double)
               : f.binary<MMod>(lhsDef, rhsDef, MIRType_Double);
        *type = Type::Double;
        return true;
    }

    if (lhsType.isSigned() && rhsType.isSigned()) {
        if (expr->isKind(PNK_DIV))
            *def = f.binary<MDiv>(lhsDef, rhsDef, MIRType_Int32);
        else
            *def = f.binary<MMod>(lhsDef, rhsDef, MIRType_Int32);
        *type = Type::Intish;
        return true;
    }

    if (lhsType.isUnsigned() && rhsType.isUnsigned()) {
        if (expr->isKind(PNK_DIV))
            *def = f.binary<MAsmJSUDiv>(lhsDef, rhsDef);
        else
            *def = f.binary<MAsmJSUMod>(lhsDef, rhsDef);
        *type = Type::Intish;
        return true;
    }

    return f.failf(expr, "arguments to / or %% must both be double, signed, or unsigned; "
                   "%s and %s are given", lhsType.toChars(), rhsType.toChars());
}

static bool
CheckComparison(FunctionCompiler &f, ParseNode *comp, MDefinition **def, Type *type)
{
    JS_ASSERT(comp->isKind(PNK_LT) || comp->isKind(PNK_LE) || comp->isKind(PNK_GT) ||
              comp->isKind(PNK_GE) || comp->isKind(PNK_EQ) || comp->isKind(PNK_NE));
    ParseNode *lhs = BinaryLeft(comp);
    ParseNode *rhs = BinaryRight(comp);

    MDefinition *lhsDef, *rhsDef;
    Type lhsType, rhsType;
    if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
        return false;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if ((lhsType.isSigned() && rhsType.isSigned()) || (lhsType.isUnsigned() && rhsType.isUnsigned())) {
        MCompare::CompareType compareType = (lhsType.isUnsigned() && rhsType.isUnsigned())
                                            ? MCompare::Compare_UInt32
                                            : MCompare::Compare_Int32;
        *def = f.compare(lhsDef, rhsDef, comp->getOp(), compareType);
        *type = Type::Int;
        return true;
    }

    if (lhsType.isDouble() && rhsType.isDouble()) {
        *def = f.compare(lhsDef, rhsDef, comp->getOp(), MCompare::Compare_Double);
        *type = Type::Int;
        return true;
    }

    return f.failf(comp, "arguments to a comparison must both be signed, unsigned or doubles; "
                   "%s and %s are given", lhsType.toChars(), rhsType.toChars());
}

static bool
CheckBitwise(FunctionCompiler &f, ParseNode *bitwise, MDefinition **def, Type *type)
{
    ParseNode *lhs = BinaryLeft(bitwise);
    ParseNode *rhs = BinaryRight(bitwise);

    int32_t identityElement;
    bool onlyOnRight;
    switch (bitwise->getKind()) {
      case PNK_BITOR:  identityElement = 0;  onlyOnRight = false; *type = Type::Signed;   break;
      case PNK_BITAND: identityElement = -1; onlyOnRight = false; *type = Type::Signed;   break;
      case PNK_BITXOR: identityElement = 0;  onlyOnRight = false; *type = Type::Signed;   break;
      case PNK_LSH:    identityElement = 0;  onlyOnRight = true;  *type = Type::Signed;   break;
      case PNK_RSH:    identityElement = 0;  onlyOnRight = true;  *type = Type::Signed;   break;
      case PNK_URSH:   identityElement = 0;  onlyOnRight = true;  *type = Type::Unsigned; break;
      default: MOZ_ASSUME_UNREACHABLE("not a bitwise op");
    }

    if (!onlyOnRight && IsBits32(lhs, identityElement)) {
        Type rhsType;
        if (!CheckExpr(f, rhs, def, &rhsType))
            return false;
        if (!rhsType.isIntish())
            return f.failf(bitwise, "%s is not a subtype of intish", rhsType.toChars());
        return true;
    }

    if (IsBits32(rhs, identityElement)) {
        if (bitwise->isKind(PNK_BITOR) && lhs->isKind(PNK_CALL))
            return CheckCall(f, lhs, RetType::Signed, def, type);

        Type lhsType;
        if (!CheckExpr(f, lhs, def, &lhsType))
            return false;
        if (!lhsType.isIntish())
            return f.failf(bitwise, "%s is not a subtype of intish", lhsType.toChars());
        return true;
    }

    MDefinition *lhsDef;
    Type lhsType;
    if (!CheckExpr(f, lhs, &lhsDef, &lhsType))
        return false;

    MDefinition *rhsDef;
    Type rhsType;
    if (!CheckExpr(f, rhs, &rhsDef, &rhsType))
        return false;

    if (!lhsType.isIntish())
        return f.failf(lhs, "%s is not a subtype of intish", lhsType.toChars());
    if (!rhsType.isIntish())
        return f.failf(rhs, "%s is not a subtype of intish", rhsType.toChars());

    switch (bitwise->getKind()) {
      case PNK_BITOR:  *def = f.bitwise<MBitOr>(lhsDef, rhsDef); break;
      case PNK_BITAND: *def = f.bitwise<MBitAnd>(lhsDef, rhsDef); break;
      case PNK_BITXOR: *def = f.bitwise<MBitXor>(lhsDef, rhsDef); break;
      case PNK_LSH:    *def = f.bitwise<MLsh>(lhsDef, rhsDef); break;
      case PNK_RSH:    *def = f.bitwise<MRsh>(lhsDef, rhsDef); break;
      case PNK_URSH:   *def = f.bitwise<MUrsh>(lhsDef, rhsDef); break;
      default: MOZ_ASSUME_UNREACHABLE("not a bitwise op");
    }

    return true;
}

static bool
CheckExpr(FunctionCompiler &f, ParseNode *expr, MDefinition **def, Type *type)
{
    JS_CHECK_RECURSION(f.cx(), return false);

    if (!f.mirGen().ensureBallast())
        return false;

    if (IsNumericLiteral(expr))
        return CheckNumericLiteral(f, expr, def, type);

    switch (expr->getKind()) {
      case PNK_NAME:        return CheckVarRef(f, expr, def, type);
      case PNK_ELEM:        return CheckArrayLoad(f, expr, def, type);
      case PNK_ASSIGN:      return CheckAssign(f, expr, def, type);
      case PNK_CALL:        return f.fail(expr, "non-expression-statement call must be coerced");
      case PNK_POS:         return CheckPos(f, expr, def, type);
      case PNK_NOT:         return CheckNot(f, expr, def, type);
      case PNK_NEG:         return CheckNeg(f, expr, def, type);
      case PNK_BITNOT:      return CheckBitNot(f, expr, def, type);
      case PNK_COMMA:       return CheckComma(f, expr, def, type);
      case PNK_CONDITIONAL: return CheckConditional(f, expr, def, type);

      case PNK_STAR:        return CheckMultiply(f, expr, def, type);

      case PNK_ADD:
      case PNK_SUB:         return CheckAddOrSub(f, expr, def, type);

      case PNK_DIV:
      case PNK_MOD:         return CheckDivOrMod(f, expr, def, type);

      case PNK_LT:
      case PNK_LE:
      case PNK_GT:
      case PNK_GE:
      case PNK_EQ:
      case PNK_NE:          return CheckComparison(f, expr, def, type);

      case PNK_BITOR:
      case PNK_BITAND:
      case PNK_BITXOR:
      case PNK_LSH:
      case PNK_RSH:
      case PNK_URSH:        return CheckBitwise(f, expr, def, type);

      default:;
    }

    return f.fail(expr, "unsupported expression");
}

static bool
CheckStatement(FunctionCompiler &f, ParseNode *stmt, LabelVector *maybeLabels = NULL);

static bool
CheckExprStatement(FunctionCompiler &f, ParseNode *exprStmt)
{
    JS_ASSERT(exprStmt->isKind(PNK_SEMI));
    ParseNode *expr = UnaryKid(exprStmt);

    if (!expr)
        return true;

    MDefinition *_1;
    Type _2;

    if (expr->isKind(PNK_CALL))
        return CheckCall(f, expr, RetType::Void, &_1, &_2);

    return CheckExpr(f, UnaryKid(exprStmt), &_1, &_2);
}

static bool
CheckWhile(FunctionCompiler &f, ParseNode *whileStmt, const LabelVector *maybeLabels)
{
    JS_ASSERT(whileStmt->isKind(PNK_WHILE));
    ParseNode *cond = BinaryLeft(whileStmt);
    ParseNode *body = BinaryRight(whileStmt);

    MBasicBlock *loopEntry;
    if (!f.startPendingLoop(whileStmt, &loopEntry, body))
        return false;

    MDefinition *condDef;
    Type condType;
    if (!CheckExpr(f, cond, &condDef, &condType))
        return false;

    if (!condType.isInt())
        return f.failf(cond, "%s is not a subtype of int", condType.toChars());

    MBasicBlock *afterLoop;
    if (!f.branchAndStartLoopBody(condDef, &afterLoop, body, NextNode(whileStmt)))
        return false;

    if (!CheckStatement(f, body))
        return false;

    if (!f.bindContinues(whileStmt, maybeLabels))
        return false;

    return f.closeLoop(loopEntry, afterLoop);
}

static bool
CheckFor(FunctionCompiler &f, ParseNode *forStmt, const LabelVector *maybeLabels)
{
    JS_ASSERT(forStmt->isKind(PNK_FOR));
    ParseNode *forHead = BinaryLeft(forStmt);
    ParseNode *body = BinaryRight(forStmt);

    if (!forHead->isKind(PNK_FORHEAD))
        return f.fail(forHead, "unsupported for-loop statement");

    ParseNode *maybeInit = TernaryKid1(forHead);
    ParseNode *maybeCond = TernaryKid2(forHead);
    ParseNode *maybeInc = TernaryKid3(forHead);

    if (maybeInit) {
        MDefinition *_1;
        Type _2;
        if (!CheckExpr(f, maybeInit, &_1, &_2))
            return false;
    }

    MBasicBlock *loopEntry;
    if (!f.startPendingLoop(forStmt, &loopEntry, body))
        return false;

    MDefinition *condDef;
    if (maybeCond) {
        Type condType;
        if (!CheckExpr(f, maybeCond, &condDef, &condType))
            return false;

        if (!condType.isInt())
            return f.failf(maybeCond, "%s is not a subtype of int", condType.toChars());
    } else {
        condDef = f.constant(Int32Value(1));
    }

    MBasicBlock *afterLoop;
    if (!f.branchAndStartLoopBody(condDef, &afterLoop, body, NextNode(forStmt)))
        return false;

    if (!CheckStatement(f, body))
        return false;

    if (!f.bindContinues(forStmt, maybeLabels))
        return false;

    if (maybeInc) {
        MDefinition *_1;
        Type _2;
        if (!CheckExpr(f, maybeInc, &_1, &_2))
            return false;
    }

    return f.closeLoop(loopEntry, afterLoop);
}

static bool
CheckDoWhile(FunctionCompiler &f, ParseNode *whileStmt, const LabelVector *maybeLabels)
{
    JS_ASSERT(whileStmt->isKind(PNK_DOWHILE));
    ParseNode *body = BinaryLeft(whileStmt);
    ParseNode *cond = BinaryRight(whileStmt);

    MBasicBlock *loopEntry;
    if (!f.startPendingLoop(whileStmt, &loopEntry, body))
        return false;

    if (!CheckStatement(f, body))
        return false;

    if (!f.bindContinues(whileStmt, maybeLabels))
        return false;

    MDefinition *condDef;
    Type condType;
    if (!CheckExpr(f, cond, &condDef, &condType))
        return false;

    if (!condType.isInt())
        return f.failf(cond, "%s is not a subtype of int", condType.toChars());

    return f.branchAndCloseDoWhileLoop(condDef, loopEntry, NextNode(whileStmt));
}

static bool
CheckLabel(FunctionCompiler &f, ParseNode *labeledStmt, LabelVector *maybeLabels)
{
    JS_ASSERT(labeledStmt->isKind(PNK_LABEL));
    PropertyName *label = LabeledStatementLabel(labeledStmt);
    ParseNode *stmt = LabeledStatementStatement(labeledStmt);

    if (maybeLabels) {
        if (!maybeLabels->append(label))
            return false;
        if (!CheckStatement(f, stmt, maybeLabels))
            return false;
        return true;
    }

    LabelVector labels(f.cx());
    if (!labels.append(label))
        return false;

    if (!CheckStatement(f, stmt, &labels))
        return false;

    return f.bindLabeledBreaks(&labels, labeledStmt);
}

static bool
CheckIf(FunctionCompiler &f, ParseNode *ifStmt)
{
    
    
    
    
    BlockVector thenBlocks(f.cx());

    ParseNode *nextStmt = NextNode(ifStmt);
  recurse:
    JS_ASSERT(ifStmt->isKind(PNK_IF));
    ParseNode *cond = TernaryKid1(ifStmt);
    ParseNode *thenStmt = TernaryKid2(ifStmt);
    ParseNode *elseStmt = TernaryKid3(ifStmt);

    MDefinition *condDef;
    Type condType;
    if (!CheckExpr(f, cond, &condDef, &condType))
        return false;

    if (!condType.isInt())
        return f.failf(cond, "%s is not a subtype of int", condType.toChars());

    MBasicBlock *thenBlock, *elseBlock;

    ParseNode *elseBlockStmt = NULL;
    
    
    elseBlockStmt = elseStmt;
    if (elseBlockStmt == NULL)
        elseBlockStmt = nextStmt;

    if (!f.branchAndStartThen(condDef, &thenBlock, &elseBlock, thenStmt, elseBlockStmt))
        return false;

    if (!CheckStatement(f, thenStmt))
        return false;

    if (!f.appendThenBlock(&thenBlocks))
        return false;

    if (!elseStmt) {
        f.joinIf(thenBlocks, elseBlock);
    } else {
        f.switchToElse(elseBlock);

        if (elseStmt->isKind(PNK_IF)) {
            ifStmt = elseStmt;
            goto recurse;
        }

        if (!CheckStatement(f, elseStmt))
            return false;

        if (!f.joinIfElse(thenBlocks, nextStmt))
            return false;
    }

    return true;
}

static bool
CheckCaseExpr(FunctionCompiler &f, ParseNode *caseExpr, int32_t *value)
{
    if (!IsNumericLiteral(caseExpr))
        return f.fail(caseExpr, "switch case expression must be an integer literal");

    NumLit literal = ExtractNumericLiteral(caseExpr);
    switch (literal.which()) {
      case NumLit::Fixnum:
      case NumLit::NegativeInt:
        *value = literal.toInt32();
        break;
      case NumLit::OutOfRangeInt:
      case NumLit::BigUnsigned:
        return f.fail(caseExpr, "switch case expression out of integer range");
      case NumLit::Double:
        return f.fail(caseExpr, "switch case expression must be an integer literal");
    }

    return true;
}

static bool
CheckDefaultAtEnd(FunctionCompiler &f, ParseNode *stmt)
{
    for (; stmt; stmt = NextNode(stmt)) {
        JS_ASSERT(stmt->isKind(PNK_CASE) || stmt->isKind(PNK_DEFAULT));
        if (stmt->isKind(PNK_DEFAULT) && NextNode(stmt) != NULL)
            return f.fail(stmt, "default label must be at the end");
    }

    return true;
}

static bool
CheckSwitchRange(FunctionCompiler &f, ParseNode *stmt, int32_t *low, int32_t *high,
                 int32_t *tableLength)
{
    if (stmt->isKind(PNK_DEFAULT)) {
        *low = 0;
        *high = -1;
        *tableLength = 0;
        return true;
    }

    int32_t i = 0;
    if (!CheckCaseExpr(f, CaseExpr(stmt), &i))
        return false;

    *low = *high = i;

    ParseNode *initialStmt = stmt;
    for (stmt = NextNode(stmt); stmt && stmt->isKind(PNK_CASE); stmt = NextNode(stmt)) {
        int32_t i = 0;
        if (!CheckCaseExpr(f, CaseExpr(stmt), &i))
            return false;

        *low = Min(*low, i);
        *high = Max(*high, i);
    }

    int64_t i64 = (int64_t(*high) - int64_t(*low)) + 1;
    if (i64 > 4*1024*1024)
        return f.fail(initialStmt, "all switch statements generate tables; this table would be too big");

    *tableLength = int32_t(i64);
    return true;
}

static bool
CheckSwitch(FunctionCompiler &f, ParseNode *switchStmt)
{
    JS_ASSERT(switchStmt->isKind(PNK_SWITCH));
    ParseNode *switchExpr = BinaryLeft(switchStmt);
    ParseNode *switchBody = BinaryRight(switchStmt);

    if (!switchBody->isKind(PNK_STATEMENTLIST))
        return f.fail(switchBody, "switch body may not contain 'let' declarations");

    MDefinition *exprDef;
    Type exprType;
    if (!CheckExpr(f, switchExpr, &exprDef, &exprType))
        return false;

    if (!exprType.isSigned())
        return f.failf(switchExpr, "%s is not a subtype of signed", exprType.toChars());

    ParseNode *stmt = ListHead(switchBody);

    if (!CheckDefaultAtEnd(f, stmt))
        return false;

    if (!stmt)
        return true;

    int32_t low = 0, high = 0, tableLength = 0;
    if (!CheckSwitchRange(f, stmt, &low, &high, &tableLength))
        return false;

    BlockVector cases(f.cx());
    if (!cases.resize(tableLength))
        return false;

    MBasicBlock *switchBlock;
    if (!f.startSwitch(switchStmt, exprDef, low, high, &switchBlock))
        return false;

    for (; stmt && stmt->isKind(PNK_CASE); stmt = NextNode(stmt)) {
        int32_t caseValue = ExtractNumericLiteral(CaseExpr(stmt)).toInt32();
        unsigned caseIndex = caseValue - low;

        if (cases[caseIndex])
            return f.fail(stmt, "no duplicate case labels");

        if (!f.startSwitchCase(switchBlock, &cases[caseIndex], stmt))
            return false;

        if (!CheckStatement(f, CaseBody(stmt)))
            return false;
    }

    MBasicBlock *defaultBlock;
    if (!f.startSwitchDefault(switchBlock, &cases, &defaultBlock, stmt))
        return false;

    if (stmt && stmt->isKind(PNK_DEFAULT)) {
        if (!CheckStatement(f, CaseBody(stmt)))
            return false;
    }

    return f.joinSwitch(switchBlock, cases, defaultBlock);
}

static bool
CheckReturnType(FunctionCompiler &f, ParseNode *usepn, RetType retType)
{
    if (!f.hasAlreadyReturned()) {
        f.setReturnedType(retType);
        return true;
    }

    if (f.returnedType() != retType) {
        return f.failf(usepn, "%s incompatible with previous return of type %s",
                       f.returnedType().toType().toChars(), retType.toType().toChars());
    }

    return true;
}

static bool
CheckReturn(FunctionCompiler &f, ParseNode *returnStmt)
{
    ParseNode *expr = ReturnExpr(returnStmt);

    if (!expr) {
        if (!CheckReturnType(f, returnStmt, RetType::Void))
            return false;

        f.returnVoid();
        return true;
    }

    MDefinition *def;
    Type type;
    if (!CheckExpr(f, expr, &def, &type))
        return false;

    RetType retType;
    if (type.isSigned())
        retType = RetType::Signed;
    else if (type.isDouble())
        retType = RetType::Double;
    else if (type.isVoid())
        retType = RetType::Void;
    else
        return f.failf(expr, "%s is not a valid return type", type.toChars());

    if (!CheckReturnType(f, expr, retType))
        return false;

    if (retType == RetType::Void)
        f.returnVoid();
    else
        f.returnExpr(def);
    return true;
}

static bool
CheckStatementList(FunctionCompiler &f, ParseNode *stmtList)
{
    JS_ASSERT(stmtList->isKind(PNK_STATEMENTLIST));

    for (ParseNode *stmt = ListHead(stmtList); stmt; stmt = NextNode(stmt)) {
        if (!CheckStatement(f, stmt))
            return false;
    }

    return true;
}

static bool
CheckStatement(FunctionCompiler &f, ParseNode *stmt, LabelVector *maybeLabels)
{
    JS_CHECK_RECURSION(f.cx(), return false);

    if (!f.mirGen().ensureBallast())
        return false;

    switch (stmt->getKind()) {
      case PNK_SEMI:          return CheckExprStatement(f, stmt);
      case PNK_WHILE:         return CheckWhile(f, stmt, maybeLabels);
      case PNK_FOR:           return CheckFor(f, stmt, maybeLabels);
      case PNK_DOWHILE:       return CheckDoWhile(f, stmt, maybeLabels);
      case PNK_LABEL:         return CheckLabel(f, stmt, maybeLabels);
      case PNK_IF:            return CheckIf(f, stmt);
      case PNK_SWITCH:        return CheckSwitch(f, stmt);
      case PNK_RETURN:        return CheckReturn(f, stmt);
      case PNK_STATEMENTLIST: return CheckStatementList(f, stmt);
      case PNK_BREAK:         return f.addBreak(LoopControlMaybeLabel(stmt));
      case PNK_CONTINUE:      return f.addContinue(LoopControlMaybeLabel(stmt));
      default:;
    }

    return f.fail(stmt, "unexpected statement kind");
}

static bool
ParseFunction(ModuleCompiler &m, ParseNode **fnOut)
{
    TokenStream &tokenStream = m.parser().tokenStream;

    DebugOnly<TokenKind> tk = tokenStream.getToken();
    JS_ASSERT(tk == TOK_FUNCTION);

    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) != TOK_NAME)
        return false;  

    RootedPropertyName name(m.cx(), tokenStream.currentToken().name());

    ParseNode *fn = m.parser().handler.newFunctionDefinition();
    if (!fn)
        return false;

    
    RootedFunction fun(m.cx(), NewFunction(m.cx(), NullPtr(), NULL, 0, JSFunction::INTERPRETED,
                                           m.cx()->global(), name, JSFunction::FinalizeKind,
                                           TenuredObject));
    if (!fun)
        return false;

    AsmJSParseContext *outerpc = m.parser().pc;

    Directives directives(outerpc);
    FunctionBox *funbox = m.parser().newFunctionBox(fn, fun, outerpc, directives);
    if (!funbox)
        return false;

    Directives newDirectives = directives;
    AsmJSParseContext funpc(&m.parser(), outerpc, fn, funbox, &newDirectives,
                            outerpc->staticLevel + 1, outerpc->blockidGen);
    if (!funpc.init())
        return false;

    if (!m.parser().functionArgsAndBodyGeneric(fn, fun, Normal, Statement, &newDirectives))
        return false;

    if (tokenStream.hadError() || directives != newDirectives)
        return false;

    outerpc->blockidGen = funpc.blockidGen;
    fn->pn_blockid = outerpc->blockid();

    *fnOut = fn;
    return true;
}

static bool
CheckFunction(ModuleCompiler &m, LifoAlloc &lifo, MIRGenerator **mir, ModuleCompiler::Func **funcOut)
{
    int64_t before = PRMJ_Now();

    
    
    AsmJSParser::Mark mark = m.parser().mark();

    ParseNode *fn;
    if (!ParseFunction(m, &fn))
        return false;

    if (!CheckFunctionHead(m, fn))
        return false;

    FunctionCompiler f(m, fn, lifo);
    if (!f.init())
        return false;

    ParseNode *stmtIter = ListHead(FunctionStatementList(fn));

    VarTypeVector argTypes(m.cx());
    if (!CheckArguments(f, &stmtIter, &argTypes))
        return false;

    if (!CheckVariables(f, &stmtIter))
        return false;

    if (!f.prepareToEmitMIR(argTypes))
        return false;

    ParseNode *lastNonEmptyStmt = NULL;
    for (; stmtIter; stmtIter = NextNode(stmtIter)) {
        if (!CheckStatement(f, stmtIter))
            return false;
        if (!IsExpressionStatement(stmtIter))
            lastNonEmptyStmt = stmtIter;
    }

    RetType retType;
    if (!CheckFinalReturn(f, lastNonEmptyStmt, &retType))
        return false;

    Signature sig(Move(argTypes), retType);
    ModuleCompiler::Func *func;
    if (!CheckFunctionSignature(m, fn, Move(sig), FunctionName(fn), &func))
        return false;

    if (func->code()->bound())
        return m.failName(fn, "function '%s' already defined", FunctionName(fn));

    func->initSrcOffset(fn->pn_pos.begin);
    func->accumulateCompileTime((PRMJ_Now() - before) / PRMJ_USEC_PER_MSEC);

    m.parser().release(mark);

    *mir = f.extractMIR();
    *funcOut = func;
    return true;
}

static bool
GenerateCode(ModuleCompiler &m, ModuleCompiler::Func &func, MIRGenerator &mir, LIRGraph &lir)
{
    int64_t before = PRMJ_Now();

    m.masm().bind(func.code());

    ScopedJSDeletePtr<CodeGenerator> codegen(ion::GenerateCode(&mir, &lir, &m.masm()));
    if (!codegen)
        return m.fail(NULL, "internal codegen failure (probably out of memory)");

    if (!m.collectAccesses(mir))
        return false;

    ion::IonScriptCounts *counts = codegen->extractUnassociatedScriptCounts();
    if (counts && !m.addFunctionCounts(counts)) {
        js_delete(counts);
        return false;
    }

#ifdef MOZ_VTUNE
    if (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON) {
        if (!m.trackProfiledFunction(func, m.masm().size()))
            return false;
    }
#endif

#ifdef JS_ION_PERF
    if (PerfBlockEnabled()) {
        if (!m.trackPerfProfiledBlocks(mir.perfSpewer(), func, m.masm().size()))
            return false;
    } else if (PerfFuncEnabled()) {
        if (!m.trackPerfProfiledFunction(func, m.masm().size()))
            return false;
    }
#endif

    
    
    
    
    
    m.masm().resetForNewCodeGenerator();

    
    m.masm().align(CodeAlignment);

    func.accumulateCompileTime((PRMJ_Now() - before) / PRMJ_USEC_PER_MSEC);
    if (!m.maybeReportCompileTime(func))
        return false;

    
    
    
    
    return true;
}

static bool
CheckAllFunctionsDefined(ModuleCompiler &m)
{
    for (unsigned i = 0; i < m.numFunctions(); i++) {
        if (!m.function(i).code()->bound())
            return m.failName(NULL, "missing definition of function %s", m.function(i).name());
    }

    return true;
}

static bool
CheckFunctionsSequential(ModuleCompiler &m)
{
    
    
    
    LifoAlloc lifo(LIFO_ALLOC_PRIMARY_CHUNK_SIZE);

    while (PeekToken(m.parser()) == TOK_FUNCTION) {
        LifoAllocScope scope(&lifo);

        MIRGenerator *mir;
        ModuleCompiler::Func *func;
        if (!CheckFunction(m, lifo, &mir, &func))
            return false;

        int64_t before = PRMJ_Now();

        IonContext icx(m.cx(), &mir->temp());

        IonSpewNewFunction(&mir->graph(), NullPtr());

        if (!OptimizeMIR(mir))
            return m.failOffset(func->srcOffset(), "internal compiler failure (probably out of memory)");

        LIRGraph *lir = GenerateLIR(mir);
        if (!lir)
            return m.failOffset(func->srcOffset(), "internal compiler failure (probably out of memory)");

        func->accumulateCompileTime((PRMJ_Now() - before) / PRMJ_USEC_PER_MSEC);

        if (!GenerateCode(m, *func, *mir, *lir))
            return false;

        IonSpewEndFunction();
    }

    if (!CheckAllFunctionsDefined(m))
        return false;

    return true;
}

#ifdef JS_WORKER_THREADS

struct ParallelGroupState
{
    WorkerThreadState &state;
    Vector<AsmJSParallelTask> &tasks;
    int32_t outstandingJobs; 
    uint32_t compiledJobs;

    ParallelGroupState(WorkerThreadState &state, Vector<AsmJSParallelTask> &tasks)
      : state(state), tasks(tasks), outstandingJobs(0), compiledJobs(0)
    { }
};


static AsmJSParallelTask *
GetFinishedCompilation(ModuleCompiler &m, ParallelGroupState &group)
{
    AutoLockWorkerThreadState lock(m.cx()->runtime());

    while (!group.state.asmJSWorkerFailed()) {
        if (!group.state.asmJSFinishedList.empty()) {
            group.outstandingJobs--;
            return group.state.asmJSFinishedList.popCopy();
        }
        group.state.wait(WorkerThreadState::MAIN);
    }

    return NULL;
}

static bool
GenerateCodeForFinishedJob(ModuleCompiler &m, ParallelGroupState &group, AsmJSParallelTask **outTask)
{
    
    AsmJSParallelTask *task = GetFinishedCompilation(m, group);
    if (!task)
        return false;

    ModuleCompiler::Func &func = *reinterpret_cast<ModuleCompiler::Func *>(task->func);
    func.accumulateCompileTime(task->compileTime);

    {
        
        IonContext ionContext(m.cx()->compartment(), &task->mir->temp());
        if (!GenerateCode(m, func, *task->mir, *task->lir))
            return false;
    }

    group.compiledJobs++;

    
    TempAllocator &tempAlloc = task->mir->temp();
    tempAlloc.TempAllocator::~TempAllocator();
    task->lifo.releaseAll();

    *outTask = task;
    return true;
}

static inline bool
GetUnusedTask(ParallelGroupState &group, uint32_t i, AsmJSParallelTask **outTask)
{
    
    
    
    if (i >= group.tasks.length())
        return false;
    *outTask = &group.tasks[i];
    return true;
}

static bool
CheckFunctionsParallelImpl(ModuleCompiler &m, ParallelGroupState &group)
{
    JS_ASSERT(group.state.asmJSWorklist.empty());
    JS_ASSERT(group.state.asmJSFinishedList.empty());
    group.state.resetAsmJSFailureState();

    for (unsigned i = 0; PeekToken(m.parser()) == TOK_FUNCTION; i++) {
        
        AsmJSParallelTask *task = NULL;
        if (!GetUnusedTask(group, i, &task) && !GenerateCodeForFinishedJob(m, group, &task))
            return false;

        
        MIRGenerator *mir;
        ModuleCompiler::Func *func;
        if (!CheckFunction(m, task->lifo, &mir, &func))
            return false;

        
        task->init(func, mir);
        if (!StartOffThreadAsmJSCompile(m.cx(), task))
            return false;

        group.outstandingJobs++;
    }

    
    while (group.outstandingJobs > 0) {
        AsmJSParallelTask *ignored = NULL;
        if (!GenerateCodeForFinishedJob(m, group, &ignored))
            return false;
    }

    if (!CheckAllFunctionsDefined(m))
        return false;

    JS_ASSERT(group.outstandingJobs == 0);
    JS_ASSERT(group.compiledJobs == m.numFunctions());
    JS_ASSERT(group.state.asmJSWorklist.empty());
    JS_ASSERT(group.state.asmJSFinishedList.empty());
    JS_ASSERT(!group.state.asmJSWorkerFailed());
    return true;
}

static void
CancelOutstandingJobs(ModuleCompiler &m, ParallelGroupState &group)
{
    
    
    
    
    

    JS_ASSERT(group.outstandingJobs >= 0);
    if (!group.outstandingJobs)
        return;

    AutoLockWorkerThreadState lock(m.cx()->runtime());

    
    group.outstandingJobs -= group.state.asmJSWorklist.length();
    group.state.asmJSWorklist.clear();

    
    group.outstandingJobs -= group.state.asmJSFinishedList.length();
    group.state.asmJSFinishedList.clear();

    
    group.outstandingJobs -= group.state.harvestFailedAsmJSJobs();

    
    JS_ASSERT(group.outstandingJobs >= 0);
    while (group.outstandingJobs > 0) {
        group.state.wait(WorkerThreadState::MAIN);

        group.outstandingJobs -= group.state.harvestFailedAsmJSJobs();
        group.outstandingJobs -= group.state.asmJSFinishedList.length();
        group.state.asmJSFinishedList.clear();
    }

    JS_ASSERT(group.outstandingJobs == 0);
    JS_ASSERT(group.state.asmJSWorklist.empty());
    JS_ASSERT(group.state.asmJSFinishedList.empty());
}

static const size_t LIFO_ALLOC_PARALLEL_CHUNK_SIZE = 1 << 12;

static bool
CheckFunctionsParallel(ModuleCompiler &m)
{
    
    WorkerThreadState &state = *m.cx()->runtime()->workerThreadState;
    size_t numParallelJobs = state.numThreads + 1;

    
    
    Vector<AsmJSParallelTask, 0> tasks(m.cx());
    if (!tasks.initCapacity(numParallelJobs))
        return false;

    for (size_t i = 0; i < numParallelJobs; i++)
        tasks.infallibleAppend(LIFO_ALLOC_PARALLEL_CHUNK_SIZE);

    
    ParallelGroupState group(state, tasks);
    if (!CheckFunctionsParallelImpl(m, group)) {
        CancelOutstandingJobs(m, group);

        
        if (void *maybeFunc = state.maybeAsmJSFailedFunction()) {
            ModuleCompiler::Func *func = reinterpret_cast<ModuleCompiler::Func *>(maybeFunc);
            return m.failOffset(func->srcOffset(), "allocation failure during compilation");
        }

        
        return false;
    }
    return true;
}
#endif 

static bool
CheckFuncPtrTable(ModuleCompiler &m, ParseNode *var)
{
    if (!IsDefinition(var))
        return m.fail(var, "function-pointer table name must be unique");

    ParseNode *arrayLiteral = MaybeDefinitionInitializer(var);
    if (!arrayLiteral || !arrayLiteral->isKind(PNK_ARRAY))
        return m.fail(var, "function-pointer table's initializer must be an array literal");

    unsigned length = ListLength(arrayLiteral);

    if (!IsPowerOfTwo(length))
        return m.failf(arrayLiteral, "function-pointer table length must be a power of 2 (is %u)", length);

    unsigned mask = length - 1;

    ModuleCompiler::FuncPtrVector elems(m.cx());
    const Signature *firstSig = NULL;

    for (ParseNode *elem = ListHead(arrayLiteral); elem; elem = NextNode(elem)) {
        if (!elem->isKind(PNK_NAME))
            return m.fail(elem, "function-pointer table's elements must be names of functions");

        PropertyName *funcName = elem->name();
        const ModuleCompiler::Func *func = m.lookupFunction(funcName);
        if (!func)
            return m.fail(elem, "function-pointer table's elements must be names of functions");

        if (firstSig) {
            if (*firstSig != func->sig())
                return m.fail(elem, "all functions in table must have same signature");
        } else {
            firstSig = &func->sig();
        }

        if (!elems.append(func))
            return false;
    }

    Signature sig(m.cx());
    if (!sig.copy(*firstSig))
        return false;

    ModuleCompiler::FuncPtrTable *table;
    if (!CheckFuncPtrTableAgainstExisting(m, var, var->name(), Move(sig), mask, &table))
        return false;

    table->initElems(Move(elems));
    return true;
}

static bool
CheckFuncPtrTables(ModuleCompiler &m)
{
    while (true) {
        ParseNode *varStmt;
        if (!ParseVarStatement(m.parser(), &varStmt))
            return false;
        if (!varStmt)
            break;
        for (ParseNode *var = VarListHead(varStmt); var; var = NextNode(var)) {
            if (!CheckFuncPtrTable(m, var))
                return false;
        }
    }

    return true;
}

static bool
CheckModuleExportFunction(ModuleCompiler &m, ParseNode *returnExpr)
{
    if (!returnExpr->isKind(PNK_NAME))
        return m.fail(returnExpr, "export statement must be of the form 'return name'");

    PropertyName *funcName = returnExpr->name();

    const ModuleCompiler::Func *func = m.lookupFunction(funcName);
    if (!func)
        return m.failName(returnExpr, "exported function name '%s' not found", funcName);

    return m.addExportedFunction(func,  NULL);
}

static bool
CheckModuleExportObject(ModuleCompiler &m, ParseNode *object)
{
    JS_ASSERT(object->isKind(PNK_OBJECT));

    for (ParseNode *pn = ListHead(object); pn; pn = NextNode(pn)) {
        if (!IsNormalObjectField(m.cx(), pn))
            return m.fail(pn, "only normal object properties may be used in the export object literal");

        PropertyName *fieldName = ObjectNormalFieldName(m.cx(), pn);

        ParseNode *initNode = ObjectFieldInitializer(pn);
        if (!initNode->isKind(PNK_NAME))
            return m.fail(initNode, "initializer of exported object literal must be name of function");

        PropertyName *funcName = initNode->name();

        const ModuleCompiler::Func *func = m.lookupFunction(funcName);
        if (!func)
            return m.failName(initNode, "exported function name '%s' not found", funcName);

        if (!m.addExportedFunction(func, fieldName))
            return false;
    }

    return true;
}

static bool
CheckModuleReturn(ModuleCompiler &m)
{
    if (PeekToken(m.parser()) != TOK_RETURN) {
        TokenKind tk = PeekToken(m.parser());
        if (tk == TOK_RC || tk == TOK_EOF)
            return m.fail(NULL, "expecting return statement");
        return m.fail(NULL, "invalid asm.js statement");
    }

    ParseNode *returnStmt = m.parser().statement(TSF_OPERAND);
    if (!returnStmt)
        return false;

    ParseNode *returnExpr = ReturnExpr(returnStmt);
    if (!returnExpr)
        return m.fail(returnStmt, "export statement must return something");

    if (returnExpr->isKind(PNK_OBJECT)) {
        if (!CheckModuleExportObject(m, returnExpr))
            return false;
    } else {
        if (!CheckModuleExportFunction(m, returnExpr))
            return false;
    }

    
    
    
    
    m.parser().pc->lexdeps->clear();
    return true;
}


static const RegisterSet AllRegsExceptSP =
    RegisterSet(GeneralRegisterSet(Registers::AllMask &
                                   ~(uint32_t(1) << Registers::StackPointer)),
                FloatRegisterSet(FloatRegisters::AllMask));
static const RegisterSet NonVolatileRegs =
    RegisterSet(GeneralRegisterSet(Registers::NonVolatileMask),
                FloatRegisterSet(FloatRegisters::NonVolatileMask));

static void
LoadAsmJSActivationIntoRegister(MacroAssembler &masm, Register reg)
{
    masm.movePtr(ImmWord(GetIonContext()->compartment->rt), reg);
    size_t offset = offsetof(JSRuntime, mainThread) +
                    PerThreadData::offsetOfAsmJSActivationStackReadOnly();
    masm.loadPtr(Address(reg, offset), reg);
}

static void
LoadJSContextFromActivation(MacroAssembler &masm, Register activation, Register dest)
{
    masm.loadPtr(Address(activation, AsmJSActivation::offsetOfContext()), dest);
}

static void
AssertStackAlignment(MacroAssembler &masm)
{
    JS_ASSERT((AlignmentAtPrologue + masm.framePushed()) % StackAlignment == 0);
#ifdef DEBUG
    Label ok;
    JS_ASSERT(IsPowerOfTwo(StackAlignment));
    masm.branchTestPtr(Assembler::Zero, StackPointer, Imm32(StackAlignment - 1), &ok);
    masm.breakpoint();
    masm.bind(&ok);
#endif
}

template <class VectorT>
static unsigned
StackArgBytes(const VectorT &argTypes)
{
    ABIArgIter<VectorT> iter(argTypes);
    while (!iter.done())
        iter++;
    return iter.stackBytesConsumedSoFar();
}

template <class VectorT>
static unsigned
StackDecrementForCall(MacroAssembler &masm, const VectorT &argTypes, unsigned extraBytes = 0)
{
    
    
    unsigned argBytes = StackArgBytes(argTypes);
    unsigned alreadyPushed = AlignmentAtPrologue + masm.framePushed();
    return AlignBytes(alreadyPushed + extraBytes + argBytes, StackAlignment) - alreadyPushed;
}

static const unsigned FramePushedAfterSave = NonVolatileRegs.gprs().size() * STACK_SLOT_SIZE +
                                             NonVolatileRegs.fpus().size() * sizeof(double);
#ifndef JS_CPU_ARM
static bool
GenerateEntry(ModuleCompiler &m, const AsmJSModule::ExportedFunction &exportedFunc)
{
    MacroAssembler &masm = m.masm();

    
    
    
    
    
    
    masm.setFramePushed(0);
    masm.PushRegsInMask(NonVolatileRegs);

    
    
    
    
    JS_ASSERT(masm.framePushed() == FramePushedAfterSave);
    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    LoadAsmJSActivationIntoRegister(masm, activation);
    masm.movePtr(StackPointer, Operand(activation, AsmJSActivation::offsetOfErrorRejoinSP()));

#if defined(JS_CPU_X64)
    
    
    
    CodeOffsetLabel label = masm.loadRipRelativeInt64(HeapReg);
    m.addGlobalAccess(AsmJSGlobalAccess(label.offset(), m.module().heapOffset()));
#endif

    Register argv = ABIArgGenerator::NonArgReturnVolatileReg0;
    Register scratch = ABIArgGenerator::NonArgReturnVolatileReg1;
#if defined(JS_CPU_X86)
    masm.movl(Operand(StackPointer, NativeFrameSize + masm.framePushed()), argv);
#elif defined(JS_CPU_X64)
    masm.movq(IntArgReg0, argv);
    masm.Push(argv);
#endif

    
    const ModuleCompiler::Func &func = *m.lookupFunction(exportedFunc.name());
    unsigned stackDec = StackDecrementForCall(masm, func.sig().args());
    masm.reserveStack(stackDec);

    for (ABIArgTypeIter iter(func.sig().args()); !iter.done(); iter++) {
        Operand src(argv, iter.index() * sizeof(uint64_t));
        switch (iter->kind()) {
          case ABIArg::GPR:
            masm.load32(src, iter->gpr());
            break;
          case ABIArg::FPU:
            masm.loadDouble(src, iter->fpu());
            break;
          case ABIArg::Stack:
            if (iter.mirType() == MIRType_Int32) {
                masm.load32(src, scratch);
                masm.storePtr(scratch, Operand(StackPointer, iter->offsetFromArgBase()));
            } else {
                JS_ASSERT(iter.mirType() == MIRType_Double);
                masm.loadDouble(src, ScratchFloatReg);
                masm.storeDouble(ScratchFloatReg, Operand(StackPointer, iter->offsetFromArgBase()));
            }
            break;
        }
    }

    AssertStackAlignment(masm);
    masm.call(func.code());

    masm.freeStack(stackDec);

#if defined(JS_CPU_X86)
    masm.movl(Operand(StackPointer, NativeFrameSize + masm.framePushed()), argv);
#elif defined(JS_CPU_X64)
    masm.Pop(argv);
#endif

    
    switch (func.sig().retType().which()) {
      case RetType::Void:
        break;
      case RetType::Signed:
        masm.storeValue(JSVAL_TYPE_INT32, ReturnReg, Address(argv, 0));
        break;
      case RetType::Double:
        masm.canonicalizeDouble(ReturnFloatReg);
        masm.storeDouble(ReturnFloatReg, Address(argv, 0));
        break;
    }

    
    masm.PopRegsInMask(NonVolatileRegs);
    JS_ASSERT(masm.framePushed() == 0);

    masm.move32(Imm32(true), ReturnReg);
    masm.ret();
    return true;
}
#else
static bool
GenerateEntry(ModuleCompiler &m, const AsmJSModule::ExportedFunction &exportedFunc)
{
    const ModuleCompiler::Func &func = *m.lookupFunction(exportedFunc.name());

    MacroAssembler &masm = m.masm();

    
    
    
    
    
    
    masm.setFramePushed(0);
    masm.PushRegsInMask(NonVolatileRegs);
    JS_ASSERT(masm.framePushed() == FramePushedAfterSave);
    JS_ASSERT(masm.framePushed() % 8 == 0);

    
    
    
    

    LoadAsmJSActivationIntoRegister(masm, r9);
    masm.ma_str(StackPointer, Address(r9, AsmJSActivation::offsetOfErrorRejoinSP()));
    

    
    
    Register argv = r9;
    masm.movePtr(IntArgReg1, GlobalReg);  
    masm.movePtr(IntArgReg0, argv);       

    masm.ma_ldr(Operand(GlobalReg, Imm32(m.module().heapOffset())), HeapReg);
    
    JS_ASSERT(masm.framePushed() % 8 == 0);
    masm.Push(argv);
    JS_ASSERT(masm.framePushed() % 8 == 4);

    
    
    unsigned numStackArgs = 0;
    for (ABIArgTypeIter iter(func.sig().args()); !iter.done(); iter++) {
        if (iter->kind() == ABIArg::Stack)
            numStackArgs++;
    }

    
    
    
    JS_ASSERT(masm.framePushed() % 8 == 4);
    unsigned stackDec = numStackArgs * sizeof(double) + (masm.framePushed() >> 2) % 2 * sizeof(uint32_t);
    masm.reserveStack(stackDec);
    
    if(getenv("GDB_BREAK")) {
        masm.breakpoint(js::ion::Assembler::Always);
    }
    
    
    for (ABIArgTypeIter iter(func.sig().args()); !iter.done(); iter++) {
        unsigned argOffset = iter.index() * sizeof(uint64_t);
        switch (iter->kind()) {
          case ABIArg::GPR:
            masm.ma_ldr(Operand(argv, argOffset), iter->gpr());
            break;
          case ABIArg::FPU:
#if defined(JS_CPU_ARM_HARDFP)
            masm.ma_vldr(Operand(argv, argOffset), iter->fpu());
#else
            
            
            masm.ma_dataTransferN(IsLoad, 64, true, argv, Imm32(argOffset), Register::FromCode(iter->fpu().code()*2));
#endif
            break;
          case ABIArg::Stack:
            if (iter.mirType() == MIRType_Int32) {
                masm.memMove32(Address(argv, argOffset), Address(StackPointer, iter->offsetFromArgBase()));
            } else {
                masm.memMove64(Address(argv, argOffset), Address(StackPointer, iter->offsetFromArgBase()));
            }
            break;
        }
    }
    masm.ma_vimm(js_NaN, NANReg);
    masm.call(func.code());

    
    masm.freeStack(stackDec);
    masm.Pop(argv);

    
    switch (func.sig().retType().which()) {
      case RetType::Void:
        break;
      case RetType::Signed:
        masm.storeValue(JSVAL_TYPE_INT32, ReturnReg, Address(argv, 0));
        break;
      case RetType::Double:
#ifndef JS_CPU_ARM_HARDFP
        masm.ma_vxfer(r0, r1, d0);
#endif
        masm.canonicalizeDouble(ReturnFloatReg);
        masm.storeDouble(ReturnFloatReg, Address(argv, 0));
        break;
    }

    masm.PopRegsInMask(NonVolatileRegs);

    masm.ma_mov(Imm32(true), ReturnReg);
    masm.abiret();
    return true;
}
#endif

static inline bool
TryEnablingIon(JSContext *cx, AsmJSModule &module, HandleFunction fun, uint32_t exitIndex,
               int32_t argc, Value *argv)
{
    if (!fun->hasScript())
        return true;

    
    JSScript *script = fun->nonLazyScript();
    if (!script->hasIonScript())
        return true;

    
    if (fun->nargs > argc)
        return true;

    
    
    if (!types::TypeScript::ThisTypes(script)->hasType(types::Type::UndefinedType()))
        return true;
    for(uint32_t i = 0; i < fun->nargs; i++) {
        types::StackTypeSet *typeset = types::TypeScript::ArgTypes(script, i);
        types::Type type = types::Type::DoubleType();
        if (!argv[i].isDouble())
            type = types::Type::PrimitiveType(argv[i].extractNonDoubleType());
        if (!typeset->hasType(type))
            return true;
    }

    
    IonScript *ionScript = script->ionScript();
    if (!ionScript->addDependentAsmJSModule(cx, DependentAsmJSModuleExit(&module, exitIndex)))
        return false;

    module.exitIndexToGlobalDatum(exitIndex).exit = module.exit(exitIndex).ionCode();
    return true;
}

static int32_t
InvokeFromAsmJS_Ignore(JSContext *cx, int32_t exitIndex, int32_t argc, Value *argv)
{
    AsmJSModule &module = cx->mainThread().asmJSActivationStackFromOwnerThread()->module();

    RootedFunction fun(cx, module.exitIndexToGlobalDatum(exitIndex).fun);
    RootedValue fval(cx, ObjectValue(*fun));
    RootedValue rval(cx);
    if (!Invoke(cx, UndefinedValue(), fval, argc, argv, &rval))
        return false;

    if (!TryEnablingIon(cx, module, fun, exitIndex, argc, argv))
        return false;

    return true;
}

static int32_t
InvokeFromAsmJS_ToInt32(JSContext *cx, int32_t exitIndex, int32_t argc, Value *argv)
{
    AsmJSModule &module = cx->mainThread().asmJSActivationStackFromOwnerThread()->module();

    RootedFunction fun(cx, module.exitIndexToGlobalDatum(exitIndex).fun);
    RootedValue fval(cx, ObjectValue(*fun));
    RootedValue rval(cx);
    if (!Invoke(cx, UndefinedValue(), fval, argc, argv, &rval))
        return false;

    if (!TryEnablingIon(cx, module, fun, exitIndex, argc, argv))
        return false;

    int32_t i32;
    if (!ToInt32(cx, rval, &i32))
        return false;
    argv[0] = Int32Value(i32);

    return true;
}

static int32_t
InvokeFromAsmJS_ToNumber(JSContext *cx, int32_t exitIndex, int32_t argc, Value *argv)
{
    AsmJSModule &module = cx->mainThread().asmJSActivationStackFromOwnerThread()->module();

    RootedFunction fun(cx, module.exitIndexToGlobalDatum(exitIndex).fun);
    RootedValue fval(cx, ObjectValue(*fun));
    RootedValue rval(cx);
    if (!Invoke(cx, UndefinedValue(), fval, argc, argv, &rval))
        return false;

    if (!TryEnablingIon(cx, module, fun, exitIndex, argc, argv))
        return false;

    double dbl;
    if (!ToNumber(cx, rval, &dbl))
        return false;
    argv[0] = DoubleValue(dbl);

    return true;
}

static void
FillArgumentArray(ModuleCompiler &m, const VarTypeVector &argTypes,
                  unsigned offsetToArgs, unsigned offsetToCallerStackArgs,
                  Register scratch)
{
    MacroAssembler &masm = m.masm();

    for (ABIArgTypeIter i(argTypes); !i.done(); i++) {
        Address dstAddr = Address(StackPointer, offsetToArgs + i.index() * sizeof(Value));
        switch (i->kind()) {
          case ABIArg::GPR:
            masm.storeValue(JSVAL_TYPE_INT32, i->gpr(), dstAddr);
            break;
          case ABIArg::FPU: {
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
              FloatRegister fr = i->fpu();
              int srcId = fr.code() * 2;
              masm.ma_vxfer(Register::FromCode(srcId), Register::FromCode(srcId+1), fr);
#endif
              masm.canonicalizeDouble(i->fpu());
              masm.storeDouble(i->fpu(), dstAddr);
              break;
          }
          case ABIArg::Stack:
            if (i.mirType() == MIRType_Int32) {
                Address src(StackPointer, offsetToCallerStackArgs + i->offsetFromArgBase());
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
                masm.load32(src, scratch);
                masm.storeValue(JSVAL_TYPE_INT32, scratch, dstAddr);
#else
                masm.memIntToValue(src, dstAddr);
#endif
            } else {
                JS_ASSERT(i.mirType() == MIRType_Double);
                Address src(StackPointer, offsetToCallerStackArgs + i->offsetFromArgBase());
                masm.loadDouble(src, ScratchFloatReg);
                masm.canonicalizeDouble(ScratchFloatReg);
                masm.storeDouble(ScratchFloatReg, dstAddr);
            }
            break;
        }
    }
}

static void
GenerateFFIInterpreterExit(ModuleCompiler &m, const ModuleCompiler::ExitDescriptor &exit,
                           unsigned exitIndex, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();
    masm.align(CodeAlignment);
    m.setInterpExitOffset(exitIndex);
    masm.setFramePushed(0);

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    MIRType typeArray[] = { MIRType_Pointer,   
                            MIRType_Pointer,   
                            MIRType_Int32,     
                            MIRType_Pointer }; 
    MIRTypeVector invokeArgTypes(m.cx());
    invokeArgTypes.infallibleAppend(typeArray, ArrayLength(typeArray));

    
    
    unsigned arraySize = Max<size_t>(1, exit.sig().args().length()) * sizeof(Value);
    unsigned stackDec = StackDecrementForCall(masm, invokeArgTypes, arraySize);
    masm.reserveStack(stackDec);

    
    unsigned offsetToCallerStackArgs = NativeFrameSize + masm.framePushed();
    unsigned offsetToArgv = StackArgBytes(invokeArgTypes);
    Register scratch = ABIArgGenerator::NonArgReturnVolatileReg0;
    FillArgumentArray(m, exit.sig().args(), offsetToArgv, offsetToCallerStackArgs, scratch);

    
    ABIArgMIRTypeIter i(invokeArgTypes);
    Register activation = ABIArgGenerator::NonArgReturnVolatileReg1;
    LoadAsmJSActivationIntoRegister(masm, activation);

    
    if (i->kind() == ABIArg::GPR) {
        LoadJSContextFromActivation(masm, activation, i->gpr());
    } else {
        LoadJSContextFromActivation(masm, activation, scratch);
        masm.movePtr(scratch, Operand(StackPointer, i->offsetFromArgBase()));
    }
    i++;

    
    if (i->kind() == ABIArg::GPR)
        masm.mov(Imm32(exitIndex), i->gpr());
    else
        masm.mov(Imm32(exitIndex), Operand(StackPointer, i->offsetFromArgBase()));
    i++;

    
    unsigned argc = exit.sig().args().length();
    if (i->kind() == ABIArg::GPR)
        masm.mov(Imm32(argc), i->gpr());
    else
        masm.move32(Imm32(argc), Operand(StackPointer, i->offsetFromArgBase()));
    i++;

    
    Address argv(StackPointer, offsetToArgv);
    if (i->kind() == ABIArg::GPR) {
        masm.computeEffectiveAddress(argv, i->gpr());
    } else {
        masm.computeEffectiveAddress(argv, scratch);
        masm.movePtr(scratch, Operand(StackPointer, i->offsetFromArgBase()));
    }
    i++;
    JS_ASSERT(i.done());

    
    AssertStackAlignment(masm);
    switch (exit.sig().retType().which()) {
      case RetType::Void:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_Ignore)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
        break;
      case RetType::Signed:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_ToInt32)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
        masm.unboxInt32(argv, ReturnReg);
        break;
      case RetType::Double:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_ToNumber)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
        masm.loadDouble(argv, ReturnFloatReg);
        break;
    }

    
    
    masm.freeStack(stackDec);
    masm.ret();
#else
    const unsigned arrayLength = Max<size_t>(1, exit.sig().args().length());
    const unsigned arraySize = arrayLength * sizeof(Value);
    const unsigned reserveSize = AlignBytes(arraySize, StackAlignment) +
        ShadowStackSpace;
    const unsigned callerArgsOffset = reserveSize + NativeFrameSize + sizeof(int32_t);
    masm.setFramePushed(0);
    masm.Push(lr);
    masm.reserveStack(reserveSize + sizeof(int32_t));

    
    FillArgumentArray(m, exit.sig().args(), ShadowStackSpace, callerArgsOffset, IntArgReg0);

    
    Register activation = IntArgReg3;
    LoadAsmJSActivationIntoRegister(masm, activation);

    LoadJSContextFromActivation(masm, activation, IntArgReg0);

    
    masm.mov(Imm32(exitIndex), IntArgReg1);

    
    masm.mov(Imm32(exit.sig().args().length()), IntArgReg2);

    
    Address argv(StackPointer, ShadowStackSpace);
    masm.lea(Operand(argv), IntArgReg3);

    AssertStackAlignment(masm);
    switch (exit.sig().retType().which()) {
      case RetType::Void:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_Ignore)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
        break;
      case RetType::Signed:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_ToInt32)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
        masm.unboxInt32(argv, ReturnReg);
        break;
      case RetType::Double:
        masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, &InvokeFromAsmJS_ToNumber)));
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
        masm.loadValue(argv, softfpReturnOperand);
#else
        masm.loadDouble(argv, ReturnFloatReg);
#endif
        break;
    }

    masm.freeStack(reserveSize + sizeof(int32_t));
    masm.ret();
#endif
}

static int32_t
ValueToInt32(JSContext *cx, Value *val)
{
    int32_t i32;
    if (!ToInt32(cx, val[0], &i32))
        return false;
    val[0] = Int32Value(i32);

    return true;
}

static int32_t
ValueToNumber(JSContext *cx, MutableHandleValue val)
{
    double dbl;
    if (!ToNumber(cx, val, &dbl))
        return false;
    val.set(DoubleValue(dbl));

    return true;
}

static void
GenerateOOLConvert(ModuleCompiler &m, RetType retType, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();

    MIRType typeArray[] = { MIRType_Pointer,   
                            MIRType_Pointer }; 
    MIRTypeVector callArgTypes(m.cx());
    callArgTypes.infallibleAppend(typeArray, ArrayLength(typeArray));

    
    
    unsigned arraySize = sizeof(Value);
    unsigned stackDec = StackDecrementForCall(masm, callArgTypes, arraySize);
    masm.setFramePushed(0);
    masm.reserveStack(stackDec);

    
    unsigned offsetToArgv = StackArgBytes(callArgTypes);
    masm.storeValue(JSReturnOperand, Address(StackPointer, offsetToArgv));

    
    ABIArgMIRTypeIter i(callArgTypes);
    Register scratch = ABIArgGenerator::NonArgReturnVolatileReg0;

    
    Register activation = ABIArgGenerator::NonArgReturnVolatileReg1;
    LoadAsmJSActivationIntoRegister(masm, activation);
    if (i->kind() == ABIArg::GPR) {
        LoadJSContextFromActivation(masm, activation, i->gpr());
    } else {
        LoadJSContextFromActivation(masm, activation, scratch);
        masm.storePtr(scratch, Address(StackPointer, i->offsetFromArgBase()));
    }
    i++;

    
    Address argv(StackPointer, offsetToArgv);
    if (i->kind() == ABIArg::GPR) {
        masm.computeEffectiveAddress(argv, i->gpr());
    } else {
        masm.computeEffectiveAddress(argv, scratch);
        masm.storePtr(scratch, Address(StackPointer, i->offsetFromArgBase()));
    }
    i++;
    JS_ASSERT(i.done());

    
    switch (retType.which()) {
      case RetType::Signed:
          masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void *, &ValueToInt32)));
          masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
          masm.unboxInt32(Address(StackPointer, offsetToArgv), ReturnReg);
          break;
      case RetType::Double:
          masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void *, &ValueToNumber)));
          masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
          masm.loadValue(Address(StackPointer, offsetToArgv), softfpReturnOperand);
#else
          masm.loadDouble(Address(StackPointer, offsetToArgv), ReturnFloatReg);
#endif
          break;
      default:
          MOZ_ASSUME_UNREACHABLE("Unsupported convert type");
    }

    masm.freeStack(stackDec);
}

static void
EnableActivation(AsmJSActivation *activation)
{
    JSContext *cx = activation->cx();
    Activation *act = cx->mainThread().activation();
    JS_ASSERT(act->isJit());
    act->asJit()->setActive(cx);
}

static void
DisableActivation(AsmJSActivation *activation)
{
    JSContext *cx = activation->cx();
    Activation *act = cx->mainThread().activation();
    JS_ASSERT(act->isJit());
    act->asJit()->setActive(cx, false);
}

static void
GenerateFFIIonExit(ModuleCompiler &m, const ModuleCompiler::ExitDescriptor &exit,
                         unsigned exitIndex, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();
    masm.align(CodeAlignment);
    m.setIonExitOffset(exitIndex);
    masm.setFramePushed(0);

    RegisterSet restoreSet = RegisterSet::Intersect(RegisterSet::All(),
                                                    RegisterSet::Not(RegisterSet::Volatile()));
#if defined(JS_CPU_ARM)
    masm.Push(lr);
#endif
    masm.PushRegsInMask(restoreSet);

    
    

    
    MIRTypeVector emptyVector(m.cx());
    unsigned argBytes = 3 * sizeof(size_t) + (1 + exit.sig().args().length()) * sizeof(Value);
    unsigned extraBytes = 0;
#if defined(JS_CPU_ARM)
    extraBytes += sizeof(size_t);
#endif
    unsigned stackDec = StackDecrementForCall(masm, emptyVector, argBytes + extraBytes);
    masm.reserveStack(stackDec - extraBytes);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed() + extraBytes, IonFrame_Entry);
    masm.storePtr(ImmWord(uintptr_t(descriptor)), Address(StackPointer, 0));

    
    Register callee = ABIArgGenerator::NonArgReturnVolatileReg0;
    Register scratch = ABIArgGenerator::NonArgReturnVolatileReg1;

    
    unsigned globalDataOffset = m.module().exitIndexToGlobalDataOffset(exitIndex);
#if defined(JS_CPU_X64)
    CodeOffsetLabel label2 = masm.leaRipRelative(callee);
    m.addGlobalAccess(AsmJSGlobalAccess(label2.offset(), globalDataOffset));
#elif defined(JS_CPU_X86)
    CodeOffsetLabel label2 = masm.movlWithPatch(Imm32(0), callee);
    m.addGlobalAccess(AsmJSGlobalAccess(label2.offset(), globalDataOffset));
#else
    masm.lea(Operand(GlobalReg, globalDataOffset), callee);
#endif

    
    masm.loadPtr(Address(callee, offsetof(AsmJSModule::ExitDatum, fun)), callee);

    
    masm.storePtr(callee, Address(StackPointer, sizeof(size_t)));

    
    unsigned argc = exit.sig().args().length();
    masm.storePtr(ImmWord(uintptr_t(argc)), Address(StackPointer, 2 * sizeof(size_t)));

    
    masm.storeValue(UndefinedValue(), Address(StackPointer, 3 * sizeof(size_t)));

    
    unsigned offsetToArgs = 3 * sizeof(size_t) + sizeof(Value);
    unsigned offsetToCallerStackArgs = masm.framePushed();
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    offsetToCallerStackArgs += NativeFrameSize;
#else
    offsetToCallerStackArgs += ShadowStackSpace;
#endif
    FillArgumentArray(m, exit.sig().args(), offsetToArgs, offsetToCallerStackArgs, scratch);

    
    Label done, oolConvert;
    Label *maybeDebugBreakpoint = NULL;

#ifdef DEBUG
    Label ionFailed;
    maybeDebugBreakpoint = &ionFailed;
    masm.branchIfFunctionHasNoScript(callee, &ionFailed);
#endif

    masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), scratch);
    masm.loadBaselineOrIonNoArgCheck(scratch, scratch, SequentialExecution, maybeDebugBreakpoint);

    LoadAsmJSActivationIntoRegister(masm, callee);
    masm.push(scratch);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(callee);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, EnableActivation));
    masm.pop(scratch);

    
#if defined(JS_CPU_ARM) && defined(DEBUG)
    
    masm.Push(scratch);
#endif
    AssertStackAlignment(masm);
#if defined(JS_CPU_ARM) && defined(DEBUG)
    masm.freeStack(sizeof(size_t));
#endif
    masm.callIon(scratch);
    masm.freeStack(stackDec - extraBytes);

    masm.push(JSReturnReg_Type);
    masm.push(JSReturnReg_Data);
    LoadAsmJSActivationIntoRegister(masm, callee);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(callee);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, DisableActivation));
    masm.pop(JSReturnReg_Data);
    masm.pop(JSReturnReg_Type);

#ifdef DEBUG
    masm.branchTestMagicValue(Assembler::Equal, JSReturnOperand, JS_ION_ERROR, throwLabel);
    masm.branchTestMagic(Assembler::Equal, JSReturnOperand, &ionFailed);
#else
    masm.branchTestMagic(Assembler::Equal, JSReturnOperand, throwLabel);
#endif

    switch (exit.sig().retType().which()) {
      case RetType::Void:
        break;
      case RetType::Signed:
        masm.convertValueToInt32(JSReturnOperand, ReturnFloatReg, ReturnReg, &oolConvert);
        break;
      case RetType::Double:
        masm.convertValueToDouble(JSReturnOperand, ReturnFloatReg, &oolConvert);
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
        masm.boxDouble(ReturnFloatReg, softfpReturnOperand);
#endif
        break;
    }

    masm.bind(&done);
    masm.PopRegsInMask(restoreSet);
    masm.ret();

    
    if (oolConvert.used()) {
        masm.bind(&oolConvert);
        GenerateOOLConvert(m, exit.sig().retType(), throwLabel);
        masm.jump(&done);
    }

#ifdef DEBUG
    masm.bind(&ionFailed);
    masm.breakpoint();
#endif
}


static void
GenerateFFIExit(ModuleCompiler &m, const ModuleCompiler::ExitDescriptor &exit, unsigned exitIndex,
                Label *throwLabel)
{
    
    GenerateFFIInterpreterExit(m, exit, exitIndex, throwLabel);

    
    GenerateFFIIonExit(m, exit, exitIndex, throwLabel);
}




static bool
GenerateStackOverflowExit(ModuleCompiler &m, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();
    masm.align(CodeAlignment);
    masm.bind(&m.stackOverflowLabel());

#if defined(JS_CPU_X86)
    
    masm.push(Imm32(0));
#endif

    
    
    
    masm.andPtr(Imm32(~(StackAlignment - 1)), StackPointer);
    if (ShadowStackSpace)
        masm.subPtr(Imm32(ShadowStackSpace), StackPointer);

    
#if defined(JS_CPU_X86)
    LoadAsmJSActivationIntoRegister(masm, eax);
    LoadJSContextFromActivation(masm, eax, eax);
    masm.storePtr(eax, Address(StackPointer, 0));
#elif defined(JS_CPU_X64)
    LoadAsmJSActivationIntoRegister(masm, IntArgReg0);
    LoadJSContextFromActivation(masm, IntArgReg0, IntArgReg0);
#else

    
    
    LoadAsmJSActivationIntoRegister(masm, IntArgReg0);
    LoadJSContextFromActivation(masm, IntArgReg0, IntArgReg0);

#endif
    void (*pf)(JSContext*) = js_ReportOverRecursed;
    masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, pf)));
    masm.jump(throwLabel);

    return !masm.oom();
}









static bool
GenerateOperationCallbackExit(ModuleCompiler &m, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();
    masm.align(CodeAlignment);
    masm.bind(&m.operationCallbackLabel());

#ifndef JS_CPU_ARM
    
    
    
    masm.push(Imm32(0));            
    masm.pushFlags();               
    masm.setFramePushed(0);         
    masm.PushRegsInMask(AllRegsExceptSP); 

    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    Register scratch = ABIArgGenerator::NonArgReturnVolatileReg1;

    
    LoadAsmJSActivationIntoRegister(masm, activation);
    masm.loadPtr(Address(activation, AsmJSActivation::offsetOfResumePC()), scratch);
    masm.storePtr(scratch, Address(StackPointer, masm.framePushed() + sizeof(void*)));

    
    
    masm.mov(StackPointer, ABIArgGenerator::NonVolatileReg);
#if defined(JS_CPU_X86)
    
    masm.push(Imm32(0));
#endif
    masm.andPtr(Imm32(~(StackAlignment - 1)), StackPointer);
    if (ShadowStackSpace)
        masm.subPtr(Imm32(ShadowStackSpace), StackPointer);

    
#if defined(JS_CPU_X86)
    LoadJSContextFromActivation(masm, activation, scratch);
    masm.storePtr(scratch, Address(StackPointer, 0));
#elif defined(JS_CPU_X64)
    LoadJSContextFromActivation(masm, activation, IntArgReg0);
#endif

    JSBool (*pf)(JSContext*) = js_HandleExecutionInterrupt;
    masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, pf)));
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);

    
    masm.mov(ABIArgGenerator::NonVolatileReg, StackPointer);

    
    masm.PopRegsInMask(AllRegsExceptSP); 
    masm.popFlags();              
    masm.ret();                   
#else
    masm.setFramePushed(0);         
    masm.PushRegsInMask(RegisterSet(GeneralRegisterSet(Registers::AllMask & ~(1<<Registers::sp)), FloatRegisterSet(uint32_t(0))));   

    
    masm.as_mrs(r4);
    masm.as_vmrs(r5);
    
    masm.mov(sp,r6);
    
    masm.ma_and(Imm32(~7), sp, sp);

    
    LoadAsmJSActivationIntoRegister(masm, IntArgReg0);
    masm.loadPtr(Address(IntArgReg0, AsmJSActivation::offsetOfResumePC()), IntArgReg1);
    masm.storePtr(IntArgReg1, Address(r6, 14 * sizeof(uint32_t*)));

    
    masm.loadPtr(Address(IntArgReg0, AsmJSActivation::offsetOfContext()), IntArgReg0);

    masm.PushRegsInMask(RegisterSet(GeneralRegisterSet(0), FloatRegisterSet(FloatRegisters::AllMask)));   
    JSBool (*pf)(JSContext*) = js_HandleExecutionInterrupt;
    masm.call(ImmWord(JS_FUNC_TO_DATA_PTR(void*, pf)));
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, throwLabel);

    
    masm.PopRegsInMask(RegisterSet(GeneralRegisterSet(0), FloatRegisterSet(FloatRegisters::AllMask)));   
    masm.mov(r6,sp);
    masm.as_vmsr(r5);
    masm.as_msr(r4);
    
    masm.startDataTransferM(IsLoad, sp, IA, WriteBack);
    masm.transferReg(r0);
    masm.transferReg(r1);
    masm.transferReg(r2);
    masm.transferReg(r3);
    masm.transferReg(r4);
    masm.transferReg(r5);
    masm.transferReg(r6);
    masm.transferReg(r7);
    masm.transferReg(r8);
    masm.transferReg(r9);
    masm.transferReg(r10);
    masm.transferReg(r11);
    masm.transferReg(r12);
    masm.transferReg(lr);
    masm.finishDataTransfer();
    masm.ret();

#endif

    return !masm.oom();
}






static bool
GenerateThrowExit(ModuleCompiler &m, Label *throwLabel)
{
    MacroAssembler &masm = m.masm();
    masm.align(CodeAlignment);
    masm.bind(throwLabel);

    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    LoadAsmJSActivationIntoRegister(masm, activation);

    masm.setFramePushed(FramePushedAfterSave);
    masm.loadPtr(Address(activation, AsmJSActivation::offsetOfErrorRejoinSP()), StackPointer);

    masm.PopRegsInMask(NonVolatileRegs);
    JS_ASSERT(masm.framePushed() == 0);

    masm.mov(Imm32(0), ReturnReg);
    masm.abiret();

    return !masm.oom();
}

static bool
GenerateStubs(ModuleCompiler &m)
{
    for (unsigned i = 0; i < m.module().numExportedFunctions(); i++) {
        m.setEntryOffset(i);
        if (!GenerateEntry(m, m.module().exportedFunction(i)))
            return false;
    }

    Label throwLabel;

    for (ModuleCompiler::ExitMap::Range r = m.allExits(); !r.empty(); r.popFront()) {
        GenerateFFIExit(m, r.front().key, r.front().value, &throwLabel);
        if (m.masm().oom())
            return false;
    }

    if (m.stackOverflowLabel().used()) {
        if (!GenerateStackOverflowExit(m, &throwLabel))
            return false;
    }

    if (!GenerateOperationCallbackExit(m, &throwLabel))
        return false;

    if (!GenerateThrowExit(m, &throwLabel))
        return false;

    return true;
}

static bool
FinishModule(ModuleCompiler &m,
             ScopedJSDeletePtr<AsmJSModule> *module,
             ScopedJSFreePtr<char> *compilationTimeReport)
{
    TempAllocator alloc(&m.cx()->tempLifoAlloc());
    IonContext ionContext(m.cx()->compartment(), &alloc);

    if (!GenerateStubs(m))
        return false;

    return m.staticallyLink(module, compilationTimeReport);
}

static bool
CheckModule(JSContext *cx, AsmJSParser &parser, ParseNode *stmtList,
            ScopedJSDeletePtr<AsmJSModule> *module,
            ScopedJSFreePtr<char> *compilationTimeReport)
{
    ModuleCompiler m(cx, parser);
    if (!m.init())
        return false;

    if (PropertyName *moduleFunctionName = FunctionName(m.moduleFunctionNode())) {
        if (!CheckModuleLevelName(m, m.moduleFunctionNode(), moduleFunctionName))
            return false;
        m.initModuleFunctionName(moduleFunctionName);
    }

    if (!CheckFunctionHead(m, m.moduleFunctionNode()))
        return false;

    if (!CheckModuleArguments(m, m.moduleFunctionNode()))
        return false;

    if (!CheckPrecedingStatements(m, stmtList))
        return false;

    if (!CheckModuleGlobals(m))
        return false;

#ifdef JS_WORKER_THREADS
    if (OffThreadCompilationEnabled(cx)) {
        if (!CheckFunctionsParallel(m))
            return false;
    } else {
        if (!CheckFunctionsSequential(m))
            return false;
    }
#else
    if (!CheckFunctionsSequential(m))
        return false;
#endif

    m.finishFunctionBodies();

    if (!CheckFuncPtrTables(m))
        return false;

    if (!CheckModuleReturn(m))
        return false;

    TokenKind tk = PeekToken(m.parser());
    if (tk != TOK_EOF && tk != TOK_RC)
        return m.fail(NULL, "top-level export (return) must be the last statement");

    return FinishModule(m, module, compilationTimeReport);
}

static bool
Warn(JSContext *cx, int code, const char *str)
{
    return JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING, js_GetErrorMessage,
                                        NULL, code, str);
}

extern bool
EnsureAsmJSSignalHandlersInstalled(JSRuntime *rt);

bool
js::CompileAsmJS(JSContext *cx, AsmJSParser &parser, ParseNode *stmtList, bool *validated)
{
    *validated = false;

    if (!JSC::MacroAssembler::supportsFloatingPoint())
        return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Disabled by lack of floating point support");

    if (cx->runtime()->gcSystemPageSize != AsmJSPageSize)
        return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Disabled by non 4KiB system page size");

    if (!cx->hasOption(JSOPTION_ASMJS))
        return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Disabled by javascript.options.asmjs in about:config");

    if (cx->compartment()->debugMode())
        return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Disabled by debugger");

    if (!EnsureAsmJSSignalHandlersInstalled(cx->runtime()))
        return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Platform missing signal handler support");

# ifdef JS_WORKER_THREADS
    if (OffThreadCompilationEnabled(cx)) {
        if (!EnsureWorkerThreadsInitialized(cx->runtime()))
            return Warn(cx, JSMSG_USE_ASM_TYPE_FAIL, "Failed compilation thread initialization");
    }
# endif

    ScopedJSFreePtr<char> compilationTimeReport;
    ScopedJSDeletePtr<AsmJSModule> module;
    if (!CheckModule(cx, parser, stmtList, &module, &compilationTimeReport))
        return !cx->isExceptionPending();

    RootedObject moduleObj(cx, NewAsmJSModuleObject(cx, &module));
    if (!moduleObj)
        return false;

    ParseNode *fn = parser.pc->maybeFunction;
    RootedFunction origFun(cx, fn->pn_funbox->function());
    RootedPropertyName name(cx, origFun->name());
    RootedFunction moduleFun(cx, NewFunction(cx, NullPtr(), LinkAsmJS, origFun->nargs,
                                             JSFunction::NATIVE_FUN, NullPtr(), name,
                                             JSFunction::ExtendedFinalizeKind, TenuredObject));
    if (!moduleFun)
        return false;

    SetAsmJSModuleObject(moduleFun, moduleObj);

    
    
    
    
    JS_ASSERT(fn->pn_funbox->function()->isInterpreted());
    fn->pn_funbox->object = moduleFun;
    JS_ASSERT(IsAsmJSModuleNative(fn->pn_funbox->function()->native()));

    *validated = true;
    return Warn(cx, JSMSG_USE_ASM_TYPE_OK, compilationTimeReport);
}

JSBool
js::IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool available = JSC::MacroAssembler::supportsFloatingPoint() &&
                     !cx->compartment()->debugMode() &&
                     cx->hasOption(JSOPTION_ASMJS);

    args.rval().set(BooleanValue(available));
    return true;
}

static bool
IsMaybeWrappedNativeFunction(const Value &v, Native native)
{
    if (!v.isObject())
        return false;

    JSObject *obj = CheckedUnwrap(&v.toObject());
    if (!obj)
        return false;

    return obj->is<JSFunction>() && obj->as<JSFunction>().maybeNative() == native;
}

JSBool
js::IsAsmJSModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool rval = args.hasDefined(0) && IsMaybeWrappedNativeFunction(args[0], LinkAsmJS);
    args.rval().set(BooleanValue(rval));
    return true;
}

JSBool
js::IsAsmJSFunction(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool rval = args.hasDefined(0) && IsMaybeWrappedNativeFunction(args[0], CallAsmJS);
    args.rval().set(BooleanValue(rval));
    return true;
}

AsmJSModule::~AsmJSModule()
{
    if (code_) {
        for (unsigned i = 0; i < numExits(); i++) {
            AsmJSModule::ExitDatum &exitDatum = exitIndexToGlobalDatum(i);
            if (!exitDatum.fun)
                continue;

            if (!exitDatum.fun->hasScript())
                continue;

            JSScript *script = exitDatum.fun->nonLazyScript();
            if (!script->hasIonScript())
                continue;

            DependentAsmJSModuleExit exit(this, i);
            script->ionScript()->removeDependentAsmJSModule(exit);
        }
    }

    for (size_t i = 0; i < numFunctionCounts(); i++)
        js_delete(functionCounts(i));
}
