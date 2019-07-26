








#include "SkDisplayMath.h"

enum SkDisplayMath_Properties {
    SK_PROPERTY(E),
    SK_PROPERTY(LN10),
    SK_PROPERTY(LN2),
    SK_PROPERTY(LOG10E),
    SK_PROPERTY(LOG2E),
    SK_PROPERTY(PI),
    SK_PROPERTY(SQRT1_2),
    SK_PROPERTY(SQRT2)
};

const SkScalar SkDisplayMath::gConstants[] = {
    2.718281828f,   
    2.302585093f,   
    0.693147181f,   
    0.434294482f,   
    1.442695041f,   
    3.141592654f,   
    0.707106781f,   
    1.414213562f        
};

enum SkDisplayMath_Functions {
    SK_FUNCTION(abs),
    SK_FUNCTION(acos),
    SK_FUNCTION(asin),
    SK_FUNCTION(atan),
    SK_FUNCTION(atan2),
    SK_FUNCTION(ceil),
    SK_FUNCTION(cos),
    SK_FUNCTION(exp),
    SK_FUNCTION(floor),
    SK_FUNCTION(log),
    SK_FUNCTION(max),
    SK_FUNCTION(min),
    SK_FUNCTION(pow),
    SK_FUNCTION(random),
    SK_FUNCTION(round),
    SK_FUNCTION(sin),
    SK_FUNCTION(sqrt),
    SK_FUNCTION(tan)
};

const SkFunctionParamType SkDisplayMath::fFunctionParameters[] = {
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) SkType_Float,
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Array, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Array, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) SkType_Float,
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0,
    (SkFunctionParamType) SkType_Float, 
    (SkFunctionParamType) 0
};

#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkDisplayMath::fInfo[] = {
    SK_MEMBER_PROPERTY(E, Float),
    SK_MEMBER_PROPERTY(LN10, Float),
    SK_MEMBER_PROPERTY(LN2, Float),
    SK_MEMBER_PROPERTY(LOG10E, Float),
    SK_MEMBER_PROPERTY(LOG2E, Float),
    SK_MEMBER_PROPERTY(PI, Float),
    SK_MEMBER_PROPERTY(SQRT1_2, Float),
    SK_MEMBER_PROPERTY(SQRT2, Float),
    SK_MEMBER_FUNCTION(abs, Float),
    SK_MEMBER_FUNCTION(acos, Float),
    SK_MEMBER_FUNCTION(asin, Float),
    SK_MEMBER_FUNCTION(atan, Float),
    SK_MEMBER_FUNCTION(atan2, Float),
    SK_MEMBER_FUNCTION(ceil, Float),
    SK_MEMBER_FUNCTION(cos, Float),
    SK_MEMBER_FUNCTION(exp, Float),
    SK_MEMBER_FUNCTION(floor, Float),
    SK_MEMBER_FUNCTION(log, Float),
    SK_MEMBER_FUNCTION(max, Float),
    SK_MEMBER_FUNCTION(min, Float),
    SK_MEMBER_FUNCTION(pow, Float),
    SK_MEMBER_FUNCTION(random, Float),
    SK_MEMBER_FUNCTION(round, Float),
    SK_MEMBER_FUNCTION(sin, Float),
    SK_MEMBER_FUNCTION(sqrt, Float),
    SK_MEMBER_FUNCTION(tan, Float)
};

#endif

DEFINE_GET_MEMBER(SkDisplayMath);

void SkDisplayMath::executeFunction(SkDisplayable* target, int index,
        SkTDArray<SkScriptValue>& parameters, SkDisplayTypes type,
        SkScriptValue* scriptValue) {
    if (scriptValue == NULL)
        return;
    SkASSERT(target == this);
    SkScriptValue* array = parameters.begin();
    SkScriptValue* end = parameters.end();
    SkScalar input = parameters[0].fOperand.fScalar;
    SkScalar scalarResult;
    switch (index) {
        case SK_FUNCTION(abs):
            scalarResult = SkScalarAbs(input);
            break;
        case SK_FUNCTION(acos):
            scalarResult = SkScalarACos(input);
            break;
        case SK_FUNCTION(asin):
            scalarResult = SkScalarASin(input);
            break;
        case SK_FUNCTION(atan):
            scalarResult = SkScalarATan2(input, SK_Scalar1);
            break;
        case SK_FUNCTION(atan2):
            scalarResult = SkScalarATan2(input, parameters[1].fOperand.fScalar);
            break;
        case SK_FUNCTION(ceil):
            scalarResult = SkScalarCeilToScalar(input);
            break;
        case SK_FUNCTION(cos):
            scalarResult = SkScalarCos(input);
            break;
        case SK_FUNCTION(exp):
            scalarResult = SkScalarExp(input);
            break;
        case SK_FUNCTION(floor):
            scalarResult = SkScalarFloorToScalar(input);
            break;
        case SK_FUNCTION(log):
            scalarResult = SkScalarLog(input);
            break;
        case SK_FUNCTION(max):
            scalarResult = -SK_ScalarMax;
            while (array < end) {
                scalarResult = SkMaxScalar(scalarResult, array->fOperand.fScalar);
                array++;
            }
            break;
        case SK_FUNCTION(min):
            scalarResult = SK_ScalarMax;
            while (array < end) {
                scalarResult = SkMinScalar(scalarResult, array->fOperand.fScalar);
                array++;
            }
            break;
        case SK_FUNCTION(pow):
            
            scalarResult = SkScalarLog(input);
            scalarResult = SkScalarMul(parameters[1].fOperand.fScalar, scalarResult);
            scalarResult = SkScalarExp(scalarResult);
            break;
        case SK_FUNCTION(random):
            scalarResult = fRandom.nextUScalar1();
            break;
        case SK_FUNCTION(round):
            scalarResult = SkScalarRoundToScalar(input);
            break;
        case SK_FUNCTION(sin):
            scalarResult = SkScalarSin(input);
            break;
        case SK_FUNCTION(sqrt): {
            SkASSERT(parameters.count() == 1);
            SkASSERT(type == SkType_Float);
            scalarResult = SkScalarSqrt(input);
            } break;
        case SK_FUNCTION(tan):
            scalarResult = SkScalarTan(input);
            break;
        default:
            SkASSERT(0);
            scalarResult = SK_ScalarNaN;
    }
    scriptValue->fOperand.fScalar = scalarResult;
    scriptValue->fType = SkType_Float;
}

const SkFunctionParamType* SkDisplayMath::getFunctionsParameters() {
    return fFunctionParameters;
}

bool SkDisplayMath::getProperty(int index, SkScriptValue* value) const {
    if ((unsigned)index < SK_ARRAY_COUNT(gConstants)) {
        value->fOperand.fScalar = gConstants[index];
        value->fType = SkType_Float;
        return true;
    }
    SkASSERT(0);
    return false;
}
