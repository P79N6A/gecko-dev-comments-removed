





#ifndef frontend_SourceNotes_h
#define frontend_SourceNotes_h

#include <stdint.h>

#include "jstypes.h"

typedef uint8_t jssrcnote;

namespace js {























enum SrcNoteType {
    SRC_NULL        = 0,        

    SRC_IF          = 1,        
    SRC_IF_ELSE     = 2,        
    SRC_COND        = 3,        

    SRC_FOR         = 4,        

    SRC_WHILE       = 5,        


    SRC_FOR_IN      = 6,        

    SRC_CONTINUE    = 7,        
    SRC_BREAK       = 8,        
    SRC_BREAK2LABEL = 9,        
    SRC_SWITCHBREAK = 10,       

    SRC_TABLESWITCH = 11,       

    SRC_CONDSWITCH  = 12,       


    SRC_NEXTCASE    = 13,       


    SRC_ASSIGNOP    = 14,       

    SRC_HIDDEN      = 15,       

    SRC_CATCH       = 16,       

    SRC_TRY         = 17,       


    
    SRC_LAST_GETTABLE = SRC_TRY,

    SRC_COLSPAN     = 18,       
    SRC_NEWLINE     = 19,       
    SRC_SETLINE     = 20,       

    SRC_UNUSED21    = 21,
    SRC_UNUSED22    = 22,
    SRC_UNUSED23    = 23,

    SRC_XDELTA      = 24        
};


inline void
SN_MAKE_TERMINATOR(jssrcnote *sn)
{
    *sn = SRC_NULL;
}

inline bool
SN_IS_TERMINATOR(jssrcnote *sn)
{
    return *sn == SRC_NULL;
}

}  

#define SN_TYPE_BITS            5
#define SN_DELTA_BITS           3
#define SN_XDELTA_BITS          6
#define SN_TYPE_MASK            (JS_BITMASK(SN_TYPE_BITS) << SN_DELTA_BITS)
#define SN_DELTA_MASK           ((ptrdiff_t)JS_BITMASK(SN_DELTA_BITS))
#define SN_XDELTA_MASK          ((ptrdiff_t)JS_BITMASK(SN_XDELTA_BITS))

#define SN_MAKE_NOTE(sn,t,d)    (*(sn) = (jssrcnote)                          \
                                          (((t) << SN_DELTA_BITS)             \
                                           | ((d) & SN_DELTA_MASK)))
#define SN_MAKE_XDELTA(sn,d)    (*(sn) = (jssrcnote)                          \
                                          ((SRC_XDELTA << SN_DELTA_BITS)      \
                                           | ((d) & SN_XDELTA_MASK)))

#define SN_IS_XDELTA(sn)        ((*(sn) >> SN_DELTA_BITS) >= SRC_XDELTA)
#define SN_TYPE(sn)             ((js::SrcNoteType)(SN_IS_XDELTA(sn)           \
                                                   ? SRC_XDELTA               \
                                                   : *(sn) >> SN_DELTA_BITS))
#define SN_SET_TYPE(sn,type)    SN_MAKE_NOTE(sn, type, SN_DELTA(sn))
#define SN_IS_GETTABLE(sn)      (SN_TYPE(sn) <= SRC_LAST_GETTABLE)

#define SN_DELTA(sn)            ((ptrdiff_t)(SN_IS_XDELTA(sn)                 \
                                             ? *(sn) & SN_XDELTA_MASK         \
                                             : *(sn) & SN_DELTA_MASK))
#define SN_SET_DELTA(sn,delta)  (SN_IS_XDELTA(sn)                             \
                                 ? SN_MAKE_XDELTA(sn, delta)                  \
                                 : SN_MAKE_NOTE(sn, SN_TYPE(sn), delta))

#define SN_DELTA_LIMIT          ((ptrdiff_t)JS_BIT(SN_DELTA_BITS))
#define SN_XDELTA_LIMIT         ((ptrdiff_t)JS_BIT(SN_XDELTA_BITS))






#define SN_3BYTE_OFFSET_FLAG    0x80
#define SN_3BYTE_OFFSET_MASK    0x7f












#define SN_COLSPAN_DOMAIN       ptrdiff_t(SN_3BYTE_OFFSET_FLAG << 16)

#define SN_MAX_OFFSET ((size_t)((ptrdiff_t)SN_3BYTE_OFFSET_FLAG << 16) - 1)

#define SN_LENGTH(sn)           ((js_SrcNoteSpec[SN_TYPE(sn)].arity == 0) ? 1 \
                                 : js_SrcNoteLength(sn))
#define SN_NEXT(sn)             ((sn) + SN_LENGTH(sn))

struct JSSrcNoteSpec {
    const char      *name;      
    int8_t          arity;      
};

extern JS_FRIEND_DATA(const JSSrcNoteSpec) js_SrcNoteSpec[];
extern JS_FRIEND_API(unsigned)         js_SrcNoteLength(jssrcnote *sn);




extern JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, unsigned which);

#endif 
