










#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/uchriter.h"
#include "unicode/parsepos.h"
#include "unicode/parseerr.h"
#include "unicode/regex.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "patternprops.h"
#include "putilimp.h"
#include "cmemory.h"
#include "cstring.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "uassert.h"
#include "ucln_in.h"
#include "uinvchar.h"

#include "regeximp.h"
#include "regexcst.h"   
                        
#include "regexcmp.h"
#include "regexst.h"
#include "regextxt.h"



U_NAMESPACE_BEGIN







RegexCompile::RegexCompile(RegexPattern *rxp, UErrorCode &status) :
   fParenStack(status), fSetStack(status), fSetOpStack(status)
{
    
    RegexStaticSets::initGlobals(&status);

    fStatus           = &status;

    fRXPat            = rxp;
    fScanIndex        = 0;
    fLastChar         = -1;
    fPeekChar         = -1;
    fLineNum          = 1;
    fCharNum          = 0;
    fQuoteMode        = FALSE;
    fInBackslashQuote = FALSE;
    fModeFlags        = fRXPat->fFlags | 0x80000000;
    fEOLComments      = TRUE;

    fMatchOpenParen   = -1;
    fMatchCloseParen  = -1;

    if (U_SUCCESS(status) && U_FAILURE(rxp->fDeferredStatus)) {
        status = rxp->fDeferredStatus;
    }
}

static const UChar      chAmp       = 0x26;      
static const UChar      chDash      = 0x2d;      







RegexCompile::~RegexCompile() {
}

static inline void addCategory(UnicodeSet *set, int32_t value, UErrorCode& ec) {
    set->addAll(UnicodeSet().applyIntPropertyValue(UCHAR_GENERAL_CATEGORY_MASK, value, ec));
}









void    RegexCompile::compile(
                         const UnicodeString &pat,   
                         UParseError &pp,            
                         UErrorCode &e)              
{
    fRXPat->fPatternString = new UnicodeString(pat);
    UText patternText = UTEXT_INITIALIZER;
    utext_openConstUnicodeString(&patternText, fRXPat->fPatternString, &e);
    
    if (U_SUCCESS(e)) {
        compile(&patternText, pp, e);
        utext_close(&patternText);
    }
}





