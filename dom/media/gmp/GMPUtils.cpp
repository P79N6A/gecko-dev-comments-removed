





#include "GMPUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"

namespace mozilla {

bool
GetEMEVoucherPath(nsIFile** aPath)
{
  nsCOMPtr<nsIFile> path;
  NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(path));
  if (!path) {
    NS_WARNING("GetEMEVoucherPath can't get NS_GRE_DIR!");
    return false;
  }
  path->AppendNative(NS_LITERAL_CSTRING("voucher.bin"));
  path.forget(aPath);
  return true;
}

bool
EMEVoucherFileExists()
{
  nsCOMPtr<nsIFile> path;
  bool exists;
  return GetEMEVoucherPath(getter_AddRefs(path)) &&
         NS_SUCCEEDED(path->Exists(&exists)) &&
         exists;
}

} 
