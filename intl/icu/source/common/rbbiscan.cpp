














#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/uchriter.h"
#include "unicode/parsepos.h"
#include "unicode/parseerr.h"
#include "cmemory.h"
#include "cstring.h"

#include "rbbirpt.h"   
                       
#include "rbbirb.h"
#include "rbbinode.h"
#include "rbbiscan.h"
#include "rbbitblb.h"

#include "uassert.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))











static const UChar gRuleSet_rule_char_pattern[]       = {
 
    0x5b, 0x5e, 0x5b, 0x5c, 0x70, 0x7b, 0x5a, 0x7d, 0x5c, 0x75, 0x30, 0x30, 0x32, 0x30,
 
    0x2d, 0x5c, 0x75, 0x30, 0x30, 0x37, 0x66, 0x5d, 0x2d, 0x5b, 0x5c, 0x70,
 
    0x7b, 0x4c, 0x7d, 0x5d, 0x2d, 0x5b, 0x5c, 0x70, 0x7b, 0x4e, 0x7d, 0x5d, 0x5d, 0};

static const UChar gRuleSet_name_char_pattern[]       = {

    0x5b, 0x5f, 0x5c, 0x70, 0x7b, 0x4c, 0x7d, 0x5c, 0x70, 0x7b, 0x4e, 0x7d, 0x5d, 0};

static const UChar gRuleSet_digit_char_pattern[] = {

    0x5b, 0x30, 0x2d, 0x39, 0x5d, 0};

static const UChar gRuleSet_name_start_char_pattern[] = {

    0x5b, 0x5f, 0x5c, 0x70, 0x7b, 0x4c, 0x7d, 0x5d, 0 };

static const UChar kAny[] = {0x61, 0x6e, 0x79, 0x00};  


U_CDECL_BEGIN
static void U_CALLCONV RBBISetTable_deleter(void *p) {
    icu::RBBISetTableEl *px = (icu::RBBISetTableEl *)p;
    delete px->key;
    
    
    uprv_free(px);
}
U_CDECL_END

U_NAMESPACE_BEGIN






