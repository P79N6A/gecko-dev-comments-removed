




































#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include "prtypes.h"
#include "nsAtomicRefcnt.h"





typedef double gfxFloat;

# define THEBES_API






#define NS_ERROR_GFX_GENERAL_BASE (50) 


#define NS_ERROR_GFX_CMAP_MALFORMED          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_GENERAL_BASE+1)




















enum gfxBreakPriority {
    eNoBreak       = 0,
    eWordWrapBreak,
    eNormalBreak
};

#endif 
