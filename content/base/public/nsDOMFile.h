




#ifndef nsDOMFile_h__
#define nsDOMFile_h__

#include "nsICharsetDetectionObserver.h"
#include "nsIFile.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIInputStream.h"
#include "nsIJSNativeInitializer.h"
#include "nsIMutable.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIXMLHttpRequest.h"
#include "prmem.h"
#include "nsAutoPtr.h"

#include "mozilla/GuardObjects.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/indexedDB/FileInfo.h"
#include "mozilla/dom/indexedDB/FileManager.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"

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
  typedef mozilla::dom::indexedDB::FileInfo FileInfo;

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType) = 0;

  virtual const nsTArray<nsCOMPtr<nsIDOMBlob> >*
  GetSubBlobs() const { return nullptr; }

  NS_DECL_NSIDOMBLOB
  NS_DECL_NSIDOMFILE
  NS_DECL_NSIXHRSENDABLE
  NS_DECL_NSIMUTABLE

  void
  SetLazyData(const nsAString& aName, const nsAString& aContentType,
              uint64_t aLength)
  {
    NS_ASSERTION(aLength, "must have length");

    mName = aName;
    mContentType = aContentType;
    mLength = aLength;

    mIsFile = !aName.IsVoid();
  }

  bool IsSizeUnknown() const
  {
    return mLength == UINT64_MAX;
  }

protected:
  nsDOMFileBase(const nsAString& aName, const nsAString& aContentType,
                uint64_t aLength)
    : mIsFile(true), mImmutable(false), mContentType(aContentType),
      mName(aName), mStart(0), mLength(aLength)
  {
    
    mContentType.SetIsVoid(false);
  }

  nsDOMFileBase(const nsAString& aContentType, uint64_t aLength)
    : mIsFile(false), mImmutable(false), mContentType(aContentType),
      mStart(0), mLength(aLength)
  {
    
    mContentType.SetIsVoid(false);
  }

  nsDOMFileBase(const nsAString& aContentType, uint64_t aStart,
                uint64_t aLength)
    : mIsFile(false), mImmutable(false), mContentType(aContentType),
      mStart(aStart), mLength(aLength)
  {
    NS_ASSERTION(aLength != UINT64_MAX,
                 "Must know length when creating slice");
    
    mContentType.SetIsVoid(false);
  }

  virtual ~nsDOMFileBase() {}

  virtual bool IsStoredFile() const
  {
    return false;
  }

  virtual bool IsWholeFile() const
  {
    NS_NOTREACHED("Should only be called on dom blobs backed by files!");
    return false;
  }

  virtual bool IsSnapshot() const
  {
    return false;
  }

  FileInfo* GetFileInfo() const
  {
    NS_ASSERTION(IsStoredFile(), "Should only be called on stored files!");
    NS_ASSERTION(!mFileInfos.IsEmpty(), "Must have at least one file info!");

    return mFileInfos.ElementAt(0);
  }

  bool mIsFile;
  bool mImmutable;
  nsString mContentType;
  nsString mName;

  uint64_t mStart;
  uint64_t mLength;

  
  nsTArray<nsRefPtr<FileInfo> > mFileInfos;
};

class nsDOMFile : public nsDOMFileBase
{
public:
  nsDOMFile(const nsAString& aName, const nsAString& aContentType,
            uint64_t aLength)
  : nsDOMFileBase(aName, aContentType, aLength)
  { }

  nsDOMFile(const nsAString& aContentType, uint64_t aLength)
  : nsDOMFileBase(aContentType, aLength)
  { }

  nsDOMFile(const nsAString& aContentType, uint64_t aStart, uint64_t aLength)
  : nsDOMFileBase(aContentType, aStart, aLength)
  { }

  NS_DECL_ISUPPORTS
};

class nsDOMFileCC : public nsDOMFileBase
{
public:
  nsDOMFileCC(const nsAString& aName, const nsAString& aContentType,
              uint64_t aLength)
  : nsDOMFileBase(aName, aContentType, aLength)
  { }

  nsDOMFileCC(const nsAString& aContentType, uint64_t aLength)
  : nsDOMFileBase(aContentType, aLength)
  { }

  nsDOMFileCC(const nsAString& aContentType, uint64_t aStart, uint64_t aLength)
  : nsDOMFileBase(aContentType, aStart, aLength)
  { }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMFileCC, nsIDOMFile)
};