RBBIRuleScanner::RBBIRuleScanner(RBBIRuleBuilder *rb)
{
    fRB                 = rb;
    fStackPtr           = 0;
    fStack[fStackPtr]   = 0;
    fNodeStackPtr       = 0;
    fRuleNum            = 0;
    fNodeStack[0]       = NULL;

    fSymbolTable                            = NULL;
    fSetTable                               = NULL;

    fScanIndex = 0;
    fNextIndex = 0;

    fReverseRule        = FALSE;
    fLookAheadRule      = FALSE;

    fLineNum    = 1;
    fCharNum    = 0;
    fQuoteMode  = FALSE;

    
    
    if (U_FAILURE(*rb->fStatus)) {
        return;
    }

    
    
    
    
    
    
    fRuleSets[kRuleSet_rule_char-128]
        = UnicodeSet(UnicodeString(gRuleSet_rule_char_pattern),       *rb->fStatus);
    
    fRuleSets[kRuleSet_white_space-128].
        add(9, 0xd).add(0x20).add(0x85).add(0x200e, 0x200f).add(0x2028, 0x2029);
    fRuleSets[kRuleSet_name_char-128]
        = UnicodeSet(UnicodeString(gRuleSet_name_char_pattern),       *rb->fStatus);
    fRuleSets[kRuleSet_name_start_char-128]
        = UnicodeSet(UnicodeString(gRuleSet_name_start_char_pattern), *rb->fStatus);
    fRuleSets[kRuleSet_digit_char-128]
        = UnicodeSet(UnicodeString(gRuleSet_digit_char_pattern),      *rb->fStatus);
    if (*rb->fStatus == U_ILLEGAL_ARGUMENT_ERROR) {
        
        
        
        *rb->fStatus = U_BRK_INIT_ERROR;
    }
    if (U_FAILURE(*rb->fStatus)) {
        return;
    }

    fSymbolTable = new RBBISymbolTable(this, rb->fRules, *rb->fStatus);
    if (fSymbolTable == NULL) {
        *rb->fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    fSetTable    = uhash_open(uhash_hashUnicodeString, uhash_compareUnicodeString, NULL, rb->fStatus);
    if (U_FAILURE(*rb->fStatus)) {
        return;
    }
    uhash_setValueDeleter(fSetTable, RBBISetTable_deleter);
}








RBBIRuleScanner::~RBBIRuleScanner() {
    delete fSymbolTable;
    if (fSetTable != NULL) {
         uhash_close(fSetTable);
         fSetTable = NULL;

    }


    
    
    
    while (fNodeStackPtr > 0) {
        delete fNodeStack[fNodeStackPtr];
        fNodeStackPtr--;
    }

}















UBool RBBIRuleScanner::doParseActions(int32_t action)
{
    RBBINode *n       = NULL;

    UBool   returnVal = TRUE;

    switch (action) {

    case doExprStart:
        pushNewNode(RBBINode::opStart);
        fRuleNum++;
        break;


    case doExprOrOperator:
        {
            fixOpStack(RBBINode::precOpCat);
            RBBINode  *operandNode = fNodeStack[fNodeStackPtr--];
            RBBINode  *orNode      = pushNewNode(RBBINode::opOr);
            orNode->fLeftChild     = operandNode;
            operandNode->fParent   = orNode;
        }
        break;

    case doExprCatOperator:
        
        
        
        
        {
            fixOpStack(RBBINode::precOpCat);
            RBBINode  *operandNode = fNodeStack[fNodeStackPtr--];
            RBBINode  *catNode     = pushNewNode(RBBINode::opCat);
            catNode->fLeftChild    = operandNode;
            operandNode->fParent   = catNode;
        }
        break;

    case doLParen:
        
        
        
        
        
        pushNewNode(RBBINode::opLParen);
        break;

    case doExprRParen:
        fixOpStack(RBBINode::precLParen);
        break;

    case doNOP:
        break;

    case doStartAssign:
        
        

        
        
        
        
        n = fNodeStack[fNodeStackPtr-1];
        n->fFirstPos = fNextIndex;              

        
        
        pushNewNode(RBBINode::opStart);
        break;




    case doEndAssign:
        {
            
            

            
            fixOpStack(RBBINode::precStart);

            RBBINode *startExprNode  = fNodeStack[fNodeStackPtr-2];
            RBBINode *varRefNode     = fNodeStack[fNodeStackPtr-1];
            RBBINode *RHSExprNode    = fNodeStack[fNodeStackPtr];

            
            
            RHSExprNode->fFirstPos = startExprNode->fFirstPos;
            RHSExprNode->fLastPos  = fScanIndex;
            fRB->fRules.extractBetween(RHSExprNode->fFirstPos, RHSExprNode->fLastPos, RHSExprNode->fText);

            
            varRefNode->fLeftChild = RHSExprNode;
            RHSExprNode->fParent   = varRefNode;

            
            fSymbolTable->addEntry(varRefNode->fText, varRefNode, *fRB->fStatus);
            if (U_FAILURE(*fRB->fStatus)) {
                
                
                UErrorCode t = *fRB->fStatus;
                *fRB->fStatus = U_ZERO_ERROR;
                error(t);
            }

            
            delete startExprNode;
            fNodeStackPtr-=3;
            break;
        }

    case doEndOfRule:
        {
        fixOpStack(RBBINode::precStart);      
        if (U_FAILURE(*fRB->fStatus)) {       
            break;
        }
#ifdef RBBI_DEBUG
        if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "rtree")) {printNodeStack("end of rule");}
#endif
        U_ASSERT(fNodeStackPtr == 1);

        
        
        if (fLookAheadRule) {
            RBBINode  *thisRule       = fNodeStack[fNodeStackPtr];
            RBBINode  *endNode        = pushNewNode(RBBINode::endMark);
            RBBINode  *catNode        = pushNewNode(RBBINode::opCat);
            fNodeStackPtr -= 2;
            catNode->fLeftChild       = thisRule;
            catNode->fRightChild      = endNode;
            fNodeStack[fNodeStackPtr] = catNode;
            endNode->fVal             = fRuleNum;
            endNode->fLookAheadEnd    = TRUE;
        }

        
        
        
        
        
        
        
        
        RBBINode **destRules = (fReverseRule? &fRB->fReverseTree : fRB->fDefaultTree);

        if (*destRules != NULL) {
            
            
            
            
            
            RBBINode  *thisRule    = fNodeStack[fNodeStackPtr];
            RBBINode  *prevRules   = *destRules;
            RBBINode  *orNode      = pushNewNode(RBBINode::opOr);
            orNode->fLeftChild     = prevRules;
            prevRules->fParent     = orNode;
            orNode->fRightChild    = thisRule;
            thisRule->fParent      = orNode;
            *destRules             = orNode;
        }
        else
        {
            
            
            *destRules = fNodeStack[fNodeStackPtr];
        }
        fReverseRule   = FALSE;   
        fLookAheadRule = FALSE;
        fNodeStackPtr  = 0;
        }
        break;


    case doRuleError:
        error(U_BRK_RULE_SYNTAX);
        returnVal = FALSE;
        break;


    case doVariableNameExpectedErr:
        error(U_BRK_RULE_SYNTAX);
        break;


    
    
    
    
    
    
    case doUnaryOpPlus:
        {
            RBBINode  *operandNode = fNodeStack[fNodeStackPtr--];
            RBBINode  *plusNode    = pushNewNode(RBBINode::opPlus);
            plusNode->fLeftChild   = operandNode;
            operandNode->fParent   = plusNode;
        }
        break;

    case doUnaryOpQuestion:
        {
            RBBINode  *operandNode = fNodeStack[fNodeStackPtr--];
            RBBINode  *qNode       = pushNewNode(RBBINode::opQuestion);
            qNode->fLeftChild      = operandNode;
            operandNode->fParent   = qNode;
        }
        break;

    case doUnaryOpStar:
        {
            RBBINode  *operandNode = fNodeStack[fNodeStackPtr--];
            RBBINode  *starNode    = pushNewNode(RBBINode::opStar);
            starNode->fLeftChild   = operandNode;
            operandNode->fParent   = starNode;
        }
        break;

    case doRuleChar:
        
        
        
        
        
        {
            n = pushNewNode(RBBINode::setRef);
            findSetFor(UnicodeString(fC.fChar), n);
            n->fFirstPos = fScanIndex;
            n->fLastPos  = fNextIndex;
            fRB->fRules.extractBetween(n->fFirstPos, n->fLastPos, n->fText);
            break;
        }

    case doDotAny:
        
        {
            n = pushNewNode(RBBINode::setRef);
            findSetFor(UnicodeString(TRUE, kAny, 3), n);
            n->fFirstPos = fScanIndex;
            n->fLastPos  = fNextIndex;
            fRB->fRules.extractBetween(n->fFirstPos, n->fLastPos, n->fText);
            break;
        }

    case doSlash:
        
        n = pushNewNode(RBBINode::lookAhead);
        n->fVal      = fRuleNum;
        n->fFirstPos = fScanIndex;
        n->fLastPos  = fNextIndex;
        fRB->fRules.extractBetween(n->fFirstPos, n->fLastPos, n->fText);
        fLookAheadRule = TRUE;
        break;


    case doStartTagValue:
        
        n = pushNewNode(RBBINode::tag);
        n->fVal      = 0;
        n->fFirstPos = fScanIndex;
        n->fLastPos  = fNextIndex;
        break;

    case doTagDigit:
        
        {
            n = fNodeStack[fNodeStackPtr];
            uint32_t v = u_charDigitValue(fC.fChar);
            U_ASSERT(v < 10);
            n->fVal = n->fVal*10 + v;
            break;
        }

    case doTagValue:
        n = fNodeStack[fNodeStackPtr];
        n->fLastPos = fNextIndex;
        fRB->fRules.extractBetween(n->fFirstPos, n->fLastPos, n->fText);
        break;

    case doTagExpectedError:
        error(U_BRK_MALFORMED_RULE_TAG);
        returnVal = FALSE;
        break;

    case doOptionStart:
        
        fOptionStart = fScanIndex;
        break;

    case doOptionEnd:
        {
            UnicodeString opt(fRB->fRules, fOptionStart, fScanIndex-fOptionStart);
            if (opt == UNICODE_STRING("chain", 5)) {
                fRB->fChainRules = TRUE;
            } else if (opt == UNICODE_STRING("LBCMNoChain", 11)) {
                fRB->fLBCMNoChain = TRUE;
            } else if (opt == UNICODE_STRING("forward", 7)) {
                fRB->fDefaultTree   = &fRB->fForwardTree;
            } else if (opt == UNICODE_STRING("reverse", 7)) {
                fRB->fDefaultTree   = &fRB->fReverseTree;
            } else if (opt == UNICODE_STRING("safe_forward", 12)) {
                fRB->fDefaultTree   = &fRB->fSafeFwdTree;
            } else if (opt == UNICODE_STRING("safe_reverse", 12)) {
                fRB->fDefaultTree   = &fRB->fSafeRevTree;
            } else if (opt == UNICODE_STRING("lookAheadHardBreak", 18)) {
                fRB->fLookAheadHardBreak = TRUE;
            } else {
                error(U_BRK_UNRECOGNIZED_OPTION);
            }
        }
        break;

    case doReverseDir:
        fReverseRule = TRUE;
        break;

    case doStartVariableName:
        n = pushNewNode(RBBINode::varRef);
        if (U_FAILURE(*fRB->fStatus)) {
            break;
        }
        n->fFirstPos = fScanIndex;
        break;

    case doEndVariableName:
        n = fNodeStack[fNodeStackPtr];
        if (n==NULL || n->fType != RBBINode::varRef) {
            error(U_BRK_INTERNAL_ERROR);
            break;
        }
        n->fLastPos = fScanIndex;
        fRB->fRules.extractBetween(n->fFirstPos+1, n->fLastPos, n->fText);
        
        
        
        
        n->fLeftChild = fSymbolTable->lookupNode(n->fText);
        break;

    case doCheckVarDef:
        n = fNodeStack[fNodeStackPtr];
        if (n->fLeftChild == NULL) {
            error(U_BRK_UNDEFINED_VARIABLE);
            returnVal = FALSE;
        }
        break;

    case doExprFinished:
        break;

    case doRuleErrorAssignExpr:
        error(U_BRK_ASSIGN_ERROR);
        returnVal = FALSE;
        break;

    case doExit:
        returnVal = FALSE;
        break;

    case doScanUnicodeSet:
        scanSet();
        break;

    default:
        error(U_BRK_INTERNAL_ERROR);
        returnVal = FALSE;
        break;
    }
    return returnVal;
}










