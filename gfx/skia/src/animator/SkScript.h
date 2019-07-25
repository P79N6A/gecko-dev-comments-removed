








#ifndef SkScript_DEFINED
#define SkScript_DEFINED

#include "SkOperand.h"
#include "SkIntArray.h"
#include "SkTDict.h"
#include "SkTDStack.h"

class SkAnimateMaker;

class SkScriptEngine {
public:
    enum Error {
        kNoError,
        kArrayIndexOutOfBounds,
        kCouldNotFindReferencedID,
        kDotOperatorExpectsObject,
        kErrorInArrrayIndex,
        kErrorInFunctionParameters,
        kExpectedArray,
        kExpectedBooleanExpression,
        kExpectedFieldName,
        kExpectedHex,
        kExpectedIntForConditionOperator,
        kExpectedNumber,
        kExpectedNumberForArrayIndex,
        kExpectedOperator,
        kExpectedToken,
        kExpectedTokenBeforeDotOperator,
        kExpectedValue,
        kHandleMemberFailed,
        kHandleMemberFunctionFailed,
        kHandleUnboxFailed,
        kIndexOutOfRange,
        kMismatchedArrayBrace,
        kMismatchedBrackets,
        kNoFunctionHandlerFound,
        kPrematureEnd,
        kTooManyParameters,
        kTypeConversionFailed,
        kUnterminatedString
    };

    enum SkOpType {
        kNoType,
        kInt = 1,
        kScalar = 2,
        kString = 4,
        kArray = 8,
        kObject = 16

    };

    typedef bool (*_boxCallBack)(void* userStorage, SkScriptValue* result);
    typedef bool (*_functionCallBack)(const char* func, size_t len, SkTDArray<SkScriptValue>& params,
        void* userStorage, SkScriptValue* result);
    typedef bool (*_memberCallBack)(const char* member, size_t len, void* object, 
        void* userStorage, SkScriptValue* result);
    typedef bool (*_memberFunctionCallBack)(const char* member, size_t len, void* object, 
        SkTDArray<SkScriptValue>& params, void* userStorage, SkScriptValue* result);

    typedef bool (*_propertyCallBack)(const char* prop, size_t len, void* userStorage, SkScriptValue* result);
    typedef bool (*_unboxCallBack)(void* userStorage, SkScriptValue* result);
    SkScriptEngine(SkOpType returnType);
    ~SkScriptEngine();
    void boxCallBack(_boxCallBack func, void* userStorage);
    bool convertTo(SkDisplayTypes , SkScriptValue* );
    bool evaluateScript(const char** script, SkScriptValue* value);
    void forget(SkTypedArray* array);
    void functionCallBack(_functionCallBack func, void* userStorage);
    Error getError() const { return fError; }
#ifdef SK_DEBUG
    bool getErrorString(SkString* err) const;
#endif
    void memberCallBack(_memberCallBack , void* userStorage);
    void memberFunctionCallBack(_memberFunctionCallBack , void* userStorage);

    void propertyCallBack(_propertyCallBack prop, void* userStorage);
    void track(SkTypedArray* array);
    void track(SkString* string);
    void unboxCallBack(_unboxCallBack func, void* userStorage);
    static bool ConvertTo(SkScriptEngine* , SkDisplayTypes toType, SkScriptValue* value);
    static SkScalar IntToScalar(int32_t );
    static SkDisplayTypes ToDisplayType(SkOpType type);
    static SkOpType ToOpType(SkDisplayTypes type);
    static bool ValueToString(SkScriptValue value, SkString* string);

    enum CallBackType {
        kBox,
        kFunction,
        kMember,
        kMemberFunction,
    
        kProperty,
        kUnbox
    };

    struct UserCallBack {
        CallBackType fCallBackType;
        void* fUserStorage;
        union {
            _boxCallBack fBoxCallBack;
            _functionCallBack fFunctionCallBack;
            _memberCallBack fMemberCallBack;
            _memberFunctionCallBack fMemberFunctionCallBack;
    
            _propertyCallBack fPropertyCallBack;
            _unboxCallBack fUnboxCallBack;
        };
    };

