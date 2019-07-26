










#ifndef RBBIRPT_H
#define RBBIRPT_H

U_NAMESPACE_BEGIN



    static const uint8_t kRuleSet_digit_char = 128;
    static const uint8_t kRuleSet_rule_char = 129;


enum Regex_PatternParseAction {
    doLiteralChar,
    doSetEnd,
    doBackslashA,
    doSetBeginUnion,
    doNOP,
    doSetBackslash_w,
    doSetRange,
    doBackslashG,
    doPerlInline,
    doSetAddDash,
    doIntevalLowerDigit,
    doProperty,
    doBackslashX,
    doOpenAtomicParen,
    doSetLiteralEscaped,
    doPatFinish,
    doSetBackslash_D,
    doSetDifference2,
    doNamedChar,
    doNGPlus,
    doOpenLookBehindNeg,
    doIntervalError,
    doIntervalSame,
    doBackRef,
    doPlus,
    doOpenCaptureParen,
    doMismatchedParenErr,
    doBeginMatchMode,
    doEscapeError,
    doOpenNonCaptureParen,
    doDollar,
    doSetProp,
    doIntervalUpperDigit,
    doSetBegin,
    doBackslashs,
    doOpenLookBehind,
    doSetMatchMode,
    doOrOperator,
    doCaret,
    doMatchModeParen,
    doStar,
    doOpt,
    doMatchMode,
    doSuppressComments,
    doPossessiveInterval,
    doOpenLookAheadNeg,
    doBackslashW,
    doCloseParen,
    doSetOpError,
    doIntervalInit,
    doSetFinish,
    doSetIntersection2,
    doNGStar,
    doEnterQuoteMode,
    doSetAddAmp,
    doBackslashB,
    doBackslashw,
    doPossessiveOpt,
    doSetNegate,
    doRuleError,
    doBackslashb,
    doConditionalExpr,
    doPossessivePlus,
    doBadOpenParenType,
    doNGInterval,
    doSetLiteral,
    doSetNamedChar,
    doBackslashd,
    doSetBeginDifference1,
    doBackslashD,
    doExit,
    doSetBackslash_S,
    doInterval,
    doSetNoCloseError,
    doNGOpt,
    doSetPosixProp,
    doBackslashS,
    doBackslashZ,
    doSetBeginIntersection1,
    doSetBackslash_W,
    doSetBackslash_d,
    doOpenLookAhead,
    doBadModeFlag,
    doPatStart,
    doSetNamedRange,
    doPossessiveStar,
    doEscapedLiteralChar,
    doSetBackslash_s,
    doBackslashz,
    doDotAny,
    rbbiLastAction};






struct RegexTableEl {
    Regex_PatternParseAction      fAction;
    uint8_t                       fCharClass;       
                                                    
    uint8_t                       fNextState;       
                                                    
    uint8_t                       fPushState;
    UBool                         fNextChar;
};