void RBBIRuleScanner::error(UErrorCode e) {
    if (U_SUCCESS(*fRB->fStatus)) {
        *fRB->fStatus = e;
        if (fRB->fParseError) {
            fRB->fParseError->line  = fLineNum;
            fRB->fParseError->offset = fCharNum;
            fRB->fParseError->preContext[0] = 0;
            fRB->fParseError->preContext[0] = 0;
        }
    }
}



















void RBBIRuleScanner::fixOpStack(RBBINode::OpPrecedence p) {
    RBBINode *n;
    
    for (;;) {
        n = fNodeStack[fNodeStackPtr-1];   
        if (n->fPrecedence == 0) {
            RBBIDebugPuts("RBBIRuleScanner::fixOpStack, bad operator node");
            error(U_BRK_INTERNAL_ERROR);
            return;
        }

        if (n->fPrecedence < p || n->fPrecedence <= RBBINode::precLParen) {
            
            
            break;
        }
            
            
            
            n->fRightChild = fNodeStack[fNodeStackPtr];
            fNodeStack[fNodeStackPtr]->fParent = n;
            fNodeStackPtr--;
        
    }

    if (p <= RBBINode::precLParen) {
        
        
        
            
            if (n->fPrecedence != p) {
                
                
                error(U_BRK_MISMATCHED_PAREN);
            }
            fNodeStack[fNodeStackPtr-1] = fNodeStack[fNodeStackPtr];
            fNodeStackPtr--;
            
            delete n;
    }
    
}




















