

















#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "unicode/udata.h" 
#include "ucmndata.h" 
#include "udatamem.h"
#include "uassert.h"
#include "cmemory.h"
#include "utrie2.h"
#include "ubidi_props.h"
#include "ucln_cmn.h"

struct UBiDiProps {
    UDataMemory *mem;
    const int32_t *indexes;
    const uint32_t *mirrors;
    const uint8_t *jgArray;

    UTrie2 trie;
    uint8_t formatVersion[4];
};


#define INCLUDED_FROM_UBIDI_PROPS_C
#include "ubidi_props_data.h"



U_CFUNC const UBiDiProps *
ubidi_getSingleton() {
    return &ubidi_props_singleton;
}



static UBool U_CALLCONV
_enumPropertyStartsRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

U_CFUNC void
ubidi_addPropertyStarts(const UBiDiProps *bdp, const USetAdder *sa, UErrorCode *pErrorCode) {
    int32_t i, length;
    UChar32 c, start, limit;

    const uint8_t *jgArray;
    uint8_t prev, jg;

    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    
    utrie2_enum(&bdp->trie, NULL, _enumPropertyStartsRange, sa);

    
    length=bdp->indexes[UBIDI_IX_MIRROR_LENGTH];
    for(i=0; i<length; ++i) {
        c=UBIDI_GET_MIRROR_CODE_POINT(bdp->mirrors[i]);
        sa->addRange(sa->set, c, c+1);
    }

    
    start=bdp->indexes[UBIDI_IX_JG_START];
    limit=bdp->indexes[UBIDI_IX_JG_LIMIT];
    jgArray=bdp->jgArray;
    prev=0;
    while(start<limit) {
        jg=*jgArray++;
        if(jg!=prev) {
            sa->add(sa->set, start);
            prev=jg;
        }
        ++start;
    }
    if(prev!=0) {
        
        sa->add(sa->set, limit);
    }

    

    
}



U_CFUNC int32_t
ubidi_getMaxValue(const UBiDiProps *bdp, UProperty which) {
    int32_t max;

    if(bdp==NULL) {
        return -1;
    }

    max=bdp->indexes[UBIDI_MAX_VALUES_INDEX];
    switch(which) {
    case UCHAR_BIDI_CLASS:
        return (max&UBIDI_CLASS_MASK);
    case UCHAR_JOINING_GROUP:
        return (max&UBIDI_MAX_JG_MASK)>>UBIDI_MAX_JG_SHIFT;
    case UCHAR_JOINING_TYPE:
        return (max&UBIDI_JT_MASK)>>UBIDI_JT_SHIFT;
    default:
        return -1; 
    }
}

U_CAPI UCharDirection
ubidi_getClass(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    return (UCharDirection)UBIDI_GET_CLASS(props);
}

U_CFUNC UBool
ubidi_isMirrored(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    return (UBool)UBIDI_GET_FLAG(props, UBIDI_IS_MIRRORED_SHIFT);
}

U_CFUNC UChar32
ubidi_getMirror(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    int32_t delta=((int16_t)props)>>UBIDI_MIRROR_DELTA_SHIFT;
    if(delta!=UBIDI_ESC_MIRROR_DELTA) {
        return c+delta;
    } else {
        
        const uint32_t *mirrors;
        uint32_t m;
        int32_t i, length;
        UChar32 c2;

        mirrors=bdp->mirrors;
        length=bdp->indexes[UBIDI_IX_MIRROR_LENGTH];

        
        for(i=0; i<length; ++i) {
            m=mirrors[i];
            c2=UBIDI_GET_MIRROR_CODE_POINT(m);
            if(c==c2) {
                
                return UBIDI_GET_MIRROR_CODE_POINT(mirrors[UBIDI_GET_MIRROR_INDEX(m)]);
            } else if(c<c2) {
                break;
            }
        }

        
        return c;
    }
}

U_CFUNC UBool
ubidi_isBidiControl(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    return (UBool)UBIDI_GET_FLAG(props, UBIDI_BIDI_CONTROL_SHIFT);
}

U_CFUNC UBool
ubidi_isJoinControl(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    return (UBool)UBIDI_GET_FLAG(props, UBIDI_JOIN_CONTROL_SHIFT);
}

U_CFUNC UJoiningType
ubidi_getJoiningType(const UBiDiProps *bdp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&bdp->trie, c);
    return (UJoiningType)((props&UBIDI_JT_MASK)>>UBIDI_JT_SHIFT);
}

U_CFUNC UJoiningGroup
ubidi_getJoiningGroup(const UBiDiProps *bdp, UChar32 c) {
    UChar32 start, limit;

    start=bdp->indexes[UBIDI_IX_JG_START];
    limit=bdp->indexes[UBIDI_IX_JG_LIMIT];
    if(start<=c && c<limit) {
        return (UJoiningGroup)bdp->jgArray[c-start];
    } else {
        return U_JG_NO_JOINING_GROUP;
    }
}



U_CFUNC UCharDirection
u_charDirection(UChar32 c) {   
    return ubidi_getClass(&ubidi_props_singleton, c);
}

U_CFUNC UBool
u_isMirrored(UChar32 c) {
    return ubidi_isMirrored(&ubidi_props_singleton, c);
}

U_CFUNC UChar32
u_charMirror(UChar32 c) {
    return ubidi_getMirror(&ubidi_props_singleton, c);
}
