















#ifndef __UNORMIMP_H__
#define __UNORMIMP_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "udataswp.h"








enum {
    
    _NORM_QC_NFC=0x11,          
    _NORM_QC_NFKC=0x22,         
    _NORM_QC_NFD=4,             
    _NORM_QC_NFKD=8,            

    _NORM_QC_ANY_NO=0xf,

    
    _NORM_QC_MAYBE=0x10,
    _NORM_QC_ANY_MAYBE=0x30,

    _NORM_QC_MASK=0x3f,

    _NORM_COMBINES_FWD=0x40,
    _NORM_COMBINES_BACK=0x80,
    _NORM_COMBINES_ANY=0xc0,

    _NORM_CC_SHIFT=8,           
    _NORM_CC_MASK=0xff00,

    _NORM_EXTRA_SHIFT=16,               
    _NORM_EXTRA_INDEX_TOP=0xfc00,       

    _NORM_EXTRA_SURROGATE_MASK=0x3ff,
    _NORM_EXTRA_SURROGATE_TOP=0x3f0,    

    _NORM_EXTRA_HANGUL=_NORM_EXTRA_SURROGATE_TOP,
    _NORM_EXTRA_JAMO_L,
    _NORM_EXTRA_JAMO_V,
    _NORM_EXTRA_JAMO_T
};


#define _NORM_MIN_SPECIAL       0xfc000000
#define _NORM_SURROGATES_TOP    0xfff00000
#define _NORM_MIN_HANGUL        0xfff00000
#define _NORM_MIN_JAMO_V        0xfff20000
#define _NORM_JAMO_V_TOP        0xfff30000


enum {
    _NORM_AUX_COMP_EX_SHIFT=10,
    _NORM_AUX_UNSAFE_SHIFT=11,
    _NORM_AUX_NFC_SKIPPABLE_F_SHIFT=12
};

#define _NORM_AUX_MAX_FNC           ((int32_t)1<<_NORM_AUX_COMP_EX_SHIFT)

#define _NORM_AUX_FNC_MASK          (uint32_t)(_NORM_AUX_MAX_FNC-1)
#define _NORM_AUX_COMP_EX_MASK      ((uint32_t)1<<_NORM_AUX_COMP_EX_SHIFT)
#define _NORM_AUX_UNSAFE_MASK       ((uint32_t)1<<_NORM_AUX_UNSAFE_SHIFT)
#define _NORM_AUX_NFC_SKIP_F_MASK   ((uint32_t)1<<_NORM_AUX_NFC_SKIPPABLE_F_SHIFT)


enum {
    _NORM_SET_INDEX_CANON_SETS_LENGTH,      
    _NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH, 
    _NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH,

    
    _NORM_SET_INDEX_NX_CJK_COMPAT_OFFSET,   

    _NORM_SET_INDEX_NX_UNICODE32_OFFSET,    

    _NORM_SET_INDEX_NX_RESERVED_OFFSET,     


    _NORM_SET_INDEX_TOP=32                  
};




#define _NORM_MAX_CANON_SETS            0x4000


#define _NORM_CANON_SET_BMP_MASK        0xc000
#define _NORM_CANON_SET_BMP_IS_INDEX    0x4000


enum {
    _NORM_INDEX_TRIE_SIZE,              
    _NORM_INDEX_UCHAR_COUNT,            

    _NORM_INDEX_COMBINE_DATA_COUNT,     
    _NORM_INDEX_COMBINE_FWD_COUNT,      
    _NORM_INDEX_COMBINE_BOTH_COUNT,     
    _NORM_INDEX_COMBINE_BACK_COUNT,     

    _NORM_INDEX_MIN_NFC_NO_MAYBE,       
    _NORM_INDEX_MIN_NFKC_NO_MAYBE,      
    _NORM_INDEX_MIN_NFD_NO_MAYBE,       
    _NORM_INDEX_MIN_NFKD_NO_MAYBE,      

    _NORM_INDEX_FCD_TRIE_SIZE,          

    _NORM_INDEX_AUX_TRIE_SIZE,          
    _NORM_INDEX_CANON_SET_COUNT,        

    _NORM_INDEX_TOP=32                  
};

enum {
    
    _NORM_MIN_WITH_LEAD_CC=0x300
};

enum {
    






    _NORM_DECOMP_FLAG_LENGTH_HAS_CC=0x80,
    


    _NORM_DECOMP_LENGTH_MASK=0x7f
};


enum {
    
    UNORM_NX_HANGUL=1,
    
    UNORM_NX_CJK_COMPAT=2
};
































































































































































































































































































































#endif 

#endif