void RBBIRuleScanner::findSetFor(const UnicodeString &s, RBBINode *node, UnicodeSet *setToAdopt) {

    RBBISetTableEl   *el;

    
    
    
    el = (RBBISetTableEl *)uhash_get(fSetTable, &s);
    if (el != NULL) {
        delete setToAdopt;
        node->fLeftChild = el->val;
        U_ASSERT(node->fLeftChild->fType == RBBINode::uset);
        return;
    }

    
    
    
    if (setToAdopt == NULL) {
        if (s.compare(kAny, -1) == 0) {
            setToAdopt = new UnicodeSet(0x000000, 0x10ffff);
        } else {
            UChar32 c;
            c = s.char32At(0);
            setToAdopt = new UnicodeSet(c, c);
        }
    }

    
    
    
    
    RBBINode *usetNode    = new RBBINode(RBBINode::uset);
    if (usetNode == NULL) {
        error(U_MEMORY_ALLOCATION_ERROR);
        return;
    }
    usetNode->fInputSet   = setToAdopt;
    usetNode->fParent     = node;
    node->fLeftChild      = usetNode;
    usetNode->fText = s;


    
    
    
    fRB->fUSetNodes->addElement(usetNode, *fRB->fStatus);


    
    
    
    el      = (RBBISetTableEl *)uprv_malloc(sizeof(RBBISetTableEl));
    UnicodeString *tkey = new UnicodeString(s);
    if (tkey == NULL || el == NULL || setToAdopt == NULL) {
        
        delete tkey;
        tkey = NULL;
        uprv_free(el);
        el = NULL;
        delete setToAdopt;
        setToAdopt = NULL;

        error(U_MEMORY_ALLOCATION_ERROR);
        return;
    }
    el->key = tkey;
    el->val = usetNode;
    uhash_put(fSetTable, el->key, el, fRB->fStatus);

    return;
}








