




































#include "nsGraphicsStateGTK.h"


nsGraphicsState::nsGraphicsState()
{
  mMatrix = nsnull;
  mClipRegion = nsnull;
  mColor = 0;
  mLineStyle = nsLineStyle_kSolid;
  mFontMetrics = nsnull;
}

nsGraphicsState::~nsGraphicsState()
{
  NS_IF_RELEASE(mFontMetrics);
}

nsGraphicsStatePool::nsGraphicsStatePool()
{
	mFreeList = nsnull;
}









 nsGraphicsState *
nsGraphicsStatePool::GetNewGS()
{
  nsGraphicsStatePool * thePool = PrivateGetPool();

  return thePool->PrivateGetNewGS();
}

 void
nsGraphicsStatePool::ReleaseGS(nsGraphicsState* aGS)
{
  nsGraphicsStatePool * thePool = PrivateGetPool();

  thePool->PrivateReleaseGS(aGS);
}











nsGraphicsStatePool *
nsGraphicsStatePool::gsThePool = nsnull;


nsGraphicsStatePool *
nsGraphicsStatePool::PrivateGetPool()
{
  if (nsnull == gsThePool)
  {
    gsThePool = new nsGraphicsStatePool();
  }

  return gsThePool;
}



nsGraphicsStatePool::~nsGraphicsStatePool()
{
	nsGraphicsState* gs = mFreeList;
	while (gs != nsnull) {
		nsGraphicsState* next = gs->mNext;
		delete gs;
		gs = next;
	}
}

nsGraphicsState *
nsGraphicsStatePool::PrivateGetNewGS()
{
	nsGraphicsState* gs = mFreeList;
	if (gs != nsnull) {
		mFreeList = gs->mNext;
		return gs;
	}
	return new nsGraphicsState;
}

void
nsGraphicsStatePool::PrivateReleaseGS(nsGraphicsState* aGS)
{
  
	aGS->mNext = mFreeList;
	mFreeList = aGS;
}


