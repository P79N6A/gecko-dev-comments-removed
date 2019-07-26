











#ifndef RBBIRPT_H
#define RBBIRPT_H

U_NAMESPACE_BEGIN



    static const uint8_t kRuleSet_digit_char = 128;
    static const uint8_t kRuleSet_name_char = 129;
    static const uint8_t kRuleSet_name_start_char = 130;
    static const uint8_t kRuleSet_rule_char = 131;
    static const uint8_t kRuleSet_white_space = 132;


enum RBBI_RuleParseAction {
    doCheckVarDef,
    doDotAny,
    doEndAssign,
    doEndOfRule,
    doEndVariableName,
    doExit,
    doExprCatOperator,
    doExprFinished,
    doExprOrOperator,
    doExprRParen,
    doExprStart,
    doLParen,
    doNOP,
    doOptionEnd,
    doOptionStart,
    doReverseDir,
    doRuleChar,
    doRuleError,
    doRuleErrorAssignExpr,
    doScanUnicodeSet,
    doSlash,
    doStartAssign,
    doStartTagValue,
    doStartVariableName,
    doTagDigit,
    doTagExpectedError,
    doTagValue,
    doUnaryOpPlus,
    doUnaryOpQuestion,
    doUnaryOpStar,
    doVariableNameExpectedErr,
    rbbiLastAction};






struct RBBIRuleTableEl {
    RBBI_RuleParseAction          fAction;
    uint8_t                       fCharClass;       
                                                    
    uint8_t                       fNextState;       
                                                    
    uint8_t                       fPushState;
    UBool                         fNextChar;
};