static const UChar      chCR        = 0x0d;      
static const UChar      chLF        = 0x0a;
static const UChar      chNEL       = 0x85;      
static const UChar      chLS        = 0x2028;    
static const UChar      chApos      = 0x27;      
static const UChar      chPound     = 0x23;      
static const UChar      chBackSlash = 0x5c;      
static const UChar      chLParen    = 0x28;
static const UChar      chRParen    = 0x29;








UnicodeString RBBIRuleScanner::stripRules(const UnicodeString &rules) {
    UnicodeString strippedRules;
    int rulesLength = rules.length();
    for (int idx = 0; idx < rulesLength; ) {
        UChar ch = rules[idx++];
        if (ch == chPound) {
            while (idx < rulesLength
                && ch != chCR && ch != chLF && ch != chNEL)
            {
                ch = rules[idx++];
            }
        }
        if (!u_isISOControl(ch)) {
            strippedRules.append(ch);
        }
    }
    
    return strippedRules;
}









UChar32  RBBIRuleScanner::nextCharLL() {
    UChar32  ch;

    if (fNextIndex >= fRB->fRules.length()) {
        return (UChar32)-1;
    }
    ch         = fRB->fRules.char32At(fNextIndex);
    fNextIndex = fRB->fRules.moveIndex32(fNextIndex, 1);

    if (ch == chCR ||
        ch == chNEL ||
        ch == chLS   ||
        (ch == chLF && fLastChar != chCR)) {
        
        
        fLineNum++;
        fCharNum=0;
        if (fQuoteMode) {
            error(U_BRK_NEW_LINE_IN_QUOTED_STRING);
            fQuoteMode = FALSE;
        }
    }
    else {
        
        
        if (ch != chLF) {
            fCharNum++;
        }
    }
    fLastChar = ch;
    return ch;
}









