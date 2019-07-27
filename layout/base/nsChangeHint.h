






#ifndef nsChangeHint_h___
#define nsChangeHint_h___

#include "nsDebug.h"
#include "mozilla/Types.h"



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

  




  nsChangeHint_ReconstructFrame = 0x400,

  



  nsChangeHint_UpdateOverflow = 0x800,

  



  nsChangeHint_UpdateSubtreeOverflow = 0x1000,

  



  nsChangeHint_UpdatePostTransformOverflow = 0x2000,

  



  nsChangeHint_ChildrenOnlyTransform = 0x4000,

  









  nsChangeHint_RecomputePosition = 0x8000,

  





  nsChangeHint_AddOrRemoveTransform = 0x10000,

  





  nsChangeHint_BorderStyleNoneChange = 0x20000,

  



  nsChangeHint_UpdateTextPath = 0x40000,

  



  nsChangeHint_SchedulePaint = 0x80000,

  













  nsChangeHint_NeutralChange = 0x100000

  
  
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










#define nsChangeHint_Hints_NotHandledForDescendants nsChangeHint( \
          nsChangeHint_UpdateTransformLayer | \
          nsChangeHint_UpdateEffects | \
          nsChangeHint_UpdateOpacityLayer | \
          nsChangeHint_UpdateOverflow | \
          nsChangeHint_UpdatePostTransformOverflow | \
          nsChangeHint_ChildrenOnlyTransform | \
          nsChangeHint_RecomputePosition | \
          nsChangeHint_AddOrRemoveTransform | \
          nsChangeHint_BorderStyleNoneChange | \
          nsChangeHint_NeedReflow | \
          nsChangeHint_ClearAncestorIntrinsics)

inline nsChangeHint NS_HintsNotHandledForDescendantsIn(nsChangeHint aChangeHint) {
  nsChangeHint result = nsChangeHint(aChangeHint & (
    nsChangeHint_UpdateTransformLayer |
    nsChangeHint_UpdateEffects |
    nsChangeHint_UpdateOpacityLayer |
    nsChangeHint_UpdateOverflow |
    nsChangeHint_UpdatePostTransformOverflow |
    nsChangeHint_ChildrenOnlyTransform |
    nsChangeHint_RecomputePosition |
    nsChangeHint_AddOrRemoveTransform |
    nsChangeHint_BorderStyleNoneChange));

  if (!NS_IsHintSubset(nsChangeHint_NeedDirtyReflow, aChangeHint) &&
      NS_IsHintSubset(nsChangeHint_NeedReflow, aChangeHint)) {
    
    
    NS_UpdateHint(result, nsChangeHint_NeedReflow);
  }

  if (!NS_IsHintSubset(nsChangeHint_ClearDescendantIntrinsics, aChangeHint) &&
      NS_IsHintSubset(nsChangeHint_ClearAncestorIntrinsics, aChangeHint)) {
    
    
    NS_UpdateHint(result, nsChangeHint_ClearAncestorIntrinsics);
  }

  NS_ABORT_IF_FALSE(NS_IsHintSubset(result,
                                    nsChangeHint_Hints_NotHandledForDescendants),
                    "something is inconsistent");

  return result;
}


#define NS_STYLE_HINT_NONE \
  nsChangeHint(0)
#define NS_STYLE_HINT_VISUAL \
  nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView | \
               nsChangeHint_SchedulePaint)
#define nsChangeHint_AllReflowHints                     \
  nsChangeHint(nsChangeHint_NeedReflow |                \
               nsChangeHint_ClearAncestorIntrinsics |   \
               nsChangeHint_ClearDescendantIntrinsics | \
               nsChangeHint_NeedDirtyReflow)
#define NS_STYLE_HINT_REFLOW \
  nsChangeHint(NS_STYLE_HINT_VISUAL | nsChangeHint_AllReflowHints)
#define NS_STYLE_HINT_FRAMECHANGE \
  nsChangeHint(NS_STYLE_HINT_REFLOW | nsChangeHint_ReconstructFrame)
















enum nsRestyleHint {
  
  
  
  
  eRestyle_Self = (1<<0),

  
  eRestyle_Subtree = (1<<1),

  
  
  eRestyle_LaterSiblings = (1<<2),

  
  
  
  
  
  eRestyle_CSSTransitions = (1<<3),

  
  
  
  
  
  eRestyle_CSSAnimations = (1<<4),

  
  
  
  
  
  eRestyle_SVGAttrAnimations = (1<<5),

  
  
  
  
  
  
  
  eRestyle_StyleAttribute = (1<<6),

  
  
  eRestyle_Force = (1<<8),

  
  
  
  eRestyle_ForceDescendants = (1<<9),
};



typedef decltype(nsRestyleHint(0) + nsRestyleHint(0)) nsRestyleHint_size_t;

inline nsRestyleHint operator|(nsRestyleHint aLeft, nsRestyleHint aRight)
{
  return nsRestyleHint(nsRestyleHint_size_t(aLeft) |
                       nsRestyleHint_size_t(aRight));
}

inline nsRestyleHint operator&(nsRestyleHint aLeft, nsRestyleHint aRight)
{
  return nsRestyleHint(nsRestyleHint_size_t(aLeft) &
                       nsRestyleHint_size_t(aRight));
}

inline nsRestyleHint& operator|=(nsRestyleHint& aLeft, nsRestyleHint aRight)
{
  return aLeft = aLeft | aRight;
}

inline nsRestyleHint& operator&=(nsRestyleHint& aLeft, nsRestyleHint aRight)
{
  return aLeft = aLeft & aRight;
}

inline nsRestyleHint operator~(nsRestyleHint aArg)
{
  return nsRestyleHint(~nsRestyleHint_size_t(aArg));
}

inline nsRestyleHint operator^(nsRestyleHint aLeft, nsRestyleHint aRight)
{
  return nsRestyleHint(nsRestyleHint_size_t(aLeft) ^
                       nsRestyleHint_size_t(aRight));
}

inline nsRestyleHint operator^=(nsRestyleHint& aLeft, nsRestyleHint aRight)
{
  return aLeft = aLeft ^ aRight;
}

#endif 
