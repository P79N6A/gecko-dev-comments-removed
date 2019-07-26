










#ifndef _REGEXIMP_H
#define _REGEXIMP_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/uniset.h"
#include "unicode/utext.h"

#include "cmemory.h"
#include "ucase.h"

U_NAMESPACE_BEGIN





#ifdef REGEX_DEBUG   





#define REGEX_DUMP_DEBUG
#define REGEX_RUN_DEBUG



#include <stdio.h>
#endif

#ifdef REGEX_SCAN_DEBUG
#define REGEX_SCAN_DEBUG_PRINTF(a) printf a
#else
#define REGEX_SCAN_DEBUG_PRINTF(a)
#endif

#ifdef REGEX_DUMP_DEBUG
#define REGEX_DUMP_DEBUG_PRINTF(a) printf a
#else
#define REGEX_DUMP_DEBUG_PRINTF(a)
#endif

#ifdef REGEX_RUN_DEBUG
#define REGEX_RUN_DEBUG_PRINTF(a) printf a
#define REGEX_DUMP_DEBUG_PRINTF(a) printf a
#else
#define REGEX_RUN_DEBUG_PRINTF(a)
#endif






enum {
     URX_RESERVED_OP   = 0,    
     URX_RESERVED_OP_N = 255,  
     URX_BACKTRACK     = 1,    
     URX_END           = 2,
     URX_ONECHAR       = 3,    
     URX_STRING        = 4,    
     URX_STRING_LEN    = 5,    
     URX_STATE_SAVE    = 6,    
     URX_NOP           = 7,
     URX_START_CAPTURE = 8,    
     URX_END_CAPTURE   = 9,    
     URX_STATIC_SETREF = 10,   
     URX_SETREF        = 11,   
     URX_DOTANY        = 12,
     URX_JMP           = 13,   
                                                    
     URX_FAIL          = 14,   

     URX_JMP_SAV       = 15,   
     URX_BACKSLASH_B   = 16,   
     URX_BACKSLASH_G   = 17,
     URX_JMP_SAV_X     = 18,   
                               
                               
     URX_BACKSLASH_X   = 19,
     URX_BACKSLASH_Z   = 20,   

     URX_DOTANY_ALL    = 21,   
     URX_BACKSLASH_D   = 22,   
     URX_CARET         = 23,   
     URX_DOLLAR        = 24,  

     URX_CTR_INIT      = 25,   
     URX_CTR_INIT_NG   = 26,   
                               
                               
                               
                               
                               
                               

     URX_DOTANY_UNIX   = 27,   

     URX_CTR_LOOP      = 28,   
     URX_CTR_LOOP_NG   = 29,   
                               

     URX_CARET_M_UNIX  = 30,   
                               

     URX_RELOC_OPRND   = 31,   
                               
                               

     URX_STO_SP        = 32,   
                               
     URX_LD_SP         = 33,   
                               
     URX_BACKREF       = 34,   
                               
     URX_STO_INP_LOC   = 35,   
                               
     URX_JMPX          = 36,  
                               
                               
                               
                               
                               
     URX_LA_START      = 37,   
                               
                               
     URX_LA_END        = 38,   
                               
                               
     URX_ONECHAR_I     = 39,   
                               
     URX_STRING_I      = 40,   
                               
                               
                               
     URX_BACKREF_I     = 41,   
                               
                               
     URX_DOLLAR_M      = 42,   
     URX_CARET_M       = 43,   
     URX_LB_START      = 44,   
                               
     URX_LB_CONT       = 45,   
                               
                               
                               
     URX_LB_END        = 46,   
                               
                               
                               
     URX_LBN_CONT      = 47,   
                               
                               
                               
                               
     URX_LBN_END       = 48,   
                               
                               
     URX_STAT_SETREF_N = 49,   
                               
     URX_LOOP_SR_I     = 50,   
                               
     URX_LOOP_C        = 51,   
                               
                               
     URX_LOOP_DOT_I    = 52,   
                               
                               
                               
                               
                               
                               
                               
     URX_BACKSLASH_BU  = 53,   
                               
     URX_DOLLAR_D      = 54,   
     URX_DOLLAR_MD     = 55    

};



