



































#include <stdlib.h>
#include "nsIStorageStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"

int main()
{
  char kData[4096];
  memset(kData, 0, sizeof(kData));

  nsresult rv;
  nsCOMPtr<nsIStorageStream> stor;

  rv = NS_NewStorageStream(4096, PR_UINT32_MAX, getter_AddRefs(stor));
  if (NS_FAILED(rv))
    return -1;

  nsCOMPtr<nsIOutputStream> out;
  rv = stor->GetOutputStream(0, getter_AddRefs(out));
  if (NS_FAILED(rv))
    return -1;

  PRUint32 n;

  rv = out->Write(kData, sizeof(kData), &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Write(kData, sizeof(kData), &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Close();
  if (NS_FAILED(rv))
    return -1;

  out = nsnull;
  
  nsCOMPtr<nsIInputStream> in;
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  if (NS_FAILED(rv))
    return -1;

  char buf[4096];

  
  do {
    rv = in->Read(buf, sizeof(buf), &n);
    if (NS_FAILED(rv))
      return -1;
  } while (n != 0);

  rv = in->Close();
  if (NS_FAILED(rv))
    return -1;
  in = nsnull;

  
  

  rv = stor->GetOutputStream(8192, getter_AddRefs(out));
  if (NS_FAILED(rv))
    return -1;

  rv = out->Write(kData, sizeof(kData), &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Write(kData, sizeof(kData), &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Write(kData, sizeof(kData), &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Write(kData, 11, &n);
  if (NS_FAILED(rv))
    return -1;

  rv = out->Close();
  if (NS_FAILED(rv))
    return -1;

  out = nsnull;

  
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  if (NS_FAILED(rv))
    return -1;

  
  do {
    rv = in->Read(buf, sizeof(buf), &n);
    if (NS_FAILED(rv))
      return -1;
  } while (n != 0);

  rv = in->Close();
  if (NS_FAILED(rv))
    return -1;
  in = nsnull;

  return 0;
}