void    RegexCompile::compile(
                         UText *pat,                 
                         UParseError &pp,            
                         UErrorCode &e)              
{
    fStatus             = &e;
    fParseErr           = &pp;
    fStackPtr           = 0;
    fStack[fStackPtr]   = 0;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    U_ASSERT(fRXPat->fPattern == NULL || utext_nativeLength(fRXPat->fPattern) == 0);

    
    fRXPat->fPattern        = utext_clone(fRXPat->fPattern, pat, FALSE, TRUE, fStatus);
    fRXPat->fStaticSets     = RegexStaticSets::gStaticSets->fPropSets;
    fRXPat->fStaticSets8    = RegexStaticSets::gStaticSets->fPropSets8;


    
    fPatternLength = utext_nativeLength(pat);
    uint16_t                state = 1;
    const RegexTableEl      *tableEl;

    
    if (fModeFlags & UREGEX_LITERAL) {
        fQuoteMode = TRUE;
    }

    nextChar(fC);                        

    
    
    
    
    
    
    
    
    
    
    for (;;) {
        
        
        if (U_FAILURE(*fStatus)) {
            break;
        }

        U_ASSERT(state != 0);

        
        
        
        
        
        
        tableEl = &gRuleParseStateTable[state];
        REGEX_SCAN_DEBUG_PRINTF(("char, line, col = (\'%c\', %d, %d)    state=%s ",
            fC.fChar, fLineNum, fCharNum, RegexStateNames[state]));

        for (;;) {    
                      
            REGEX_SCAN_DEBUG_PRINTF(("."));
            if (tableEl->fCharClass < 127 && fC.fQuoted == FALSE &&   tableEl->fCharClass == fC.fChar) {
                
                
                
                break;
            }
            if (tableEl->fCharClass == 255) {
                
                break;
            }
            if (tableEl->fCharClass == 254 && fC.fQuoted)  {
                
                break;
            }
            if (tableEl->fCharClass == 253 && fC.fChar == (UChar32)-1)  {
                
                break;
            }

            if (tableEl->fCharClass >= 128 && tableEl->fCharClass < 240 &&   
                fC.fQuoted == FALSE &&                                       
                fC.fChar != (UChar32)-1) {                                   
                U_ASSERT(tableEl->fCharClass <= 137);
                if (RegexStaticSets::gStaticSets->fRuleSets[tableEl->fCharClass-128].contains(fC.fChar)) {
                    
                    
                    break;
                }
            }

            
            tableEl++;
        }
        REGEX_SCAN_DEBUG_PRINTF(("\n"));

        
        
        
        
        if (doParseActions(tableEl->fAction) == FALSE) {
            
            
            
            break;
        }

        if (tableEl->fPushState != 0) {
            fStackPtr++;
            if (fStackPtr >= kStackSize) {
                error(U_REGEX_INTERNAL_ERROR);
                REGEX_SCAN_DEBUG_PRINTF(("RegexCompile::parse() - state stack overflow.\n"));
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
                
                
                
                
                fStackPtr++;
                error(U_REGEX_MISMATCHED_PAREN);
            }
        }

    }

    if (U_FAILURE(*fStatus)) {
        
        
        
        while (!fSetStack.empty()) {
            delete (UnicodeSet *)fSetStack.pop();
        }
        return;
    }

    
    
    

    
    
    
    fRXPat->fMaxCaptureDigits = 1;
    int32_t  n = 10;
    int32_t  groupCount = fRXPat->fGroupMap->size();
    while (n <= groupCount) {
        fRXPat->fMaxCaptureDigits++;
        n *= 10;
    }

    
    
    
    
    
    
    
    fRXPat->fFrameSize+=RESTACKFRAME_HDRCOUNT;

    
    
    
    stripNOPs();

    
    
    
    
    
    fRXPat->fMinMatchLen = minMatchLength(3, fRXPat->fCompiledPat->size()-1);

    
    
    
    matchStartType();

    
    
    
    int32_t numSets = fRXPat->fSets->size();
    fRXPat->fSets8 = new Regex8BitSet[numSets];
    
    if (fRXPat->fSets8 == NULL) {
        e = *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    int32_t i;
    for (i=0; i<numSets; i++) {
        UnicodeSet *s = (UnicodeSet *)fRXPat->fSets->elementAt(i);
        fRXPat->fSets8[i].init(s);
    }

}















UBool RegexCompile::doParseActions(int32_t action)
{
    UBool   returnVal = TRUE;

    switch ((Regex_PatternParseAction)action) {

    case doPatStart:
        
        
        
        
        
        
        
        
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_STATE_SAVE, 2), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_JMP,  3), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_FAIL, 0), *fStatus);

        
        
        doParseActions(doOpenNonCaptureParen);
        break;

    case doPatFinish:
        
        
        
        
        
        
        
        
        handleCloseParen();
        if (fParenStack.size() > 0) {
            
            error(U_REGEX_MISMATCHED_PAREN);
        }

        
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_END, 0), *fStatus);

        
        returnVal = FALSE;
        break;



    case doOrOperator:
        
        {
            
            fixLiterals(FALSE);

            
            
            
            
            
            int32_t savePosition = fParenStack.popi();
            int32_t op = (int32_t)fRXPat->fCompiledPat->elementAti(savePosition);
            U_ASSERT(URX_TYPE(op) == URX_NOP);  
            op = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size()+1);
            fRXPat->fCompiledPat->setElementAt(op, savePosition);

            
            
            
            op = URX_BUILD(URX_JMP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);

            
            
            
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);
        }
        break;


    case doOpenCaptureParen:
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        {
            fixLiterals();
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            int32_t  varsLoc    = fRXPat->fFrameSize;    
            fRXPat->fFrameSize += 3;
            int32_t  cop        = URX_BUILD(URX_START_CAPTURE, varsLoc);
            fRXPat->fCompiledPat->addElement(cop, *fStatus);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);

            
            
            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(capturing, *fStatus);                        
            fParenStack.push(fRXPat->fCompiledPat->size()-3, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   

            
            fRXPat->fGroupMap->addElement(varsLoc, *fStatus);
        }
         break;

    case doOpenNonCaptureParen:
        
        
        
        
        
        
        {
            fixLiterals();
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);

            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(plain,      *fStatus);                       
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   
        }
         break;


    case doOpenAtomicParen:
        
        
        
        
        
        
        
        {
            fixLiterals();
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            int32_t  varLoc    = fRXPat->fDataSize;    
            fRXPat->fDataSize += 1;                    
            int32_t  stoOp     = URX_BUILD(URX_STO_SP, varLoc);
            fRXPat->fCompiledPat->addElement(stoOp, *fStatus);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);

            
            
            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(atomic, *fStatus);                           
            fParenStack.push(fRXPat->fCompiledPat->size()-3, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   
        }
        break;


    case doOpenLookAhead:
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        {
            fixLiterals();
            int32_t dataLoc = fRXPat->fDataSize;
            fRXPat->fDataSize += 2;
            int32_t op = URX_BUILD(URX_LA_START, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            op = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size()+ 2);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            op = URX_BUILD(URX_JMP, fRXPat->fCompiledPat->size()+ 3);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            
            op = URX_BUILD(URX_LA_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            op = URX_BUILD(URX_BACKTRACK, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            
            op = URX_BUILD(URX_NOP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(lookAhead, *fStatus);                        
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   
        }
        break;

    case doOpenLookAheadNeg:
        
        
        
        
        
        
        
        
        
        
        
        {
            fixLiterals();
            int32_t dataLoc = fRXPat->fDataSize;
            fRXPat->fDataSize += 2;
            int32_t op = URX_BUILD(URX_LA_START, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            op = URX_BUILD(URX_STATE_SAVE, 0);    
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            op = URX_BUILD(URX_NOP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(negLookAhead, *fStatus);                    
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   

            
        }
        break;

    case doOpenLookBehind:
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            
            fixLiterals();

            
            int32_t dataLoc = fRXPat->fDataSize;
            fRXPat->fDataSize += 4;

            
            int32_t op = URX_BUILD(URX_LB_START, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            op = URX_BUILD(URX_LB_CONT, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            fRXPat->fCompiledPat->addElement(0,  *fStatus);    
            fRXPat->fCompiledPat->addElement(0,  *fStatus);    

            
            op = URX_BUILD(URX_NOP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(lookBehind, *fStatus);                       
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   

            
        }

        break;

    case doOpenLookBehindNeg:
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            
            fixLiterals();

            
            int32_t dataLoc = fRXPat->fDataSize;
            fRXPat->fDataSize += 4;

            
            int32_t op = URX_BUILD(URX_LB_START, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            op = URX_BUILD(URX_LBN_CONT, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            fRXPat->fCompiledPat->addElement(0,  *fStatus);    
            fRXPat->fCompiledPat->addElement(0,  *fStatus);    
            fRXPat->fCompiledPat->addElement(0,  *fStatus);    

            
            op = URX_BUILD(URX_NOP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            fParenStack.push(fModeFlags, *fStatus);                       
            fParenStack.push(lookBehindN, *fStatus);                      
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   

            
        }
        break;

    case doConditionalExpr:
        
    case doPerlInline:
        
        error(U_REGEX_UNIMPLEMENTED);
        break;


    case doCloseParen:
        handleCloseParen();
        if (fParenStack.size() <= 0) {
            
            error(U_REGEX_MISMATCHED_PAREN);
        }
        break;

    case doNOP:
        break;


    case doBadOpenParenType:
    case doRuleError:
        error(U_REGEX_RULE_SYNTAX);
        break;


    case doMismatchedParenErr:
        error(U_REGEX_MISMATCHED_PAREN);
        break;

    case doPlus:
        
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        {
            int32_t  topLoc = blockTopLoc(FALSE);        
            int32_t  frameLoc;

            
            if (topLoc == fRXPat->fCompiledPat->size() - 1) {
                int32_t repeatedOp = (int32_t)fRXPat->fCompiledPat->elementAti(topLoc);

                if (URX_TYPE(repeatedOp) == URX_SETREF) {
                    
                    int32_t loopOpI = URX_BUILD(URX_LOOP_SR_I, URX_VAL(repeatedOp));
                    fRXPat->fCompiledPat->addElement(loopOpI, *fStatus);
                    frameLoc = fRXPat->fFrameSize;
                    fRXPat->fFrameSize++;
                    int32_t loopOpC = URX_BUILD(URX_LOOP_C, frameLoc);
                    fRXPat->fCompiledPat->addElement(loopOpC, *fStatus);
                    break;
                }

                if (URX_TYPE(repeatedOp) == URX_DOTANY ||
                    URX_TYPE(repeatedOp) == URX_DOTANY_ALL ||
                    URX_TYPE(repeatedOp) == URX_DOTANY_UNIX) {
                    
                    int32_t loopOpI = URX_BUILD(URX_LOOP_DOT_I, 0);
                    if (URX_TYPE(repeatedOp) == URX_DOTANY_ALL) {
                        
                        loopOpI |= 1;
                    }
                    if (fModeFlags & UREGEX_UNIX_LINES) {
                        loopOpI |= 2;
                    }
                    fRXPat->fCompiledPat->addElement(loopOpI, *fStatus);
                    frameLoc = fRXPat->fFrameSize;
                    fRXPat->fFrameSize++;
                    int32_t loopOpC = URX_BUILD(URX_LOOP_C, frameLoc);
                    fRXPat->fCompiledPat->addElement(loopOpC, *fStatus);
                    break;
                }

            }

            

            
            
            if (minMatchLength(topLoc, fRXPat->fCompiledPat->size()-1) == 0) {
                
                
                insertOp(topLoc);
                frameLoc =  fRXPat->fFrameSize;
                fRXPat->fFrameSize++;

                int32_t op = URX_BUILD(URX_STO_INP_LOC, frameLoc);
                fRXPat->fCompiledPat->setElementAt(op, topLoc);

                op = URX_BUILD(URX_JMP_SAV_X, topLoc+1);
                fRXPat->fCompiledPat->addElement(op, *fStatus);
            } else {
                
                int32_t  jmpOp  = URX_BUILD(URX_JMP_SAV, topLoc);
                fRXPat->fCompiledPat->addElement(jmpOp, *fStatus);
            }
        }
        break;

    case doNGPlus:
        
        
        
        
        {
            int32_t topLoc      = blockTopLoc(FALSE);
            int32_t saveStateOp = URX_BUILD(URX_STATE_SAVE, topLoc);
            fRXPat->fCompiledPat->addElement(saveStateOp, *fStatus);
        }
        break;


    case doOpt:
        
        
        
        
        
        
        {
            int32_t   saveStateLoc = blockTopLoc(TRUE);
            int32_t   saveStateOp  = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size());
            fRXPat->fCompiledPat->setElementAt(saveStateOp, saveStateLoc);
        }
        break;

    case doNGOpt:
        
        
        
        
        
        
        
        
        
        {
            int32_t  jmp1_loc = blockTopLoc(TRUE);
            int32_t  jmp2_loc = fRXPat->fCompiledPat->size();

            int32_t  jmp1_op  = URX_BUILD(URX_JMP, jmp2_loc+1);
            fRXPat->fCompiledPat->setElementAt(jmp1_op, jmp1_loc);

            int32_t  jmp2_op  = URX_BUILD(URX_JMP, jmp2_loc+2);
            fRXPat->fCompiledPat->addElement(jmp2_op, *fStatus);

            int32_t  save_op  = URX_BUILD(URX_STATE_SAVE, jmp1_loc+1);
            fRXPat->fCompiledPat->addElement(save_op, *fStatus);
        }
        break;


    case doStar:
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        {
            
            int32_t   topLoc = blockTopLoc(FALSE);
            int32_t   dataLoc = -1;

            
            
            if (topLoc == fRXPat->fCompiledPat->size() - 1) {
                int32_t repeatedOp = (int32_t)fRXPat->fCompiledPat->elementAti(topLoc);

                if (URX_TYPE(repeatedOp) == URX_SETREF) {
                    
                    int32_t loopOpI = URX_BUILD(URX_LOOP_SR_I, URX_VAL(repeatedOp));
                    fRXPat->fCompiledPat->setElementAt(loopOpI, topLoc);
                    dataLoc = fRXPat->fFrameSize;
                    fRXPat->fFrameSize++;
                    int32_t loopOpC = URX_BUILD(URX_LOOP_C, dataLoc);
                    fRXPat->fCompiledPat->addElement(loopOpC, *fStatus);
                    break;
                }

                if (URX_TYPE(repeatedOp) == URX_DOTANY ||
                    URX_TYPE(repeatedOp) == URX_DOTANY_ALL ||
                    URX_TYPE(repeatedOp) == URX_DOTANY_UNIX) {
                    
                    int32_t loopOpI = URX_BUILD(URX_LOOP_DOT_I, 0);
                    if (URX_TYPE(repeatedOp) == URX_DOTANY_ALL) {
                        
                        loopOpI |= 1;
                    }
                    if ((fModeFlags & UREGEX_UNIX_LINES) != 0) {
                        loopOpI |= 2;
                    }
                    fRXPat->fCompiledPat->setElementAt(loopOpI, topLoc);
                    dataLoc = fRXPat->fFrameSize;
                    fRXPat->fFrameSize++;
                    int32_t loopOpC = URX_BUILD(URX_LOOP_C, dataLoc);
                    fRXPat->fCompiledPat->addElement(loopOpC, *fStatus);
                    break;
                }
            }

            
            

            int32_t   saveStateLoc = blockTopLoc(TRUE);
            int32_t   jmpOp        = URX_BUILD(URX_JMP_SAV, saveStateLoc+1);

            
            
            if (minMatchLength(saveStateLoc, fRXPat->fCompiledPat->size()-1) == 0) {
                insertOp(saveStateLoc);
                dataLoc =  fRXPat->fFrameSize;
                fRXPat->fFrameSize++;

                int32_t op = URX_BUILD(URX_STO_INP_LOC, dataLoc);
                fRXPat->fCompiledPat->setElementAt(op, saveStateLoc+1);
                jmpOp      = URX_BUILD(URX_JMP_SAV_X, saveStateLoc+2);
            }

            
            
            int32_t continueLoc = fRXPat->fCompiledPat->size()+1;

            
            int32_t saveStateOp = URX_BUILD(URX_STATE_SAVE, continueLoc);
            fRXPat->fCompiledPat->setElementAt(saveStateOp, saveStateLoc);

            
            fRXPat->fCompiledPat->addElement(jmpOp, *fStatus);
        }
        break;

    case doNGStar:
        
        
        
        
        
        
        {
            int32_t     jmpLoc  = blockTopLoc(TRUE);                   
            int32_t     saveLoc = fRXPat->fCompiledPat->size();        
            int32_t     jmpOp   = URX_BUILD(URX_JMP, saveLoc);
            int32_t     stateSaveOp = URX_BUILD(URX_STATE_SAVE, jmpLoc+1);
            fRXPat->fCompiledPat->setElementAt(jmpOp, jmpLoc);
            fRXPat->fCompiledPat->addElement(stateSaveOp, *fStatus);
        }
        break;


    case doIntervalInit:
        
        
        
        fIntervalLow = 0;
        fIntervalUpper = -1;
        break;

    case doIntevalLowerDigit:
        
        {
            int32_t digitValue = u_charDigitValue(fC.fChar);
            U_ASSERT(digitValue >= 0);
            fIntervalLow = fIntervalLow*10 + digitValue;
            if (fIntervalLow < 0) {
                error(U_REGEX_NUMBER_TOO_BIG);
            }
        }
        break;

    case doIntervalUpperDigit:
        
        {
            if (fIntervalUpper < 0) {
                fIntervalUpper = 0;
            }
            int32_t digitValue = u_charDigitValue(fC.fChar);
            U_ASSERT(digitValue >= 0);
            fIntervalUpper = fIntervalUpper*10 + digitValue;
            if (fIntervalUpper < 0) {
                error(U_REGEX_NUMBER_TOO_BIG);
            }
        }
        break;

    case doIntervalSame:
        
        fIntervalUpper = fIntervalLow;
        break;

    case doInterval:
        
        if (compileInlineInterval() == FALSE) {
            compileInterval(URX_CTR_INIT, URX_CTR_LOOP);
        }
        break;

    case doPossessiveInterval:
        
        {
            
            
            
            
            int32_t topLoc = blockTopLoc(FALSE);

            
            compileInterval(URX_CTR_INIT, URX_CTR_LOOP);

            
            

            
            insertOp(topLoc);
            int32_t  varLoc    = fRXPat->fDataSize;    
            fRXPat->fDataSize += 1;                    
            int32_t  op        = URX_BUILD(URX_STO_SP, varLoc);
            fRXPat->fCompiledPat->setElementAt(op, topLoc);

            int32_t loopOp = (int32_t)fRXPat->fCompiledPat->popi();
            U_ASSERT(URX_TYPE(loopOp) == URX_CTR_LOOP && URX_VAL(loopOp) == topLoc);
            loopOp++;     
            fRXPat->fCompiledPat->push(loopOp, *fStatus);

            
            op = URX_BUILD(URX_LD_SP, varLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }

        break;

    case doNGInterval:
        
        compileInterval(URX_CTR_INIT_NG, URX_CTR_LOOP_NG);
        break;

    case doIntervalError:
        error(U_REGEX_BAD_INTERVAL);
        break;

    case doLiteralChar:
        
        literalChar(fC.fChar);
        break;


    case doEscapedLiteralChar:
        
        
        if ((fModeFlags & UREGEX_ERROR_ON_UNKNOWN_ESCAPES) != 0 &&
            ((fC.fChar >= 0x41 && fC.fChar<= 0x5A) ||     
            (fC.fChar >= 0x61 && fC.fChar <= 0x7a))) {   
               error(U_REGEX_BAD_ESCAPE_SEQUENCE);
             }
        literalChar(fC.fChar);
        break;


    case doDotAny:
        
        {
            fixLiterals(FALSE);
            int32_t   op;
            if (fModeFlags & UREGEX_DOTALL) {
                op = URX_BUILD(URX_DOTANY_ALL, 0);
            } else if (fModeFlags & UREGEX_UNIX_LINES) {
                op = URX_BUILD(URX_DOTANY_UNIX, 0);
            } else {
                op = URX_BUILD(URX_DOTANY, 0);
            }
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;

    case doCaret:
        {
            fixLiterals(FALSE);
            int32_t op = 0;
            if (       (fModeFlags & UREGEX_MULTILINE) == 0 && (fModeFlags & UREGEX_UNIX_LINES) == 0) {
                op = URX_CARET;
            } else if ((fModeFlags & UREGEX_MULTILINE) != 0 && (fModeFlags & UREGEX_UNIX_LINES) == 0) {
                op = URX_CARET_M;
            } else if ((fModeFlags & UREGEX_MULTILINE) == 0 && (fModeFlags & UREGEX_UNIX_LINES) != 0) {
                op = URX_CARET;   
            } else if ((fModeFlags & UREGEX_MULTILINE) != 0 && (fModeFlags & UREGEX_UNIX_LINES) != 0) {
                op = URX_CARET_M_UNIX;
            }
            fRXPat->fCompiledPat->addElement(URX_BUILD(op, 0), *fStatus);
        }
        break;

    case doDollar:
        {
            fixLiterals(FALSE);
            int32_t op = 0;
            if (       (fModeFlags & UREGEX_MULTILINE) == 0 && (fModeFlags & UREGEX_UNIX_LINES) == 0) {
                op = URX_DOLLAR;
            } else if ((fModeFlags & UREGEX_MULTILINE) != 0 && (fModeFlags & UREGEX_UNIX_LINES) == 0) {
                op = URX_DOLLAR_M;
            } else if ((fModeFlags & UREGEX_MULTILINE) == 0 && (fModeFlags & UREGEX_UNIX_LINES) != 0) {
                op = URX_DOLLAR_D;
            } else if ((fModeFlags & UREGEX_MULTILINE) != 0 && (fModeFlags & UREGEX_UNIX_LINES) != 0) {
                op = URX_DOLLAR_MD;
            }
            fRXPat->fCompiledPat->addElement(URX_BUILD(op, 0), *fStatus);
        }
        break;

    case doBackslashA:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_CARET, 0), *fStatus);
        break;

    case doBackslashB:
        {
            #if  UCONFIG_NO_BREAK_ITERATION==1
            if (fModeFlags & UREGEX_UWORD) {
                error(U_UNSUPPORTED_ERROR);
            }
            #endif
            fixLiterals(FALSE);
            int32_t op = (fModeFlags & UREGEX_UWORD)? URX_BACKSLASH_BU : URX_BACKSLASH_B;
            fRXPat->fCompiledPat->addElement(URX_BUILD(op, 1), *fStatus);
        }
        break;

    case doBackslashb:
        {
            #if  UCONFIG_NO_BREAK_ITERATION==1
            if (fModeFlags & UREGEX_UWORD) {
                error(U_UNSUPPORTED_ERROR);
            }
            #endif
            fixLiterals(FALSE);
            int32_t op = (fModeFlags & UREGEX_UWORD)? URX_BACKSLASH_BU : URX_BACKSLASH_B;
            fRXPat->fCompiledPat->addElement(URX_BUILD(op, 0), *fStatus);
        }
        break;

    case doBackslashD:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_D, 1), *fStatus);
        break;

    case doBackslashd:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_D, 0), *fStatus);
        break;

    case doBackslashG:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_G, 0), *fStatus);
        break;

    case doBackslashS:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(
            URX_BUILD(URX_STAT_SETREF_N, URX_ISSPACE_SET), *fStatus);
        break;

    case doBackslashs:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(
            URX_BUILD(URX_STATIC_SETREF, URX_ISSPACE_SET), *fStatus);
        break;

    case doBackslashW:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(
            URX_BUILD(URX_STAT_SETREF_N, URX_ISWORD_SET), *fStatus);
        break;

    case doBackslashw:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(
            URX_BUILD(URX_STATIC_SETREF, URX_ISWORD_SET), *fStatus);
        break;

    case doBackslashX:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_X, 0), *fStatus);
        break;


    case doBackslashZ:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_DOLLAR, 0), *fStatus);
        break;

    case doBackslashz:
        fixLiterals(FALSE);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_Z, 0), *fStatus);
        break;

    case doEscapeError:
        error(U_REGEX_BAD_ESCAPE_SEQUENCE);
        break;

    case doExit:
        fixLiterals(FALSE);
        returnVal = FALSE;
        break;

    case doProperty:
        {
            fixLiterals(FALSE);
            UnicodeSet *theSet = scanProp();
            compileSet(theSet);
        }
        break;

    case doNamedChar:
        {
            UChar32 c = scanNamedChar();
            literalChar(c);
        }
        break;
        

    case doBackRef:
        
        
        
        
        {
            int32_t  numCaptureGroups = fRXPat->fGroupMap->size();
            int32_t  groupNum = 0;
            UChar32  c        = fC.fChar;

            for (;;) {
                
                int32_t digit = u_charDigitValue(c);
                groupNum = groupNum * 10 + digit;
                if (groupNum >= numCaptureGroups) {
                    break;
                }
                c = peekCharLL();
                if (RegexStaticSets::gStaticSets->fRuleDigitsAlias->contains(c) == FALSE) {
                    break;
                }
                nextCharLL();
            }

            
            
            
            
            
            U_ASSERT(groupNum > 0);  
                                     
            fixLiterals(FALSE);
            int32_t  op;
            if (fModeFlags & UREGEX_CASE_INSENSITIVE) {
                op = URX_BUILD(URX_BACKREF_I, groupNum);
            } else {
                op = URX_BUILD(URX_BACKREF, groupNum);
            }
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;


    case doPossessivePlus:
        
        
        
        
        
        
        
        
        
        
        
        
        {
            
            int32_t   topLoc = blockTopLoc(TRUE);
            int32_t   stoLoc = fRXPat->fDataSize;
            fRXPat->fDataSize++;       
            int32_t   op     = URX_BUILD(URX_STO_SP, stoLoc);
            fRXPat->fCompiledPat->setElementAt(op, topLoc);

            
            op = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size()+2);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            op = URX_BUILD(URX_JMP, topLoc+1);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            op = URX_BUILD(URX_LD_SP, stoLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;

    case doPossessiveStar:
        
        
        
        
        
        
        
        
        
        {
            
            int32_t   topLoc = blockTopLoc(TRUE);
            insertOp(topLoc);

            
            int32_t   stoLoc = fRXPat->fDataSize;
            fRXPat->fDataSize++;       
            int32_t   op     = URX_BUILD(URX_STO_SP, stoLoc);
            fRXPat->fCompiledPat->setElementAt(op, topLoc);

            
            int32_t L7 = fRXPat->fCompiledPat->size()+1;
            op = URX_BUILD(URX_STATE_SAVE, L7);
            fRXPat->fCompiledPat->setElementAt(op, topLoc+1);

            
            op = URX_BUILD(URX_JMP, topLoc+1);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            op = URX_BUILD(URX_LD_SP, stoLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;

    case doPossessiveOpt:
        
        
        
        
        
        
        
        
        {
            
            int32_t   topLoc = blockTopLoc(TRUE);
            insertOp(topLoc);

            
            int32_t   stoLoc = fRXPat->fDataSize;
            fRXPat->fDataSize++;       
            int32_t   op     = URX_BUILD(URX_STO_SP, stoLoc);
            fRXPat->fCompiledPat->setElementAt(op, topLoc);

            
            int32_t   continueLoc = fRXPat->fCompiledPat->size()+1;
            op = URX_BUILD(URX_STATE_SAVE, continueLoc);
            fRXPat->fCompiledPat->setElementAt(op, topLoc+1);

            
            op = URX_BUILD(URX_LD_SP, stoLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;


    case doBeginMatchMode:
        fNewModeFlags = fModeFlags;
        fSetModeFlag  = TRUE;
        break;

    case doMatchMode:   
        {
            int32_t  bit = 0;
            switch (fC.fChar) {
            case 0x69:    bit = UREGEX_CASE_INSENSITIVE; break;
            case 0x64:    bit = UREGEX_UNIX_LINES;       break;
            case 0x6d:    bit = UREGEX_MULTILINE;        break;
            case 0x73:    bit = UREGEX_DOTALL;           break;
            case 0x75:    bit = 0;   break;
            case 0x77:    bit = UREGEX_UWORD;            break;
            case 0x78:    bit = UREGEX_COMMENTS;         break;
            case 0x2d:    fSetModeFlag = FALSE;          break;
            default:
                U_ASSERT(FALSE);   
                                   
            }
            if (fSetModeFlag) {
                fNewModeFlags |= bit;
            } else {
                fNewModeFlags &= ~bit;
            }
        }
        break;

    case doSetMatchMode:
        
        fixLiterals();

        
        
        U_ASSERT(fNewModeFlags < 0);
        fModeFlags = fNewModeFlags;

        break;


    case doMatchModeParen:
        
        
        
        
        
        
        
        
        {
            fixLiterals(FALSE);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);

            
            
            
            fParenStack.push(fModeFlags, *fStatus);
            fParenStack.push(flags, *fStatus);                            
            fParenStack.push(fRXPat->fCompiledPat->size()-2, *fStatus);   
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   

            
            U_ASSERT(fNewModeFlags < 0);
            fModeFlags = fNewModeFlags;
        }
        break;

    case doBadModeFlag:
        error(U_REGEX_INVALID_FLAG);
        break;

    case doSuppressComments:
        
        
        
        fEOLComments = FALSE;
        break;


    case doSetAddAmp:
        {
          UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
          set->add(chAmp);
        }
        break;

    case doSetAddDash:
        {
          UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
          set->add(chDash);
        }
        break;

     case doSetBackslash_s:
        {
         UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
         set->addAll(*RegexStaticSets::gStaticSets->fPropSets[URX_ISSPACE_SET]);
         break;
        }

     case doSetBackslash_S:
        {
            UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
            UnicodeSet SSet(*RegexStaticSets::gStaticSets->fPropSets[URX_ISSPACE_SET]);
            SSet.complement();
            set->addAll(SSet);
            break;
        }

    case doSetBackslash_d:
        {
            UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
            
            addCategory(set, U_GC_ND_MASK, *fStatus);
            break;
        }

    case doSetBackslash_D:
        {
            UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
            UnicodeSet digits;
            
            digits.applyIntPropertyValue(UCHAR_GENERAL_CATEGORY_MASK, U_GC_ND_MASK, *fStatus);
            digits.complement();
            set->addAll(digits);
            break;
        }

    case doSetBackslash_w:
        {
            UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
            set->addAll(*RegexStaticSets::gStaticSets->fPropSets[URX_ISWORD_SET]);
            break;
        }

    case doSetBackslash_W:
        {
            UnicodeSet *set = (UnicodeSet *)fSetStack.peek();
            UnicodeSet SSet(*RegexStaticSets::gStaticSets->fPropSets[URX_ISWORD_SET]);
            SSet.complement();
            set->addAll(SSet);
            break;
        }

    case doSetBegin:
        fixLiterals(FALSE);
        fSetStack.push(new UnicodeSet(), *fStatus);
        fSetOpStack.push(setStart, *fStatus);
        if ((fModeFlags & UREGEX_CASE_INSENSITIVE) != 0) {
            fSetOpStack.push(setCaseClose, *fStatus);
        }
        break;

    case doSetBeginDifference1:
        
        
        
        
        setPushOp(setDifference1);
        fSetOpStack.push(setStart, *fStatus);
        if ((fModeFlags & UREGEX_CASE_INSENSITIVE) != 0) {
            fSetOpStack.push(setCaseClose, *fStatus);
        }
        break;

    case doSetBeginIntersection1:
        
        
        setPushOp(setIntersection1);
        fSetOpStack.push(setStart, *fStatus);
        if ((fModeFlags & UREGEX_CASE_INSENSITIVE) != 0) {
            fSetOpStack.push(setCaseClose, *fStatus);
        }
        break;

    case doSetBeginUnion:
        
        
        setPushOp(setUnion);
        fSetOpStack.push(setStart, *fStatus);
        if ((fModeFlags & UREGEX_CASE_INSENSITIVE) != 0) {
            fSetOpStack.push(setCaseClose, *fStatus);
        }
        break;

    case doSetDifference2:
        
        
        setPushOp(setDifference2);
        break;

    case doSetEnd:
        
        
        
        setEval(setEnd);
        U_ASSERT(fSetOpStack.peeki()==setStart);
        fSetOpStack.popi();
        break;

    case doSetFinish:
        {
        
        
        
        
        U_ASSERT(fSetOpStack.empty());
        UnicodeSet *theSet = (UnicodeSet *)fSetStack.pop();
        U_ASSERT(fSetStack.empty());
        compileSet(theSet);
        break;
        }
        
    case doSetIntersection2:
        
        setPushOp(setIntersection2);
        break;

    case doSetLiteral:
        
        
        
        
        
        {
            setEval(setUnion);
            UnicodeSet *s = (UnicodeSet *)fSetStack.peek();
            s->add(fC.fChar);
            fLastSetLiteral = fC.fChar;
            break;
        }

    case doSetLiteralEscaped:
        
        
        
        {
            if ((fModeFlags & UREGEX_ERROR_ON_UNKNOWN_ESCAPES) != 0 &&
                ((fC.fChar >= 0x41 && fC.fChar<= 0x5A) ||     
                 (fC.fChar >= 0x61 && fC.fChar <= 0x7a))) {   
                error(U_REGEX_BAD_ESCAPE_SEQUENCE);
            }
            setEval(setUnion);
            UnicodeSet *s = (UnicodeSet *)fSetStack.peek();
            s->add(fC.fChar);
            fLastSetLiteral = fC.fChar;
            break;
        }

        case doSetNamedChar:
        
        
        
        {
            UChar32  c = scanNamedChar();
            setEval(setUnion);
            UnicodeSet *s = (UnicodeSet *)fSetStack.peek();
            s->add(c);
            fLastSetLiteral = c;
            break;
        }

    case doSetNamedRange:
        
        
        
        
        
        {
            UChar32  c = scanNamedChar();
            if (U_SUCCESS(*fStatus) && fLastSetLiteral > c) {
                error(U_REGEX_INVALID_RANGE);
            }
            UnicodeSet *s = (UnicodeSet *)fSetStack.peek();
            s->add(fLastSetLiteral, c);
            fLastSetLiteral = c;
            break;
        }


    case  doSetNegate:
        
        
        
        
        
        
        
        {
            int32_t  tosOp = fSetOpStack.peeki();
            if (tosOp == setCaseClose) {
                fSetOpStack.popi();
                fSetOpStack.push(setNegation, *fStatus);
                fSetOpStack.push(setCaseClose, *fStatus);
            } else {
                fSetOpStack.push(setNegation, *fStatus);
            }
        }
        break;

    case doSetNoCloseError:
        error(U_REGEX_MISSING_CLOSE_BRACKET);
        break;

    case doSetOpError:
        error(U_REGEX_RULE_SYNTAX);   
        break;

    case doSetPosixProp:
        {
            UnicodeSet *s = scanPosixProp();
            if (s != NULL) {
                UnicodeSet *tos = (UnicodeSet *)fSetStack.peek();
                tos->addAll(*s);
                delete s;
            }  
        }
        break;
        
    case doSetProp:
        
        {
            UnicodeSet *s = scanProp();
            if (s != NULL) {
                UnicodeSet *tos = (UnicodeSet *)fSetStack.peek();
                tos->addAll(*s);
                delete s;
            }  
        }
        break;


    case doSetRange:
        
        
        
        
        
        {
        if (fLastSetLiteral > fC.fChar) {
            error(U_REGEX_INVALID_RANGE);  
        }
        UnicodeSet *s = (UnicodeSet *)fSetStack.peek();
        s->add(fLastSetLiteral, fC.fChar);
        break;
        }

    default:
        U_ASSERT(FALSE);
        error(U_REGEX_INTERNAL_ERROR);
        break;
    }

    if (U_FAILURE(*fStatus)) {
        returnVal = FALSE;
    }

    return returnVal;
}











void RegexCompile::literalChar(UChar32 c)  {
    fLiteralChars.append(c);
}














void    RegexCompile::fixLiterals(UBool split) {
    int32_t  op = 0;                       

    
    
    if (fLiteralChars.length() == 0) {
        return;
    }

    int32_t indexOfLastCodePoint = fLiteralChars.moveIndex32(fLiteralChars.length(), -1);
    UChar32 lastCodePoint = fLiteralChars.char32At(indexOfLastCodePoint);

    
    
    
    

    if (split) {
        fLiteralChars.truncate(indexOfLastCodePoint);
        fixLiterals(FALSE);   
                              
                              

        literalChar(lastCodePoint);  
        fixLiterals(FALSE);          
        return;
    }

    
    
    if (fModeFlags & UREGEX_CASE_INSENSITIVE) {
        fLiteralChars.foldCase();
        indexOfLastCodePoint = fLiteralChars.moveIndex32(fLiteralChars.length(), -1);
        lastCodePoint = fLiteralChars.char32At(indexOfLastCodePoint);
    }

    if (indexOfLastCodePoint == 0) {
        
        if ((fModeFlags & UREGEX_CASE_INSENSITIVE) && 
                 u_hasBinaryProperty(lastCodePoint, UCHAR_CASE_SENSITIVE)) {
            op = URX_BUILD(URX_ONECHAR_I, lastCodePoint);
        } else {
            op = URX_BUILD(URX_ONECHAR, lastCodePoint);
        }
        fRXPat->fCompiledPat->addElement(op, *fStatus);
    } else {
        
        if (fModeFlags & UREGEX_CASE_INSENSITIVE) {
            op = URX_BUILD(URX_STRING_I, fRXPat->fLiteralText.length());
        } else {
            
            
            op = URX_BUILD(URX_STRING, fRXPat->fLiteralText.length());
        }
        fRXPat->fCompiledPat->addElement(op, *fStatus);
        op = URX_BUILD(URX_STRING_LEN, fLiteralChars.length());
        fRXPat->fCompiledPat->addElement(op, *fStatus);
        
        
        fRXPat->fLiteralText.append(fLiteralChars);
    }

    fLiteralChars.remove();
}















void   RegexCompile::insertOp(int32_t where) {
    UVector64 *code = fRXPat->fCompiledPat;
    U_ASSERT(where>0 && where < code->size());

    int32_t  nop = URX_BUILD(URX_NOP, 0);
    code->insertElementAt(nop, where, *fStatus);

    
    
    int32_t loc;
    for (loc=0; loc<code->size(); loc++) {
        int32_t op = (int32_t)code->elementAti(loc);
        int32_t opType = URX_TYPE(op);
        int32_t opValue = URX_VAL(op);
        if ((opType == URX_JMP         ||
            opType == URX_JMPX         ||
            opType == URX_STATE_SAVE   ||
            opType == URX_CTR_LOOP     ||
            opType == URX_CTR_LOOP_NG  ||
            opType == URX_JMP_SAV      ||
            opType == URX_JMP_SAV_X    ||
            opType == URX_RELOC_OPRND)    && opValue > where) {
            
            
            opValue++;
            op = URX_BUILD(opType, opValue);
            code->setElementAt(op, loc);
        }
    }

    
    
    for (loc=0; loc<fParenStack.size(); loc++) {
        int32_t x = fParenStack.elementAti(loc);
        U_ASSERT(x < code->size());
        if (x>where) {
            x++;
            fParenStack.setElementAt(x, loc);
        }
    }

    if (fMatchCloseParen > where) {
        fMatchCloseParen++;
    }
    if (fMatchOpenParen > where) {
        fMatchOpenParen++;
    }
}





















int32_t   RegexCompile::blockTopLoc(UBool reserveLoc) {
    int32_t   theLoc;
    fixLiterals(TRUE);  
                        
    if (fRXPat->fCompiledPat->size() == fMatchCloseParen)
    {
        
        theLoc = fMatchOpenParen;   
        U_ASSERT(theLoc > 0);
        U_ASSERT(URX_TYPE(((uint32_t)fRXPat->fCompiledPat->elementAti(theLoc))) == URX_NOP);
    }
    else {
        
        
        
        theLoc = fRXPat->fCompiledPat->size()-1;
        int32_t opAtTheLoc = (int32_t)fRXPat->fCompiledPat->elementAti(theLoc);
        if (URX_TYPE(opAtTheLoc) == URX_STRING_LEN) {
            
            
            theLoc--;
        }
        if (reserveLoc) {
            int32_t  nop = URX_BUILD(URX_NOP, 0);
            fRXPat->fCompiledPat->insertElementAt(nop, theLoc, *fStatus);
        }
    }
    return theLoc;
}















void  RegexCompile::handleCloseParen() {
    int32_t   patIdx;
    int32_t   patOp;
    if (fParenStack.size() <= 0) {
        error(U_REGEX_MISMATCHED_PAREN);
        return;
    }

    
    fixLiterals(FALSE);

    
    
    
    
    for (;;) {
        patIdx = fParenStack.popi();
        if (patIdx < 0) {
            
            break;
        }
        U_ASSERT(patIdx>0 && patIdx <= fRXPat->fCompiledPat->size());
        patOp = (int32_t)fRXPat->fCompiledPat->elementAti(patIdx);
        U_ASSERT(URX_VAL(patOp) == 0);          
        patOp |= fRXPat->fCompiledPat->size();  
        fRXPat->fCompiledPat->setElementAt(patOp, patIdx);
        fMatchOpenParen     = patIdx;
    }

    
    
    
    fModeFlags = fParenStack.popi();
    U_ASSERT(fModeFlags < 0);

    
    

    switch (patIdx) {
    case plain:
    case flags:
        
        
        break;
    case capturing:
        
        
        
        
        {
            int32_t   captureOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen+1);
            U_ASSERT(URX_TYPE(captureOp) == URX_START_CAPTURE);

            int32_t   frameVarLocation = URX_VAL(captureOp);
            int32_t   endCaptureOp = URX_BUILD(URX_END_CAPTURE, frameVarLocation);
            fRXPat->fCompiledPat->addElement(endCaptureOp, *fStatus);
        }
        break;
    case atomic:
        
        
        
        {
            int32_t   stoOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen+1);
            U_ASSERT(URX_TYPE(stoOp) == URX_STO_SP);
            int32_t   stoLoc = URX_VAL(stoOp);
            int32_t   ldOp   = URX_BUILD(URX_LD_SP, stoLoc);
            fRXPat->fCompiledPat->addElement(ldOp, *fStatus);
        }
        break;

    case lookAhead:
        {
            int32_t  startOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen-5);
            U_ASSERT(URX_TYPE(startOp) == URX_LA_START);
            int32_t dataLoc  = URX_VAL(startOp);
            int32_t op       = URX_BUILD(URX_LA_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
        }
        break;

    case negLookAhead:
        {
            
            int32_t  startOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen-1);
            U_ASSERT(URX_TYPE(startOp) == URX_LA_START);
            int32_t dataLoc  = URX_VAL(startOp);
            int32_t op       = URX_BUILD(URX_LA_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            op               = URX_BUILD(URX_BACKTRACK, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
            op               = URX_BUILD(URX_LA_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            int32_t saveOp   = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen);
            U_ASSERT(URX_TYPE(saveOp) == URX_STATE_SAVE);
            int32_t dest     = fRXPat->fCompiledPat->size()-1;
            saveOp           = URX_BUILD(URX_STATE_SAVE, dest);
            fRXPat->fCompiledPat->setElementAt(saveOp, fMatchOpenParen);
        }
        break;

    case lookBehind:
        {
            

            
            int32_t  startOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen-4);
            U_ASSERT(URX_TYPE(startOp) == URX_LB_START);
            int32_t dataLoc  = URX_VAL(startOp);
            int32_t op       = URX_BUILD(URX_LB_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);
                    op       = URX_BUILD(URX_LA_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            
            int32_t patEnd   = fRXPat->fCompiledPat->size() - 1;
            int32_t minML    = minMatchLength(fMatchOpenParen, patEnd);
            int32_t maxML    = maxMatchLength(fMatchOpenParen, patEnd);
            if (maxML == INT32_MAX) {
                error(U_REGEX_LOOK_BEHIND_LIMIT);
                break;
            }
            U_ASSERT(minML <= maxML);

            
            
            fRXPat->fCompiledPat->setElementAt(minML,  fMatchOpenParen-2);
            fRXPat->fCompiledPat->setElementAt(maxML,  fMatchOpenParen-1);

        }
        break;



    case lookBehindN:
        {
            

            
            int32_t  startOp = (int32_t)fRXPat->fCompiledPat->elementAti(fMatchOpenParen-5);
            U_ASSERT(URX_TYPE(startOp) == URX_LB_START);
            int32_t dataLoc  = URX_VAL(startOp);
            int32_t op       = URX_BUILD(URX_LBN_END, dataLoc);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            
            
            
            int32_t patEnd   = fRXPat->fCompiledPat->size() - 1;
            int32_t minML    = minMatchLength(fMatchOpenParen, patEnd);
            int32_t maxML    = maxMatchLength(fMatchOpenParen, patEnd);
            if (maxML == INT32_MAX) {
                error(U_REGEX_LOOK_BEHIND_LIMIT);
                break;
            }
            U_ASSERT(minML <= maxML);

            
            
            fRXPat->fCompiledPat->setElementAt(minML,  fMatchOpenParen-3);
            fRXPat->fCompiledPat->setElementAt(maxML,  fMatchOpenParen-2);

            
            
            op = URX_BUILD(URX_RELOC_OPRND, fRXPat->fCompiledPat->size());
            fRXPat->fCompiledPat->setElementAt(op,  fMatchOpenParen-1);
        }
        break;



    default:
        U_ASSERT(FALSE);
    }

    
    
    
    fMatchCloseParen = fRXPat->fCompiledPat->size();
}









void        RegexCompile::compileSet(UnicodeSet *theSet)
{
    if (theSet == NULL) {
        return;
    }
    
    
    
    
    theSet->removeAllStrings();
    int32_t  setSize = theSet->size();

    switch (setSize) {
    case 0:
        {
            
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKTRACK, 0), *fStatus);
            delete theSet;
        }
        break;

    case 1:
        {
            
            
            
            literalChar(theSet->charAt(0));
            delete theSet;
        }
        break;

    default:
        {
            
            
            int32_t setNumber = fRXPat->fSets->size();
            fRXPat->fSets->addElement(theSet, *fStatus);
            int32_t setOp = URX_BUILD(URX_SETREF, setNumber);
            fRXPat->fCompiledPat->addElement(setOp, *fStatus);
        }
    }
}



















