















#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/regex.h"
#include "uprops.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "ucln_in.h"
#include "umutex.h"

#include "regexcst.h"   
                        
#include "regexst.h"



U_NAMESPACE_BEGIN













static const UChar gRuleSet_rule_char_pattern[]       = {
 
    0x5b, 0x5e, 0x5c, 0x2a, 0x5c, 0x3f, 0x5c, 0x2b, 0x5c, 0x5b, 0x5c, 0x28, 0x5c, 0x29,
 
    0x5c, 0x7b,0x5c, 0x7d, 0x5c, 0x5e, 0x5c, 0x24, 0x5c, 0x7c, 0x5c, 0x5c, 0x5c, 0x2e, 0x5d, 0};





static const UChar gUnescapeCharPattern[] = {

    0x5b, 0x61, 0x63, 0x65, 0x66, 0x6e, 0x72, 0x74, 0x75, 0x55, 0x78, 0x5d, 0};





static const UChar gIsWordPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x62, 0x65, 0x74, 0x69, 0x63, 0x7d,

          0x5c, 0x70, 0x7b, 0x4d, 0x7d,

          0x5c, 0x70, 0x7b, 0x4e, 0x64, 0x7d,

          0x5c, 0x70, 0x7b, 0x50, 0x63, 0x7d,

          0x5c, 0x75, 0x32, 0x30, 0x30, 0x63, 0x5c, 0x75, 0x32, 0x30, 0x30, 0x64, 0x5d, 0};





static const UChar gIsSpacePattern[] = {

        0x5b, 0x5c, 0x70, 0x7b, 0x57, 0x68, 0x69, 0x74, 0x65, 0x53, 0x70, 0x61, 0x63, 0x65, 0x7d, 0x5d, 0};





static const UChar gGC_ControlPattern[] = {

    0x5b, 0x5b, 0x3a, 0x5A, 0x6c, 0x3a, 0x5d, 0x5b, 0x3a, 0x5A, 0x70, 0x3a, 0x5d,

    0x5b, 0x3a, 0x43, 0x63, 0x3a, 0x5d, 0x5b, 0x3a, 0x43, 0x66, 0x3a, 0x5d, 0x2d,

    0x5b, 0x3a, 0x47, 0x72, 0x61, 0x70, 0x68, 0x65, 0x6d, 0x65, 0x5f,

    0x45, 0x78, 0x74, 0x65, 0x6e, 0x64, 0x3a, 0x5d, 0x5d, 0};

static const UChar gGC_ExtendPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x47, 0x72, 0x61, 0x70, 0x68, 0x65, 0x6d, 0x65, 0x5f,

    0x45, 0x78, 0x74, 0x65, 0x6e, 0x64, 0x7d, 0x5d, 0};

static const UChar gGC_LPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,

    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x7d,  0x5d, 0};

static const UChar gGC_VPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,

    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x56, 0x7d,  0x5d, 0};

static const UChar gGC_TPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,

    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x54, 0x7d, 0x5d, 0};

static const UChar gGC_LVPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,

    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x56, 0x7d, 0x5d, 0};

static const UChar gGC_LVTPattern[] = {

    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,

    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x56, 0x54, 0x7d, 0x5d, 0};


RegexStaticSets *RegexStaticSets::gStaticSets = NULL;
UInitOnce gStaticSetsInitOnce = U_INITONCE_INITIALIZER;

