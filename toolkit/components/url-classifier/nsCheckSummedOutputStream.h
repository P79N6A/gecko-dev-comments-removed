





































#ifndef nsCheckSummedOutputStream_h__
#define nsCheckSummedOutputStream_h__

#include "nsILocalFile.h"
#include "nsIFile.h"
#include "nsIOutputStream.h"
#include "nsICryptoHash.h"
#include "nsNetCID.h"
#include "nsString.h"
#include "../../../netwerk/base/src/nsFileStreams.h"
#include "nsToolkitCompsCID.h"

class nsCheckSummedOutputStream : public nsSafeFileOutputStream
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  static const PRUint32 CHECKSUM_SIZE = 16;

  nsCheckSummedOutputStream() {}
  virtual ~nsCheckSummedOutputStream() { nsSafeFileOutputStream::Close(); }

  NS_IMETHOD Finish();
  NS_IMETHOD Write(const char *buf, PRUint32 count, PRUint32 *result);
  NS_IMETHOD Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags);

protected:
  nsCOMPtr<nsICryptoHash> mHash;
  nsCAutoString mCheckSum;
};


inline nsresult
NS_NewCheckSummedOutputStream(nsIOutputStream **result,
                              nsIFile         *file,
                              PRInt32         ioFlags       = -1,
                              PRInt32         perm          = -1,
                              PRInt32         behaviorFlags =  0)
{
    nsCOMPtr<nsIFileOutputStream> out = new nsCheckSummedOutputStream();
    nsresult rv = out->Init(file, ioFlags, perm, behaviorFlags);
    if (NS_SUCCEEDED(rv))
      NS_ADDREF(*result = out);  
    return rv;
}

#endif
