





































 

#ifndef _NS_FILESELECTOR_H_
#define _NS_FILESELECTOR_H_
 
#include <Navigation.h>

class nsFileSelector
{

public: 
	nsFileSelector();
	~nsFileSelector();
	
	OSErr	SelectFile(FSSpecPtr aOutFile);
	OSErr	SelectFolder(FSSpecPtr aOutFolder);
	
private:
	OSErr	GetCWD(long *aOutDirID, short *aOutVRefNum);
	
	FSSpecPtr	mFile;
};


#ifdef __cplusplus
extern "C" {
#endif

pascal void
OurNavEventFunction(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
					NavCallBackUserData callBackUD);

#ifdef __cplusplus
}
#endif

#define ERR_CHECK(_func) 			\
			err = _func;			\
			if (err != noErr)		\
				return err;

#endif _NS_FILESELECTOR_H_