void        RegexCompile::compileInterval(int32_t InitOp,  int32_t LoopOp)
{
    
    
    int32_t   topOfBlock = blockTopLoc(TRUE);
    insertOp(topOfBlock);
    insertOp(topOfBlock);
    insertOp(topOfBlock);

    
    
    int32_t   counterLoc = fRXPat->fFrameSize;
    fRXPat->fFrameSize++;

    int32_t   op = URX_BUILD(InitOp, counterLoc);
    fRXPat->fCompiledPat->setElementAt(op, topOfBlock);

    
    
    
    
    int32_t loopEnd = fRXPat->fCompiledPat->size();
    op = URX_BUILD(URX_RELOC_OPRND, loopEnd);
    fRXPat->fCompiledPat->setElementAt(op, topOfBlock+1);

    
    fRXPat->fCompiledPat->setElementAt(fIntervalLow, topOfBlock+2);
    fRXPat->fCompiledPat->setElementAt(fIntervalUpper, topOfBlock+3);

    
    
    op = URX_BUILD(LoopOp, topOfBlock);
    fRXPat->fCompiledPat->addElement(op, *fStatus);

    if ((fIntervalLow & 0xff000000) != 0 ||
        (fIntervalUpper > 0 && (fIntervalUpper & 0xff000000) != 0)) {
            error(U_REGEX_NUMBER_TOO_BIG);
        }

    if (fIntervalLow > fIntervalUpper && fIntervalUpper != -1) {
        error(U_REGEX_MAX_LT_MIN);
    }
}



