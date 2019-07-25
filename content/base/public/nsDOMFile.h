





































#ifndef nsDOMFile_h__
#define nsDOMFile_h__

#include "nsICharsetDetectionObserver.h"
#include "nsIFile.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIDOMFileError.h"
#include "nsIInputStream.h"
#include "nsIJSNativeInitializer.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozilla/AutoRestore.h"
#include "nsString.h"
#include "nsIXMLHttpRequest.h"
#include "prmem.h"
#include "nsAutoPtr.h"

class nsIFile;
class nsIInputStream;
class nsIClassInfo;
class nsIBlobBuilder;

nsresult NS_NewBlobBuilder(nsISupports* *aSupports);
void ParseSize(PRInt64 aSize, PRInt64& aStart, PRInt64& aEnd);

class nsDOMFile : public nsIDOMFile,
                  public nsIXHRSendable,
                  public nsICharsetDetectionObserver,
                  public nsIJSNativeInitializer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLOB
  NS_DECL_NSIDOMFILE
  NS_DECL_NSIXHRSENDABLE

  nsDOMFile(nsIFile *aFile, const nsAString& aContentType)
    : mFile(aFile),
      mContentType(aContentType),
      mIsFullFile(true)
  {}

  nsDOMFile(nsIFile *aFile)
    : mFile(aFile),
      mIsFullFile(true)
  {}

  nsDOMFile(const nsDOMFile* aOther, PRUint64 aStart, PRUint64 aLength,
            const nsAString& aContentType)
    : mFile(aOther->mFile),
      mStart(aOther->mIsFullFile ? aStart :
                                   (aOther->mStart + aStart)),
      mLength(aLength),
      mContentType(aContentType),
      mIsFullFile(false)
  {
    NS_ASSERTION(mFile, "must have file");
    
    mContentType.SetIsVoid(PR_FALSE);
  }

  virtual ~nsDOMFile() {}

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        PRUint32 aArgc,
                        jsval* aArgv);

  
  static nsresult
  NewFile(nsISupports* *aNewObject);

protected:
  nsCOMPtr<nsIFile> mFile;

  
  PRUint64 mStart;
  PRUint64 mLength;

  nsString mContentType;
  
  bool mIsFullFile;

  
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
      mDataOwner(new DataOwner(aMemoryBuffer)),
      mName(aName)
  {
    mStart = 0;
    mLength = aLength;
  }

  nsDOMMemoryFile(const nsDOMMemoryFile* aOther, PRUint64 aStart,
                  PRUint64 aLength, const nsAString& aContentType)
    : nsDOMFile(nsnull, aContentType),
      mDataOwner(aOther->mDataOwner)
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");

    mIsFullFile = false;
    mStart = aOther->mStart + aStart;
    mLength = aLength;

    
    mContentType.SetIsVoid(PR_FALSE);
  }

  NS_IMETHOD GetName(nsAString&);
  NS_IMETHOD GetSize(PRUint64*);
  NS_IMETHOD GetInternalStream(nsIInputStream**);
  NS_IMETHOD GetMozFullPathInternal(nsAString&);
  NS_IMETHOD MozSlice(PRInt64 aStart, PRInt64 aEnd,
                      const nsAString& aContentType, PRUint8 optional_argc,
                      nsIDOMBlob **aBlob);

protected:
  friend class DataOwnerAdapter; 
  class DataOwner {
  public:
    NS_INLINE_DECL_REFCOUNTING(DataOwner)
    DataOwner(void* aMemoryBuffer)
      : mData(aMemoryBuffer)
    {
    }
    ~DataOwner() {
      PR_Free(mData);
    }
    void* mData;
  };

  
  nsRefPtr<DataOwner> mDataOwner;

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
  nsDOMFileInternalUrlHolder(nsIDOMBlob* aFile, nsIPrincipal* aPrincipal
                             MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM);
  ~nsDOMFileInternalUrlHolder();
  nsAutoString mUrl;
private:
  MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#endif
