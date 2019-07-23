




































#include "nsGREResProperties.h"
#include "nsILocalFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsNetUtil.h"

nsGREResProperties::nsGREResProperties(const nsACString& aFile)
{
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(file));
  if (NS_FAILED(rv))
    return;

  file->AppendNative(NS_LITERAL_CSTRING("res"));
  file->AppendNative(aFile);

  nsCOMPtr<nsILocalFile> lf(do_QueryInterface(file));
  NS_ENSURE_TRUE(lf, );

  nsCOMPtr<nsIInputStream> inStr;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), lf);
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
