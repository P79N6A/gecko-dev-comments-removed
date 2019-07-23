




































#include "nsGREResProperties.h"
#include "nsILocalFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsNetUtil.h"

nsGREResProperties::nsGREResProperties(const nsACString& aFile)
{
  nsresult rv;
  nsCOMPtr<nsIIOService> ioservice(do_GetIOService(&rv));
  nsCOMPtr<nsIInputStream> inStr;
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIChannel> channel;
  printf("nsGREResProperties %s\n", aFile.BeginReading());
  if (NS_FAILED(rv))
    return;

  rv = ioservice->NewURI(NS_LITERAL_CSTRING("resource://gre-resources/") + aFile,
                         nsnull, nsnull, getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return;

  rv = NS_NewChannel(getter_AddRefs(channel), uri, ioservice);
  if (NS_FAILED(rv))
    return;

  rv = channel->Open(getter_AddRefs(inStr));
  if (NS_FAILED(rv))
    return;

  mProps = do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
  if (mProps) {
    rv = mProps->Load(inStr);
    if (NS_FAILED(rv))
      mProps = nsnull;
  }
}

PRBool nsGREResProperties::DidLoad() const
{
  return mProps != nsnull;
}

nsresult nsGREResProperties::Get(const nsAString& aKey, nsAString& aValue)
{
  if (!mProps)
    return NS_ERROR_NOT_INITIALIZED;

  return mProps->GetStringProperty(NS_ConvertUTF16toUTF8(aKey), aValue);
}
