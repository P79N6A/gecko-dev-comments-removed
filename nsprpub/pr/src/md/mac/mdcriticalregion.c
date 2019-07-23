






































#include "mdcriticalregion.h"
#include <MacErrors.h>








typedef struct MDCriticalRegionData_struct {
  MPTaskID        mMPTaskID;          
  UInt32          mDepthCount;        
  MPSemaphoreID   mMPSemaphoreID;     
} MDCriticalRegionData, *MDCriticalRegionDataPtr;


OSStatus
MD_CriticalRegionCreate(MDCriticalRegionID * outCriticalRegionID)
{
  MDCriticalRegionDataPtr newCriticalRegionPtr;
  MPSemaphoreID           mpSemaphoreID;
  OSStatus                err = noErr;

  if (outCriticalRegionID == NULL)
    return paramErr;

  *outCriticalRegionID = NULL;

  newCriticalRegionPtr = (MDCriticalRegionDataPtr)MPAllocateAligned(sizeof(MDCriticalRegionData),
                        kMPAllocateDefaultAligned, kMPAllocateClearMask);
  if (newCriticalRegionPtr == NULL)
    return memFullErr;

  
  err = MPCreateBinarySemaphore(&mpSemaphoreID);
  if (err == noErr)
  {
    newCriticalRegionPtr->mMPTaskID = kInvalidID;
    newCriticalRegionPtr->mDepthCount = 0;
    newCriticalRegionPtr->mMPSemaphoreID = mpSemaphoreID;

    *outCriticalRegionID = (MDCriticalRegionID)newCriticalRegionPtr;
  }
  else
  {
    MPFree((LogicalAddress)newCriticalRegionPtr);
  }

  return err;
}

OSStatus
MD_CriticalRegionDelete(MDCriticalRegionID inCriticalRegionID)
{
  MDCriticalRegionDataPtr criticalRegion = (MDCriticalRegionDataPtr)inCriticalRegionID;
  OSStatus                err = noErr;

  if (criticalRegion == NULL)
    return paramErr;

  if ((criticalRegion->mMPTaskID != kInvalidID) && (criticalRegion->mDepthCount > 0))
    return kMPInsufficientResourcesErr;

  if (criticalRegion->mMPSemaphoreID != kInvalidID)
    err = MPDeleteSemaphore(criticalRegion->mMPSemaphoreID);
  if (noErr != err) return err;

  criticalRegion->mMPSemaphoreID = kInvalidID;
  MPFree((LogicalAddress) criticalRegion);

  return noErr;
}

OSStatus
MD_CriticalRegionEnter(MDCriticalRegionID inCriticalRegionID, Duration inTimeout)
{
  MDCriticalRegionDataPtr criticalRegion = (MDCriticalRegionDataPtr)inCriticalRegionID;
  MPTaskID                currentTaskID = MPCurrentTaskID();
  OSStatus                err = noErr;

  if (criticalRegion == NULL)
    return paramErr;

  
  if (currentTaskID == criticalRegion->mMPTaskID)
  {
    
    criticalRegion->mDepthCount++;
    
    return noErr;
  }

  
  err = MPWaitOnSemaphore(criticalRegion->mMPSemaphoreID, inTimeout);
  
  if (noErr != err) return err;

  
  criticalRegion->mMPTaskID = currentTaskID;
  criticalRegion->mDepthCount = 1;

  return noErr;
}

OSStatus
MD_CriticalRegionExit(MDCriticalRegionID inCriticalRegionID)
{
  MDCriticalRegionDataPtr   criticalRegion = (MDCriticalRegionDataPtr)inCriticalRegionID;
  MPTaskID                  currentTaskID = MPCurrentTaskID();
  OSStatus                  err = noErr;

  
  if (currentTaskID != criticalRegion->mMPTaskID)
    return kMPInsufficientResourcesErr;

  
  if (criticalRegion->mDepthCount == 0)
    return kMPInsufficientResourcesErr;

  
  criticalRegion->mDepthCount--;

  
  if (criticalRegion->mDepthCount == 0)
  {
    
    criticalRegion->mMPTaskID = kInvalidID;
    
    err = MPSignalSemaphore(criticalRegion->mMPSemaphoreID);
  }
  return err;
}

