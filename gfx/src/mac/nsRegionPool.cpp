




































#include "nsRegionPool.h"


NS_EXPORT nsNativeRegionPool sNativeRegionPool;



nsNativeRegionPool::nsNativeRegionPool()
{
	mRegionSlots = nsnull;
	mEmptySlots = nsnull;
}



nsNativeRegionPool::~nsNativeRegionPool()
{
	
	if (mRegionSlots != nsnull) {
		nsRegionSlot* slot = mRegionSlots;
		while (slot != nsnull) {
			::DisposeRgn(slot->mRegion);
			nsRegionSlot* next = slot->mNext;
			delete slot;
			slot = next;
		}
	}
	
	
	if (mEmptySlots != nsnull) {
		nsRegionSlot* slot = mEmptySlots;
		while (slot != nsnull) {
			nsRegionSlot* next = slot->mNext;
			delete slot;
			slot = next;
		}
	}
}



RgnHandle nsNativeRegionPool::GetNewRegion()
{
	nsRegionSlot* slot = mRegionSlots;
	if (slot != nsnull) {
		RgnHandle region = slot->mRegion;

		
		mRegionSlots = slot->mNext;
		
		
		slot->mRegion = nsnull;
		slot->mNext = mEmptySlots;
		mEmptySlots = slot;
		
		
		::SetEmptyRgn(region);
		return region;
	}
	
	
	
	return (::NewRgn());
}



void nsNativeRegionPool::ReleaseRegion(RgnHandle aRgnHandle)
{
	nsRegionSlot* slot = mEmptySlots;
	if (slot != nsnull)
		mEmptySlots = slot->mNext;
	else
		slot = new nsRegionSlot;
	
	if (slot != nsnull) {
		
		slot->mRegion = aRgnHandle;
		slot->mNext = mRegionSlots;
		mRegionSlots = slot;
	} else {
		
		::DisposeRgn(aRgnHandle);
	}
}

