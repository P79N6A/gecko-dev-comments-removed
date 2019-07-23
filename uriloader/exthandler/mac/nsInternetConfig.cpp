



































 
#include "nsInternetConfig.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsDebug.h"

#include <CodeFragments.h>   
#include <Processes.h>
ICInstance nsInternetConfig::sInstance = NULL;
long nsInternetConfig::sSeed = 0;
PRInt32  nsInternetConfig::sRefCount = 0;



static OSType GetAppCreatorCode()
{
  ProcessSerialNumber psn = { 0, kCurrentProcess } ;
  ProcessInfoRec      procInfo;
  
  procInfo.processInfoLength = sizeof(ProcessInfoRec);
  procInfo.processName = nsnull;
  procInfo.processAppSpec = nsnull;
  
  GetProcessInformation(&psn, &procInfo);
  return procInfo.processSignature;  
}



ICInstance nsInternetConfig::GetInstance()
{
	if ( !sInstance )
	{
		OSStatus err;
		if ((long)ICStart == kUnresolvedCFragSymbolAddress )
			return sInstance;                          
                                                                                 
                                                                                  
		OSType creator = GetAppCreatorCode();
		err = ::ICStart( &sInstance, creator  );
		if ( err != noErr )
		{
			::ICStop( sInstance );
		}
		else
		{
#if !TARGET_CARBON
			::ICFindConfigFile( sInstance, 0 , nil );
#endif
			::ICGetSeed( sInstance, &sSeed );
		}
	}
	return sInstance;
}

PRBool nsInternetConfig::HasSeedChanged()
{
	ICInstance instance = nsInternetConfig::GetInstance();
	if ( instance )
	{
		long newSeed = 0;
		::ICGetSeed( sInstance, &newSeed );
		if ( newSeed != sSeed )
		{
			sSeed = newSeed;
			return PR_TRUE;
		}
	}
	return PR_FALSE;
}

nsInternetConfig::nsInternetConfig()
{
	sRefCount++;
}

nsInternetConfig::~nsInternetConfig()
{
	sRefCount--;
	if ( sRefCount == 0 && sInstance)
	{
		::ICStop( sInstance );
		sInstance = NULL;
	}
}

