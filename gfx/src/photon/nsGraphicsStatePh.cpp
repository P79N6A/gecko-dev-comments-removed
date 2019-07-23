




































#include "nsGraphicsStatePh.h"




nsGraphicsStatePool *
nsGraphicsStatePool::gsThePool = nsnull;


inline nsGraphicsStatePool *
nsGraphicsStatePool::PrivateGetPool()
{
  if (nsnull == gsThePool)
  {
    gsThePool = new nsGraphicsStatePool();
  }

  return gsThePool;
}

inline nsGraphicsState *
nsGraphicsStatePool::PrivateGetNewGS()
{
	nsGraphicsState* gs = mFreeList;
	if (gs != nsnull) {
		mFreeList = gs->mNext;
		return gs;
	}
	return new nsGraphicsState;
}

inline void nsGraphicsStatePool::PrivateReleaseGS(nsGraphicsState* aGS)
{
  
  aGS->mNext = mFreeList;
  mFreeList = aGS;
}


