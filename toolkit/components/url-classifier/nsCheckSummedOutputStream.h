




#ifndef nsCheckSummedOutputStream_h__
#define nsCheckSummedOutputStream_h__

#include "nsILocalFile.h"
#include "nsIFile.h"
#include "nsIOutputStream.h"
#include "nsICryptoHash.h"
#include "nsNetCID.h"
#include "nsString.h"
#include "../../../netwerk/base/nsFileStreams.h"
#include "nsToolkitCompsCID.h"

class nsCheckSummedOutputStream : public nsSafeFileOutputStream
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  static const uint32_t CHECKSUM_SIZE = 16;

  nsCheckSummedOutputStream() {}

  NS_IMETHOD Finish() override;
  NS_IMETHOD Write(const char *buf, uint32_t count, uint32_t *result) override;
  NS_IMETHOD Init(nsIFile* file, int32_t ioFlags, int32_t perm, int32_t behaviorFlags) override;

protected:
  virtual ~nsCheckSummedOutputStream() { nsSafeFileOutputStream::Close(); }

  nsCOMPtr<nsICryptoHash> mHash;
  nsAutoCString mCheckSum;
};


inline nsresult
NS_NewCheckSummedOutputStream(nsIOutputStream **result,
                              nsIFile         *file,
                              int32_t         ioFlags       = -1,
                              int32_t         perm          = -1,
                              int32_t         behaviorFlags =  0)
{
    nsCOMPtr<nsIFileOutputStream> out = new nsCheckSummedOutputStream();
    nsresult rv = out->Init(file, ioFlags, perm, behaviorFlags);
    if (NS_SUCCEEDED(rv)) {
      out.forget(result);
    }
    return rv;
}

#endif
