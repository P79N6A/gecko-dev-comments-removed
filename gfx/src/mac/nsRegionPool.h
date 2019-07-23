




































#ifndef nsRegionPool_h___
#define nsRegionPool_h___

#include "nscore.h"
#include <Quickdraw.h>


class NS_EXPORT nsNativeRegionPool
{
public:
	nsNativeRegionPool();
	~nsNativeRegionPool();

	RgnHandle					GetNewRegion();
	void							ReleaseRegion(RgnHandle aRgnHandle);

private:
	struct nsRegionSlot {
		RgnHandle				mRegion;
		nsRegionSlot*		mNext;
	};

	nsRegionSlot*			mRegionSlots;
	nsRegionSlot*			mEmptySlots;
};



extern nsNativeRegionPool sNativeRegionPool;



class StRegionFromPool 
{
public:
	StRegionFromPool()
	{
		mRegionH = sNativeRegionPool.GetNewRegion();
	}
	
	~StRegionFromPool()
	{
		if ( mRegionH )
			sNativeRegionPool.ReleaseRegion(mRegionH);
	}

	operator RgnHandle() const
	{
		return mRegionH;
	}

	private:
		RgnHandle mRegionH;
};



#endif 
