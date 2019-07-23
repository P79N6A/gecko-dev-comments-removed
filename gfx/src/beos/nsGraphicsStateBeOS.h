




































#ifndef nsGraphicsStateBeOS_h___
#define nsGraphicsStateBeOS_h___

#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsTransform2D.h"
#include "nsRegionBeOS.h"
#include "nsCOMPtr.h"

class nsGraphicsState
{
public:

  nsTransform2D  *mMatrix;
  nsCOMPtr<nsIRegion> mClipRegion;
  nscolor         mColor;
  nsLineStyle     mLineStyle;
  nsIFontMetrics *mFontMetrics;

  nsGraphicsState *mNext; 

  friend class nsGraphicsStatePool;

#ifndef USE_GS_POOL
  friend class nsRenderingContextBeOS;
#endif

private:
  nsGraphicsState();
  ~nsGraphicsState();
};

class nsGraphicsStatePool
{
public:

  static nsGraphicsState * GetNewGS();
  static void              ReleaseGS(nsGraphicsState* aGS);
  
  nsGraphicsStatePool();
  ~nsGraphicsStatePool();
  
private:
  nsGraphicsState*	mFreeList;
  
  static nsGraphicsStatePool * PrivateGetPool();
  nsGraphicsState *            PrivateGetNewGS();
  void                         PrivateReleaseGS(nsGraphicsState* aGS);
  
  static nsGraphicsStatePool * gsThePool;
};

#endif 
