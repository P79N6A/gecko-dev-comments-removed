





#include "LETypes.h"
#include "LEScripts.h"
#include "OpenTypeTables.h"
#include "OpenTypeUtilities.h"
#include "IndicReordering.h"

U_NAMESPACE_BEGIN


#define _x1  (1 << CF_INDEX_SHIFT)
#define _x2  (2 << CF_INDEX_SHIFT)
#define _x3  (3 << CF_INDEX_SHIFT)
#define _x4  (4 << CF_INDEX_SHIFT)
#define _x5  (5 << CF_INDEX_SHIFT)
#define _x6  (6 << CF_INDEX_SHIFT)
#define _x7  (7 << CF_INDEX_SHIFT)
#define _x8  (8 << CF_INDEX_SHIFT)
#define _x9  (9 << CF_INDEX_SHIFT)


#define _xx  (CC_RESERVED)
#define _ma  (CC_VOWEL_MODIFIER | CF_POS_ABOVE)
#define _mp  (CC_VOWEL_MODIFIER | CF_POS_AFTER)
#define _sa  (CC_STRESS_MARK | CF_POS_ABOVE)
#define _sb  (CC_STRESS_MARK | CF_POS_BELOW)
#define _iv  (CC_INDEPENDENT_VOWEL)
#define _i2  (CC_INDEPENDENT_VOWEL_2)
#define _i3  (CC_INDEPENDENT_VOWEL_3)
#define _ct  (CC_CONSONANT | CF_CONSONANT)
#define _cn  (CC_CONSONANT_WITH_NUKTA | CF_CONSONANT)
#define _nu  (CC_NUKTA)
#define _dv  (CC_DEPENDENT_VOWEL)
#define _dl  (_dv | CF_POS_BEFORE)
#define _db  (_dv | CF_POS_BELOW)
#define _da  (_dv | CF_POS_ABOVE)
#define _dr  (_dv | CF_POS_AFTER)
#define _lm  (_dv | CF_LENGTH_MARK)
#define _l1  (CC_SPLIT_VOWEL_PIECE_1 | CF_POS_BEFORE)
#define _a1  (CC_SPLIT_VOWEL_PIECE_1 | CF_POS_ABOVE)
#define _b2  (CC_SPLIT_VOWEL_PIECE_2 | CF_POS_BELOW)
#define _r2  (CC_SPLIT_VOWEL_PIECE_2 | CF_POS_AFTER)
#define _m2  (CC_SPLIT_VOWEL_PIECE_2 | CF_LENGTH_MARK)
#define _m3  (CC_SPLIT_VOWEL_PIECE_3 | CF_LENGTH_MARK)
#define _vr  (CC_VIRAMA)
#define _al  (CC_AL_LAKUNA)


#define _s1  (_dv | _x1)
#define _s2  (_dv | _x2)
#define _s3  (_dv | _x3)
#define _s4  (_dv | _x4)
#define _s5  (_dv | _x5)
#define _s6  (_dv | _x6)
#define _s7  (_dv | _x7)
#define _s8  (_dv | _x8)
#define _s9  (_dv | _x9)




#define _bb  (_ct | CF_BELOW_BASE)
#define _pb  (_ct | CF_POST_BASE)
#define _fb  (_ct | CF_PRE_BASE)
#define _vt  (_bb | CF_VATTU)
#define _rv  (_vt | CF_REPH)
#define _rp  (_pb | CF_REPH)
#define _rb  (_bb | CF_REPH)




static const IndicClassTable::CharClass devaCharClasses[] =
{
    _xx, _ma, _ma, _mp, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, 
    _iv, _iv, _iv, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _cn, _ct, _ct, _ct, _ct, _ct, _ct, 
    _rv, _cn, _ct, _ct, _cn, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _dr, _dl, 
    _dr, _db, _db, _db, _db, _da, _da, _da, _da, _dr, _dr, _dr, _dr, _vr, _xx, _xx, 
    _xx, _sa, _sb, _sa, _sa, _xx, _xx, _xx, _cn, _cn, _cn, _cn, _cn, _cn, _cn, _cn, 
    _iv, _iv, _db, _db, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx                                                                             
};

static const IndicClassTable::CharClass bengCharClasses[] =
{
    _xx, _ma, _mp, _mp, _xx, _i2, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _i2, 
    _iv, _xx, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _bb, _ct, _ct, _pb, 
    _rv, _xx, _ct, _xx, _xx, _xx, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _r2, _dl, 
    _dr, _db, _db, _db, _db, _xx, _xx, _l1, _dl, _xx, _xx, _s1, _s2, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _m2, _xx, _xx, _xx, _xx, _cn, _cn, _xx, _cn, 
    _iv, _iv, _dv, _dv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _rv, _ct, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx                           
};

