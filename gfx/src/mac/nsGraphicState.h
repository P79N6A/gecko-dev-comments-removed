





































#ifndef nsGraphicState_h___
#define nsGraphicState_h___

#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsCRT.h"

#ifndef __QUICKDRAW__
#include <Quickdraw.h>
#endif



class nsGraphicState
{
public:
  nsGraphicState();
  ~nsGraphicState();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

	void				Clear();
	void				Init(nsIDrawingSurface* aSurface);
	void				Init(CGrafPtr aPort);
	void				Init(nsIWidget* aWindow);
	void				Duplicate(nsGraphicState* aGS);	
																							
	void				SetChanges(PRUint32 aChanges) { mChanges = aChanges; }
	PRUint32		GetChanges() { return mChanges; }	

protected:
	RgnHandle		DuplicateRgn(RgnHandle aRgn, RgnHandle aDestRgn = nsnull);

public:
	nsTransform2D 				mTMatrix; 						

	PRInt32               mOffx;
  PRInt32               mOffy;

  RgnHandle							mMainRegion;
  RgnHandle			    		mClipRegion;

  nscolor               mColor;
  PRInt32               mFont;
  nsIFontMetrics * 			mFontMetrics;
	PRInt32               mCurrFontHandle;
  nsLineStyle           mLineStyle;
	
	PRUint32							mChanges;							

private:
	friend class nsGraphicStatePool;
	nsGraphicState*				mNext;								
};



class nsGraphicStatePool
{
public:
	nsGraphicStatePool();
	~nsGraphicStatePool();

	nsGraphicState*		GetNewGS();
	void							ReleaseGS(nsGraphicState* aGS);

private:

	nsGraphicState*	mFreeList;
};



extern nsGraphicStatePool sGraphicStatePool;




#endif 