UBool RegexCompile::compileInlineInterval() {
    if (fIntervalUpper > 10 || fIntervalUpper < fIntervalLow) {
        
        
        return FALSE;
    }

    int32_t   topOfBlock = blockTopLoc(FALSE);
    if (fIntervalUpper == 0) {
        
        fRXPat->fCompiledPat->setSize(topOfBlock);
        return TRUE;
    }

    if (topOfBlock != fRXPat->fCompiledPat->size()-1 && fIntervalUpper != 1) {
        
        
        
        
        return FALSE;
    }

    
    
    int32_t op = (int32_t)fRXPat->fCompiledPat->elementAti(topOfBlock);

    
    
    
    int32_t endOfSequenceLoc = fRXPat->fCompiledPat->size()-1
                                + fIntervalUpper + (fIntervalUpper-fIntervalLow);
    int32_t saveOp = URX_BUILD(URX_STATE_SAVE, endOfSequenceLoc);
    if (fIntervalLow == 0) {
        insertOp(topOfBlock);
        fRXPat->fCompiledPat->setElementAt(saveOp, topOfBlock);
    }



    
    
    
    int32_t i;
    for (i=1; i<fIntervalUpper; i++ ) {
        if (i == fIntervalLow) {
            fRXPat->fCompiledPat->addElement(saveOp, *fStatus);
        }
        if (i > fIntervalLow) {
            fRXPat->fCompiledPat->addElement(saveOp, *fStatus);
        }
        fRXPat->fCompiledPat->addElement(op, *fStatus);
    }
    return TRUE;
}














