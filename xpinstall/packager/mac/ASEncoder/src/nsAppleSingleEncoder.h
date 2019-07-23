












































#ifndef macintosh
#error Sorry! This is Mac only functionality!
#endif

#pragma options align=mac68k
 
#ifndef _NS_APPLESINGLEENCODER_H_
#define _NS_APPLESINGLEENCODER_H_

#include <MacTypes.h>
#include <Files.h>

class nsAppleSingleEncoder
{

public:
	nsAppleSingleEncoder();
	~nsAppleSingleEncoder();
	
	static Boolean		HasResourceFork(FSSpecPtr aFile);
	OSErr				Encode(FSSpecPtr aFile);
	OSErr				Encode(FSSpecPtr aInFile, FSSpecPtr aOutFile);
	OSErr				EncodeFolder(FSSpecPtr aFolder);
	void				ReInit();
	
	static OSErr		FSpGetCatInfo(CInfoPBRec *pb, FSSpecPtr aFile);
	
private:
	FSSpecPtr			mInFile;
	FSSpecPtr			mOutFile;
	FSSpecPtr			mTransient;
	short				mTransRefNum;
	
	OSErr				WriteHeader();
	OSErr				WriteEntryDescs();
	OSErr				WriteEntries();
	OSErr				WriteResourceFork();
	OSErr				WriteDataFork();
};

#ifdef __cplusplus
extern "C" {
#endif

pascal void
EncodeDirIterateFilter(const CInfoPBRec * const cpbPtr, Boolean *quitFlag, void *yourDataPtr);

#ifdef __cplusplus
}
#endif

#define kTransientName			"\pzz__ASEncoder_TMP__zz"
#define kAppleSingleMagicNum 	0x00051600
#define kAppleSingleVerNum		0x00020000
#define kNumASEntries			6
#define kConvertTime 			1265437696L

#define ERR_CHECK(_func) 			\
			err = _func;			\
			if (err != noErr)		\
				return err;

#pragma options align=reset

#endif

	