




































#include "PhRemoteClient.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "prsystem.h"
#include "prlog.h"
#include "prenv.h"
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <Pt.h>

#define MOZ_REMOTE_MSG_TYPE					100

XRemoteClient::XRemoteClient()
{
	mInitialized = PR_FALSE;
}

XRemoteClient::~XRemoteClient()
{
  if (mInitialized)
    Shutdown();
}

NS_IMPL_ISUPPORTS1(XRemoteClient, nsIXRemoteClient)

NS_IMETHODIMP
XRemoteClient::Init (void)
{

  if (mInitialized)
    return NS_OK;

	
	PtInit( NULL );

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
XRemoteClient::SendCommand (const char *aProgram, const char *aUsername,
                            const char *aProfile, const char *aCommand,
                            char **aResponse, PRBool *aWindowFound)
{
  *aWindowFound = PR_TRUE;

	char RemoteServerName[128];

	sprintf( RemoteServerName, "%s_RemoteServer", aProgram ? aProgram : "mozilla" );

	PtConnectionClient_t *cnt = PtConnectionFindName( RemoteServerName, 0, 0 );
	if( !cnt ) {
		
		*aWindowFound = PR_FALSE;
		return NS_OK;
		}

	if( PtConnectionSend( cnt, MOZ_REMOTE_MSG_TYPE, aCommand, NULL, strlen( aCommand ), 0 ) < 0 )
		return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
XRemoteClient::Shutdown (void)
{

  if (!mInitialized)
    return NS_OK;

  
  mInitialized = PR_FALSE;

  return NS_OK;
}
