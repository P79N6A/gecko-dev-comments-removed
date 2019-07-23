

































 

#include "nsOSHelperAppService.h"
#include "nsMIMEInfoBeOS.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURL.h"
#include "nsILocalFile.h"
#include "nsIProcess.h"
#include "prenv.h"      
#include <stdlib.h>		

#include <Message.h>
#include <Mime.h>
#include <String.h>
#include <Path.h>
#include <Entry.h>
#include <Roster.h>

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, PR_LOG_DEBUG)

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
}

nsOSHelperAppService::~nsOSHelperAppService()
{}

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists)
{
	LOG(("-- nsOSHelperAppService::OSProtocolHandlerExists for '%s'\n",
	     aProtocolScheme));
	
	*aHandlerExists = PR_FALSE;
	if (aProtocolScheme && *aProtocolScheme)
	{
		BString protoStr(aProtocolScheme);
		protoStr.Prepend("application/x-vnd.Be.URL.");
		BMimeType protocol;
		if (protocol.SetTo(protoStr.String()) == B_OK)
		{
			if (protocol.IsInstalled())
				*aHandlerExists = PR_TRUE;
		}
		if ((!*aHandlerExists) && (!strcmp("mailto", aProtocolScheme)))
		{
			
			if (protocol.SetTo("text/x-email") == B_OK)
			{
				if (protocol.IsInstalled())
					*aHandlerExists = PR_TRUE;
			}
		}
	}

	return NS_OK;
}


nsresult nsOSHelperAppService::SetMIMEInfoForType(const char *aMIMEType, nsMIMEInfoBeOS**_retval) {

	LOG(("-- nsOSHelperAppService::SetMIMEInfoForType: %s\n",aMIMEType));

	nsresult rv = NS_ERROR_FAILURE;

	nsMIMEInfoBeOS* mimeInfo = new nsMIMEInfoBeOS(aMIMEType);
	if (mimeInfo) {
		NS_ADDREF(mimeInfo);
		BMimeType mimeType(aMIMEType);
		BMessage data;
		int32 index = 0;
		BString strData;
		LOG(("   Adding extensions:\n"));
		if (mimeType.GetFileExtensions(&data) == B_OK) {
			while (data.FindString("extensions",index,&strData) == B_OK) {
				
				
				if (strData.ByteAt(0) == '.')
					strData.RemoveFirst(".");
				mimeInfo->AppendExtension(nsDependentCString(strData.String()));
				LOG(("      %s\n",strData.String()));
				index++;
			}
		}

		char desc[B_MIME_TYPE_LENGTH + 1];
		if (mimeType.GetShortDescription(desc) == B_OK) {
			mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(desc));
		} else {
			if (mimeType.GetLongDescription(desc) == B_OK) {
				mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(desc));
			} else {
				mimeInfo->SetDescription(NS_ConvertUTF8toUTF16(aMIMEType));
			}
		}
		
		LOG(("    Description: %s\n",desc));

		
		char appSig[B_MIME_TYPE_LENGTH + 1];
		bool doSave = true;
		if (mimeType.GetPreferredApp(appSig) == B_OK) {
			LOG(("    Got preferred ap\n"));
			BMimeType app(appSig);
			entry_ref ref;
			BEntry entry;
			BPath path;
			if ((app.GetAppHint(&ref) == B_OK) &&
			        (entry.SetTo(&ref, false) == B_OK) &&
			        (entry.GetPath(&path) == B_OK)) {

				LOG(("    Got our path!\n"));
				nsCOMPtr<nsIFile> handlerFile;
				rv = GetFileTokenForPath(NS_ConvertUTF8toUTF16(path.Path()).get(), getter_AddRefs(handlerFile));

				if (NS_SUCCEEDED(rv)) {
					mimeInfo->SetDefaultApplication(handlerFile);
					mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
					mimeInfo->SetDefaultDescription(NS_ConvertUTF8toUTF16(path.Leaf()));
					LOG(("    Preferred App: %s\n",path.Leaf()));
					doSave = false;
				}
			}
		}
		if (doSave) {
			mimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);
			LOG(("    No Preferred App\n"));
		}

		*_retval = mimeInfo;
		rv = NS_OK;
	}
	else
		rv = NS_ERROR_FAILURE;

	return rv;
}

nsresult nsOSHelperAppService::GetMimeInfoFromExtension(const char *aFileExt,
        nsMIMEInfoBeOS ** _retval) {
	
	if (!aFileExt || !*aFileExt)
		return NS_ERROR_INVALID_ARG;

	LOG(("Here we do an extension lookup for '%s'\n", aFileExt));

	BMimeType mimeType;

	if (BMimeType::GuessMimeType(aFileExt, &mimeType)  == B_OK)
		return SetMIMEInfoForType(mimeType.Type(), _retval);

	
	return NS_ERROR_FAILURE;
}

nsresult nsOSHelperAppService::GetMimeInfoFromMIMEType(const char *aMIMEType,
        nsMIMEInfoBeOS ** _retval) {
	
	if (!aMIMEType || !*aMIMEType)
		return NS_ERROR_INVALID_ARG;

	LOG(("Here we do a mimetype lookup for '%s'\n", aMIMEType));
	
	BMimeType mimeType(aMIMEType);
	if (mimeType.IsInstalled())
		return SetMIMEInfoForType(aMIMEType, _retval);
	
	return NS_ERROR_FAILURE;
}

already_AddRefed<nsIMIMEInfo>
nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, PRBool* aFound)
{
  *aFound = PR_TRUE;
  nsMIMEInfoBeOS* mi = nsnull;
  const nsCString& flatType = PromiseFlatCString(aMIMEType);
  const nsCString& flatExt = PromiseFlatCString(aFileExt);
  GetMimeInfoFromMIMEType(flatType.get(), &mi);
  if (mi)
    return mi;

  GetMimeInfoFromExtension(flatExt.get(), &mi);
  if (mi && !aMIMEType.IsEmpty())
    mi->SetMIMEType(aMIMEType);
  if (mi)
    return mi;

  *aFound = PR_FALSE;
  mi = new nsMIMEInfoBeOS(flatType);
  if (!mi)
    return nsnull;
  NS_ADDREF(mi);
  if (!aFileExt.IsEmpty())
    mi->AppendExtension(aFileExt);

  return mi;
}

already_AddRefed<nsIHandlerInfo>
nsOSHelperAppService::GetProtocolInfoFromOS(const nsACString &aScheme)
{
  NS_ASSERTION(!aScheme.IsEmpty(), "No scheme was specified!");

  PRBool exists;
  nsresult rv = OSProtocolHandlerExists(nsPromiseFlatCString(aScheme).get(),
                                        &exists);
  if (NS_FAILED(rv) || !exists)
    return nsnull;

  nsMIMEInfoBeOS *handlerInfo =
	new nsMIMEInfoBeOS(aScheme, nsMIMEInfoBase::eProtocolInfo);
  NS_ENSURE_TRUE(handlerInfo, nsnull);
  NS_ADDREF(handlerInfo);

  nsAutoString desc;
  GetApplicationDescription(aScheme, desc);
  handlerInfo->SetDefaultDescription(desc);

  return handlerInfo;
}

