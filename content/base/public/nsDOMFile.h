





































#ifndef nsDOMFile_h__
#define nsDOMFile_h__

#include "nsICharsetDetectionObserver.h"
#include "nsIFile.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIDOMFileError.h"
#include "nsIInputStream.h"
#include "nsIJSNativeInitializer.h"
#include "nsIMutable.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozilla/AutoRestore.h"
#include "nsString.h"
#include "nsIXMLHttpRequest.h"
#include "prmem.h"
#include "nsAutoPtr.h"

#ifndef PR_UINT64_MAX
#define PR_UINT64_MAX (~(PRUint64)(0))
#endif

class nsIFile;
class nsIInputStream;
class nsIClassInfo;
class nsIBlobBuilder;

nsresult NS_NewBlobBuilder(nsISupports* *aSupports);

class nsDOMFileBase : public nsIDOMFile,
                      public nsIXHRSendable,
                      public nsIMutable
{
public:

  nsDOMFileBase(const nsAString& aName, const nsAString& aContentType,
                PRUint64 aLength)
    : mIsFile(true), mImmutable(false), mContentType(aContentType),
      mName(aName), mStart(0), mLength(aLength)
  {
    
    mContentType.SetIsVoid(PR_FALSE);
  }

  nsDOMFileBase(const nsAString& aContentType, PRUint64 aLength)
    : mIsFile(false), mImmutable(false), mContentType(aContentType),
      mStart(0), mLength(aLength)
  {
    
    mContentType.SetIsVoid(PR_FALSE);
  }

  nsDOMFileBase(const nsAString& aContentType,
                PRUint64 aStart, PRUint64 aLength)
    : mIsFile(false), mImmutable(false), mContentType(aContentType),
      mStart(aStart), mLength(aLength)
  {
    NS_ASSERTION(aLength != PR_UINT64_MAX,
                 "Must know length when creating slice");
    
    mContentType.SetIsVoid(PR_FALSE);
  }

  virtual ~nsDOMFileBase() {}

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(PRUint64 aStart, PRUint64 aLength,
              const nsAString& aContentType) = 0;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLOB
  NS_DECL_NSIDOMFILE
  NS_DECL_NSIXHRSENDABLE
  NS_DECL_NSIMUTABLE

protected:
  bool IsSizeUnknown()
  {
    return mLength == PR_UINT64_MAX;
  }

  bool mIsFile;
  bool mImmutable;
  nsString mContentType;
  nsString mName;

  PRUint64 mStart;
  PRUint64 mLength;
};

class nsDOMFileFile : public nsDOMFileBase,
                      public nsIJSNativeInitializer
{
public:
  
  nsDOMFileFile(nsIFile *aFile)
    : nsDOMFileBase(EmptyString(), EmptyString(), PR_UINT64_MAX),
      mFile(aFile), mWholeFile(true)
  {
    NS_ASSERTION(mFile, "must have file");
    
    mContentType.SetIsVoid(PR_TRUE);
    mFile->GetLeafName(mName);
  }

  
  nsDOMFileFile(nsIFile *aFile, const nsAString& aContentType,
                nsISupports *aCacheToken = nsnull)
    : nsDOMFileBase(aContentType, PR_UINT64_MAX),
      mFile(aFile), mWholeFile(true),
      mCacheToken(aCacheToken)
  {
    NS_ASSERTION(mFile, "must have file");
  }

  
  nsDOMFileFile()
    : nsDOMFileBase(EmptyString(), EmptyString(), PR_UINT64_MAX),
      mWholeFile(true)
  {
    
    mContentType.SetIsVoid(PR_TRUE);
    mName.SetIsVoid(PR_TRUE);
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        PRUint32 aArgc,
                        jsval* aArgv);

  
  NS_IMETHOD GetSize(PRUint64* aSize);
  NS_IMETHOD GetType(nsAString& aType);
  NS_IMETHOD GetMozFullPathInternal(nsAString& aFullPath);
  NS_IMETHOD GetInternalStream(nsIInputStream**);

  
  static nsresult
  NewFile(nsISupports* *aNewObject);

protected:
  
  nsDOMFileFile(const nsDOMFileFile* aOther, PRUint64 aStart, PRUint64 aLength,
                const nsAString& aContentType)
    : nsDOMFileBase(aContentType, aOther->mStart + aStart, aLength),
      mFile(aOther->mFile), mWholeFile(false),
      mCacheToken(aOther->mCacheToken)
  {
    NS_ASSERTION(mFile, "must have file");
    mImmutable = aOther->mImmutable;
  }
  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(PRUint64 aStart, PRUint64 aLength,
              const nsAString& aContentType);

  nsCOMPtr<nsIFile> mFile;
  bool mWholeFile;
  nsCOMPtr<nsISupports> mCacheToken;
};

class nsDOMMemoryFile : public nsDOMFileBase
{
public:
  
  nsDOMMemoryFile(void *aMemoryBuffer,
                  PRUint64 aLength,
                  const nsAString& aName,
                  const nsAString& aContentType)
    : nsDOMFileBase(aName, aContentType, aLength),
      mDataOwner(new DataOwner(aMemoryBuffer))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  
  nsDOMMemoryFile(void *aMemoryBuffer,
                  PRUint64 aLength,
                  const nsAString& aContentType)
    : nsDOMFileBase(aContentType, aLength),
      mDataOwner(new DataOwner(aMemoryBuffer))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  NS_IMETHOD GetInternalStream(nsIInputStream**);

protected:
  
  nsDOMMemoryFile(const nsDOMMemoryFile* aOther, PRUint64 aStart,
                  PRUint64 aLength, const nsAString& aContentType)
    : nsDOMFileBase(aContentType, aOther->mStart + aStart, aLength),
      mDataOwner(aOther->mDataOwner)
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
    mImmutable = aOther->mImmutable;
  }
  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(PRUint64 aStart, PRUint64 aLength,
              const nsAString& aContentType);

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
};

class nsDOMFileList : public nsIDOMFileList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILELIST

  bool Append(nsIDOMFile *aFile) { return mFiles.AppendObject(aFile); }

  bool Remove(PRUint32 aIndex) { return mFiles.RemoveObjectAt(aIndex); }
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