static const IndicClassTable::CharClass punjCharClasses[] =
{
    _xx, _ma, _ma, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _xx, _iv, 
    _iv, _xx, _xx, _i3, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _bb, 
    _vt, _xx, _ct, _cn, _xx, _bb, _cn, _xx, _ct, _bb, _xx, _xx, _nu, _xx, _dr, _dl, 
    _dr, _b2, _db, _xx, _xx, _xx, _xx, _da, _da, _xx, _xx, _a1, _da, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _cn, _cn, _cn, _ct, _xx, _cn, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _ma, _ma, _xx, _xx, _xx                                                         
};

static const IndicClassTable::CharClass gujrCharClasses[] =
{
    _xx, _ma, _ma, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _xx, _iv, 
    _iv, _iv, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _ct, 
    _rv, _xx, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _dr, _dl, 
    _dr, _db, _db, _db, _db, _da, _xx, _da, _da, _dr, _xx, _dr, _dr, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _iv, _iv, _db, _db, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  
};

#if 1
static const IndicClassTable::CharClass oryaCharClasses[] =
{
    _xx, _ma, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _iv, 
    _iv, _xx, _xx, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _ct, _bb, 
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _pb, 
    _rb, _xx, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _nu, _xx, _dr, _da, 
    _dr, _db, _db, _db, _xx, _xx, _xx, _dl, _s1, _xx, _xx, _s2, _s3, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _da, _dr, _xx, _xx, _xx, _xx, _cn, _cn, _xx, _pb, 
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _bb                                                                        
};
#else
static const IndicClassTable::CharClass oryaCharClasses[] =
{
    _xx, _ma, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _iv, 
    _iv, _xx, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _bb, _ct, _ct, _ct, _bb, _xx, _ct, _ct, _bb, _bb, _bb, _pb, 
    _rb, _xx, _bb, _bb, _xx, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _r2, _da, 
    _dr, _db, _db, _db, _xx, _xx, _xx, _l1, _s1, _xx, _xx, _s2, _s3, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _m2, _m2, _xx, _xx, _xx, _xx, _cn, _cn, _xx, _cn, 
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _ct                                                                        
};
#endif

static const IndicClassTable::CharClass tamlCharClasses[] =
{
    _xx, _xx, _ma, _xx, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _iv, _iv, 
    _iv, _xx, _iv, _iv, _iv, _ct, _xx, _xx, _xx, _ct, _ct, _xx, _ct, _xx, _ct, _ct, 
    _xx, _xx, _xx, _ct, _ct, _xx, _xx, _xx, _ct, _ct, _ct, _xx, _xx, _xx, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _xx, _xx, _r2, _dr, 
    _da, _dr, _dr, _xx, _xx, _xx, _l1, _l1, _dl, _xx, _s1, _s2, _s3, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _m2, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx                                                                   
};




static const IndicClassTable::CharClass teluCharClasses[] =
{
    _xx, _mp, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, 
    _iv, _xx, _iv, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, 
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _bb, 
    _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _xx, _xx, _da, _da, 
    _da, _dr, _dr, _lm, _lm, _xx, _a1, _da, _s1, _xx, _da, _da, _da, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _da, _m2, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  
};







static const IndicClassTable::CharClass kndaCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, 
    _iv, _xx, _iv, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, 
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _bb, 
    _rb, _ct, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _xx, _xx, _dr, _da, 
    _s1, _dr, _r2, _lm, _lm, _xx, _a1, _s2, _s3, _xx, _s4, _s5, _da, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _m3, _m2, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _ct, _xx, 
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  
};



static const IndicClassTable::CharClass mlymCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, 
    _iv, _xx, _iv, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _pb, 
    _fb, _fb, _bb, _ct, _ct, _pb, _ct, _ct, _ct, _ct, _xx, _xx, _xx, _xx, _r2, _dr, 
    _dr, _dr, _dr, _dr, _xx, _xx, _l1, _l1, _dl, _xx, _s1, _s2, _s3, _vr, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _m2, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  
};
 
static const IndicClassTable::CharClass sinhCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, 
    _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, 
    _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _xx, _xx, 
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _xx, _al, _xx, _xx, _xx, _xx, _dr, 
    _dr, _dr, _da, _da, _db, _xx, _db, _xx, _dr, _dl, _s1, _dl, _s2, _s3, _s4, _dr, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _dr, _dr, _xx                                                         
};




static const SplitMatra bengSplitTable[] = {{0x09C7, 0x09BE}, {0x09C7, 0x09D7}};

static const SplitMatra oryaSplitTable[] = {{0x0B47, 0x0B56}, {0x0B47, 0x0B3E}, {0x0B47, 0x0B57}};

static const SplitMatra tamlSplitTable[] = {{0x0BC6, 0x0BBE}, {0x0BC7, 0x0BBE}, {0x0BC6, 0x0BD7}};

static const SplitMatra teluSplitTable[] = {{0x0C46, 0x0C56}};

static const SplitMatra kndaSplitTable[] = {{0x0CBF, 0x0CD5}, {0x0CC6, 0x0CD5}, {0x0CC6, 0x0CD6}, {0x0CC6, 0x0CC2},
                                            {0x0CC6, 0x0CC2, 0x0CD5}};

