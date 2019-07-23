



































#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIURL.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"

#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsILocalFile.h"

#include "nsStringAPI.h"
#include "nsCRT.h"
#include "prprf.h"

#ifdef XP_WIN
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef XP_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef XP_OS2
#include <os2.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

class StreamToFile : public nsIStreamListener {
public:
  StreamToFile(FILE* fp);

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetBindInfo(nsIURI* aURL);
  NS_IMETHOD OnProgress(nsIURI* aURL, PRInt32 Progress, PRInt32 ProgressMax);
  NS_IMETHOD OnStatus(nsIURI* aURL, const nsString& aMsg);
  NS_IMETHOD OnStartRequest(nsIRequest* aRequest, nsISupports *);
  NS_IMETHOD OnDataAvailable(nsIRequest* aRequest, nsISupports *, nsIInputStream *pIStream, PRUint32 aOffset, PRUint32 aCount);
  NS_IMETHOD OnStopRequest(nsIRequest* aRequest, nsISupports *, PRUint32 status);

  PRBool IsDone() const { return mDone; }
  PRBool HaveError() const { return mError; }

protected:
  virtual ~StreamToFile();

  PRBool mDone;
  PRBool mError;
  FILE* mFile;
};

StreamToFile::StreamToFile(FILE* fp)
{
  mDone = PR_FALSE;
  mError = PR_FALSE;
  mFile = fp;
}

NS_IMPL_ISUPPORTS1(StreamToFile, nsIStreamListener)

StreamToFile::~StreamToFile()
{
  if (nsnull != mFile) {
    fclose(mFile);
  }
}

NS_IMETHODIMP
StreamToFile::GetBindInfo(nsIURI* aURL)
{
  return 0;
}

NS_IMETHODIMP
StreamToFile::OnProgress(nsIURI* aURL, PRInt32 Progress, PRInt32 ProgressMax)
{
  return 0;
}

NS_IMETHODIMP
StreamToFile::OnStatus(nsIURI* aURL, const nsString& aMsg)
{
  return 0;
}

NS_IMETHODIMP
StreamToFile::OnStartRequest(nsIRequest *aRequest, nsISupports *aSomething)
{
  return 0;
}

NS_IMETHODIMP
StreamToFile::OnDataAvailable(
  nsIRequest* aRequest,
  nsISupports *,
  nsIInputStream *pIStream,
  PRUint32 aOffset,
  PRUint32 aCount)
{
  PRUint32 len;
  do {
    char buffer[4000];
    nsresult err = pIStream->Read(buffer, sizeof(buffer), &len);
    if (NS_SUCCEEDED(err)) {
      if (nsnull != mFile) {
        fwrite(buffer, 1, len, mFile);
      }
    }
  } while (len > 0);
  return 0;
}


NS_IMETHODIMP
StreamToFile::OnStopRequest(nsIRequest *aRequest, nsISupports *aSomething, PRUint32 status)
{
  mDone = PR_TRUE;
  if (0 != status) {
    mError = PR_TRUE;
  }
  return 0;
}





class PageGrabber {
public:
  PageGrabber();
  ~PageGrabber();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsILocalFile *aDirectory);

  nsresult Grab(const nsCString& aURL);

protected:
  nsILocalFile* NextFile(const char* aExtension);

  nsILocalFile *mDirectory;
  PRInt32 mFileNum;
};

PageGrabber::PageGrabber()
{
}

PageGrabber::~PageGrabber()
{
}

nsresult
PageGrabber::Init(nsILocalFile *aDirectory)
{
  mDirectory = aDirectory;
  return NS_OK;
}

nsILocalFile*
PageGrabber::NextFile(const char* aExtension)
{
  nsCAutoString name(NS_LITERAL_CSTRING("grab."));
  name += nsDependentCString(aExtension);
  nsIFile *cfile;
  mDirectory->Clone(&cfile);
  nsCOMPtr<nsILocalFile> file = do_QueryInterface(cfile);
  file->AppendRelativeNativePath(name);
  file->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0660);
  return file;
}

nsresult
PageGrabber::Grab(const nsCString& aURL)
{
  nsresult rv;
  
  nsCOMPtr<nsILocalFile> file = NextFile("html");
  if (!file) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  FILE* fp;
  rv = file->OpenANSIFileDesc("wb", &fp);
  if (NS_FAILED(rv)) {
    return rv;
  }
  printf("Copying ");
  fputs(aURL.get(), stdout);
  nsAutoString path;
  file->GetPath(path);
  NS_ConvertUTF16toUTF8 cpath(path);
  printf(" to %s\n", cpath.get());

  
  nsCOMPtr<nsIURI> url;

  nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  rv = ioService->NewURI(aURL, nsnull, nsnull, getter_AddRefs(url));
  nsIChannel *channel = nsnull;
  rv = ioService->NewChannelFromURI(url, &channel);
  if (NS_FAILED(rv)) return rv;

  
  StreamToFile* copier = new StreamToFile(fp);
  if (!copier)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(copier);

  rv = channel->AsyncOpen(copier, nsnull);

  if (NS_FAILED(rv)) {
    NS_RELEASE(copier);
    return rv;
  }
    
  
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  while ( !copier->IsDone() ) {
    if (!NS_ProcessNextEvent(thread))
      break;
  }

  rv = copier->HaveError() ? NS_ERROR_FAILURE : NS_OK;
  NS_RELEASE(copier);

  return rv;
}



int
main(int argc, char **argv)
{
  nsString url_address;

  if (argc != 3) {
    fprintf(stderr, "Usage: grabpage url directory\n");
    return -1;
  }
  PageGrabber* grabber = new PageGrabber();
  if(grabber) {
    nsCOMPtr <nsILocalFile> directory(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));;
    if (NS_FAILED(directory->InitWithNativePath(nsDependentCString(argv[2])))) {
      fprintf(stderr, "InitWithNativePath failed\n");
      return -2;
    }
    grabber->Init(directory);
    if (NS_OK != grabber->Grab(nsDependentCString(argv[1]))) {
      return -1;
    }
  }
  return 0;
}