static const struct RegexTableEl gRuleParseStateTable[] = {
    {doNOP, 0, 0, 0, TRUE}
    , {doPatStart, 255, 2,0,  FALSE}     
    , {doLiteralChar, 254, 14,0,  TRUE}     
    , {doLiteralChar, 129, 14,0,  TRUE}     
    , {doSetBegin, 91 , 104, 182, TRUE}     
    , {doNOP, 40 , 27,0,  TRUE}     
    , {doDotAny, 46 , 14,0,  TRUE}     
    , {doCaret, 94 , 14,0,  TRUE}     
    , {doDollar, 36 , 14,0,  TRUE}     
    , {doNOP, 92 , 84,0,  TRUE}     
    , {doOrOperator, 124 , 2,0,  TRUE}     
    , {doCloseParen, 41 , 255,0,  TRUE}     
    , {doPatFinish, 253, 2,0,  FALSE}     
    , {doRuleError, 255, 183,0,  FALSE}     
    , {doNOP, 42 , 63,0,  TRUE}     
    , {doNOP, 43 , 66,0,  TRUE}     
    , {doNOP, 63 , 69,0,  TRUE}     
    , {doIntervalInit, 123 , 72,0,  TRUE}     
    , {doNOP, 40 , 23,0,  TRUE}     
    , {doNOP, 255, 20,0,  FALSE}     
    , {doOrOperator, 124 , 2,0,  TRUE}     
    , {doCloseParen, 41 , 255,0,  TRUE}     
    , {doNOP, 255, 2,0,  FALSE}     
    , {doSuppressComments, 63 , 25,0,  TRUE}     
    , {doNOP, 255, 27,0,  FALSE}     
    , {doNOP, 35 , 49, 14, TRUE}     
    , {doNOP, 255, 29,0,  FALSE}     
    , {doSuppressComments, 63 , 29,0,  TRUE}     
    , {doOpenCaptureParen, 255, 2, 14, FALSE}     
    , {doOpenNonCaptureParen, 58 , 2, 14, TRUE}     
    , {doOpenAtomicParen, 62 , 2, 14, TRUE}     
    , {doOpenLookAhead, 61 , 2, 20, TRUE}     
    , {doOpenLookAheadNeg, 33 , 2, 20, TRUE}     
    , {doNOP, 60 , 46,0,  TRUE}     
    , {doNOP, 35 , 49, 2, TRUE}     
    , {doBeginMatchMode, 105 , 52,0,  FALSE}     
    , {doBeginMatchMode, 100 , 52,0,  FALSE}     
    , {doBeginMatchMode, 109 , 52,0,  FALSE}     
    , {doBeginMatchMode, 115 , 52,0,  FALSE}     
    , {doBeginMatchMode, 117 , 52,0,  FALSE}     
    , {doBeginMatchMode, 119 , 52,0,  FALSE}     
    , {doBeginMatchMode, 120 , 52,0,  FALSE}     
    , {doBeginMatchMode, 45 , 52,0,  FALSE}     
    , {doConditionalExpr, 40 , 183,0,  TRUE}     
    , {doPerlInline, 123 , 183,0,  TRUE}     
    , {doBadOpenParenType, 255, 183,0,  FALSE}     
    , {doOpenLookBehind, 61 , 2, 20, TRUE}     
    , {doOpenLookBehindNeg, 33 , 2, 20, TRUE}     
    , {doBadOpenParenType, 255, 183,0,  FALSE}     
    , {doNOP, 41 , 255,0,  TRUE}     
    , {doMismatchedParenErr, 253, 183,0,  FALSE}     
    , {doNOP, 255, 49,0,  TRUE}     
    , {doMatchMode, 105 , 52,0,  TRUE}     
    , {doMatchMode, 100 , 52,0,  TRUE}     
    , {doMatchMode, 109 , 52,0,  TRUE}     
    , {doMatchMode, 115 , 52,0,  TRUE}     
    , {doMatchMode, 117 , 52,0,  TRUE}     
    , {doMatchMode, 119 , 52,0,  TRUE}     
    , {doMatchMode, 120 , 52,0,  TRUE}     
    , {doMatchMode, 45 , 52,0,  TRUE}     
    , {doSetMatchMode, 41 , 2,0,  TRUE}     
    , {doMatchModeParen, 58 , 2, 14, TRUE}     
    , {doBadModeFlag, 255, 183,0,  FALSE}     
    , {doNGStar, 63 , 20,0,  TRUE}     
    , {doPossessiveStar, 43 , 20,0,  TRUE}     
    , {doStar, 255, 20,0,  FALSE}     
    , {doNGPlus, 63 , 20,0,  TRUE}     
    , {doPossessivePlus, 43 , 20,0,  TRUE}     
    , {doPlus, 255, 20,0,  FALSE}     
    , {doNGOpt, 63 , 20,0,  TRUE}     
    , {doPossessiveOpt, 43 , 20,0,  TRUE}     
    , {doOpt, 255, 20,0,  FALSE}     
    , {doNOP, 128, 74,0,  FALSE}     
    , {doIntervalError, 255, 183,0,  FALSE}     
    , {doIntevalLowerDigit, 128, 74,0,  TRUE}     
    , {doNOP, 44 , 78,0,  TRUE}     
    , {doIntervalSame, 125 , 81,0,  TRUE}     
    , {doIntervalError, 255, 183,0,  FALSE}     
    , {doIntervalUpperDigit, 128, 78,0,  TRUE}     
    , {doNOP, 125 , 81,0,  TRUE}     
    , {doIntervalError, 255, 183,0,  FALSE}     
    , {doNGInterval, 63 , 20,0,  TRUE}     
    , {doPossessiveInterval, 43 , 20,0,  TRUE}     
    , {doInterval, 255, 20,0,  FALSE}     
    , {doBackslashA, 65 , 2,0,  TRUE}     
    , {doBackslashB, 66 , 2,0,  TRUE}     
    , {doBackslashb, 98 , 2,0,  TRUE}     
    , {doBackslashd, 100 , 14,0,  TRUE}     
    , {doBackslashD, 68 , 14,0,  TRUE}     
    , {doBackslashG, 71 , 2,0,  TRUE}     
    , {doNamedChar, 78 , 14,0,  FALSE}     
    , {doProperty, 112 , 14,0,  FALSE}     
    , {doProperty, 80 , 14,0,  FALSE}     
    , {doEnterQuoteMode, 81 , 2,0,  TRUE}     
    , {doBackslashS, 83 , 14,0,  TRUE}     
    , {doBackslashs, 115 , 14,0,  TRUE}     
    , {doBackslashW, 87 , 14,0,  TRUE}     
    , {doBackslashw, 119 , 14,0,  TRUE}     
    , {doBackslashX, 88 , 14,0,  TRUE}     
    , {doBackslashZ, 90 , 2,0,  TRUE}     
    , {doBackslashz, 122 , 2,0,  TRUE}     
    , {doBackRef, 128, 14,0,  TRUE}     
    , {doEscapeError, 253, 183,0,  FALSE}     
    , {doEscapedLiteralChar, 255, 14,0,  TRUE}     
    , {doSetNegate, 94 , 107,0,  TRUE}     
    , {doSetPosixProp, 58 , 109,0,  FALSE}     
    , {doNOP, 255, 107,0,  FALSE}     
    , {doSetLiteral, 93 , 122,0,  TRUE}     
    , {doNOP, 255, 112,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doNOP, 58 , 112,0,  FALSE}     
    , {doRuleError, 255, 183,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 104, 129, TRUE}     
    , {doNOP, 92 , 172,0,  TRUE}     
    , {doNOP, 45 , 118,0,  TRUE}     
    , {doNOP, 38 , 120,0,  TRUE}     
    , {doSetLiteral, 255, 122,0,  TRUE}     
    , {doRuleError, 45 , 183,0,  FALSE}     
    , {doSetAddDash, 255, 122,0,  FALSE}     
    , {doRuleError, 38 , 183,0,  FALSE}     
    , {doSetAddAmp, 255, 122,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 104, 129, TRUE}     
    , {doNOP, 45 , 159,0,  TRUE}     
    , {doNOP, 38 , 150,0,  TRUE}     
    , {doNOP, 92 , 172,0,  TRUE}     
    , {doSetNoCloseError, 253, 183,0,  FALSE}     
    , {doSetLiteral, 255, 122,0,  TRUE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 104, 129, TRUE}     
    , {doNOP, 45 , 152,0,  TRUE}     
    , {doNOP, 38 , 147,0,  TRUE}     
    , {doNOP, 92 , 172,0,  TRUE}     
    , {doSetNoCloseError, 253, 183,0,  FALSE}     
    , {doSetLiteral, 255, 122,0,  TRUE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 104, 129, TRUE}     
    , {doNOP, 45 , 155,0,  TRUE}     
    , {doNOP, 38 , 157,0,  TRUE}     
    , {doNOP, 92 , 172,0,  TRUE}     
    , {doSetNoCloseError, 253, 183,0,  FALSE}     
    , {doSetLiteral, 255, 122,0,  TRUE}     
    , {doSetBeginUnion, 91 , 104, 129, TRUE}     
    , {doSetOpError, 93 , 183,0,  FALSE}     
    , {doNOP, 92 , 172,0,  TRUE}     
    , {doSetLiteral, 255, 122,0,  TRUE}     
    , {doSetBeginIntersection1, 91 , 104, 129, TRUE}     
    , {doSetIntersection2, 38 , 143,0,  TRUE}     
    , {doSetAddAmp, 255, 122,0,  FALSE}     
    , {doSetIntersection2, 38 , 143,0,  TRUE}     
    , {doSetAddAmp, 255, 122,0,  FALSE}     
    , {doSetBeginDifference1, 91 , 104, 129, TRUE}     
    , {doSetDifference2, 45 , 143,0,  TRUE}     
    , {doSetAddDash, 255, 122,0,  FALSE}     
    , {doSetDifference2, 45 , 143,0,  TRUE}     
    , {doSetAddDash, 255, 122,0,  FALSE}     
    , {doSetIntersection2, 38 , 143,0,  TRUE}     
    , {doSetAddAmp, 255, 122,0,  FALSE}     
    , {doSetDifference2, 45 , 143,0,  TRUE}     
    , {doSetAddDash, 91 , 122,0,  FALSE}     
    , {doSetAddDash, 93 , 122,0,  FALSE}     
    , {doNOP, 92 , 164,0,  TRUE}     
    , {doSetRange, 255, 136,0,  TRUE}     
    , {doSetOpError, 115 , 183,0,  FALSE}     
    , {doSetOpError, 83 , 183,0,  FALSE}     
    , {doSetOpError, 119 , 183,0,  FALSE}     
    , {doSetOpError, 87 , 183,0,  FALSE}     
    , {doSetOpError, 100 , 183,0,  FALSE}     
    , {doSetOpError, 68 , 183,0,  FALSE}     
    , {doSetNamedRange, 78 , 136,0,  FALSE}     
    , {doSetRange, 255, 136,0,  TRUE}     
    , {doSetProp, 112 , 129,0,  FALSE}     
    , {doSetProp, 80 , 129,0,  FALSE}     
    , {doSetNamedChar, 78 , 122,0,  FALSE}     
    , {doSetBackslash_s, 115 , 136,0,  TRUE}     
    , {doSetBackslash_S, 83 , 136,0,  TRUE}     
    , {doSetBackslash_w, 119 , 136,0,  TRUE}     
    , {doSetBackslash_W, 87 , 136,0,  TRUE}     
    , {doSetBackslash_d, 100 , 136,0,  TRUE}     
    , {doSetBackslash_D, 68 , 136,0,  TRUE}     
    , {doSetLiteralEscaped, 255, 122,0,  TRUE}     
    , {doSetFinish, 255, 14,0,  FALSE}     
    , {doExit, 255, 183,0,  TRUE}     
 };