static const struct RBBIRuleTableEl gRuleParseStateTable[] = {
    {doNOP, 0, 0, 0, TRUE}
    , {doExprStart, 254, 21, 8, FALSE}     
    , {doNOP, 132, 1,0,  TRUE}     
    , {doExprStart, 36 , 80, 90, FALSE}     
    , {doNOP, 33 , 11,0,  TRUE}     
    , {doNOP, 59 , 1,0,  TRUE}     
    , {doNOP, 252, 0,0,  FALSE}     
    , {doExprStart, 255, 21, 8, FALSE}     
    , {doEndOfRule, 59 , 1,0,  TRUE}     
    , {doNOP, 132, 8,0,  TRUE}     
    , {doRuleError, 255, 95,0,  FALSE}     
    , {doNOP, 33 , 13,0,  TRUE}     
    , {doReverseDir, 255, 20, 8, FALSE}     
    , {doOptionStart, 130, 15,0,  TRUE}     
    , {doRuleError, 255, 95,0,  FALSE}     
    , {doNOP, 129, 15,0,  TRUE}     
    , {doOptionEnd, 255, 17,0,  FALSE}     
    , {doNOP, 59 , 1,0,  TRUE}     
    , {doNOP, 132, 17,0,  TRUE}     
    , {doRuleError, 255, 95,0,  FALSE}     
    , {doExprStart, 255, 21, 8, FALSE}     
    , {doRuleChar, 254, 30,0,  TRUE}     
    , {doNOP, 132, 21,0,  TRUE}     
    , {doRuleChar, 131, 30,0,  TRUE}     
    , {doNOP, 91 , 86, 30, FALSE}     
    , {doLParen, 40 , 21, 30, TRUE}     
    , {doNOP, 36 , 80, 29, FALSE}     
    , {doDotAny, 46 , 30,0,  TRUE}     
    , {doRuleError, 255, 95,0,  FALSE}     
    , {doCheckVarDef, 255, 30,0,  FALSE}     
    , {doNOP, 132, 30,0,  TRUE}     
    , {doUnaryOpStar, 42 , 35,0,  TRUE}     
    , {doUnaryOpPlus, 43 , 35,0,  TRUE}     
    , {doUnaryOpQuestion, 63 , 35,0,  TRUE}     
    , {doNOP, 255, 35,0,  FALSE}     
    , {doExprCatOperator, 254, 21,0,  FALSE}     
    , {doNOP, 132, 35,0,  TRUE}     
    , {doExprCatOperator, 131, 21,0,  FALSE}     
    , {doExprCatOperator, 91 , 21,0,  FALSE}     
    , {doExprCatOperator, 40 , 21,0,  FALSE}     
    , {doExprCatOperator, 36 , 21,0,  FALSE}     
    , {doExprCatOperator, 46 , 21,0,  FALSE}     
    , {doExprCatOperator, 47 , 47,0,  FALSE}     
    , {doExprCatOperator, 123 , 59,0,  TRUE}     
    , {doExprOrOperator, 124 , 21,0,  TRUE}     
    , {doExprRParen, 41 , 255,0,  TRUE}     
    , {doExprFinished, 255, 255,0,  FALSE}     
    , {doSlash, 47 , 49,0,  TRUE}     
    , {doNOP, 255, 95,0,  FALSE}     
    , {doExprCatOperator, 254, 21,0,  FALSE}     
    , {doNOP, 132, 35,0,  TRUE}     
    , {doExprCatOperator, 131, 21,0,  FALSE}     
    , {doExprCatOperator, 91 , 21,0,  FALSE}     
    , {doExprCatOperator, 40 , 21,0,  FALSE}     
    , {doExprCatOperator, 36 , 21,0,  FALSE}     
    , {doExprCatOperator, 46 , 21,0,  FALSE}     
    , {doExprOrOperator, 124 , 21,0,  TRUE}     
    , {doExprRParen, 41 , 255,0,  TRUE}     
    , {doExprFinished, 255, 255,0,  FALSE}     
    , {doNOP, 132, 59,0,  TRUE}     
    , {doStartTagValue, 128, 62,0,  FALSE}     
    , {doTagExpectedError, 255, 95,0,  FALSE}     
    , {doNOP, 132, 66,0,  TRUE}     
    , {doNOP, 125 , 66,0,  FALSE}     
    , {doTagDigit, 128, 62,0,  TRUE}     
    , {doTagExpectedError, 255, 95,0,  FALSE}     
    , {doNOP, 132, 66,0,  TRUE}     
    , {doTagValue, 125 , 69,0,  TRUE}     
    , {doTagExpectedError, 255, 95,0,  FALSE}     
    , {doExprCatOperator, 254, 21,0,  FALSE}     
    , {doNOP, 132, 69,0,  TRUE}     
    , {doExprCatOperator, 131, 21,0,  FALSE}     
    , {doExprCatOperator, 91 , 21,0,  FALSE}     
    , {doExprCatOperator, 40 , 21,0,  FALSE}     
    , {doExprCatOperator, 36 , 21,0,  FALSE}     
    , {doExprCatOperator, 46 , 21,0,  FALSE}     
    , {doExprCatOperator, 47 , 47,0,  FALSE}     
    , {doExprOrOperator, 124 , 21,0,  TRUE}     
    , {doExprRParen, 41 , 255,0,  TRUE}     
    , {doExprFinished, 255, 255,0,  FALSE}     
    , {doStartVariableName, 36 , 82,0,  TRUE}     
    , {doNOP, 255, 95,0,  FALSE}     
    , {doNOP, 130, 84,0,  TRUE}     
    , {doVariableNameExpectedErr, 255, 95,0,  FALSE}     
    , {doNOP, 129, 84,0,  TRUE}     
    , {doEndVariableName, 255, 255,0,  FALSE}     
    , {doScanUnicodeSet, 91 , 255,0,  TRUE}     
    , {doScanUnicodeSet, 112 , 255,0,  TRUE}     
    , {doScanUnicodeSet, 80 , 255,0,  TRUE}     
    , {doNOP, 255, 95,0,  FALSE}     
    , {doNOP, 132, 90,0,  TRUE}     
    , {doStartAssign, 61 , 21, 93, TRUE}     
    , {doNOP, 255, 29, 8, FALSE}     
    , {doEndAssign, 59 , 1,0,  TRUE}     
    , {doRuleErrorAssignExpr, 255, 95,0,  FALSE}     
    , {doExit, 255, 95,0,  TRUE}     
 };
#ifdef RBBI_DEBUG
static const char * const RBBIRuleStateNames[] = {    0,
     "start",
    0,
    0,
    0,
    0,
    0,
    0,
     "break-rule-end",
    0,
    0,
     "rev-option",
    0,
     "option-scan1",
    0,
     "option-scan2",
    0,
     "option-scan3",
    0,
    0,
     "reverse-rule",
     "term",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "term-var-ref",
     "expr-mod",
    0,
    0,
    0,
    0,
     "expr-cont",
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
     "look-ahead",
    0,
     "expr-cont-no-slash",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "tag-open",
    0,
    0,
     "tag-value",
    0,
    0,
    0,
     "tag-close",
    0,
    0,
     "expr-cont-no-tag",
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
     "scan-var-name",
    0,
     "scan-var-start",
    0,
     "scan-var-body",
    0,
     "scan-unicode-set",
    0,
    0,
    0,
     "assign-or-rule",
    0,
    0,
     "assign-end",
    0,
     "errorDeath",
    0};
#endif

U_NAMESPACE_END
#endif
