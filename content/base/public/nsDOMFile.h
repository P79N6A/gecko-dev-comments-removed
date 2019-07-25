





































#ifndef nsDOMFile_h__
#define nsDOMFile_h__

#include "nsICharsetDetectionObserver.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIDOMFileError.h"
#include "nsIInputStream.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozilla/AutoRestore.h"
#include "nsString.h"
#include "nsIXMLHttpRequest.h"

class nsIFile;
class nsIInputStream;

class nsDOMFile : public nsIDOMFile,
                  public nsIXHRSendable,
                  public nsICharsetDetectionObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILE
  NS_DECL_NSIXHRSENDABLE

  nsDOMFile(nsIFile *aFile, const nsAString& aContentType)
    : mFile(aFile),
      mContentType(aContentType)
  {}

  nsDOMFile(nsIFile *aFile)
    : mFile(aFile)
  {}

  virtual ~nsDOMFile() {}

  
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

class nsDOMMemoryFile : public nsDOMFile
{
public:
  nsDOMMemoryFile(void *aMemoryBuffer,
                  PRUint64 aLength,
                  const nsAString& aName,
                  const nsAString& aContentType)
    : nsDOMFile(nsnull, aContentType),
      mInternalData(aMemoryBuffer), mLength(aLength), mName(aName)
  { }

  ~nsDOMMemoryFile();

  NS_IMETHOD GetName(nsAString&);
  NS_IMETHOD GetSize(PRUint64*);
  NS_IMETHOD GetInternalStream(nsIInputStream**);
  NS_IMETHOD GetMozFullPath(nsAString&);
  NS_IMETHOD GetMozFullPathInternal(nsAString&);

protected:
  void* mInternalData;
  PRUint64 mLength;
  nsString mName;
};

class nsDOMFileList : public nsIDOMFileList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILELIST

  PRBool Append(nsIDOMFile *aFile) { return mFiles.AppendObject(aFile); }

  PRBool Remove(PRUint32 aIndex) { return mFiles.RemoveObjectAt(aIndex); }
  void Clear() { return mFiles.Clear(); }

  nsIDOMFile* GetItemAt(PRUint32 aIndex)
  {
    return mFiles.SafeObjectAt(aIndex);
  }

  static nsDOMFileList* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMFileList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == static_cast<nsIDOMFileList*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMFileList*>(aSupports);
  }

private:
  nsCOMArray<nsIDOMFile> mFiles;
};

class nsDOMFileError : public nsIDOMFileError
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILEERROR

  nsDOMFileError(PRUint16 aErrorCode) : mCode(aErrorCode) {}

private:
  PRUint16 mCode;
};

class NS_STACK_CLASS nsDOMFileInternalUrlHolder {
public:
  nsDOMFileInternalUrlHolder(nsIDOMFile* aFile, nsIPrincipal* aPrincipal
                             MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM);
  ~nsDOMFileInternalUrlHolder();
  nsAutoString mUrl;
private:
  MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#endif
