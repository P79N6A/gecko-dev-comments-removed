





































#ifndef nsDOMFile_h__
#define nsDOMFile_h__

#include "nsICharsetDetectionObserver.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIInputStream.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIDOMDocument;
class nsIFile;
class nsIInputStream;

class nsDOMFile : public nsIDOMFile,
                  public nsICharsetDetectionObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILE

  nsDOMFile(nsIFile *aFile)
    : mFile(aFile)
  {}
  ~nsDOMFile() {}

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);

private:
  nsCOMPtr<nsIFile> mFile;
  nsString mContentType;
  nsCString mCharset;

  nsresult GuessCharset(nsIInputStream *aStream,
                        nsACString &aCharset);
  nsresult ConvertStream(nsIInputStream *aStream, const char *aCharset,
                         nsAString &aResult);
};

class nsDOMFileList : public nsIDOMFileList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILELIST

  PRBool Append(nsIDOMFile *aFile) { return mFiles.AppendObject(aFile); }

  PRBool Remove(PRUint32 aIndex) { return mFiles.RemoveObjectAt(aIndex); }
  void Clear() { return mFiles.Clear(); }

private:
  nsCOMArray<nsIDOMFile> mFiles;
};

#endif