static const char * const RegexStateNames[] = {    0,
     "start",
     "term",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "expr-quant",
    0,
    0,
    0,
    0,
    0,
     "expr-cont",
    0,
    0,
     "open-paren-quant",
    0,
     "open-paren-quant2",
    0,
     "open-paren",
    0,
     "open-paren-extended",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "open-paren-lookbehind",
    0,
    0,
     "paren-comment",
    0,
    0,
     "paren-flag",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "quant-star",
    0,
    0,
     "quant-plus",
    0,
    0,
     "quant-opt",
    0,
    0,
     "interval-open",
    0,
     "interval-lower",
    0,
    0,
    0,
     "interval-upper",
    0,
    0,
     "interval-type",
    0,
    0,
     "backslash",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "set-open",
    0,
    0,
     "set-open2",
    0,
     "set-posix",
    0,
    0,
     "set-start",
    0,
    0,
    0,
    0,
    0,
     "set-start-dash",
    0,
     "set-start-amp",
    0,
     "set-after-lit",
    0,
    0,
    0,
    0,
    0,
    0,
     "set-after-set",
    0,
    0,
    0,
    0,
    0,
    0,
     "set-after-range",
    0,
    0,
    0,
    0,
    0,
    0,
     "set-after-op",
    0,
    0,
    0,
     "set-set-amp",
    0,
    0,
     "set-lit-amp",
    0,
     "set-set-dash",
    0,
    0,
     "set-range-dash",
    0,
     "set-range-amp",
    0,
     "set-lit-dash",
    0,
    0,
    0,
    0,
     "set-lit-dash-escape",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "set-escape",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "set-finish",
     "errorDeath",
    0};

U_NAMESPACE_END
#endif
