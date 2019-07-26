

















#ifndef __UBIDI_PROPS_H__
#define __UBIDI_PROPS_H__

#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "uset_imp.h"
#include "udataswp.h"

U_CDECL_BEGIN



struct UBiDiProps;
typedef struct UBiDiProps UBiDiProps;

U_CFUNC const UBiDiProps *
ubidi_getSingleton(void);

U_CFUNC void
ubidi_addPropertyStarts(const UBiDiProps *bdp, const USetAdder *sa, UErrorCode *pErrorCode);



U_CFUNC int32_t
ubidi_getMaxValue(const UBiDiProps *bdp, UProperty which);

U_CAPI UCharDirection
ubidi_getClass(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UBool
ubidi_isMirrored(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UChar32
ubidi_getMirror(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UBool
ubidi_isBidiControl(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UBool
ubidi_isJoinControl(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UJoiningType
ubidi_getJoiningType(const UBiDiProps *bdp, UChar32 c);

U_CFUNC UJoiningGroup
ubidi_getJoiningGroup(const UBiDiProps *bdp, UChar32 c);



#define UBIDI_DATA_NAME "ubidi"
#define UBIDI_DATA_TYPE "icu"


#define UBIDI_FMT_0 0x42
#define UBIDI_FMT_1 0x69
#define UBIDI_FMT_2 0x44
#define UBIDI_FMT_3 0x69


enum {
    UBIDI_IX_INDEX_TOP,
    UBIDI_IX_LENGTH,
    UBIDI_IX_TRIE_SIZE,
    UBIDI_IX_MIRROR_LENGTH,

    UBIDI_IX_JG_START,
    UBIDI_IX_JG_LIMIT,

    UBIDI_MAX_VALUES_INDEX=15,
    UBIDI_IX_TOP=16
};



enum {
      
    UBIDI_JT_SHIFT=5,           

    

    UBIDI_JOIN_CONTROL_SHIFT=10,
    UBIDI_BIDI_CONTROL_SHIFT=11,

    UBIDI_IS_MIRRORED_SHIFT=12,         
    UBIDI_MIRROR_DELTA_SHIFT=13,        

    UBIDI_MAX_JG_SHIFT=16               
};

#define UBIDI_CLASS_MASK        0x0000001f
#define UBIDI_JT_MASK           0x000000e0

#define UBIDI_MAX_JG_MASK       0x00ff0000

#define UBIDI_GET_CLASS(props) ((props)&UBIDI_CLASS_MASK)
#define UBIDI_GET_FLAG(props, shift) (((props)>>(shift))&1)

enum {
    UBIDI_ESC_MIRROR_DELTA=-4,
    UBIDI_MIN_MIRROR_DELTA=-3,
    UBIDI_MAX_MIRROR_DELTA=3
};



enum {
    
    UBIDI_MIRROR_INDEX_SHIFT=21,
    UBIDI_MAX_MIRROR_INDEX=0x7ff
};

#define UBIDI_GET_MIRROR_CODE_POINT(m) (UChar32)((m)&0x1fffff)

#define UBIDI_GET_MIRROR_INDEX(m) ((m)>>UBIDI_MIRROR_INDEX_SHIFT)

U_CDECL_END

#endif
