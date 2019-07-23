






































#include "CThrobber.h"

#include <LString.h>
#include <LStream.h>
#include <UDrawingState.h>
#include <UResourceMgr.h>

#include <QuickTimeComponents.h>





CThrobber::CThrobber() :
   mMovieResID(-1), mMovieHandle(nil),
   mMovie(nil), mMovieController(nil)
{
}


CThrobber::CThrobber(LStream*	inStream) :
   LControl(inStream),
   mMovieResID(-1), mMovieHandle(nil),
   mMovie(nil), mMovieController(nil)
{
   *inStream >> mMovieResID;
}


CThrobber::~CThrobber()
{
    if (mMovieController)
        ::DisposeMovieController(mMovieController);
    if (mMovieHandle)
        ::DisposeHandle(mMovieHandle);   
}


void CThrobber::FinishCreateSelf()
{
   CreateMovie();
}


void CThrobber::ShowSelf()
{
    LControl::ShowSelf();
    
    if (!mMovieController)
        return;
    
    ::MCSetClip(mMovieController, NULL, NULL);
}


void CThrobber::HideSelf()
{
    LControl::HideSelf();
    
    if (!mMovieController)
        return;    
    
    StRegion emptyRgn;
    ::MCSetClip(mMovieController, NULL, emptyRgn);
}


void CThrobber::DrawSelf()
{
    if (mMovieController)
        ::MCDraw(mMovieController, GetMacWindow());
}


void CThrobber::ResizeFrameBy(SInt16		inWidthDelta,
                					SInt16		inHeightDelta,
                					Boolean	   inRefresh)
{
	LControl::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
	
	if (!mMovieController)
	    return;
	    
	FocusDraw();

	Rect newFrame;
	::MCGetControllerBoundsRect(mMovieController, &newFrame);
	newFrame.right += inWidthDelta;
	newFrame.bottom += inHeightDelta;
	::MCSetControllerBoundsRect(mMovieController, &newFrame);
}


void CThrobber::MoveBy(SInt32		inHorizDelta,
				           SInt32		inVertDelta,
							  Boolean	inRefresh)
{
	LControl::MoveBy(inHorizDelta, inVertDelta, inRefresh);

	if (!mMovieController)
	    return;
 
 	FocusDraw();
 
	Rect newFrame;	
	::MCGetControllerBoundsRect(mMovieController, &newFrame);
	::OffsetRect(&newFrame, inHorizDelta, inVertDelta);
	::MCSetControllerBoundsRect(mMovieController, &newFrame);
}


void CThrobber::SpendTime(const EventRecord	&inMacEvent)
{
	FocusDraw();
	::MCIsPlayerEvent(mMovieController, &inMacEvent);
}

	
void CThrobber::Start()
{
	if (!mMovieController)
	    return;
 
    ::StartMovie(mMovie);
    StartRepeating();
}


void CThrobber::Stop()
{
	if (!mMovieController)
	    return;
 
    StopRepeating();
    ::StopMovie(mMovie);
    ::GoToBeginningOfMovie(mMovie);
    Refresh();
}


void CThrobber::CreateMovie()
{
    OSErr err;
    Handle dataRef = NULL;
        
    try {
        
        StApplicationContext appResContext;
        
        mMovieHandle = ::Get1Resource('GIF ', 128);
        ThrowIfResFail_(mMovieHandle);
        ::DetachResource(mMovieHandle);
        
        
    	err = ::PtrToHand(&mMovieHandle, &dataRef, sizeof(Handle));
    	ThrowIfError_(err);
    	err = ::PtrAndHand("\p", dataRef, 1); 
    	ThrowIfError_(err);
        long	fileTypeAtom[3];
    	fileTypeAtom[0] = sizeof(long) * 3;
    	fileTypeAtom[1] = kDataRefExtensionMacOSFileType;
    	fileTypeAtom[2] = kQTFileTypeGIF;
    	err = ::PtrAndHand(fileTypeAtom, dataRef, sizeof(long) * 3);
    	ThrowIfError_(err);
    	
    	err = ::NewMovieFromDataRef(&mMovie, newMovieActive, NULL, dataRef, HandleDataHandlerSubType);
        ThrowIfError_(err);
    	DisposeHandle(dataRef);
    	dataRef = NULL;
    	
    	
    	Rect	movieBounds;
    	::GetMovieBox(mMovie, &movieBounds);
    	::MacOffsetRect(&movieBounds, (SInt16) -movieBounds.left, (SInt16) -movieBounds.top);
    	::SetMovieBox(mMovie, &movieBounds);

    	::SetMovieGWorld(mMovie, (CGrafPtr) GetMacPort(), nil);

    	Rect	frame;
    	CalcLocalFrameRect(frame);
    	mMovieController = ::NewMovieController(mMovie, &frame, mcTopLeftMovie + mcNotVisible);
        
    	::MCDoAction(mMovieController, mcActionSetLooping, (void *)1);
    	::MCMovieChanged(mMovieController, mMovie);
    }
    catch (...) {
        if (dataRef)
            DisposeHandle(dataRef);
        
        
    }
}
