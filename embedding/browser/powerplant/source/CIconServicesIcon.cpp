





































 
#include "CIconServicesIcon.h"

#include "UResourceMgr.h"
#include "UMemoryMgr.h"

#include <Processes.h>





OSType CIconServicesIcon::mgAppCreator;
FSSpec CIconServicesIcon::mgIconFileSpec;

CIconServicesIcon::CIconServicesIcon(const SPaneInfo&	inPaneInfo,
	                                 MessageT			inValueMessage,
                                     OSType             inIconType,
                                     SInt16             inIconResID) :
    LControl(inPaneInfo, inValueMessage),
    mIconType(inIconType), mIconResID(inIconResID),
    mAlignmentType(kAlignAbsoluteCenter), mIconRef(nil),
    mbIsPressed(false)
{
    Init();
}


CIconServicesIcon::CIconServicesIcon(LStream*	inStream) :
    LControl(inStream),
    mAlignmentType(kAlignAbsoluteCenter), mIconRef(nil),
    mbIsPressed(false)
{
    *inStream >> mIconType;
    *inStream >> mIconResID;
    
    Init();
}


CIconServicesIcon::~CIconServicesIcon()
{
    ReleaseIconRef();
}

void CIconServicesIcon::DrawSelf()
{
  if (!mIconRef)
    return;
    
	Rect	iconRect;
	CalcLocalFrameRect(iconRect);
    AdjustIconRect(iconRect);
    	
	IconTransformType transform;
	if (mbIsPressed)
	    transform = kTransformSelected;
	else if (mEnabled != triState_On)
	    transform = kTransformDisabled;
	else
	    transform = kTransformNone;
	
	
	
	
	StRegion cleanRgn;
	if (::IconRefToRgn(cleanRgn,
	                   &iconRect,
	                   mAlignmentType,
	                   kIconServicesNormalUsageFlag,
                       mIconRef) == noErr)
        ::EraseRgn(cleanRgn);
    
    ::PlotIconRef(&iconRect,
                  mAlignmentType,
                  transform,
                  kIconServicesNormalUsageFlag,
                  mIconRef);
}

void CIconServicesIcon::EnableSelf()
{
    Refresh();
}

void CIconServicesIcon::DisableSelf()
{
    Refresh();
}

SInt16 CIconServicesIcon::FindHotSpot(Point	inPoint) const
{
	  Boolean inHotSpot = PointInHotSpot(inPoint, 0);
	  return inHotSpot ? 1 : 0;
}

Boolean CIconServicesIcon::PointInHotSpot(Point		inPoint,
								          SInt16	inHotSpot) const
{
  if (!mIconRef)
    return false;
    
	Rect	iconRect;
	CalcLocalFrameRect(iconRect);
    AdjustIconRect(iconRect);

    return ::PtInIconRef(&inPoint, &iconRect, mAlignmentType, kIconServicesNormalUsageFlag, mIconRef);
}

void CIconServicesIcon::HotSpotAction(SInt16		,
	                                  Boolean		inCurrInside,
	                                  Boolean		inPrevInside)
{
	if (inCurrInside != inPrevInside)
	{
	    mbIsPressed = inCurrInside;
	    Draw(nil);	
    }
}

void CIconServicesIcon::HotSpotResult(SInt16	)
{
	BroadcastValueMessage();
}

void CIconServicesIcon::Init()
{
    static bool gInitialized;
    
    if (!gInitialized)
    {
        OSErr err;
        
        
        
        long response;
        err = ::Gestalt(gestaltIconUtilitiesAttr, &response);
        ThrowIfError_(err);
        if (!(response & gestaltIconUtilitiesHasIconServices))
            Throw_(-12345);
        
        ProcessSerialNumber psn;
        err = ::GetCurrentProcess(&psn);
        ThrowIfError_(err);
        
        ProcessInfoRec info;
        info.processInfoLength = sizeof(info);
        info.processName = nil;
        info.processAppSpec = nil;
        err = ::GetProcessInformation(&psn, &info);
        ThrowIfError_(err);
        mgAppCreator = info.processSignature;
        
        
        
        
        
        
        StResLoad resLoadState(false);
        StResource resHandle('icns', mIconResID); 
        SInt16 resRefNum = ::HomeResFile(resHandle);
        if (resRefNum != -1)
        {
          FCBPBRec pb;
      
          pb.ioNamePtr = mgIconFileSpec.name;
          pb.ioVRefNum = 0;
          pb.ioRefNum = resRefNum;
          pb.ioFCBIndx = 0;
          err = PBGetFCBInfoSync(&pb);
          if (err == noErr)
          {
              mgIconFileSpec.vRefNum = pb.ioFCBVRefNum;
              mgIconFileSpec.parID = pb.ioFCBParID;
          }
        }
        gInitialized = true;
    }
    GetIconRef();
}

void CIconServicesIcon::AdjustIconRect(Rect& ioRect) const
{
    SDimension16 frameSize;
    GetFrameSize(frameSize);
    SInt16 iconSize = (frameSize.width <= 16 && frameSize.height <= 16) ? 16 : 32;
    
	ioRect.top += ((ioRect.bottom - ioRect.top) - iconSize) / 2;
	ioRect.left += ((ioRect.right - ioRect.left) - iconSize) / 2;
    ioRect.right = ioRect.left + iconSize;
    ioRect.bottom = ioRect.top + iconSize;
}
	
void CIconServicesIcon::GetIconRef()
{
    
    
    
    
    ::RegisterIconRefFromResource(mgAppCreator, mIconType, &mgIconFileSpec, mIconResID, &mIconRef);
}

void CIconServicesIcon::ReleaseIconRef()
{
    if (mIconRef)
    {
        ::ReleaseIconRef(mIconRef);
        mIconRef = nil;
    }
}
