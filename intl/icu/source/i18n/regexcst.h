










#ifndef RBBIRPT_H
#define RBBIRPT_H

U_NAMESPACE_BEGIN



    static const uint8_t kRuleSet_ascii_letter = 128;
    static const uint8_t kRuleSet_digit_char = 129;
    static const uint8_t kRuleSet_rule_char = 130;


enum Regex_PatternParseAction {
    doSetBackslash_V,
    doSetBackslash_h,
    doBeginNamedBackRef,
    doSetMatchMode,
    doEnterQuoteMode,
    doOpenCaptureParen,
    doContinueNamedCapture,
    doSetBackslash_d,
    doBeginMatchMode,
    doBackslashX,
    doSetPosixProp,
    doIntervalError,
    doSetLiteralEscaped,
    doSetBackslash_s,
    doNOP,
    doBackslashv,
    doOpenLookBehind,
    doPatStart,
    doPossessiveInterval,
    doOpenAtomicParen,
    doOpenLookAheadNeg,
    doBackslashd,
    doBackslashZ,
    doIntervalUpperDigit,
    doBadNamedCapture,
    doSetDifference2,
    doSetAddAmp,
    doSetNamedChar,
    doNamedChar,
    doSetBackslash_H,
    doBackslashb,
    doBackslashz,
    doSetBeginDifference1,
    doOpenLookAhead,
    doMatchModeParen,
    doBackslashV,
    doIntevalLowerDigit,
    doCaret,
    doSetEnd,
    doSetNegate,
    doBackslashS,
    doOrOperator,
    doBackslashB,
    doBackslashw,
    doBackslashR,
    doRuleError,
    doDotAny,
    doMatchMode,
    doSetBackslash_W,
    doNGPlus,
    doSetBackslash_D,
    doPossessiveOpt,
    doSetNamedRange,
    doConditionalExpr,
    doBackslashs,
    doPossessiveStar,
    doPlus,
    doBadOpenParenType,
    doCloseParen,
    doNGInterval,
    doSetProp,
    doBackRef,
    doSetBeginUnion,
    doEscapeError,
    doOpt,
    doSetBeginIntersection1,
    doPossessivePlus,
    doBackslashD,
    doOpenLookBehindNeg,
    doSetBegin,
    doSetIntersection2,
    doCompleteNamedBackRef,
    doSetRange,
    doDollar,
    doBackslashH,
    doExit,
    doNGOpt,
    doOpenNonCaptureParen,
    doBackslashA,
    doSetBackslash_v,
    doBackslashh,
    doBadModeFlag,
    doSetNoCloseError,
    doIntervalSame,
    doSetAddDash,
    doBackslashW,
    doPerlInline,
    doSetOpError,
    doSetLiteral,
    doPatFinish,
    doBeginNamedCapture,
    doEscapedLiteralChar,
    doLiteralChar,
    doSuppressComments,
    doMismatchedParenErr,
    doNGStar,
    doSetFinish,
    doInterval,
    doBackslashG,
    doStar,
    doSetBackslash_w,
    doSetBackslash_S,
    doProperty,
    doContinueNamedBackRef,
    doIntervalInit,
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
    , {doLiteralChar, 130, 14,0,  TRUE}     
    , {doSetBegin, 91 , 123, 205, TRUE}     
    , {doNOP, 40 , 27,0,  TRUE}     
    , {doDotAny, 46 , 14,0,  TRUE}     
    , {doCaret, 94 , 14,0,  TRUE}     
    , {doDollar, 36 , 14,0,  TRUE}     
    , {doNOP, 92 , 89,0,  TRUE}     
    , {doOrOperator, 124 , 2,0,  TRUE}     
    , {doCloseParen, 41 , 255,0,  TRUE}     
    , {doPatFinish, 253, 2,0,  FALSE}     
    , {doRuleError, 255, 206,0,  FALSE}     
    , {doNOP, 42 , 68,0,  TRUE}     
    , {doNOP, 43 , 71,0,  TRUE}     
    , {doNOP, 63 , 74,0,  TRUE}     
    , {doIntervalInit, 123 , 77,0,  TRUE}     
    , {doNOP, 40 , 23,0,  TRUE}     
    , {doNOP, 255, 20,0,  FALSE}     
    , {doOrOperator, 124 , 2,0,  TRUE}     
    , {doCloseParen, 41 , 255,0,  TRUE}     
    , {doNOP, 255, 2,0,  FALSE}     
    , {doSuppressComments, 63 , 25,0,  TRUE}     
    , {doNOP, 255, 27,0,  FALSE}     
    , {doNOP, 35 , 50, 14, TRUE}     
    , {doNOP, 255, 29,0,  FALSE}     
    , {doSuppressComments, 63 , 29,0,  TRUE}     
    , {doOpenCaptureParen, 255, 2, 14, FALSE}     
    , {doOpenNonCaptureParen, 58 , 2, 14, TRUE}     
    , {doOpenAtomicParen, 62 , 2, 14, TRUE}     
    , {doOpenLookAhead, 61 , 2, 20, TRUE}     
    , {doOpenLookAheadNeg, 33 , 2, 20, TRUE}     
    , {doNOP, 60 , 46,0,  TRUE}     
    , {doNOP, 35 , 50, 2, TRUE}     
    , {doBeginMatchMode, 105 , 53,0,  FALSE}     
    , {doBeginMatchMode, 100 , 53,0,  FALSE}     
    , {doBeginMatchMode, 109 , 53,0,  FALSE}     
    , {doBeginMatchMode, 115 , 53,0,  FALSE}     
    , {doBeginMatchMode, 117 , 53,0,  FALSE}     
    , {doBeginMatchMode, 119 , 53,0,  FALSE}     
    , {doBeginMatchMode, 120 , 53,0,  FALSE}     
    , {doBeginMatchMode, 45 , 53,0,  FALSE}     
    , {doConditionalExpr, 40 , 206,0,  TRUE}     
    , {doPerlInline, 123 , 206,0,  TRUE}     
    , {doBadOpenParenType, 255, 206,0,  FALSE}     
    , {doOpenLookBehind, 61 , 2, 20, TRUE}     
    , {doOpenLookBehindNeg, 33 , 2, 20, TRUE}     
    , {doBeginNamedCapture, 128, 64,0,  FALSE}     
    , {doBadOpenParenType, 255, 206,0,  FALSE}     
    , {doNOP, 41 , 255,0,  TRUE}     
    , {doMismatchedParenErr, 253, 206,0,  FALSE}     
    , {doNOP, 255, 50,0,  TRUE}     
    , {doMatchMode, 105 , 53,0,  TRUE}     
    , {doMatchMode, 100 , 53,0,  TRUE}     
    , {doMatchMode, 109 , 53,0,  TRUE}     
    , {doMatchMode, 115 , 53,0,  TRUE}     
    , {doMatchMode, 117 , 53,0,  TRUE}     
    , {doMatchMode, 119 , 53,0,  TRUE}     
    , {doMatchMode, 120 , 53,0,  TRUE}     
    , {doMatchMode, 45 , 53,0,  TRUE}     
    , {doSetMatchMode, 41 , 2,0,  TRUE}     
    , {doMatchModeParen, 58 , 2, 14, TRUE}     
    , {doBadModeFlag, 255, 206,0,  FALSE}     
    , {doContinueNamedCapture, 128, 64,0,  TRUE}     
    , {doContinueNamedCapture, 129, 64,0,  TRUE}     
    , {doOpenCaptureParen, 62 , 2, 14, TRUE}     
    , {doBadNamedCapture, 255, 206,0,  FALSE}     
    , {doNGStar, 63 , 20,0,  TRUE}     
    , {doPossessiveStar, 43 , 20,0,  TRUE}     
    , {doStar, 255, 20,0,  FALSE}     
    , {doNGPlus, 63 , 20,0,  TRUE}     
    , {doPossessivePlus, 43 , 20,0,  TRUE}     
    , {doPlus, 255, 20,0,  FALSE}     
    , {doNGOpt, 63 , 20,0,  TRUE}     
    , {doPossessiveOpt, 43 , 20,0,  TRUE}     
    , {doOpt, 255, 20,0,  FALSE}     
    , {doNOP, 129, 79,0,  FALSE}     
    , {doIntervalError, 255, 206,0,  FALSE}     
    , {doIntevalLowerDigit, 129, 79,0,  TRUE}     
    , {doNOP, 44 , 83,0,  TRUE}     
    , {doIntervalSame, 125 , 86,0,  TRUE}     
    , {doIntervalError, 255, 206,0,  FALSE}     
    , {doIntervalUpperDigit, 129, 83,0,  TRUE}     
    , {doNOP, 125 , 86,0,  TRUE}     
    , {doIntervalError, 255, 206,0,  FALSE}     
    , {doNGInterval, 63 , 20,0,  TRUE}     
    , {doPossessiveInterval, 43 , 20,0,  TRUE}     
    , {doInterval, 255, 20,0,  FALSE}     
    , {doBackslashA, 65 , 2,0,  TRUE}     
    , {doBackslashB, 66 , 2,0,  TRUE}     
    , {doBackslashb, 98 , 2,0,  TRUE}     
    , {doBackslashd, 100 , 14,0,  TRUE}     
    , {doBackslashD, 68 , 14,0,  TRUE}     
    , {doBackslashG, 71 , 2,0,  TRUE}     
    , {doBackslashh, 104 , 14,0,  TRUE}     
    , {doBackslashH, 72 , 14,0,  TRUE}     
    , {doNOP, 107 , 115,0,  TRUE}     
    , {doNamedChar, 78 , 14,0,  FALSE}     
    , {doProperty, 112 , 14,0,  FALSE}     
    , {doProperty, 80 , 14,0,  FALSE}     
    , {doBackslashR, 82 , 14,0,  TRUE}     
    , {doEnterQuoteMode, 81 , 2,0,  TRUE}     
    , {doBackslashS, 83 , 14,0,  TRUE}     
    , {doBackslashs, 115 , 14,0,  TRUE}     
    , {doBackslashv, 118 , 14,0,  TRUE}     
    , {doBackslashV, 86 , 14,0,  TRUE}     
    , {doBackslashW, 87 , 14,0,  TRUE}     
    , {doBackslashw, 119 , 14,0,  TRUE}     
    , {doBackslashX, 88 , 14,0,  TRUE}     
    , {doBackslashZ, 90 , 2,0,  TRUE}     
    , {doBackslashz, 122 , 2,0,  TRUE}     
    , {doBackRef, 129, 14,0,  TRUE}     
    , {doEscapeError, 253, 206,0,  FALSE}     
    , {doEscapedLiteralChar, 255, 14,0,  TRUE}     
    , {doBeginNamedBackRef, 60 , 117,0,  TRUE}     
    , {doBadNamedCapture, 255, 206,0,  FALSE}     
    , {doContinueNamedBackRef, 128, 119,0,  TRUE}     
    , {doBadNamedCapture, 255, 206,0,  FALSE}     
    , {doContinueNamedBackRef, 128, 119,0,  TRUE}     
    , {doContinueNamedBackRef, 129, 119,0,  TRUE}     
    , {doCompleteNamedBackRef, 62 , 14,0,  TRUE}     
    , {doBadNamedCapture, 255, 206,0,  FALSE}     
    , {doSetNegate, 94 , 126,0,  TRUE}     
    , {doSetPosixProp, 58 , 128,0,  FALSE}     
    , {doNOP, 255, 126,0,  FALSE}     
    , {doSetLiteral, 93 , 141,0,  TRUE}     
    , {doNOP, 255, 131,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doNOP, 58 , 131,0,  FALSE}     
    , {doRuleError, 255, 206,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 123, 148, TRUE}     
    , {doNOP, 92 , 191,0,  TRUE}     
    , {doNOP, 45 , 137,0,  TRUE}     
    , {doNOP, 38 , 139,0,  TRUE}     
    , {doSetLiteral, 255, 141,0,  TRUE}     
    , {doRuleError, 45 , 206,0,  FALSE}     
    , {doSetAddDash, 255, 141,0,  FALSE}     
    , {doRuleError, 38 , 206,0,  FALSE}     
    , {doSetAddAmp, 255, 141,0,  FALSE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 123, 148, TRUE}     
    , {doNOP, 45 , 178,0,  TRUE}     
    , {doNOP, 38 , 169,0,  TRUE}     
    , {doNOP, 92 , 191,0,  TRUE}     
    , {doSetNoCloseError, 253, 206,0,  FALSE}     
    , {doSetLiteral, 255, 141,0,  TRUE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 123, 148, TRUE}     
    , {doNOP, 45 , 171,0,  TRUE}     
    , {doNOP, 38 , 166,0,  TRUE}     
    , {doNOP, 92 , 191,0,  TRUE}     
    , {doSetNoCloseError, 253, 206,0,  FALSE}     
    , {doSetLiteral, 255, 141,0,  TRUE}     
    , {doSetEnd, 93 , 255,0,  TRUE}     
    , {doSetBeginUnion, 91 , 123, 148, TRUE}     
    , {doNOP, 45 , 174,0,  TRUE}     
    , {doNOP, 38 , 176,0,  TRUE}     
    , {doNOP, 92 , 191,0,  TRUE}     
    , {doSetNoCloseError, 253, 206,0,  FALSE}     
    , {doSetLiteral, 255, 141,0,  TRUE}     
    , {doSetBeginUnion, 91 , 123, 148, TRUE}     
    , {doSetOpError, 93 , 206,0,  FALSE}     
    , {doNOP, 92 , 191,0,  TRUE}     
    , {doSetLiteral, 255, 141,0,  TRUE}     
    , {doSetBeginIntersection1, 91 , 123, 148, TRUE}     
    , {doSetIntersection2, 38 , 162,0,  TRUE}     
    , {doSetAddAmp, 255, 141,0,  FALSE}     
    , {doSetIntersection2, 38 , 162,0,  TRUE}     
    , {doSetAddAmp, 255, 141,0,  FALSE}     
    , {doSetBeginDifference1, 91 , 123, 148, TRUE}     
    , {doSetDifference2, 45 , 162,0,  TRUE}     
    , {doSetAddDash, 255, 141,0,  FALSE}     
    , {doSetDifference2, 45 , 162,0,  TRUE}     
    , {doSetAddDash, 255, 141,0,  FALSE}     
    , {doSetIntersection2, 38 , 162,0,  TRUE}     
    , {doSetAddAmp, 255, 141,0,  FALSE}     
    , {doSetDifference2, 45 , 162,0,  TRUE}     
    , {doSetAddDash, 91 , 141,0,  FALSE}     
    , {doSetAddDash, 93 , 141,0,  FALSE}     
    , {doNOP, 92 , 183,0,  TRUE}     
    , {doSetRange, 255, 155,0,  TRUE}     
    , {doSetOpError, 115 , 206,0,  FALSE}     
    , {doSetOpError, 83 , 206,0,  FALSE}     
    , {doSetOpError, 119 , 206,0,  FALSE}     
    , {doSetOpError, 87 , 206,0,  FALSE}     
    , {doSetOpError, 100 , 206,0,  FALSE}     
    , {doSetOpError, 68 , 206,0,  FALSE}     
    , {doSetNamedRange, 78 , 155,0,  FALSE}     
    , {doSetRange, 255, 155,0,  TRUE}     
    , {doSetProp, 112 , 148,0,  FALSE}     
    , {doSetProp, 80 , 148,0,  FALSE}     
    , {doSetNamedChar, 78 , 141,0,  FALSE}     
    , {doSetBackslash_s, 115 , 155,0,  TRUE}     
    , {doSetBackslash_S, 83 , 155,0,  TRUE}     
    , {doSetBackslash_w, 119 , 155,0,  TRUE}     
    , {doSetBackslash_W, 87 , 155,0,  TRUE}     
    , {doSetBackslash_d, 100 , 155,0,  TRUE}     
    , {doSetBackslash_D, 68 , 155,0,  TRUE}     
    , {doSetBackslash_h, 104 , 155,0,  TRUE}     
    , {doSetBackslash_H, 72 , 155,0,  TRUE}     
    , {doSetBackslash_v, 118 , 155,0,  TRUE}     
    , {doSetBackslash_V, 86 , 155,0,  TRUE}     
    , {doSetLiteralEscaped, 255, 141,0,  TRUE}     
    , {doSetFinish, 255, 14,0,  FALSE}     
    , {doExit, 255, 206,0,  TRUE}     
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
     "named-capture",
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
    0,
    0,
    0,
    0,
    0,
    0,
     "named-backref",
    0,
     "named-backref-2",
    0,
     "named-backref-3",
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
    0,
    0,
    0,
    0,
     "set-finish",
     "errorDeath",
    0};

U_NAMESPACE_END
#endif