    enum SkOp {
        kUnassigned,
        kAdd,
        kAddInt = kAdd,
        kAddScalar,
        kAddString, 
        kArrayOp,
        kBitAnd,
        kBitNot,
        kBitOr,
        kDivide,
        kDivideInt = kDivide,
        kDivideScalar,
        kElse,
        kEqual,
        kEqualInt = kEqual,
        kEqualScalar,
        kEqualString,
        kFlipOps,
        kGreaterEqual,
        kGreaterEqualInt = kGreaterEqual,
        kGreaterEqualScalar,
        kGreaterEqualString,
        kIf,
        kLogicalAnd,
        kLogicalNot,
        kLogicalOr,
        kMinus,
        kMinusInt = kMinus,
        kMinusScalar,
        kModulo,
        kModuloInt = kModulo,
        kModuloScalar,
        kMultiply,
        kMultiplyInt = kMultiply,
        kMultiplyScalar,
        kParen,
        kShiftLeft,
        kShiftRight,    
        kSubtract,
        kSubtractInt = kSubtract,
        kSubtractScalar,
        kXor,
        kArtificialOp = 0x40
    };

    enum SkOpBias {
        kNoBias,
        kTowardsNumber = 0,
        kTowardsString
    };
    
protected:

    struct SkOperatorAttributes {
        unsigned int fLeftType : 3; 
        unsigned int fRightType : 3;     
        SkOpBias fBias : 1;
    };

    struct SkSuppress { 
        SkOp fOperator; 
        int fOpStackDepth; 
        SkBool8 fSuppress; 
        SkBool8 fElse; 
    };

    static const SkOperatorAttributes gOpAttributes[];
    static const signed char gPrecedence[];
    int arithmeticOp(char ch, char nextChar, bool lastPush);
    void commonCallBack(CallBackType type, UserCallBack& callBack, void* userStorage);
    bool convertParams(SkTDArray<SkScriptValue>&, const SkFunctionParamType* ,
                                    int paramTypeCount);
    void convertToString(SkOperand& operand, SkDisplayTypes type) {
        SkScriptValue scriptValue;
        scriptValue.fOperand = operand;
        scriptValue.fType = type;
        convertTo(SkType_String, &scriptValue);
        operand = scriptValue.fOperand;
    }
    bool evaluateDot(const char*& script, bool suppressed);
    bool evaluateDotParam(const char*& script, bool suppressed, const char* field, size_t fieldLength);
    bool functionParams(const char** scriptPtr, SkTDArray<SkScriptValue>& params);
    bool handleArrayIndexer(const char** scriptPtr, bool suppressed);
    bool handleBox(SkScriptValue* value);
    bool handleFunction(const char** scriptPtr, bool suppressed);
    bool handleMember(const char* field, size_t len, void* object);
    bool handleMemberFunction(const char* field, size_t len, void* object, SkTDArray<SkScriptValue>& params);

    bool handleProperty(bool suppressed);
    bool handleUnbox(SkScriptValue* scriptValue);
    bool innerScript(const char** scriptPtr, SkScriptValue* value);
    int logicalOp(char ch, char nextChar);
    Error opError();
    bool processOp();
    void setAnimateMaker(SkAnimateMaker* maker) { fMaker = maker; }
    bool setError(Error , const char* pos);
    enum SkBraceStyle {
    
        kArrayBrace,
        kFunctionBrace
    };

#if 0
    SkIntArray(SkBraceStyle) fBraceStack;       
    SkIntArray(SkOp) fOpStack;
    SkIntArray(SkOpType) fTypeStack;
    SkTDOperandArray fOperandStack;
    SkTDArray<SkSuppress> fSuppressStack;
#else
    SkTDStack<SkBraceStyle> fBraceStack;        
    SkTDStack<SkOp> fOpStack;
    SkTDStack<SkOpType> fTypeStack;
    SkTDStack<SkOperand> fOperandStack;
    SkTDStack<SkSuppress> fSuppressStack;
#endif
    SkAnimateMaker* fMaker;
    SkTDTypedArrayArray fTrackArray;
    SkTDStringArray fTrackString;
    const char* fToken; 
    size_t fTokenLength;
    SkTDArray<UserCallBack> fUserCallBacks;
    SkOpType fReturnType;
    Error fError;
    int fErrorPosition;
private:
    friend class SkTypedArray;
#ifdef SK_SUPPORT_UNITTEST
public:
    static void UnitTest();
#endif
};

#ifdef SK_SUPPORT_UNITTEST

struct SkScriptNAnswer {
    const char* fScript;
    SkDisplayTypes fType;
    int32_t fIntAnswer;
    SkScalar fScalarAnswer;
    const char* fStringAnswer;
};

#endif

#endif
