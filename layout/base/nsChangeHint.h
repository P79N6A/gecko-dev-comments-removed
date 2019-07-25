






































#ifndef nsChangeHint_h___
#define nsChangeHint_h___

#include "prtypes.h"



enum nsChangeHint {
  
  nsChangeHint_RepaintFrame = 0x01,

  
  
  nsChangeHint_NeedReflow = 0x02,

  
  
  nsChangeHint_ClearAncestorIntrinsics = 0x04,

  
  
  nsChangeHint_ClearDescendantIntrinsics = 0x08,

  
  
  
  nsChangeHint_NeedDirtyReflow = 0x10,

  
  nsChangeHint_SyncFrameView = 0x20,

  
  nsChangeHint_UpdateCursor = 0x40,

  








  nsChangeHint_UpdateEffects = 0x80,

  



  nsChangeHint_UpdateOpacityLayer = 0x100,
  nsChangeHint_UpdateTransformLayer = 0x200,

  
  
  nsChangeHint_ReconstructFrame = 0x400
};




inline void operator<(nsChangeHint s1, nsChangeHint s2) {}
inline void operator>(nsChangeHint s1, nsChangeHint s2) {}
inline void operator!=(nsChangeHint s1, nsChangeHint s2) {}
inline void operator==(nsChangeHint s1, nsChangeHint s2) {}
inline void operator<=(nsChangeHint s1, nsChangeHint s2) {}
inline void operator>=(nsChangeHint s1, nsChangeHint s2) {}




inline nsChangeHint NS_CombineHint(nsChangeHint aH1, nsChangeHint aH2) {
  return (nsChangeHint)(aH1 | aH2);
}


inline nsChangeHint NS_SubtractHint(nsChangeHint aH1, nsChangeHint aH2) {
  return (nsChangeHint)(aH1 & ~aH2);
}



inline bool NS_UpdateHint(nsChangeHint& aDest, nsChangeHint aSrc) {
  nsChangeHint r = (nsChangeHint)(aDest | aSrc);
  bool changed = (int)r != (int)aDest;
  aDest = r;
  return changed;
}


inline bool NS_IsHintSubset(nsChangeHint aSubset, nsChangeHint aSuperSet) {
  return (aSubset & aSuperSet) == aSubset;
}


#define NS_STYLE_HINT_NONE \
  nsChangeHint(0)
#define NS_STYLE_HINT_VISUAL \
  nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView)
#define nsChangeHint_ReflowFrame                        \
  nsChangeHint(nsChangeHint_NeedReflow |                \
               nsChangeHint_ClearAncestorIntrinsics |   \
               nsChangeHint_ClearDescendantIntrinsics | \
               nsChangeHint_NeedDirtyReflow)
#define NS_STYLE_HINT_REFLOW \
  nsChangeHint(NS_STYLE_HINT_VISUAL | nsChangeHint_ReflowFrame)
#define NS_STYLE_HINT_FRAMECHANGE \
  nsChangeHint(NS_STYLE_HINT_REFLOW | nsChangeHint_ReconstructFrame)






enum nsRestyleHint {
  eRestyle_Self = 0x1,
  eRestyle_Subtree = 0x2, 
  eRestyle_LaterSiblings = 0x4 
};


#endif 