#define URX_OPCODE_NAMES       \
        "               ",     \
        "BACKTRACK",           \
        "END",                 \
        "ONECHAR",             \
        "STRING",              \
        "STRING_LEN",          \
        "STATE_SAVE",          \
        "NOP",                 \
        "START_CAPTURE",       \
        "END_CAPTURE",         \
        "URX_STATIC_SETREF",   \
        "SETREF",              \
        "DOTANY",              \
        "JMP",                 \
        "FAIL",                \
        "JMP_SAV",             \
        "BACKSLASH_B",         \
        "BACKSLASH_G",         \
        "JMP_SAV_X",           \
        "BACKSLASH_X",         \
        "BACKSLASH_Z",         \
        "DOTANY_ALL",          \
        "BACKSLASH_D",         \
        "CARET",               \
        "DOLLAR",              \
        "CTR_INIT",            \
        "CTR_INIT_NG",         \
        "DOTANY_UNIX",         \
        "CTR_LOOP",            \
        "CTR_LOOP_NG",         \
        "URX_CARET_M_UNIX",    \
        "RELOC_OPRND",         \
        "STO_SP",              \
        "LD_SP",               \
        "BACKREF",             \
        "STO_INP_LOC",         \
        "JMPX",                \
        "LA_START",            \
        "LA_END",              \
        "ONECHAR_I",           \
        "STRING_I",            \
        "BACKREF_I",           \
        "DOLLAR_M",            \
        "CARET_M",             \
        "LB_START",            \
        "LB_CONT",             \
        "LB_END",              \
        "LBN_CONT",            \
        "LBN_END",             \
        "STAT_SETREF_N",       \
        "LOOP_SR_I",           \
        "LOOP_C",              \
        "LOOP_DOT_I",          \
        "BACKSLASH_BU",        \
        "DOLLAR_D",            \
        "DOLLAR_MD"





#define URX_BUILD(type, val) (int32_t)((type << 24) | (val))
#define URX_TYPE(x)          ((uint32_t)(x) >> 24)
#define URX_VAL(x)           ((x) & 0xffffff)






enum {
     URX_ISWORD_SET  = 1,
     URX_ISALNUM_SET = 2,
     URX_ISALPHA_SET = 3,
     URX_ISSPACE_SET = 4,

     URX_GC_NORMAL,          
     URX_GC_EXTEND,
     URX_GC_CONTROL,
     URX_GC_L,
     URX_GC_LV,
     URX_GC_LVT,
     URX_GC_V,
     URX_GC_T,

     URX_LAST_SET,

     URX_NEG_SET     = 0x800000          
                                         
};





struct REStackFrame {
    
    int64_t            fInputIdx;        
    int64_t            fPatIdx;          
                                         
    
    int64_t            fExtra[1];        
                                         
                                         
                                         
};

#define RESTACKFRAME_HDRCOUNT 2





enum StartOfMatch {
    START_NO_INFO,             
    START_CHAR,                
    START_SET,                 
    START_START,               
    START_LINE,                
    START_STRING               
};

#define START_OF_MATCH_STR(v) ((v)==START_NO_INFO? "START_NO_INFO" : \
                               (v)==START_CHAR?    "START_CHAR"    : \
                               (v)==START_SET?     "START_SET"     : \
                               (v)==START_START?   "START_START"   : \
                               (v)==START_LINE?    "START_LINE"    : \
                               (v)==START_STRING?  "START_STRING"  : \
                                                   "ILLEGAL")




struct Regex8BitSet : public UMemory {
    inline Regex8BitSet();
    inline void operator = (const Regex8BitSet &s);
    inline void init(const UnicodeSet *src);
    inline UBool contains(UChar32 c);
    inline void  add(UChar32 c);
    int8_t d[32];
};

inline Regex8BitSet::Regex8BitSet() {
    uprv_memset(d, 0, sizeof(d));
}

inline UBool Regex8BitSet::contains(UChar32 c) {
    
    return ((d[c>>3] & 1 <<(c&7)) != 0);
}

inline void  Regex8BitSet::add(UChar32 c) {
    d[c>>3] |= 1 << (c&7);
}

inline void Regex8BitSet::init(const UnicodeSet *s) {
    if (s != NULL) {
        for (int32_t i=0; i<=255; i++) {
            if (s->contains(i)) {
                this->add(i);
            }
        }
    }
}

inline void Regex8BitSet::operator = (const Regex8BitSet &s) {
   uprv_memcpy(d, s.d, sizeof(d));
}







class CaseFoldingUTextIterator: public UMemory {
      public:
        CaseFoldingUTextIterator(UText &text);
        ~CaseFoldingUTextIterator();

        UChar32 next();           

        UBool   inExpansion();    
                                  
                                  
      private:
        UText             &fUText;
        const  UCaseProps *fcsp;
        const  UChar      *fFoldChars;
        int32_t            fFoldLength;
        int32_t            fFoldIndex;

};







class CaseFoldingUCharIterator: public UMemory {
      public:
        CaseFoldingUCharIterator(const UChar *chars, int64_t start, int64_t limit);
        ~CaseFoldingUCharIterator();

        UChar32 next();           

        UBool   inExpansion();    
                                  
                                  

        int64_t  getIndex();      

      private:
        const  UChar      *fChars;
        int64_t            fIndex;
        int64_t            fLimit;
        const  UCaseProps *fcsp;
        const  UChar      *fFoldChars;
        int32_t            fFoldLength;
        int32_t            fFoldIndex;

};

U_NAMESPACE_END
#endif