RegexStaticSets::RegexStaticSets(UErrorCode *status)
:
fUnescapeCharSet(UnicodeString(TRUE, gUnescapeCharPattern, -1), *status),
fRuleDigitsAlias(NULL),
fEmptyText(NULL)
{
    
    int i;
    for (i=0; i<URX_LAST_SET; i++) {
        fPropSets[i] = NULL;
    }
    
    fPropSets[URX_ISWORD_SET]  = new UnicodeSet(UnicodeString(TRUE, gIsWordPattern, -1),     *status);
    fPropSets[URX_ISSPACE_SET] = new UnicodeSet(UnicodeString(TRUE, gIsSpacePattern, -1),    *status);
    fPropSets[URX_GC_EXTEND]   = new UnicodeSet(UnicodeString(TRUE, gGC_ExtendPattern, -1),  *status);
    fPropSets[URX_GC_CONTROL]  = new UnicodeSet(UnicodeString(TRUE, gGC_ControlPattern, -1), *status);
    fPropSets[URX_GC_L]        = new UnicodeSet(UnicodeString(TRUE, gGC_LPattern, -1),       *status);
    fPropSets[URX_GC_V]        = new UnicodeSet(UnicodeString(TRUE, gGC_VPattern, -1),       *status);
    fPropSets[URX_GC_T]        = new UnicodeSet(UnicodeString(TRUE, gGC_TPattern, -1),       *status);
    fPropSets[URX_GC_LV]       = new UnicodeSet(UnicodeString(TRUE, gGC_LVPattern, -1),      *status);
    fPropSets[URX_GC_LVT]      = new UnicodeSet(UnicodeString(TRUE, gGC_LVTPattern, -1),     *status);
    
    
    if (fPropSets[URX_ISWORD_SET] == NULL || fPropSets[URX_ISSPACE_SET] == NULL || fPropSets[URX_GC_EXTEND] == NULL || 
        fPropSets[URX_GC_CONTROL] == NULL || fPropSets[URX_GC_L] == NULL || fPropSets[URX_GC_V] == NULL || 
        fPropSets[URX_GC_T] == NULL || fPropSets[URX_GC_LV] == NULL || fPropSets[URX_GC_LVT] == NULL) {
        goto ExitConstrDeleteAll;
    }
    if (U_FAILURE(*status)) {
        
        
        return;
    }


    
    
    
    


    
    
    
    
    fPropSets[URX_GC_NORMAL] = new UnicodeSet(0, UnicodeSet::MAX_VALUE);
    
    if (fPropSets[URX_GC_NORMAL] == NULL) {
    	goto ExitConstrDeleteAll;
    }
    fPropSets[URX_GC_NORMAL]->remove(0xac00, 0xd7a4);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_CONTROL]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_L]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_V]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_T]);

    
    
    for (i=0; i<URX_LAST_SET; i++) {
        if (fPropSets[i]) {
            fPropSets[i]->compact();
            fPropSets8[i].init(fPropSets[i]);
        }
    }

    
    fRuleSets[kRuleSet_rule_char-128]   = UnicodeSet(UnicodeString(TRUE, gRuleSet_rule_char_pattern, -1),   *status);
    fRuleSets[kRuleSet_digit_char-128].add((UChar)0x30, (UChar)0x39);    
    fRuleSets[kRuleSet_ascii_letter-128].add((UChar)0x41, (UChar)0x5A);  
    fRuleSets[kRuleSet_ascii_letter-128].add((UChar)0x61, (UChar)0x7A);  
    fRuleDigitsAlias = &fRuleSets[kRuleSet_digit_char-128];
    for (i=0; i<UPRV_LENGTHOF(fRuleSets); i++) {
        fRuleSets[i].compact();
    }
    
    
    fEmptyText = utext_openUChars(NULL, NULL, 0, status);
    
    if (U_SUCCESS(*status)) {
        return;
    }

ExitConstrDeleteAll: 
    for (i=0; i<URX_LAST_SET; i++) {
        delete fPropSets[i];
        fPropSets[i] = NULL;
    }
    if (U_SUCCESS(*status)) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }
}


RegexStaticSets::~RegexStaticSets() {
    int32_t i;

    for (i=0; i<URX_LAST_SET; i++) {
        delete fPropSets[i];
        fPropSets[i] = NULL;
    }
    fRuleDigitsAlias = NULL;
    
    utext_close(fEmptyText);
}








UBool
RegexStaticSets::cleanup(void) {
    delete RegexStaticSets::gStaticSets;
    RegexStaticSets::gStaticSets = NULL;
    gStaticSetsInitOnce.reset();
    return TRUE;
}

U_CDECL_BEGIN
static UBool U_CALLCONV
regex_cleanup(void) {
    return RegexStaticSets::cleanup();
}

static void U_CALLCONV initStaticSets(UErrorCode &status) {
    U_ASSERT(RegexStaticSets::gStaticSets == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_REGEX, regex_cleanup);
    RegexStaticSets::gStaticSets = new RegexStaticSets(&status);
    if (U_FAILURE(status)) {
        delete RegexStaticSets::gStaticSets;
        RegexStaticSets::gStaticSets = NULL;
    }
    if (RegexStaticSets::gStaticSets == NULL && U_SUCCESS(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}
U_CDECL_END

void RegexStaticSets::initGlobals(UErrorCode *status) {
    umtx_initOnce(gStaticSetsInitOnce, &initStaticSets, *status);
}

U_NAMESPACE_END
#endif  
