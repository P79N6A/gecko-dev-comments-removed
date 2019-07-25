


































#include "zlib.h"
#include "zipstruct.h"

#include "CertReader.h"

#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsISignatureVerifier.h"
#include "nsIInputStream.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsPICertNotification.h"


#include "nsNetUtil.h"


#define MAX_SIGNATURE_SIZE (32*1024)










static unsigned int xtoint (unsigned char *ii)
{
  return (int) (ii [0]) | ((int) ii [1] << 8);
}









static unsigned long xtolong (unsigned char *ll)
{
  unsigned long ret;

  ret =  ((((unsigned long) ll [0]) <<  0) |
          (((unsigned long) ll [1]) <<  8) |
          (((unsigned long) ll [2]) << 16) |
          (((unsigned long) ll [3]) << 24) );

  return ret;
}

static int my_inflate(unsigned char* compr, PRUint32 comprLen, unsigned char* uncompr, PRUint32 uncomprLen)
{
  int err;
  z_stream d_stream; 
  memset (&d_stream, 0, sizeof (d_stream));

  
  if (uncomprLen < 10)
    return -1;

  *uncompr = '\0';

  if (inflateInit2 (&d_stream, -MAX_WBITS) != Z_OK)
    return -1;

  d_stream.next_in  = compr;
  d_stream.avail_in = (uInt)comprLen;

  d_stream.next_out = uncompr;
  d_stream.avail_out = (uInt)uncomprLen;

  err = inflate(&d_stream, Z_NO_FLUSH);

  if (err != Z_OK && err != Z_STREAM_END) {
    inflateEnd(&d_stream);
    return -1;
  }

  err = inflateEnd(&d_stream);
  if (err != Z_OK) {
    return -1;
  }
  return 0;
}

CertReader::CertReader(nsIURI* aURI, nsISupports* aContext, nsPICertNotification* aObs):
  mContext(aContext),
  mURI(aURI),
  mObserver(aObs)
{
}

CertReader::~CertReader()
{
}

NS_IMPL_ISUPPORTS2(CertReader, nsIStreamListener, nsIRequestObserver)

NS_IMETHODIMP
CertReader::OnStartRequest(nsIRequest *request, nsISupports* context)
{
  mVerifier = do_GetService(SIGNATURE_VERIFIER_CONTRACTID);
  if (!mVerifier)
    return NS_BINDING_ABORTED;

  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = request->GetLoadGroup(getter_AddRefs(loadGroup));
  if (NS_SUCCEEDED(rv) && loadGroup)
    loadGroup->RemoveRequest(request, nsnull, NS_BINDING_RETARGETED);

  mLeftoverBuffer.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
CertReader::OnDataAvailable(nsIRequest *request,
                            nsISupports* context,
                            nsIInputStream *aIStream,
                            PRUint32 aSourceOffset,
                            PRUint32 aLength)
{
  if (!mVerifier)
    return NS_BINDING_ABORTED;

  char buf[4096];
  PRUint32 amt, size;
  nsresult rv;

  while (aLength)
  {
    size = NS_MIN(aLength, sizeof(buf));

    rv = aIStream->Read(buf, size, &amt);
    if (NS_FAILED(rv))
      return rv;

    aLength -= amt;

    mLeftoverBuffer.Append(buf, amt);

    if (mLeftoverBuffer.Length() < ZIPLOCAL_SIZE)
      continue;

    const char* caret = mLeftoverBuffer.get();

    ZipLocal_* ziplocal = (ZipLocal_*) caret;

    if (xtolong(ziplocal->signature) != LOCALSIG)
      return NS_BINDING_ABORTED;

    
    PRUint32 fileEntryLen = (ZIPLOCAL_SIZE +
                             xtoint(ziplocal->filename_len) +
                             xtoint(ziplocal->extrafield_len) +
                             xtolong(ziplocal->size));


    
    if (fileEntryLen > MAX_SIGNATURE_SIZE)
      return NS_BINDING_ABORTED;

    if (mLeftoverBuffer.Length() < fileEntryLen)
    {
      
      continue;
    }

    

    int err = 0;
    unsigned char* orgData = nsnull;
    unsigned char* sigData = nsnull;
    const char* data = (caret +
                        ZIPLOCAL_SIZE +
                        xtoint(ziplocal->filename_len) +
                        xtoint(ziplocal->extrafield_len));

    PRUint32 sigSize = 0;
    PRUint32 orgSize = xtolong ((unsigned char *) ziplocal->orglen);
    PRUint32 cSize   = xtolong ((unsigned char *) ziplocal->size);

    switch (xtoint(ziplocal->method))
    {
      case STORED:
        
        sigSize = cSize;
        sigData = (unsigned char*)data;
        break;

      case DEFLATED:
        if (orgSize == 0 || orgSize > MAX_SIGNATURE_SIZE)
          return NS_BINDING_ABORTED;

        orgData = (unsigned char*)malloc(orgSize);
        if (!orgData)
          return NS_BINDING_ABORTED;

        err = my_inflate((unsigned char*)data,
                         cSize,
                         orgData,
                         orgSize);

        sigSize = orgSize;
        sigData = orgData;
        break;

      default:
        
        err = Z_DATA_ERROR;
        break;
    }

    if (err == 0)
    {
      PRInt32 verifyError;
      rv = mVerifier->VerifySignature((char*)sigData, sigSize, nsnull, 0,
                                 &verifyError, getter_AddRefs(mPrincipal));
    }
    if (orgData)
        free(orgData);

    
    return NS_BINDING_ABORTED;
  }

  return NS_OK; 
}

NS_IMETHODIMP
CertReader::OnStopRequest(nsIRequest *request, nsISupports* context,
                          nsresult aStatus)
{
    mObserver->OnCertAvailable(mURI,
                               mContext,
                               aStatus,
                               mPrincipal);

    return NS_OK;
}


