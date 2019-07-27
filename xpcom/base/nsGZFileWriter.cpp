





#include "nsGZFileWriter.h"
#include "nsIFile.h"
#include "nsString.h"
#include "zlib.h"

#ifdef XP_WIN
#include <io.h>
#define _dup dup
#else
#include <unistd.h>
#endif

NS_IMPL_ISUPPORTS(nsGZFileWriter, nsIGZFileWriter)

nsGZFileWriter::nsGZFileWriter()
  : mInitialized(false)
  , mFinished(false)
{
}

nsGZFileWriter::~nsGZFileWriter()
{
  if (mInitialized && !mFinished) {
    Finish();
  }
}

NS_IMETHODIMP
nsGZFileWriter::Init(nsIFile* aFile)
{
  if (NS_WARN_IF(mInitialized) ||
      NS_WARN_IF(mFinished)) {
    return NS_ERROR_FAILURE;
  }

  
  

  FILE* file;
  nsresult rv = aFile->OpenANSIFileDesc("wb", &file);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mGZFile = gzdopen(dup(fileno(file)), "wb");
  fclose(file);

  
  if (NS_WARN_IF(!mGZFile)) {
    return NS_ERROR_FAILURE;
  }

  mInitialized = true;

  return NS_OK;
}

NS_IMETHODIMP
nsGZFileWriter::Write(const nsACString& aStr)
{
  if (NS_WARN_IF(!mInitialized) ||
      NS_WARN_IF(mFinished)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  if (aStr.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  int rv = gzwrite(mGZFile, aStr.BeginReading(), aStr.Length());
  if (NS_WARN_IF(rv != static_cast<int>(aStr.Length()))) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGZFileWriter::Finish()
{
  if (NS_WARN_IF(!mInitialized) ||
      NS_WARN_IF(mFinished)) {
    return NS_ERROR_FAILURE;
  }

  mFinished = true;
  gzclose(mGZFile);

  
  
  return NS_OK;
}
