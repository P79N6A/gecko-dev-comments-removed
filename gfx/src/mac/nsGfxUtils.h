



































 
#ifndef nsGfxUtils_h_
#define nsGfxUtils_h_

#ifndef CarbonHelpers_h__
#include "nsCarbonHelpers.h"
#endif 

#if DEBUG && !defined(XP_MACOSX)
#include "macstdlibextras.h"
#endif

#include <LowMem.h>





inline PRBool CurrentPortIsWMPort()
{
  return PR_FALSE;
}








inline PRBool ValidateDrawingState()
{
  CGrafPtr    curPort;
  GDHandle    curDevice;
  
  GetGWorld(&curPort, &curDevice);
  
  
  if (CurrentPortIsWMPort() && (FrontWindow() != nil))
    return false;


  
  
  
  {
    GDHandle    thisDevice = GetDeviceList();
    while (thisDevice)
    {
      if (thisDevice == curDevice)
         break;
    
      thisDevice = GetNextDevice(thisDevice);
    }

    if ((thisDevice == nil) && !IsPortOffscreen(curPort))    
      return false;
  }

  return true;
}





class nsGraphicsUtils
{
public:

  
  
  
  
  
  
  static void SafeSetPort(CGrafPtr newPort)
  {
    ::SetGWorld(newPort, ::IsPortOffscreen(newPort) ? nsnull : ::GetMainDevice());
  }
  
  
  
  
  
  
  
  static void SafeSetPortWindowPort(WindowPtr window)
  {
    SafeSetPort(::GetWindowPort(window));
  }

  
  
  
  
  
  static void SetPortToKnownGoodPort()
  {
    WindowPtr firstWindow = GetTheWindowList();
    if (firstWindow)
      ::SetGWorld(::GetWindowPort(firstWindow), ::GetMainDevice());
  }

};










class StPortSetter
{
public:
	StPortSetter(CGrafPtr newPort)
	{
		InitSetter(newPort);
	}

	StPortSetter(WindowPtr window)
	{
		InitSetter(GetWindowPort(window));
	}
	
	~StPortSetter()
	{
	  if (mPortChanged)
  		::SetGWorld(mOldPort, mOldDevice);
	  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
	}

protected:
  void InitSetter(CGrafPtr newPort)
	{
	  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
	  
	  
	  mPortChanged = (newPort != CGrafPtr(GetQDGlobalsThePort()));
	  if (mPortChanged)
	  {
  		::GetGWorld(&mOldPort, &mOldDevice);
  		::SetGWorld(newPort, ::IsPortOffscreen(newPort) ? nsnull : ::GetMainDevice());
		}
	}

protected:
  Boolean     mPortChanged;
	CGrafPtr		mOldPort;
	GDHandle    mOldDevice;
};






class StOriginSetter
{
public:

  StOriginSetter(WindowRef wind, const Point* newOrigin = nsnull)
  {
    ::GetWindowPortBounds(wind, &mSavePortRect);
    if (newOrigin)
      ::SetOrigin(newOrigin->h, newOrigin->v);
    else
      ::SetOrigin(0, 0);
  }
  
  StOriginSetter(CGrafPtr grafPort, const Point* newOrigin = nsnull)
  {
    ::GetPortBounds(grafPort, &mSavePortRect);
    if (newOrigin)
      ::SetOrigin(newOrigin->h, newOrigin->v);
    else
      ::SetOrigin(0, 0);
  }
  
  ~StOriginSetter()
  {
    ::SetOrigin(mSavePortRect.left, mSavePortRect.top);
  }

protected:
  
  Rect    mSavePortRect;

};








class StGWorldPortSetter
{
public:
	StGWorldPortSetter(GWorldPtr destGWorld)
	{
	  NS_ASSERTION(::IsPortOffscreen(destGWorld), "StGWorldPortSetter should only be used for GWorlds");
	  ::GetGWorld(&mOldPort, &mOldDevice);
		::SetGWorld(destGWorld, nsnull);
	}
	
	~StGWorldPortSetter()
	{
    ::SetGWorld(mOldPort, mOldDevice);
	  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
	}

protected:
	GWorldPtr		mOldPort;
  GDHandle    mOldDevice;
};





class StTextStyleSetter
{
public:
	StTextStyleSetter(SInt16 fontID, SInt16 fontSize, SInt16 fontFace)
	{
	  SetPortFontStyle(fontID, fontSize, fontFace);
	}
	
	StTextStyleSetter(TextStyle& theStyle)
	{
	  SetPortFontStyle(theStyle.tsFont, theStyle.tsSize, theStyle.tsFace);
	}
	
	~StTextStyleSetter()
	{
  	::TextFont(mFontID);
  	::TextSize(mFontSize);
  	::TextFace(mFontFace);
	}

protected:

  void SetPortFontStyle(SInt16 fontID, SInt16 fontSize, SInt16 fontFace)
	{
	  CGrafPtr curPort;
	  ::GetPort((GrafPtr*)&curPort);
	  
	  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");

    mFontID = ::GetPortTextFont(curPort);
    mFontSize = ::GetPortTextSize(curPort);
    mFontFace = ::GetPortTextFace(curPort);
	  
  	::TextFont(fontID);
  	::TextSize(fontSize);
  	::TextFace(fontFace);

	}

protected:
	SInt16		mFontID;
	SInt16		mFontSize;
	SInt16    mFontFace;
};









class StPixelLocker
{
public:
				
										StPixelLocker(PixMapHandle thePixMap)
										:	mPixMap(thePixMap)
										,	mPixelState(0)
										{
											if (mPixMap) {
											mPixelState = ::GetPixelsState(mPixMap);
											::LockPixels(mPixMap);
										}
										}
										
										~StPixelLocker()
										{
											if (mPixMap)
											::SetPixelsState(mPixMap, mPixelState);
										}

protected:


		PixMapHandle		mPixMap;
		GWorldFlags			mPixelState;

};







class StHandleLocker
{
public:

                    StHandleLocker(Handle theHandle)
                    :	mHandle(theHandle)
                    {
                      if (mHandle)
                      {
                    	  mOldHandleState = ::HGetState(mHandle);
                    	  ::HLock(mHandle);
                      }										  
                    }

                    ~StHandleLocker()
                    {
                      if (mHandle)
                        ::HSetState(mHandle, mOldHandleState);
                    }

protected:

    Handle          mHandle;
    SInt8           mOldHandleState;
};








class StColorSpaceReleaser
{
public:
  StColorSpaceReleaser(CGColorSpaceRef inColorSpace)
  : mColorSpace(inColorSpace)
  {
  }

  ~StColorSpaceReleaser()
  {
    
    
    ::CGColorSpaceRelease(mColorSpace);
  }

private:
  CGColorSpaceRef mColorSpace;
};

#endif 
