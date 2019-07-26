










#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/brkiter.h"
#include "unicode/rbbi.h"
#include "unicode/ubrk.h"
#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/uchriter.h"
#include "unicode/parsepos.h"
#include "unicode/parseerr.h"
#include "cmemory.h"
#include "cstring.h"

#include "rbbirb.h"
#include "rbbinode.h"

#include "rbbiscan.h"
#include "rbbisetb.h"
#include "rbbitblb.h"
#include "rbbidata.h"


U_NAMESPACE_BEGIN







RBBIRuleBuilder::RBBIRuleBuilder(const UnicodeString   &rules,
                                       UParseError     *parseErr,
                                       UErrorCode      &status)
 : fRules(rules)
{
    fStatus = &status; 
    fParseError = parseErr;
    fDebugEnv   = NULL;
#ifdef RBBI_DEBUG
    fDebugEnv   = getenv("U_RBBIDEBUG");
#endif


    fForwardTree        = NULL;
    fReverseTree        = NULL;
    fSafeFwdTree        = NULL;
    fSafeRevTree        = NULL;
    fDefaultTree        = &fForwardTree;
    fForwardTables      = NULL;
    fReverseTables      = NULL;
    fSafeFwdTables      = NULL;
    fSafeRevTables      = NULL;
    fRuleStatusVals     = NULL;
    fChainRules         = FALSE;
    fLBCMNoChain        = FALSE;
    fLookAheadHardBreak = FALSE;
    fUSetNodes          = NULL;
    fRuleStatusVals     = NULL;
    fScanner            = NULL;
    fSetBuilder         = NULL;
    if (parseErr) {
        uprv_memset(parseErr, 0, sizeof(UParseError));
    }

    if (U_FAILURE(status)) {
        return;
    }

    fUSetNodes          = new UVector(status); 
    fRuleStatusVals     = new UVector(status);
    fScanner            = new RBBIRuleScanner(this);
    fSetBuilder         = new RBBISetBuilder(this);
    if (U_FAILURE(status)) {
        return;
    }
    if(fSetBuilder == 0 || fScanner == 0 || fUSetNodes == 0 || fRuleStatusVals == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}








RBBIRuleBuilder::~RBBIRuleBuilder() {

    int        i;
    for (i=0; ; i++) {
        RBBINode *n = (RBBINode *)fUSetNodes->elementAt(i);
        if (n==NULL) {
            break;
        }
        delete n;
    }

    delete fUSetNodes;
    delete fSetBuilder;
    delete fForwardTables;
    delete fReverseTables;
    delete fSafeFwdTables;
    delete fSafeRevTables;

    delete fForwardTree;
    delete fReverseTree;
    delete fSafeFwdTree;
    delete fSafeRevTree;
    delete fScanner;
    delete fRuleStatusVals;
}












static int32_t align8(int32_t i) {return (i+7) & 0xfffffff8;}

RBBIDataHeader *RBBIRuleBuilder::flattenData() {
    int32_t    i;

    if (U_FAILURE(*fStatus)) {
        return NULL;
    }

    
    UnicodeString strippedRules((const UnicodeString&)RBBIRuleScanner::stripRules(fRules));

    
    
    
    
    
    int32_t headerSize        = align8(sizeof(RBBIDataHeader));
    int32_t forwardTableSize  = align8(fForwardTables->getTableSize());
    int32_t reverseTableSize  = align8(fReverseTables->getTableSize());
    int32_t safeFwdTableSize  = align8(fSafeFwdTables->getTableSize());
    int32_t safeRevTableSize  = align8(fSafeRevTables->getTableSize());
    int32_t trieSize          = align8(fSetBuilder->getTrieSize());
    int32_t statusTableSize   = align8(fRuleStatusVals->size() * sizeof(int32_t));
    int32_t rulesSize         = align8((strippedRules.length()+1) * sizeof(UChar));

    int32_t         totalSize = headerSize + forwardTableSize + reverseTableSize
                                + safeFwdTableSize + safeRevTableSize 
                                + statusTableSize + trieSize + rulesSize;

    RBBIDataHeader  *data     = (RBBIDataHeader *)uprv_malloc(totalSize);
    if (data == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(data, 0, totalSize);


    data->fMagic            = 0xb1a0;
    data->fFormatVersion[0] = 3;
    data->fFormatVersion[1] = 1;
    data->fFormatVersion[2] = 0;
    data->fFormatVersion[3] = 0;
    data->fLength           = totalSize;
    data->fCatCount         = fSetBuilder->getNumCharCategories();

    data->fFTable        = headerSize;
    data->fFTableLen     = forwardTableSize;
    data->fRTable        = data->fFTable  + forwardTableSize;
    data->fRTableLen     = reverseTableSize;
    data->fSFTable       = data->fRTable  + reverseTableSize;
    data->fSFTableLen    = safeFwdTableSize;
    data->fSRTable       = data->fSFTable + safeFwdTableSize;
    data->fSRTableLen    = safeRevTableSize;

    data->fTrie          = data->fSRTable + safeRevTableSize;
    data->fTrieLen       = fSetBuilder->getTrieSize();
    data->fStatusTable   = data->fTrie    + trieSize;
    data->fStatusTableLen= statusTableSize;
    data->fRuleSource    = data->fStatusTable + statusTableSize;
    data->fRuleSourceLen = strippedRules.length() * sizeof(UChar);

    uprv_memset(data->fReserved, 0, sizeof(data->fReserved));

    fForwardTables->exportTable((uint8_t *)data + data->fFTable);
    fReverseTables->exportTable((uint8_t *)data + data->fRTable);
    fSafeFwdTables->exportTable((uint8_t *)data + data->fSFTable);
    fSafeRevTables->exportTable((uint8_t *)data + data->fSRTable);
    fSetBuilder->serializeTrie ((uint8_t *)data + data->fTrie);

    int32_t *ruleStatusTable = (int32_t *)((uint8_t *)data + data->fStatusTable);
    for (i=0; i<fRuleStatusVals->size(); i++) {
        ruleStatusTable[i] = fRuleStatusVals->elementAti(i);
    }

    strippedRules.extract((UChar *)((uint8_t *)data+data->fRuleSource), rulesSize/2+1, *fStatus);

    return data;
}












BreakIterator *
RBBIRuleBuilder::createRuleBasedBreakIterator( const UnicodeString    &rules,
                                    UParseError      *parseError,
                                    UErrorCode       &status)
{
    

    
    
    
    
    RBBIRuleBuilder  builder(rules, parseError, status);
    if (U_FAILURE(status)) { 
        return NULL;
    }
    builder.fScanner->parse();

    
    
    
    
    
    
    builder.fSetBuilder->build();


    
    
    
    builder.fForwardTables = new RBBITableBuilder(&builder, &builder.fForwardTree);
    builder.fReverseTables = new RBBITableBuilder(&builder, &builder.fReverseTree);
    builder.fSafeFwdTables = new RBBITableBuilder(&builder, &builder.fSafeFwdTree);
    builder.fSafeRevTables = new RBBITableBuilder(&builder, &builder.fSafeRevTree);
    if (builder.fForwardTables == NULL || builder.fReverseTables == NULL ||
        builder.fSafeFwdTables == NULL || builder.fSafeRevTables == NULL)
    {
        status = U_MEMORY_ALLOCATION_ERROR;
        delete builder.fForwardTables; builder.fForwardTables = NULL;
        delete builder.fReverseTables; builder.fReverseTables = NULL;
        delete builder.fSafeFwdTables; builder.fSafeFwdTables = NULL;
        delete builder.fSafeRevTables; builder.fSafeRevTables = NULL;
        return NULL;
    }

    builder.fForwardTables->build();
    builder.fReverseTables->build();
    builder.fSafeFwdTables->build();
    builder.fSafeRevTables->build();

#ifdef RBBI_DEBUG
    if (builder.fDebugEnv && uprv_strstr(builder.fDebugEnv, "states")) {
        builder.fForwardTables->printRuleStatusTable();
    }
#endif

    
    
    
    
    RBBIDataHeader *data = builder.flattenData(); 
    if (U_FAILURE(*builder.fStatus)) {
        return NULL;
    }


    
    
    


    
    
    
    
    
    RuleBasedBreakIterator *This = new RuleBasedBreakIterator(data, status);
    if (U_FAILURE(status)) {
        delete This;
        This = NULL;
    } 
    else if(This == NULL) { 
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return This;
}

U_NAMESPACE_END

#endif 