void RBBIRuleScanner::nextChar(RBBIRuleChar &c) {

    
    

    fScanIndex = fNextIndex;
    c.fChar    = nextCharLL();
    c.fEscaped = FALSE;

    
    
    
    
    if (c.fChar == chApos) {
        if (fRB->fRules.char32At(fNextIndex) == chApos) {
            c.fChar    = nextCharLL();        
            c.fEscaped = TRUE;                
        }
        else
        {
            
            
            
            fQuoteMode = !fQuoteMode;
            if (fQuoteMode == TRUE) {
                c.fChar = chLParen;
            } else {
                c.fChar = chRParen;
            }
            c.fEscaped = FALSE;      
            return;
        }
    }

    if (fQuoteMode) {
        c.fEscaped = TRUE;
    }
    else
    {
        
        
        if (c.fChar == chPound) {
            
            
            
            
            
            for (;;) {
                c.fChar = nextCharLL();
                if (c.fChar == (UChar32)-1 ||  
                    c.fChar == chCR     ||
                    c.fChar == chLF     ||
                    c.fChar == chNEL    ||
                    c.fChar == chLS)       {break;}
            }
        }
        if (c.fChar == (UChar32)-1) {
            return;
        }

        
        
        
        
        if (c.fChar == chBackSlash) {
            c.fEscaped = TRUE;
            int32_t startX = fNextIndex;
            c.fChar = fRB->fRules.unescapeAt(fNextIndex);
            if (fNextIndex == startX) {
                error(U_BRK_HEX_DIGITS_EXPECTED);
            }
            fCharNum += fNextIndex-startX;
        }
    }
    
}









void RBBIRuleScanner::parse() {
    uint16_t                state;
    const RBBIRuleTableEl  *tableEl;

    if (U_FAILURE(*fRB->fStatus)) {
        return;
    }

    state = 1;
    nextChar(fC);
    
    
    
    
    
    
    
    
    for (;;) {
        
        
        if (U_FAILURE(*fRB->fStatus)) {
            break;
        }

        
        
        if (state == 0) {
            break;
        }

        
        
        
        
        
        
        tableEl = &gRuleParseStateTable[state];
        #ifdef RBBI_DEBUG
            if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "scan")) {
                RBBIDebugPrintf("char, line, col = (\'%c\', %d, %d)    state=%s ",
                    fC.fChar, fLineNum, fCharNum, RBBIRuleStateNames[state]);
            }
        #endif

        for (;;) {
            #ifdef RBBI_DEBUG
                if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "scan")) { RBBIDebugPrintf(".");}
            #endif
            if (tableEl->fCharClass < 127 && fC.fEscaped == FALSE &&   tableEl->fCharClass == fC.fChar) {
                
                
                
                break;
            }
            if (tableEl->fCharClass == 255) {
                
                break;
            }
            if (tableEl->fCharClass == 254 && fC.fEscaped)  {
                
                break;
            }
            if (tableEl->fCharClass == 253 && fC.fEscaped &&
                (fC.fChar == 0x50 || fC.fChar == 0x70 ))  {
                
                break;
            }
            if (tableEl->fCharClass == 252 && fC.fChar == (UChar32)-1)  {
                
                break;
            }

            if (tableEl->fCharClass >= 128 && tableEl->fCharClass < 240 &&   
                fC.fEscaped == FALSE &&                                      
                fC.fChar != (UChar32)-1) {                                   
                U_ASSERT((tableEl->fCharClass-128) < LENGTHOF(fRuleSets));
                if (fRuleSets[tableEl->fCharClass-128].contains(fC.fChar)) {
                    
                    
                    break;
                }
            }

            
            tableEl++;
        }
        if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "scan")) { RBBIDebugPuts("");}

        
        
        
        
        if (doParseActions((int32_t)tableEl->fAction) == FALSE) {
            
            
            
            break;
        }

        if (tableEl->fPushState != 0) {
            fStackPtr++;
            if (fStackPtr >= kStackSize) {
                error(U_BRK_INTERNAL_ERROR);
                RBBIDebugPuts("RBBIRuleScanner::parse() - state stack overflow.");
                fStackPtr--;
            }
            fStack[fStackPtr] = tableEl->fPushState;
        }

        if (tableEl->fNextChar) {
            nextChar(fC);
        }

        
        
        if (tableEl->fNextState != 255) {
            state = tableEl->fNextState;
        } else {
            state = fStack[fStackPtr];
            fStackPtr--;
            if (fStackPtr < 0) {
                error(U_BRK_INTERNAL_ERROR);
                RBBIDebugPuts("RBBIRuleScanner::parse() - state stack underflow.");
                fStackPtr++;
            }
        }

    }

    
    
    
    if (fRB->fReverseTree == NULL) {
        fRB->fReverseTree  = pushNewNode(RBBINode::opStar);
        RBBINode  *operand = pushNewNode(RBBINode::setRef);
        findSetFor(UnicodeString(TRUE, kAny, 3), operand);
        fRB->fReverseTree->fLeftChild = operand;
        operand->fParent              = fRB->fReverseTree;
        fNodeStackPtr -= 2;
    }


    
    
    
    
    
