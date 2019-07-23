




































#ifndef nsMacResources_h__
#define nsMacResources_h__

#include "nsError.h"

class nsMacResources
{
public:
		static nsresult		OpenLocalResourceFile();
		static nsresult		CloseLocalResourceFile();

		
		static void 			SetLocalResourceFile(short aRefNum)	{mRefNum = aRefNum;}
		static short			GetLocalResourceFile()							{return mRefNum;}

private:
		static short		mRefNum;
		static short		mSaveResFile;
};

#endif 

