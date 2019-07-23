






































#ifndef nsChangeHint_h___
#define nsChangeHint_h___

#include "prtypes.h"



enum nsChangeHint {
  nsChangeHint_RepaintFrame = 0x01,  
  nsChangeHint_ReflowFrame = 0x02,   
  nsChangeHint_SyncFrameView = 0x04, 
  nsChangeHint_UpdateCursor = 0x08,  
  








  nsChangeHint_UpdateEffects = 0x10,
  nsChangeHint_ReconstructFrame = 0x20   
                                         
  
  
};

#ifdef DEBUG_roc



inline void operator<(nsChangeHint s1, nsChangeHint s2) {}
inline void operator>(nsChangeHint s1, nsChangeHint s2) {}
inline void operator!=(nsChangeHint s1, nsChangeHint s2) {}
inline void operator==(nsChangeHint s1, nsChangeHint s2) {}
inline void operator<=(nsChangeHint s1, nsChangeHint s2) {}
inline void operator>=(nsChangeHint s1, nsChangeHint s2) {}
#endif




inline nsChangeHint NS_CombineHint(nsChangeHint aH1, nsChangeHint aH2) {
  return (nsChangeHint)(aH1 | aH2);
}


inline nsChangeHint NS_SubtractHint(nsChangeHint aH1, nsChangeHint aH2) {
  return (nsChangeHint)(aH1 & ~aH2);
}



inline PRBool NS_UpdateHint(nsChangeHint& aDest, nsChangeHint aSrc) {
  nsChangeHint r = (nsChangeHint)(aDest | aSrc);
  PRBool changed = (int)r != (int)aDest;
  aDest = r;
  return changed;
}


inline PRBool NS_IsHintSubset(nsChangeHint aSubset, nsChangeHint aSuperSet) {
  return (aSubset & aSuperSet) == aSubset;
}


#define NS_STYLE_HINT_NONE \
  nsChangeHint(0)
#define NS_STYLE_HINT_VISUAL \
  nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView)
#define NS_STYLE_HINT_REFLOW \
  nsChangeHint(NS_STYLE_HINT_VISUAL | nsChangeHint_ReflowFrame)
#define NS_STYLE_HINT_FRAMECHANGE \
  nsChangeHint(NS_STYLE_HINT_REFLOW | nsChangeHint_ReconstructFrame)







enum nsReStyleHint {
  eReStyle_Self = 0x1,
  eReStyle_LaterSiblings = 0x2
};


#endif 