class nsDOMFileFile : public nsDOMFile,
                      public nsIJSNativeInitializer
{
public:
  
  nsDOMFileFile(nsIFile *aFile)
    : nsDOMFile(EmptyString(), EmptyString(), UINT64_MAX),
      mFile(aFile), mWholeFile(true), mStoredFile(false)
  {
    NS_ASSERTION(mFile, "must have file");
    
    mContentType.SetIsVoid(true);
    mFile->GetLeafName(mName);
  }

  
  nsDOMFileFile(const nsAString& aName, const nsAString& aContentType,
                uint64_t aLength, nsIFile *aFile)
    : nsDOMFile(aName, aContentType, aLength),
      mFile(aFile), mWholeFile(true), mStoredFile(false)
  {
    NS_ASSERTION(mFile, "must have file");
  }

  
  nsDOMFileFile(nsIFile *aFile, const nsAString& aContentType,
                nsISupports *aCacheToken)
    : nsDOMFile(aContentType, UINT64_MAX),
      mFile(aFile), mWholeFile(true), mStoredFile(false),
      mCacheToken(aCacheToken)
  {
    NS_ASSERTION(mFile, "must have file");
  }

  
  nsDOMFileFile(nsIFile *aFile, const nsAString& aName)
    : nsDOMFile(aName, EmptyString(), UINT64_MAX),
      mFile(aFile), mWholeFile(true), mStoredFile(false)
  {
    NS_ASSERTION(mFile, "must have file");
    
    mContentType.SetIsVoid(true);
  }

  
  nsDOMFileFile(const nsAString& aName, const nsAString& aContentType,
                uint64_t aLength, nsIFile* aFile,
                FileInfo* aFileInfo)
    : nsDOMFile(aName, aContentType, aLength),
      mFile(aFile), mWholeFile(true), mStoredFile(true)
  {
    NS_ASSERTION(mFile, "must have file");
    mFileInfos.AppendElement(aFileInfo);
  }

  
  nsDOMFileFile(const nsAString& aContentType, uint64_t aLength,
                nsIFile* aFile, FileInfo* aFileInfo)
    : nsDOMFile(aContentType, aLength),
      mFile(aFile), mWholeFile(true), mStoredFile(true)
  {
    NS_ASSERTION(mFile, "must have file");
    mFileInfos.AppendElement(aFileInfo);
  }

  
  nsDOMFileFile()
    : nsDOMFile(EmptyString(), EmptyString(), UINT64_MAX),
      mWholeFile(true), mStoredFile(false)
  {
    
    mContentType.SetIsVoid(true);
    mName.SetIsVoid(true);
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        uint32_t aArgc,
                        jsval* aArgv);

  
  NS_IMETHOD GetSize(uint64_t* aSize);
  NS_IMETHOD GetType(nsAString& aType);
  NS_IMETHOD GetLastModifiedDate(JSContext* cx, JS::Value *aLastModifiedDate);
  NS_IMETHOD GetMozFullPathInternal(nsAString& aFullPath);
  NS_IMETHOD GetInternalStream(nsIInputStream**);

  
  static nsresult
  NewFile(nsISupports* *aNewObject);

protected:
  
  nsDOMFileFile(const nsDOMFileFile* aOther, uint64_t aStart, uint64_t aLength,
                const nsAString& aContentType)
    : nsDOMFile(aContentType, aOther->mStart + aStart, aLength),
      mFile(aOther->mFile), mWholeFile(false),
      mStoredFile(aOther->mStoredFile), mCacheToken(aOther->mCacheToken)
  {
    NS_ASSERTION(mFile, "must have file");
    mImmutable = aOther->mImmutable;

    if (mStoredFile) {
      FileInfo* fileInfo;

      using mozilla::dom::indexedDB::IndexedDatabaseManager;

      if (IndexedDatabaseManager::IsClosed()) {
        fileInfo = aOther->GetFileInfo();
      }
      else {
        mozilla::MutexAutoLock lock(IndexedDatabaseManager::FileMutex());
        fileInfo = aOther->GetFileInfo();
      }

      mFileInfos.AppendElement(fileInfo);
    }
  }

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType);

  virtual bool IsStoredFile() const
  {
    return mStoredFile;
  }

  virtual bool IsWholeFile() const
  {
    return mWholeFile;
  }

  nsCOMPtr<nsIFile> mFile;
  bool mWholeFile;
  bool mStoredFile;
  nsCOMPtr<nsISupports> mCacheToken;
};

class nsDOMMemoryFile : public nsDOMFile
{
public:
  
  nsDOMMemoryFile(void *aMemoryBuffer,
                  uint64_t aLength,
                  const nsAString& aName,
                  const nsAString& aContentType)
    : nsDOMFile(aName, aContentType, aLength),
      mDataOwner(new DataOwner(aMemoryBuffer))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  
  nsDOMMemoryFile(void *aMemoryBuffer,
                  uint64_t aLength,
                  const nsAString& aContentType)
    : nsDOMFile(aContentType, aLength),
      mDataOwner(new DataOwner(aMemoryBuffer))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  NS_IMETHOD GetInternalStream(nsIInputStream**);

protected:
  
  nsDOMMemoryFile(const nsDOMMemoryFile* aOther, uint64_t aStart,
                  uint64_t aLength, const nsAString& aContentType)
    : nsDOMFile(aContentType, aOther->mStart + aStart, aLength),
      mDataOwner(aOther->mDataOwner)
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
    mImmutable = aOther->mImmutable;
  }
  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType);

  friend class DataOwnerAdapter; 
  class DataOwner {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DataOwner)
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

class nsDOMFileList MOZ_FINAL : public nsIDOMFileList,
                                public nsWrapperCache
{
public:
  nsDOMFileList(nsISupports *aParent) : mParent(aParent)
  {
    SetIsDOMBinding();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMFileList)

  NS_DECL_NSIDOMFILELIST

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return mParent;
  }

  void Disconnect()
  {
    mParent = nullptr;
  }

  bool Append(nsIDOMFile *aFile) { return mFiles.AppendObject(aFile); }

  bool Remove(uint32_t aIndex) { return mFiles.RemoveObjectAt(aIndex); }
  void Clear() { return mFiles.Clear(); }

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
  nsISupports *mParent;
};

class NS_STACK_CLASS nsDOMFileInternalUrlHolder {
public:
  nsDOMFileInternalUrlHolder(nsIDOMBlob* aFile, nsIPrincipal* aPrincipal
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
  ~nsDOMFileInternalUrlHolder();
  nsAutoString mUrl;
private:
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#endif
