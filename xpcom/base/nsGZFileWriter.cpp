




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

NS_IMPL_ISUPPORTS1(nsGZFileWriter, nsIGZFileWriter)

nsGZFileWriter::nsGZFileWriter()
  : mInitialized(false)
  , mFinished(false)
{}

nsGZFileWriter::~nsGZFileWriter()
{
  if (mInitialized && !mFinished) {
    Finish();
  }
}

NS_IMETHODIMP
nsGZFileWriter::Init(nsIFile* aFile)
{
  NS_ENSURE_FALSE(mInitialized, NS_ERROR_FAILURE);
  NS_ENSURE_FALSE(mFinished, NS_ERROR_FAILURE);

  
  

  FILE* file;
  nsresult rv = aFile->OpenANSIFileDesc("w", &file);
  NS_ENSURE_SUCCESS(rv, rv);

  mGZFile = gzdopen(dup(fileno(file)), "w");
  fclose(file);

  
  NS_ENSURE_TRUE(mGZFile, NS_ERROR_FAILURE);
  mInitialized = true;

  return NS_OK;
}

NS_IMETHODIMP
nsGZFileWriter::Write(const nsACString& aStr)
{
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mFinished, NS_ERROR_FAILURE);

  
  
  
  
  if (aStr.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  int rv = gzwrite(mGZFile, aStr.BeginReading(), aStr.Length());
  NS_ENSURE_TRUE(rv == static_cast<int>(aStr.Length()), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGZFileWriter::Finish()
{
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mFinished, NS_ERROR_FAILURE);

  mFinished = true;
  gzclose(mGZFile);

  
  
  return NS_OK;
}