static const SplitMatra mlymSplitTable[] = {{0x0D46, 0x0D3E}, {0x0D47, 0x0D3E}, {0x0D46, 0x0D57}};

 
static const SplitMatra sinhSplitTable[] = {{0x0DD9, 0x0DCA}, {0x0DD9, 0x0DCF}, {0x0DD9, 0x0DCF, 0x0DCA},
                                            {0x0DD9, 0x0DDF}};








#define DEVA_SCRIPT_FLAGS (SF_EYELASH_RA | SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define BENG_SCRIPT_FLAGS (SF_REPH_AFTER_BELOW | SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define PUNJ_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define GUJR_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define ORYA_SCRIPT_FLAGS (SF_REPH_AFTER_BELOW | SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define TAML_SCRIPT_FLAGS (SF_MPRE_FIXUP | SF_NO_POST_BASE_LIMIT | SF_FILTER_ZERO_WIDTH)
#define TELU_SCRIPT_FLAGS (SF_MATRAS_AFTER_BASE | SF_FILTER_ZERO_WIDTH | 3)
#define KNDA_SCRIPT_FLAGS (SF_MATRAS_AFTER_BASE | SF_FILTER_ZERO_WIDTH | 3)
#define MLYM_SCRIPT_FLAGS (SF_MPRE_FIXUP | SF_NO_POST_BASE_LIMIT /*| SF_FILTER_ZERO_WIDTH*/)
#define SINH_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT)




static const IndicClassTable devaClassTable = {0x0900, 0x0970, 2, DEVA_SCRIPT_FLAGS, devaCharClasses, NULL};

static const IndicClassTable bengClassTable = {0x0980, 0x09FA, 3, BENG_SCRIPT_FLAGS, bengCharClasses, bengSplitTable};

static const IndicClassTable punjClassTable = {0x0A00, 0x0A74, 2, PUNJ_SCRIPT_FLAGS, punjCharClasses, NULL};

static const IndicClassTable gujrClassTable = {0x0A80, 0x0AEF, 2, GUJR_SCRIPT_FLAGS, gujrCharClasses, NULL};

static const IndicClassTable oryaClassTable = {0x0B00, 0x0B71, 3, ORYA_SCRIPT_FLAGS, oryaCharClasses, oryaSplitTable};

static const IndicClassTable tamlClassTable = {0x0B80, 0x0BF2, 3, TAML_SCRIPT_FLAGS, tamlCharClasses, tamlSplitTable};

static const IndicClassTable teluClassTable = {0x0C00, 0x0C6F, 3, TELU_SCRIPT_FLAGS, teluCharClasses, teluSplitTable};

static const IndicClassTable kndaClassTable = {0x0C80, 0x0CEF, 4, KNDA_SCRIPT_FLAGS, kndaCharClasses, kndaSplitTable};

static const IndicClassTable mlymClassTable = {0x0D00, 0x0D6F, 4, MLYM_SCRIPT_FLAGS, mlymCharClasses, mlymSplitTable};

static const IndicClassTable sinhClassTable = {0x0D80, 0x0DF4, 4, SINH_SCRIPT_FLAGS, sinhCharClasses, sinhSplitTable};




static const IndicClassTable * const indicClassTables[scriptCodeCount] = {
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &bengClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &devaClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &gujrClassTable, 
    &punjClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &kndaClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &mlymClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    &oryaClassTable, 
    NULL,            
    &sinhClassTable, 
    NULL,            
    &tamlClassTable, 
    &teluClassTable, 
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL,            
    NULL             
};

IndicClassTable::CharClass IndicClassTable::getCharClass(LEUnicode ch) const
{
    if (ch == C_SIGN_ZWJ) {
        return CF_CONSONANT | CC_ZERO_WIDTH_MARK;
    }

    if (ch == C_SIGN_ZWNJ) {
        return CC_ZERO_WIDTH_MARK;
    }

    if (ch < firstChar || ch > lastChar) {
        return CC_RESERVED;
    }

    return classTable[ch - firstChar];
}

const IndicClassTable *IndicClassTable::getScriptClassTable(le_int32 scriptCode)
{
    if (scriptCode < 0 || scriptCode >= scriptCodeCount) {
        return NULL;
    }

    return indicClassTables[scriptCode];
}

le_int32 IndicReordering::getWorstCaseExpansion(le_int32 scriptCode)
{
    const IndicClassTable *classTable = IndicClassTable::getScriptClassTable(scriptCode);

    if (classTable == NULL) {
        return 1;
    }

    return classTable->getWorstCaseExpansion();
}

le_bool IndicReordering::getFilterZeroWidth(le_int32 scriptCode)
{
    const IndicClassTable *classTable = IndicClassTable::getScriptClassTable(scriptCode);

    if (classTable == NULL) {
        return TRUE;
    }

    return classTable->getFilterZeroWidth();
}

U_NAMESPACE_END