void   RegexCompile::matchStartType() {
    if (U_FAILURE(*fStatus)) {
        return;
    }


    int32_t    loc;                    
    int32_t    op;                     
    int32_t    opType;                 
    int32_t    currentLen = 0;         
    int32_t    numInitialStrings = 0;  

    UBool      atStart = TRUE;         
                                       
                                       

    
    
    
    
    
    int32_t end = fRXPat->fCompiledPat->size();
    UVector32  forwardedLength(end+1, *fStatus);
    forwardedLength.setSize(end+1);
    for (loc=3; loc<end; loc++) {
        forwardedLength.setElementAt(INT32_MAX, loc);
    }

    for (loc = 3; loc<end; loc++) {
        op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
        opType = URX_TYPE(op);

        
        
        
        
        if (forwardedLength.elementAti(loc) < currentLen) {
            currentLen = forwardedLength.elementAti(loc);
            U_ASSERT(currentLen>=0 && currentLen < INT32_MAX);
        }

        switch (opType) {
            
        case URX_RESERVED_OP:
        case URX_END:
        case URX_FAIL:
        case URX_STRING_LEN:
        case URX_NOP:
        case URX_START_CAPTURE:
        case URX_END_CAPTURE:
        case URX_BACKSLASH_B:
        case URX_BACKSLASH_BU:
        case URX_BACKSLASH_G:
        case URX_BACKSLASH_Z:
        case URX_DOLLAR:
        case URX_DOLLAR_M:
        case URX_DOLLAR_D:
        case URX_DOLLAR_MD:
        case URX_RELOC_OPRND:
        case URX_STO_INP_LOC:
        case URX_BACKREF:         
        case URX_BACKREF_I:
                
        case URX_STO_SP:          
        case URX_LD_SP:
            break;

        case URX_CARET:
            if (atStart) {
                fRXPat->fStartType = START_START;
            }
            break;

        case URX_CARET_M:
        case URX_CARET_M_UNIX:
            if (atStart) {
                fRXPat->fStartType = START_LINE;
            }
            break;

        case URX_ONECHAR:
            if (currentLen == 0) {
                
                
                fRXPat->fInitialChars->add(URX_VAL(op));
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;


        case URX_SETREF:
            if (currentLen == 0) {
                int32_t  sn = URX_VAL(op);
                U_ASSERT(sn > 0 && sn < fRXPat->fSets->size());
                const UnicodeSet *s = (UnicodeSet *)fRXPat->fSets->elementAt(sn);
                fRXPat->fInitialChars->addAll(*s);
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;

        case URX_LOOP_SR_I:
            
            
            if (currentLen == 0) {
                int32_t  sn = URX_VAL(op);
                U_ASSERT(sn > 0 && sn < fRXPat->fSets->size());
                const UnicodeSet *s = (UnicodeSet *)fRXPat->fSets->elementAt(sn);
                fRXPat->fInitialChars->addAll(*s);
                numInitialStrings += 2;
            }
            atStart = FALSE;
            break;

        case URX_LOOP_DOT_I:
            if (currentLen == 0) {
                
                
                fRXPat->fInitialChars->clear();
                fRXPat->fInitialChars->complement();
                numInitialStrings += 2;
            }
            atStart = FALSE;
            break;


        case URX_STATIC_SETREF:
            if (currentLen == 0) {
                int32_t  sn = URX_VAL(op);
                U_ASSERT(sn>0 && sn<URX_LAST_SET);
                const UnicodeSet *s = fRXPat->fStaticSets[sn];
                fRXPat->fInitialChars->addAll(*s);
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;



        case URX_STAT_SETREF_N:
            if (currentLen == 0) {
                int32_t  sn = URX_VAL(op);
                const UnicodeSet *s = fRXPat->fStaticSets[sn];
                UnicodeSet sc(*s);
                sc.complement();
                fRXPat->fInitialChars->addAll(sc);
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;



        case URX_BACKSLASH_D:
            
             if (currentLen == 0) {
                 UnicodeSet s;
                 s.applyIntPropertyValue(UCHAR_GENERAL_CATEGORY_MASK, U_GC_ND_MASK, *fStatus);
                 if (URX_VAL(op) != 0) {
                     s.complement();
                 }
                 fRXPat->fInitialChars->addAll(s);
                 numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;


        case URX_ONECHAR_I:
            
            if (currentLen == 0) {
                UChar32  c = URX_VAL(op);
                if (u_hasBinaryProperty(c, UCHAR_CASE_SENSITIVE)) {

                    
                    
                    
                    
                    
                    
                    

                    fRXPat->fInitialChars->clear();
                    fRXPat->fInitialChars->complement();
                } else {
                    
                    
                    fRXPat->fInitialChars->add(c);
                }
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;


        case URX_BACKSLASH_X:   
        case URX_DOTANY_ALL:    
        case URX_DOTANY:
        case URX_DOTANY_UNIX:
            if (currentLen == 0) {
                
                
                fRXPat->fInitialChars->clear();
                fRXPat->fInitialChars->complement();
                numInitialStrings += 2;
            }
            currentLen++;
            atStart = FALSE;
            break;


        case URX_JMPX:
            loc++;             
        case URX_JMP:
            {
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest < loc) {
                    
                    
                    currentLen = forwardedLength.elementAti(loc+1);

                } else {
                    
                    U_ASSERT(jmpDest <= end+1);
                    if (forwardedLength.elementAti(jmpDest) > currentLen) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                }
            }
            atStart = FALSE;
            break;

        case URX_JMP_SAV:
        case URX_JMP_SAV_X:
            
            
            atStart = FALSE;
            break;

        case URX_BACKTRACK:
            
            
            currentLen = forwardedLength.elementAti(loc+1);
            atStart = FALSE;
            break;


        case URX_STATE_SAVE:
            {
                
                
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest > loc) {
                    if (currentLen < forwardedLength.elementAti(jmpDest)) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                }
            }
            atStart = FALSE;
            break;




        case URX_STRING:
            {
                loc++;
                int32_t stringLenOp = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                int32_t stringLen   = URX_VAL(stringLenOp);
                U_ASSERT(URX_TYPE(stringLenOp) == URX_STRING_LEN);
                U_ASSERT(stringLenOp >= 2);
                if (currentLen == 0) {
                    
                    
                    int32_t stringStartIdx = URX_VAL(op);
                    UChar32  c = fRXPat->fLiteralText.char32At(stringStartIdx);
                    fRXPat->fInitialChars->add(c);

                    
                    
                    numInitialStrings++;
                    fRXPat->fInitialStringIdx = stringStartIdx;
                    fRXPat->fInitialStringLen = stringLen;
                }

                currentLen += stringLen;
                atStart = FALSE;
            }
            break;

        case URX_STRING_I:
            {
                
                
                
                loc++;
                int32_t stringLenOp = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                int32_t stringLen   = URX_VAL(stringLenOp);
                U_ASSERT(URX_TYPE(stringLenOp) == URX_STRING_LEN);
                U_ASSERT(stringLenOp >= 2);
                if (currentLen == 0) {
                    
                    
                    int32_t stringStartIdx = URX_VAL(op);
                    UChar32  c = fRXPat->fLiteralText.char32At(stringStartIdx);
                    UnicodeSet s(c, c);

                    
                    
                    
                    s.clear();
                    s.complement();

                    fRXPat->fInitialChars->addAll(s);
                    numInitialStrings += 2;  
                }
                currentLen += stringLen;
                atStart = FALSE;
            }
            break;

        case URX_CTR_INIT:
        case URX_CTR_INIT_NG:
            {
                
                
                
                
                
                
                
                int32_t loopEndLoc   = (int32_t)fRXPat->fCompiledPat->elementAti(loc+1);
                        loopEndLoc   = URX_VAL(loopEndLoc);
                int32_t minLoopCount = (int32_t)fRXPat->fCompiledPat->elementAti(loc+2);
                if (minLoopCount == 0) {
                    
                    
                    
                    U_ASSERT(loopEndLoc <= end+1);
                    if (forwardedLength.elementAti(loopEndLoc) > currentLen) {
                        forwardedLength.setElementAt(currentLen, loopEndLoc);
                    }
                }
                loc+=3;  
            }
            atStart = FALSE;
            break;


        case URX_CTR_LOOP:
        case URX_CTR_LOOP_NG:
            
            
            atStart = FALSE;
            break;

        case URX_LOOP_C:
            
            
            atStart = FALSE;
            break;


        case URX_LA_START:
        case URX_LB_START:
            {
                
                
                
                
                
                
                int32_t  depth = (opType == URX_LA_START? 2: 1);
                for (;;) {
                    loc++;
                    op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                    if (URX_TYPE(op) == URX_LA_START) {
                        depth+=2;
                    }
                    if (URX_TYPE(op) == URX_LB_START) {
                        depth++;
                    }
                    if (URX_TYPE(op) == URX_LA_END || URX_TYPE(op)==URX_LBN_END) {
                        depth--;
                        if (depth == 0) {
                            break;
                        }
                    }
                    if (URX_TYPE(op) == URX_STATE_SAVE) {
                        
                        
                        int32_t  jmpDest = URX_VAL(op);
                        if (jmpDest > loc) {
                            if (currentLen < forwardedLength.elementAti(jmpDest)) {
                                forwardedLength.setElementAt(currentLen, jmpDest);
                            }
                        }
                    }
                    U_ASSERT(loc <= end);
                }
            }
            break;

        case URX_LA_END:
        case URX_LB_CONT:
        case URX_LB_END:
        case URX_LBN_CONT:
        case URX_LBN_END:
            U_ASSERT(FALSE);     
                                 

            break;

        default:
            U_ASSERT(FALSE);
            }

        }


    
    
    if (forwardedLength.elementAti(end+1) < currentLen) {
        currentLen = forwardedLength.elementAti(end+1);
    }


    fRXPat->fInitialChars8->init(fRXPat->fInitialChars);


    
    
    
    
    
    
    
    
    if (fRXPat->fStartType == START_START) {
        
        
    } else if (numInitialStrings == 1 && fRXPat->fMinMatchLen > 0) {
        
        UChar32  c = fRXPat->fLiteralText.char32At(fRXPat->fInitialStringIdx);
        U_ASSERT(fRXPat->fInitialChars->contains(c));
        fRXPat->fStartType   = START_STRING;
        fRXPat->fInitialChar = c;
    } else if (fRXPat->fStartType == START_LINE) {
        
        
    } else if (fRXPat->fMinMatchLen == 0) {
        
        fRXPat->fStartType = START_NO_INFO;
    } else if (fRXPat->fInitialChars->size() == 1) {
        
        fRXPat->fStartType   = START_CHAR;
        fRXPat->fInitialChar = fRXPat->fInitialChars->charAt(0);
        U_ASSERT(fRXPat->fInitialChar != (UChar32)-1);
    } else if (fRXPat->fInitialChars->contains((UChar32)0, (UChar32)0x10ffff) == FALSE &&
        fRXPat->fMinMatchLen > 0) {
        
        fRXPat->fStartType = START_SET;
    } else {
        
        fRXPat->fStartType = START_NO_INFO;
    }

    return;
}

















int32_t   RegexCompile::minMatchLength(int32_t start, int32_t end) {
    if (U_FAILURE(*fStatus)) {
        return 0;
    }

    U_ASSERT(start <= end);
    U_ASSERT(end < fRXPat->fCompiledPat->size());


    int32_t    loc;
    int32_t    op;
    int32_t    opType;
    int32_t    currentLen = 0;


    
    
    
    
    
    UVector32  forwardedLength(end+2, *fStatus);
    forwardedLength.setSize(end+2);
    for (loc=start; loc<=end+1; loc++) {
        forwardedLength.setElementAt(INT32_MAX, loc);
    }

    for (loc = start; loc<=end; loc++) {
        op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
        opType = URX_TYPE(op);

        
        
        
        
        
                                                               
        if (forwardedLength.elementAti(loc) < currentLen) {
            currentLen = forwardedLength.elementAti(loc);
            U_ASSERT(currentLen>=0 && currentLen < INT32_MAX);
        }

        switch (opType) {
            
        case URX_RESERVED_OP:
        case URX_END:
        case URX_STRING_LEN:
        case URX_NOP:
        case URX_START_CAPTURE:
        case URX_END_CAPTURE:
        case URX_BACKSLASH_B:
        case URX_BACKSLASH_BU:
        case URX_BACKSLASH_G:
        case URX_BACKSLASH_Z:
        case URX_CARET:
        case URX_DOLLAR:
        case URX_DOLLAR_M:
        case URX_DOLLAR_D:
        case URX_DOLLAR_MD:
        case URX_RELOC_OPRND:
        case URX_STO_INP_LOC:
        case URX_CARET_M:
        case URX_CARET_M_UNIX:
        case URX_BACKREF:         
        case URX_BACKREF_I:

        case URX_STO_SP:          
        case URX_LD_SP:

        case URX_JMP_SAV:
        case URX_JMP_SAV_X:
            break;


            
            
        case URX_ONECHAR:
        case URX_STATIC_SETREF:
        case URX_STAT_SETREF_N:
        case URX_SETREF:
        case URX_BACKSLASH_D:
        case URX_ONECHAR_I:
        case URX_BACKSLASH_X:   
        case URX_DOTANY_ALL:    
        case URX_DOTANY:
        case URX_DOTANY_UNIX:
            currentLen++;
            break;


        case URX_JMPX:
            loc++;              
                                
        case URX_JMP:
            {
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest < loc) {
                    
                    
                    currentLen = forwardedLength.elementAti(loc+1);
                } else {
                    
                    U_ASSERT(jmpDest <= end+1);
                    if (forwardedLength.elementAti(jmpDest) > currentLen) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                }
            }
            break;

        case URX_BACKTRACK:
            {
                
                
                currentLen = forwardedLength.elementAti(loc+1);
            }
            break;


        case URX_STATE_SAVE:
            {
                
                
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest > loc) {
                    if (currentLen < forwardedLength.elementAti(jmpDest)) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                }
            }
            break;


        case URX_STRING:
            {
                loc++;
                int32_t stringLenOp = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                currentLen += URX_VAL(stringLenOp);
            }
            break;


        case URX_STRING_I:
            {
                loc++;
                
                
                
                
                
                currentLen += 1;
            }
            break;

        case URX_CTR_INIT:
        case URX_CTR_INIT_NG:
            {
                
                
                
                
                
                int32_t loopEndLoc   = (int32_t)fRXPat->fCompiledPat->elementAti(loc+1);
                        loopEndLoc   = URX_VAL(loopEndLoc);
                int32_t minLoopCount = (int32_t)fRXPat->fCompiledPat->elementAti(loc+2);
                if (minLoopCount == 0) {
                    loc = loopEndLoc;
                } else {
                    loc+=3;  
                }
            }
            break;


        case URX_CTR_LOOP:
        case URX_CTR_LOOP_NG:
            
            
            break;

        case URX_LOOP_SR_I:
        case URX_LOOP_DOT_I:
        case URX_LOOP_C:
            
            
            break;


        case URX_LA_START:
        case URX_LB_START:
            {
                
                
                
                
                
                int32_t  depth = (opType == URX_LA_START? 2: 1);;
                for (;;) {
                    loc++;
                    op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                    if (URX_TYPE(op) == URX_LA_START) {
                        
                        
                        depth += 2;
                    }
                    if (URX_TYPE(op) == URX_LB_START) {
                        depth++;
                    }
                    if (URX_TYPE(op) == URX_LA_END) {
                        depth--;
                        if (depth == 0) {
                            break;
                        }
                    }
                    if (URX_TYPE(op)==URX_LBN_END) {
                        depth--;
                        if (depth == 0) {
                            break;
                        }
                    }
                    if (URX_TYPE(op) == URX_STATE_SAVE) {
                        
                        
                        int32_t  jmpDest = URX_VAL(op);
                        if (jmpDest > loc) {
                            if (currentLen < forwardedLength.elementAti(jmpDest)) {
                                forwardedLength.setElementAt(currentLen, jmpDest);
                            }
                        }
                    }
                    U_ASSERT(loc <= end);
                }
            }
            break;

        case URX_LA_END:
        case URX_LB_CONT:
        case URX_LB_END:
        case URX_LBN_CONT:
        case URX_LBN_END:
            
            
            break;

        default:
            U_ASSERT(FALSE);
            }

        }

    
    
    if (forwardedLength.elementAti(end+1) < currentLen) {
        currentLen = forwardedLength.elementAti(end+1);
        U_ASSERT(currentLen>=0 && currentLen < INT32_MAX);
    }

    return currentLen;
}




static int32_t safeIncrement(int32_t val, int32_t delta) {
    if (INT32_MAX - val > delta) {
        return val + delta;
    } else {
        return INT32_MAX;
    }
}













int32_t   RegexCompile::maxMatchLength(int32_t start, int32_t end) {
    if (U_FAILURE(*fStatus)) {
        return 0;
    }
    U_ASSERT(start <= end);
    U_ASSERT(end < fRXPat->fCompiledPat->size());


    int32_t    loc;
    int32_t    op;
    int32_t    opType;
    int32_t    currentLen = 0;
    UVector32  forwardedLength(end+1, *fStatus);
    forwardedLength.setSize(end+1);

    for (loc=start; loc<=end; loc++) {
        forwardedLength.setElementAt(0, loc);
    }

    for (loc = start; loc<=end; loc++) {
        op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
        opType = URX_TYPE(op);

        
        
        
        
        if (forwardedLength.elementAti(loc) > currentLen) {
            currentLen = forwardedLength.elementAti(loc);
        }

        switch (opType) {
            
        case URX_RESERVED_OP:
        case URX_END:
        case URX_STRING_LEN:
        case URX_NOP:
        case URX_START_CAPTURE:
        case URX_END_CAPTURE:
        case URX_BACKSLASH_B:
        case URX_BACKSLASH_BU:
        case URX_BACKSLASH_G:
        case URX_BACKSLASH_Z:
        case URX_CARET:
        case URX_DOLLAR:
        case URX_DOLLAR_M:
        case URX_DOLLAR_D:
        case URX_DOLLAR_MD:
        case URX_RELOC_OPRND:
        case URX_STO_INP_LOC:
        case URX_CARET_M:
        case URX_CARET_M_UNIX:

        case URX_STO_SP:          
        case URX_LD_SP:

        case URX_LB_END:
        case URX_LB_CONT:
        case URX_LBN_CONT:
        case URX_LBN_END:
            break;


            
            
            
        case URX_BACKREF:         
        case URX_BACKREF_I:
        case URX_BACKSLASH_X:   
            currentLen = INT32_MAX;
            break;


            
            
        case URX_STATIC_SETREF:
        case URX_STAT_SETREF_N:
        case URX_SETREF:
        case URX_BACKSLASH_D:
        case URX_ONECHAR_I:
        case URX_DOTANY_ALL:
        case URX_DOTANY:
        case URX_DOTANY_UNIX:
            currentLen = safeIncrement(currentLen, 2);
            break;

            
            
        case URX_ONECHAR:
            currentLen = safeIncrement(currentLen, 1);
            if (URX_VAL(op) > 0x10000) {
                currentLen = safeIncrement(currentLen, 1);
            }
            break;

            
            
        case URX_JMP:
        case URX_JMPX:
        case URX_JMP_SAV:
        case URX_JMP_SAV_X:
            {
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest < loc) {
                    
                    currentLen = INT32_MAX;
                } else {
                    
                    if (forwardedLength.elementAti(jmpDest) < currentLen) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                    currentLen = 0;
                }
            }
            break;

        case URX_BACKTRACK:
            
            
            currentLen = forwardedLength.elementAti(loc+1);
            break;


        case URX_STATE_SAVE:
            {
                
                
                
                
                int32_t  jmpDest = URX_VAL(op);
                if (jmpDest > loc) {
                    if (currentLen > forwardedLength.elementAti(jmpDest)) {
                        forwardedLength.setElementAt(currentLen, jmpDest);
                    }
                } else {
                    currentLen = INT32_MAX;
                }
            }
            break;




        case URX_STRING:
            {
                loc++;
                int32_t stringLenOp = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                currentLen = safeIncrement(currentLen, URX_VAL(stringLenOp));
                break;
            }

        case URX_STRING_I:
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            {
                loc++;
                int32_t stringLenOp = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                currentLen = safeIncrement(currentLen, URX_VAL(stringLenOp));
            }
            break;

        case URX_CTR_INIT:
        case URX_CTR_INIT_NG:
        case URX_CTR_LOOP:
        case URX_CTR_LOOP_NG:
        case URX_LOOP_SR_I:
        case URX_LOOP_DOT_I:
        case URX_LOOP_C:
            
            
            
            currentLen = INT32_MAX;
            break;



        case URX_LA_START:
        case URX_LA_END:
            
            
            
            break;

            
            
            
            

        case URX_LB_START:
            {
                
                
                int32_t  depth = 0;
                for (;;) {
                    loc++;
                    op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
                    if (URX_TYPE(op) == URX_LA_START || URX_TYPE(op) == URX_LB_START) {
                        depth++;
                    }
                    if (URX_TYPE(op) == URX_LA_END || URX_TYPE(op)==URX_LBN_END) {
                        if (depth == 0) {
                            break;
                        }
                        depth--;
                    }
                    U_ASSERT(loc < end);
                }
            }
            break;

        default:
            U_ASSERT(FALSE);
        }


        if (currentLen == INT32_MAX) {
            
            
            break;
        }

    }
    return currentLen;

}














void RegexCompile::stripNOPs() {

    if (U_FAILURE(*fStatus)) {
        return;
    }

    int32_t    end = fRXPat->fCompiledPat->size();
    UVector32  deltas(end, *fStatus);

    
    
    int32_t   loc;
    int32_t   d = 0;
    for (loc=0; loc<end; loc++) {
        deltas.addElement(d, *fStatus);
        int32_t op = (int32_t)fRXPat->fCompiledPat->elementAti(loc);
        if (URX_TYPE(op) == URX_NOP) {
            d++;
        }
    }
    
    UnicodeString caseStringBuffer;

    
    
    
    
    int32_t src;
    int32_t dst = 0;
    for (src=0; src<end; src++) {
        int32_t op = (int32_t)fRXPat->fCompiledPat->elementAti(src);
        int32_t opType = URX_TYPE(op);
        switch (opType) {
        case URX_NOP:
            break;

        case URX_STATE_SAVE:
        case URX_JMP:
        case URX_CTR_LOOP:
        case URX_CTR_LOOP_NG:
        case URX_RELOC_OPRND:
        case URX_JMPX:
        case URX_JMP_SAV:
        case URX_JMP_SAV_X:
            
            {
                int32_t  operandAddress = URX_VAL(op);
                U_ASSERT(operandAddress>=0 && operandAddress<deltas.size());
                int32_t fixedOperandAddress = operandAddress - deltas.elementAti(operandAddress);
                op = URX_BUILD(opType, fixedOperandAddress);
                fRXPat->fCompiledPat->setElementAt(op, dst);
                dst++;
                break;
            }

        case URX_BACKREF:
        case URX_BACKREF_I:
            {
                int32_t where = URX_VAL(op);
                if (where > fRXPat->fGroupMap->size()) {
                    error(U_REGEX_INVALID_BACK_REF);
                    break;
                }
                where = fRXPat->fGroupMap->elementAti(where-1);
                op    = URX_BUILD(opType, where);
                fRXPat->fCompiledPat->setElementAt(op, dst);
                dst++;
                
                fRXPat->fNeedsAltInput = TRUE;
                break;
            }
        case URX_RESERVED_OP:
        case URX_RESERVED_OP_N:
        case URX_BACKTRACK:
        case URX_END:
        case URX_ONECHAR:
        case URX_STRING:
        case URX_STRING_LEN:
        case URX_START_CAPTURE:
        case URX_END_CAPTURE:
        case URX_STATIC_SETREF:
        case URX_STAT_SETREF_N:
        case URX_SETREF:
        case URX_DOTANY:
        case URX_FAIL:
        case URX_BACKSLASH_B:
        case URX_BACKSLASH_BU:
        case URX_BACKSLASH_G:
        case URX_BACKSLASH_X:
        case URX_BACKSLASH_Z:
        case URX_DOTANY_ALL:
        case URX_BACKSLASH_D:
        case URX_CARET:
        case URX_DOLLAR:
        case URX_CTR_INIT:
        case URX_CTR_INIT_NG:
        case URX_DOTANY_UNIX:
        case URX_STO_SP:
        case URX_LD_SP:
        case URX_STO_INP_LOC:
        case URX_LA_START:
        case URX_LA_END:
        case URX_ONECHAR_I:
        case URX_STRING_I:
        case URX_DOLLAR_M:
        case URX_CARET_M:
        case URX_CARET_M_UNIX:
        case URX_LB_START:
        case URX_LB_CONT:
        case URX_LB_END:
        case URX_LBN_CONT:
        case URX_LBN_END:
        case URX_LOOP_SR_I:
        case URX_LOOP_DOT_I:
        case URX_LOOP_C:
        case URX_DOLLAR_D:
        case URX_DOLLAR_MD:
            
            fRXPat->fCompiledPat->setElementAt(op, dst);
            dst++;
            break;

        default:
            
            U_ASSERT(FALSE);
            error(U_REGEX_INTERNAL_ERROR);
        }
    }

    fRXPat->fCompiledPat->setSize(dst);
}










void RegexCompile::error(UErrorCode e) {
    if (U_SUCCESS(*fStatus)) {
        *fStatus = e;
        
        
        
        
        if (fLineNum > 0x7FFFFFFF) {
            fParseErr->line   = 0;
            fParseErr->offset = -1;
        } else if (fCharNum > 0x7FFFFFFF) {
            fParseErr->line   = (int32_t)fLineNum;
            fParseErr->offset = -1;
        } else {
            fParseErr->line   = (int32_t)fLineNum;
            fParseErr->offset = (int32_t)fCharNum;
        }
        
        UErrorCode status = U_ZERO_ERROR; 

        
        
        uprv_memset(fParseErr->preContext,  0, sizeof(fParseErr->preContext));
        uprv_memset(fParseErr->postContext, 0, sizeof(fParseErr->postContext));
        utext_extract(fRXPat->fPattern, fScanIndex-U_PARSE_CONTEXT_LEN+1, fScanIndex, fParseErr->preContext, U_PARSE_CONTEXT_LEN, &status);
        utext_extract(fRXPat->fPattern, fScanIndex, fScanIndex+U_PARSE_CONTEXT_LEN-1, fParseErr->postContext, U_PARSE_CONTEXT_LEN, &status);
    }
}







static const UChar      chCR        = 0x0d;      
static const UChar      chLF        = 0x0a;      
static const UChar      chPound     = 0x23;      
static const UChar      chDigit0    = 0x30;      
static const UChar      chDigit7    = 0x37;      
static const UChar      chColon     = 0x3A;      
static const UChar      chE         = 0x45;      
static const UChar      chQ         = 0x51;      

static const UChar      chP         = 0x50;      
static const UChar      chBackSlash = 0x5c;      

static const UChar      chRBracket  = 0x5d;      
static const UChar      chUp        = 0x5e;      
static const UChar      chLowerP    = 0x70;
static const UChar      chLBrace    = 0x7b;      
static const UChar      chRBrace    = 0x7d;      
static const UChar      chNEL       = 0x85;      
static const UChar      chLS        = 0x2028;    









UChar32  RegexCompile::nextCharLL() {
    UChar32       ch;

    if (fPeekChar != -1) {
        ch = fPeekChar;
        fPeekChar = -1;
        return ch;
    }
    
    
    ch = UTEXT_NEXT32(fRXPat->fPattern);
    if (ch == U_SENTINEL) {
        return ch;
    }

    if (ch == chCR ||
        ch == chNEL ||
        ch == chLS   ||
        (ch == chLF && fLastChar != chCR)) {
        
        
        fLineNum++;
        fCharNum=0;
    }
    else {
        
        
        if (ch != chLF) {
            fCharNum++;
        }
    }
    fLastChar = ch;
    return ch;
}







UChar32  RegexCompile::peekCharLL() {
    if (fPeekChar == -1) {
        fPeekChar = nextCharLL();
    }
    return fPeekChar;
}









void RegexCompile::nextChar(RegexPatternChar &c) {

    fScanIndex = UTEXT_GETNATIVEINDEX(fRXPat->fPattern);
    c.fChar    = nextCharLL();
    c.fQuoted  = FALSE;

    if (fQuoteMode) {
        c.fQuoted = TRUE;
        if ((c.fChar==chBackSlash && peekCharLL()==chE && ((fModeFlags & UREGEX_LITERAL) == 0)) || 
            c.fChar == (UChar32)-1) {
            fQuoteMode = FALSE;  
            nextCharLL();        
            nextChar(c);         
        }
    }
    else if (fInBackslashQuote) {
        
        
        
        
        fInBackslashQuote = FALSE;
    }
    else
    {
        
        
        if (fModeFlags & UREGEX_COMMENTS) {
            
            
            
            
            for (;;) {
                if (c.fChar == (UChar32)-1) {
                    break;     
                }
                if  (c.fChar == chPound && fEOLComments == TRUE) {
                    
                    for (;;) {
                        c.fChar = nextCharLL();
                        if (c.fChar == (UChar32)-1 ||  
                            c.fChar == chCR        ||
                            c.fChar == chLF        ||
                            c.fChar == chNEL       ||
                            c.fChar == chLS)       {
                            break;
                        }
                    }
                }
                
                if (PatternProps::isWhiteSpace(c.fChar) == FALSE) {
                    break;
                }
                c.fChar = nextCharLL();
            }
        }

        
        
        
        if (c.fChar == chBackSlash) {
            int64_t pos = UTEXT_GETNATIVEINDEX(fRXPat->fPattern);
            if (RegexStaticSets::gStaticSets->fUnescapeCharSet.contains(peekCharLL())) {
                
                
                
                
                
                nextCharLL();                 
                c.fQuoted = TRUE;
                
                if (UTEXT_FULL_TEXT_IN_CHUNK(fRXPat->fPattern, fPatternLength)) {
                    int32_t endIndex = (int32_t)pos;
                    c.fChar = u_unescapeAt(uregex_ucstr_unescape_charAt, &endIndex, (int32_t)fPatternLength, (void *)fRXPat->fPattern->chunkContents);
                    
                    if (endIndex == pos) {
                        error(U_REGEX_BAD_ESCAPE_SEQUENCE);
                    }
                    fCharNum += endIndex - pos;
                    UTEXT_SETNATIVEINDEX(fRXPat->fPattern, endIndex);
                } else {
                    int32_t offset = 0;
                    struct URegexUTextUnescapeCharContext context = U_REGEX_UTEXT_UNESCAPE_CONTEXT(fRXPat->fPattern);
                    
                    UTEXT_SETNATIVEINDEX(fRXPat->fPattern, pos);
                    c.fChar = u_unescapeAt(uregex_utext_unescape_charAt, &offset, INT32_MAX, &context);

                    if (offset == 0) {
                        error(U_REGEX_BAD_ESCAPE_SEQUENCE);
                    } else if (context.lastOffset == offset) {
                        UTEXT_PREVIOUS32(fRXPat->fPattern);
                    } else if (context.lastOffset != offset-1) {
                        utext_moveIndex32(fRXPat->fPattern, offset - context.lastOffset - 1);
                    }
                    fCharNum += offset;
                }
            }
            else if (peekCharLL() == chDigit0) {
                
                
                
                
                
                
                
                c.fChar = 0;
                nextCharLL();    
                int index;
                for (index=0; index<3; index++) {
                    int32_t ch = peekCharLL();
                    if (ch<chDigit0 || ch>chDigit7) {
                        if (index==0) {
                           
                           error(U_REGEX_BAD_ESCAPE_SEQUENCE);
                        }
                        break;
                    }
                    c.fChar <<= 3;
                    c.fChar += ch&7;
                    if (c.fChar <= 255) {
                        nextCharLL();
                    } else {
                        
                        c.fChar >>= 3;
                    }
                }
                c.fQuoted = TRUE; 
            } 
            else if (peekCharLL() == chQ) {
                
                fQuoteMode = TRUE;
                nextCharLL();       
                nextChar(c);        
            }
            else
            {
                
                
                
                fInBackslashQuote = TRUE;
            }
        }
    }

    
    
    
    fEOLComments = TRUE;

    
}






 







UChar32  RegexCompile::scanNamedChar() {
    if (U_FAILURE(*fStatus)) {
        return 0;
    }

    nextChar(fC);
    if (fC.fChar != chLBrace) {
        error(U_REGEX_PROPERTY_SYNTAX);
        return 0;
    }
    
    UnicodeString  charName;
    for (;;) {
        nextChar(fC);
        if (fC.fChar == chRBrace) {
            break;
        }
        if (fC.fChar == -1) {
            error(U_REGEX_PROPERTY_SYNTAX);
            return 0;
        }
        charName.append(fC.fChar);
    }
    
    char name[100];
    if (!uprv_isInvariantUString(charName.getBuffer(), charName.length()) ||
         (uint32_t)charName.length()>=sizeof(name)) {
        
        
        
        error(U_REGEX_PROPERTY_SYNTAX);
        return 0;
    }
    charName.extract(0, charName.length(), name, sizeof(name), US_INV);

    UChar32  theChar = u_charFromName(U_UNICODE_CHAR_NAME, name, fStatus);
    if (U_FAILURE(*fStatus)) {
        error(U_REGEX_PROPERTY_SYNTAX);
    }

    nextChar(fC);      
    return theChar;
}













UnicodeSet *RegexCompile::scanProp() {
    UnicodeSet    *uset = NULL;

    if (U_FAILURE(*fStatus)) {
        return NULL;
    }
    U_ASSERT(fC.fChar == chLowerP || fC.fChar == chP);
    UBool negated = (fC.fChar == chP);

    UnicodeString propertyName;
    nextChar(fC);
    if (fC.fChar != chLBrace) {
        error(U_REGEX_PROPERTY_SYNTAX);
        return NULL;
    }
    for (;;) {
        nextChar(fC);
        if (fC.fChar == chRBrace) {
            break;
        }
        if (fC.fChar == -1) {
            
            error(U_REGEX_PROPERTY_SYNTAX);
            return NULL;
        }
        propertyName.append(fC.fChar);
    }
    uset = createSetForProperty(propertyName, negated);
    nextChar(fC);    
    return uset;
}




















UnicodeSet *RegexCompile::scanPosixProp() {
    UnicodeSet    *uset = NULL;

    if (U_FAILURE(*fStatus)) {
        return NULL;
    }

    U_ASSERT(fC.fChar == chColon);

    
    
    int64_t     savedScanIndex        = fScanIndex;
    int64_t     savedNextIndex        = UTEXT_GETNATIVEINDEX(fRXPat->fPattern);
    UBool       savedQuoteMode        = fQuoteMode;
    UBool       savedInBackslashQuote = fInBackslashQuote;
    UBool       savedEOLComments      = fEOLComments;
    int64_t     savedLineNum          = fLineNum;
    int64_t     savedCharNum          = fCharNum;
    UChar32     savedLastChar         = fLastChar;
    UChar32     savedPeekChar         = fPeekChar;
    RegexPatternChar savedfC          = fC;

    
    
    

    UnicodeString propName;
    UBool         negated  = FALSE;

    
    nextChar(fC);
    if (fC.fChar == chUp) {
       negated = TRUE;
       nextChar(fC);
    }
    
    
    UBool  sawPropSetTerminator = FALSE;
    for (;;) {
        propName.append(fC.fChar);
        nextChar(fC);
        if (fC.fQuoted || fC.fChar == -1) {
            
            break;
        }
        if (fC.fChar == chColon) {
            nextChar(fC);
            if (fC.fChar == chRBracket) {
                sawPropSetTerminator = TRUE;
            }
            break;
        }
    }
    
    if (sawPropSetTerminator) {
        uset = createSetForProperty(propName, negated);
    }
    else
    {
        
        
        
        
        fScanIndex        = savedScanIndex;
        fQuoteMode        = savedQuoteMode;
        fInBackslashQuote = savedInBackslashQuote;
        fEOLComments      = savedEOLComments;
        fLineNum          = savedLineNum;
        fCharNum          = savedCharNum;
        fLastChar         = savedLastChar;
        fPeekChar         = savedPeekChar;
        fC                = savedfC;
        UTEXT_SETNATIVEINDEX(fRXPat->fPattern, savedNextIndex);
    }
    return uset;
}

static inline void addIdentifierIgnorable(UnicodeSet *set, UErrorCode& ec) {
    set->add(0, 8).add(0x0e, 0x1b).add(0x7f, 0x9f);
    addCategory(set, U_GC_CF_MASK, ec);
}







static const UChar posSetPrefix[] = {0x5b, 0x5c, 0x70, 0x7b, 0}; 
static const UChar negSetPrefix[] = {0x5b, 0x5c, 0x50, 0x7b, 0}; 
UnicodeSet *RegexCompile::createSetForProperty(const UnicodeString &propName, UBool negated) {
    UnicodeString   setExpr;
    UnicodeSet      *set;
    uint32_t        usetFlags = 0;
    
    if (U_FAILURE(*fStatus)) {
        return NULL;
    }

    
    
    
    if (negated) {
        setExpr.append(negSetPrefix, -1);
    } else {
        setExpr.append(posSetPrefix, -1);
    }
    setExpr.append(propName);
    setExpr.append(chRBrace);
    setExpr.append(chRBracket);
    if (fModeFlags & UREGEX_CASE_INSENSITIVE) {
        usetFlags |= USET_CASE_INSENSITIVE;
    }
    set = new UnicodeSet(setExpr, usetFlags, NULL, *fStatus);
    if (U_SUCCESS(*fStatus)) {
       return set;
    }
    delete set;
    set = NULL;
    
    
    

    
    
    
    if (propName.caseCompare(UNICODE_STRING_SIMPLE("word"), 0) == 0) {
        *fStatus = U_ZERO_ERROR;
        set = new UnicodeSet(*(fRXPat->fStaticSets[URX_ISWORD_SET]));
        if (set == NULL) {
            *fStatus = U_MEMORY_ALLOCATION_ERROR;
            return set;
        }
        if (negated) {
            set->complement();
        }
        return set;
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    UnicodeString mPropName = propName;
    if (mPropName.caseCompare(UNICODE_STRING_SIMPLE("InGreek"), 0) == 0) {
        mPropName = UNICODE_STRING_SIMPLE("InGreek and Coptic");
    }
    if (mPropName.caseCompare(UNICODE_STRING_SIMPLE("InCombining Marks for Symbols"), 0) == 0 ||
        mPropName.caseCompare(UNICODE_STRING_SIMPLE("InCombiningMarksforSymbols"), 0) == 0) {
        mPropName = UNICODE_STRING_SIMPLE("InCombining Diacritical Marks for Symbols");
    }
    else if (mPropName.compare(UNICODE_STRING_SIMPLE("all")) == 0) {
        mPropName = UNICODE_STRING_SIMPLE("javaValidCodePoint");
    }
    
    
    
    
    static const UChar IN[] = {0x49, 0x6E, 0};  
    static const UChar BLOCK[] = {0x42, 0x6C, 0x6f, 0x63, 0x6b, 0x3d, 00};  
    if (mPropName.startsWith(IN, 2) && propName.length()>=3) {
        setExpr.truncate(4);   
        setExpr.append(BLOCK, -1);
        setExpr.append(UnicodeString(mPropName, 2));  
        setExpr.append(chRBrace);
        setExpr.append(chRBracket);
        *fStatus = U_ZERO_ERROR;
        set = new UnicodeSet(setExpr, usetFlags, NULL, *fStatus);
        if (U_SUCCESS(*fStatus)) {
            return set;
        }
        delete set;
        set = NULL;
    }

    if (propName.startsWith(UNICODE_STRING_SIMPLE("java")) ||
        propName.compare(UNICODE_STRING_SIMPLE("all")) == 0)
    {
        UErrorCode localStatus = U_ZERO_ERROR;
        
        set = new UnicodeSet();
        
        
        
        
        if (mPropName.compare(UNICODE_STRING_SIMPLE("javaDefined")) == 0) {
            addCategory(set, U_GC_CN_MASK, localStatus);
            set->complement();
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaDigit")) == 0) {
            addCategory(set, U_GC_ND_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaIdentifierIgnorable")) == 0) {
            addIdentifierIgnorable(set, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaISOControl")) == 0) {
            set->add(0, 0x1F).add(0x7F, 0x9F);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaJavaIdentifierPart")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
            addCategory(set, U_GC_SC_MASK, localStatus);
            addCategory(set, U_GC_PC_MASK, localStatus);
            addCategory(set, U_GC_ND_MASK, localStatus);
            addCategory(set, U_GC_NL_MASK, localStatus);
            addCategory(set, U_GC_MC_MASK, localStatus);
            addCategory(set, U_GC_MN_MASK, localStatus);
            addIdentifierIgnorable(set, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaJavaIdentifierStart")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
            addCategory(set, U_GC_NL_MASK, localStatus);
            addCategory(set, U_GC_SC_MASK, localStatus);
            addCategory(set, U_GC_PC_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaLetter")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaLetterOrDigit")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
            addCategory(set, U_GC_ND_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaLowerCase")) == 0) {
            addCategory(set, U_GC_LL_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaMirrored")) == 0) {
            set->applyIntPropertyValue(UCHAR_BIDI_MIRRORED, 1, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaSpaceChar")) == 0) {
            addCategory(set, U_GC_Z_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaSupplementaryCodePoint")) == 0) {
            set->add(0x10000, UnicodeSet::MAX_VALUE);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaTitleCase")) == 0) {
            addCategory(set, U_GC_LT_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaUnicodeIdentifierStart")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
            addCategory(set, U_GC_NL_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaUnicodeIdentifierPart")) == 0) {
            addCategory(set, U_GC_L_MASK, localStatus);
            addCategory(set, U_GC_PC_MASK, localStatus);
            addCategory(set, U_GC_ND_MASK, localStatus);
            addCategory(set, U_GC_NL_MASK, localStatus);
            addCategory(set, U_GC_MC_MASK, localStatus);
            addCategory(set, U_GC_MN_MASK, localStatus);
            addIdentifierIgnorable(set, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaUpperCase")) == 0) {
            addCategory(set, U_GC_LU_MASK, localStatus);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaValidCodePoint")) == 0) {
            set->add(0, UnicodeSet::MAX_VALUE);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("javaWhitespace")) == 0) {
            addCategory(set, U_GC_Z_MASK, localStatus);
            set->removeAll(UnicodeSet().add(0xa0).add(0x2007).add(0x202f));
            set->add(9, 0x0d).add(0x1c, 0x1f);
        }
        else if (mPropName.compare(UNICODE_STRING_SIMPLE("all")) == 0) {
            set->add(0, UnicodeSet::MAX_VALUE);
        }

        if (U_SUCCESS(localStatus) && !set->isEmpty()) {
            *fStatus = U_ZERO_ERROR;
            if (usetFlags & USET_CASE_INSENSITIVE) {
                set->closeOver(USET_CASE_INSENSITIVE);
            }
            if (negated) {
                set->complement();
            }
            return set;
        }
        delete set;
        set = NULL;
    }
    error(*fStatus);
    return NULL; 
}









void RegexCompile::setEval(int32_t nextOp) {
    UnicodeSet *rightOperand = NULL;
    UnicodeSet *leftOperand  = NULL;
    for (;;) {
        U_ASSERT(fSetOpStack.empty()==FALSE);
        int32_t pendingSetOperation = fSetOpStack.peeki();
        if ((pendingSetOperation&0xffff0000) < (nextOp&0xffff0000)) {
            break;
        }
        fSetOpStack.popi();
        U_ASSERT(fSetStack.empty() == FALSE);
        rightOperand = (UnicodeSet *)fSetStack.peek();
        switch (pendingSetOperation) {
            case setNegation:
                rightOperand->complement();
                break;
            case setCaseClose:
                
                rightOperand->closeOver(USET_CASE_INSENSITIVE);
                rightOperand->removeAllStrings();
                break;
            case setDifference1:
            case setDifference2:
                fSetStack.pop();
                leftOperand = (UnicodeSet *)fSetStack.peek();
                leftOperand->removeAll(*rightOperand);
                delete rightOperand;
                break;
            case setIntersection1:
            case setIntersection2:
                fSetStack.pop();
                leftOperand = (UnicodeSet *)fSetStack.peek();
                leftOperand->retainAll(*rightOperand);
                delete rightOperand;
                break;
            case setUnion:
                fSetStack.pop();
                leftOperand = (UnicodeSet *)fSetStack.peek();
                leftOperand->addAll(*rightOperand);
                delete rightOperand;
                break;
            default:
                U_ASSERT(FALSE);
                break;
            }
        }
    }

void RegexCompile::setPushOp(int32_t op) {
    setEval(op);
    fSetOpStack.push(op, *fStatus);
    fSetStack.push(new UnicodeSet(), *fStatus);
}

U_NAMESPACE_END
#endif  

