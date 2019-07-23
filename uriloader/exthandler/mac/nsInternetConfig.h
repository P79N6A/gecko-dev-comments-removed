



































 
#ifndef __NS_INTERNETCONFIG_H__
#define __NS_INTERNETCONFIG_H__

#include <Carbon/Carbon.h>
#include "prtypes.h"
#include "nsError.h"

class nsInternetConfig
{
public:
	nsInternetConfig();
	~nsInternetConfig();

	static ICInstance GetInstance();
	static  PRBool		HasSeedChanged();
private:
	static	ICInstance sInstance;
	static  long sSeed;
	static  PRInt32 sRefCount;
};

#endif 
