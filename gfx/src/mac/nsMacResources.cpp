




































#include "nsMacResources.h"
#include <Resources.h>
#include <MacWindows.h>


short nsMacResources::mRefNum				= kResFileNotOpened;
short nsMacResources::mSaveResFile	= 0;

pascal OSErr __NSInitialize(const CFragInitBlock *theInitBlock);
pascal OSErr __initializeResources(const CFragInitBlock *theInitBlock);

pascal void __NSTerminate(void);
pascal void __terminateResources(void);




pascal OSErr __initializeResources(const CFragInitBlock *theInitBlock)
{
    OSErr err = __NSInitialize(theInitBlock);
    if (err)
    	return err;

	short saveResFile = ::CurResFile();

	short refNum = FSpOpenResFile(theInitBlock->fragLocator.u.onDisk.fileSpec, fsRdPerm);
	nsMacResources::SetLocalResourceFile(refNum);

	::UseResFile(saveResFile);

	return ::ResError();
}





pascal void __terminateResources(void)
{
	::CloseResFile(nsMacResources::GetLocalResourceFile());
    __NSTerminate();
}





nsresult nsMacResources::OpenLocalResourceFile()
{
	if (mRefNum == kResFileNotOpened)
		return NS_ERROR_NOT_INITIALIZED;

	mSaveResFile = ::CurResFile();
	::UseResFile(mRefNum);

	return (::ResError() == noErr ? NS_OK : NS_ERROR_FAILURE);
}






nsresult nsMacResources::CloseLocalResourceFile()
{
	if (mRefNum == kResFileNotOpened)
		return NS_ERROR_NOT_INITIALIZED;

	::UseResFile(mSaveResFile);

	return (::ResError() == noErr ? NS_OK : NS_ERROR_FAILURE);
}

