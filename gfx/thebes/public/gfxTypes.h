




































#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include "prtypes.h"





typedef double gfxFloat;

#if defined(MOZ_STATIC_BUILD)
# define THEBES_API
#elif defined(IMPL_THEBES)
# define THEBES_API NS_EXPORT
#else
# define THEBES_API NS_IMPORT
#endif






#define NS_ERROR_GFX_GENERAL_BASE (50) 


#define NS_ERROR_GFX_CMAP_MALFORMED          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_GENERAL_BASE+1)




















enum gfxBreakPriority {
    eNoBreak       = 0,
    eWordWrapBreak,
    eNormalBreak
};





#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"

#define THEBES_INLINE_DECL_REFCOUNTING(_class)                                \
public:                                                                       \
    nsrefcnt AddRef(void) {                                                   \
        NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");             \
        ++mRefCnt;                                                            \
        NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                 \
        return mRefCnt;                                                       \
    }                                                                         \
    nsrefcnt Release(void) {                                                  \
        NS_PRECONDITION(0 != mRefCnt, "dup release");                         \
        --mRefCnt;                                                            \
        NS_LOG_RELEASE(this, mRefCnt, #_class);                               \
        if (mRefCnt == 0) {                                                   \
            mRefCnt = 1; /* stabilize */                                      \
            NS_DELETEXPCOM(this);                                             \
            return 0;                                                         \
        }                                                                     \
        return mRefCnt;                                                       \
    }                                                                         \
protected:                                                                    \
    nsAutoRefCnt mRefCnt;                                                     \
public:

#endif 
