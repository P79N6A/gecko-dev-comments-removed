




































#ifndef nsGraphicsStatePh_h___
#define nsGraphicsStatePh_h___

#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsTransform2D.h"
#include "nsRegionPh.h"
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
  friend class nsRenderingContextPh;
#endif

private:
  inline nsGraphicsState()
		{
		mMatrix = nsnull;
		mClipRegion = nsnull;
		mColor = NS_RGB(0, 0, 0);
		mLineStyle = nsLineStyle_kSolid;
		mFontMetrics = nsnull;
		}

	inline
  ~nsGraphicsState()
		{
		NS_IF_RELEASE(mFontMetrics);
		}
};

class nsGraphicsStatePool
{
public:

  static
	inline nsGraphicsState * GetNewGS()
		{
  	nsGraphicsStatePool * thePool = PrivateGetPool();
  	return thePool->PrivateGetNewGS();
		}

  static inline void ReleaseGS(nsGraphicsState* aGS)
		{
  	nsGraphicsStatePool * thePool = PrivateGetPool();
  	thePool->PrivateReleaseGS(aGS);
		}
  
  
	inline
  nsGraphicsStatePool() { mFreeList = nsnull; }

  inline ~nsGraphicsStatePool()
		{
  	nsGraphicsState* gs = mFreeList;
  	while (gs != nsnull) {
    	nsGraphicsState* next = gs->mNext;
    	delete gs;
    	gs = next;
  		}
		}
  
private:
  nsGraphicsState*	mFreeList;
  
  inline static nsGraphicsStatePool * PrivateGetPool();
  inline nsGraphicsState *            PrivateGetNewGS();
  inline void                  			  PrivateReleaseGS(nsGraphicsState* aGS);
  
  static nsGraphicsStatePool * gsThePool;
};

#endif 