#ifdef RBBI_DEBUG
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "symbols")) {fSymbolTable->rbbiSymtablePrint();}
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "ptree"))
    {
        RBBIDebugPrintf("Completed Forward Rules Parse Tree...\n");
        fRB->fForwardTree->printTree(TRUE);
        RBBIDebugPrintf("\nCompleted Reverse Rules Parse Tree...\n");
        fRB->fReverseTree->printTree(TRUE);
        RBBIDebugPrintf("\nCompleted Safe Point Forward Rules Parse Tree...\n");
        fRB->fSafeFwdTree->printTree(TRUE);
        RBBIDebugPrintf("\nCompleted Safe Point Reverse Rules Parse Tree...\n");
        fRB->fSafeRevTree->printTree(TRUE);
    }
#endif
}







#ifdef RBBI_DEBUG
void RBBIRuleScanner::printNodeStack(const char *title) {
    int i;
    RBBIDebugPrintf("%s.  Dumping node stack...\n", title);
    for (i=fNodeStackPtr; i>0; i--) {fNodeStack[i]->printTree(TRUE);}
}
#endif










RBBINode  *RBBIRuleScanner::pushNewNode(RBBINode::NodeType  t) {
    fNodeStackPtr++;
    if (fNodeStackPtr >= kStackSize) {
        error(U_BRK_INTERNAL_ERROR);
        RBBIDebugPuts("RBBIRuleScanner::pushNewNode - stack overflow.");
        *fRB->fStatus = U_BRK_INTERNAL_ERROR;
        return NULL;
    }
    fNodeStack[fNodeStackPtr] = new RBBINode(t);
    if (fNodeStack[fNodeStackPtr] == NULL) {
        *fRB->fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    return fNodeStack[fNodeStackPtr];
}

















void RBBIRuleScanner::scanSet() {
    UnicodeSet    *uset;
    ParsePosition  pos;
    int            startPos;
    int            i;

    if (U_FAILURE(*fRB->fStatus)) {
        return;
    }

    pos.setIndex(fScanIndex);
    startPos = fScanIndex;
    UErrorCode localStatus = U_ZERO_ERROR;
    uset = new UnicodeSet();
    if (uset == NULL) {
        localStatus = U_MEMORY_ALLOCATION_ERROR;
    } else {
        uset->applyPatternIgnoreSpace(fRB->fRules, pos, fSymbolTable, localStatus);
    }
    if (U_FAILURE(localStatus)) {
        
        
        #ifdef RBBI_DEBUG
            RBBIDebugPrintf("UnicodeSet parse postion.ErrorIndex = %d\n", pos.getIndex());
        #endif
        error(localStatus);
        delete uset;
        return;
    }

    
    
    U_ASSERT(uset!=NULL);
    if (uset->isEmpty()) {
        
        
        
        
        error(U_BRK_RULE_EMPTY_SET);
        delete uset;
        return;
    }


    
    
    
    i = pos.getIndex();
    for (;;) {
        if (fNextIndex >= i) {
            break;
        }
        nextCharLL();
    }

    if (U_SUCCESS(*fRB->fStatus)) {
        RBBINode         *n;

        n = pushNewNode(RBBINode::setRef);
        n->fFirstPos = startPos;
        n->fLastPos  = fNextIndex;
        fRB->fRules.extractBetween(n->fFirstPos, n->fLastPos, n->fText);
        
        
        
        
        
        
        findSetFor(n->fText, n, uset);
    }

}

U_NAMESPACE_END

#endif 
