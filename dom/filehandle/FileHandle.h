





#ifndef mozilla_dom_FileHandle_h
#define mozilla_dom_FileHandle_h

#include "MainThreadUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/FileModeBinding.h"
#include "mozilla/dom/Nullable.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIRunnable.h"
#include "nsTArray.h"

class nsAString;
class nsIDOMBlob;

namespace mozilla {
namespace dom {

class FileHelper;
class FileRequestBase;
class FileService;
class FinishHelper;
class MetadataHelper;
class MutableFileBase;




class FileHandleBase
{
public:
  enum RequestMode
  {
    NORMAL = 0, 
    PARALLEL
  };

  enum ReadyState
  {
    INITIAL = 0,
    LOADING,
    FINISHING,
    DONE
  };

private:
  friend class FileHelper;
  friend class FileService;
  friend class FinishHelper;
  friend class MetadataHelper;

  ReadyState mReadyState;
  FileMode mMode;
  RequestMode mRequestMode;
  uint64_t mLocation;
  uint32_t mPendingRequests;

  nsTArray<nsCOMPtr<nsISupports>> mParallelStreams;
  nsCOMPtr<nsISupports> mStream;

  bool mAborted;
  bool mCreating;

public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;

  nsresult
  CreateParallelStream(nsISupports** aStream);

  nsresult
  GetOrCreateStream(nsISupports** aStream);

  bool
  IsOpen() const;

  bool
  IsAborted() const
  {
    return mAborted;
  }

  void
  SetCreating()
  {
    mCreating = true;
  }

  virtual MutableFileBase*
  MutableFile() const = 0;

  nsresult
  OpenInputStream(bool aWholeFile, uint64_t aStart, uint64_t aLength,
                  nsIInputStream** aResult);

  
  FileMode
  Mode() const
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    return mMode;
  }

  bool
  Active() const
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    return IsOpen();
  }

  Nullable<uint64_t>
  GetLocation() const
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    if (mLocation == UINT64_MAX) {
      return Nullable<uint64_t>();
    }

    return Nullable<uint64_t>(mLocation);
  }

  void
  SetLocation(const Nullable<uint64_t>& aLocation)
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    
    if (aLocation.IsNull()) {
      mLocation = UINT64_MAX;
    } else {
      mLocation = aLocation.Value();
    }
  }

  already_AddRefed<FileRequestBase>
  Read(uint64_t aSize, bool aHasEncoding, const nsAString& aEncoding,
       ErrorResult& aRv);

  already_AddRefed<FileRequestBase>
  Truncate(const Optional<uint64_t>& aSize, ErrorResult& aRv);

  already_AddRefed<FileRequestBase>
  Flush(ErrorResult& aRv);

  void
  Abort(ErrorResult& aRv);

protected:
  FileHandleBase(FileMode aMode,
                 RequestMode aRequestMode);
  ~FileHandleBase();

  void
  OnNewRequest();

  void
  OnRequestFinished();

  void
  OnReturnToEventLoop();

  virtual nsresult
  OnCompleteOrAbort(bool aAborted) = 0;

  bool
  CheckState(ErrorResult& aRv);

  bool
  CheckStateAndArgumentsForRead(uint64_t aSize, ErrorResult& aRv);

  bool
  CheckStateForWrite(ErrorResult& aRv);

  virtual bool
  CheckWindow() = 0;

  virtual already_AddRefed<FileRequestBase>
  GenerateFileRequest() = 0;

  template<class T>
  already_AddRefed<FileRequestBase>
  WriteOrAppend(const T& aValue, bool aAppend, ErrorResult& aRv)
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    
    if (!CheckStateForWrite(aRv)) {
      return nullptr;
    }

    
    if (!aAppend && mLocation == UINT64_MAX) {
      aRv.Throw(NS_ERROR_DOM_FILEHANDLE_NOT_ALLOWED_ERR);
      return nullptr;
    }

    uint64_t length;
    nsCOMPtr<nsIInputStream> stream = GetInputStream(aValue, &length, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    if (!length) {
      return nullptr;
    }

    
    if (!CheckWindow()) {
      return nullptr;
    }

    return WriteInternal(stream, length, aAppend, aRv);
  }

  already_AddRefed<FileRequestBase>
  WriteInternal(nsIInputStream* aInputStream, uint64_t aInputLength,
                bool aAppend, ErrorResult& aRv);

  nsresult
  Finish();

  static already_AddRefed<nsIInputStream>
  GetInputStream(const ArrayBuffer& aValue, uint64_t* aInputLength,
                 ErrorResult& aRv);

  static already_AddRefed<nsIInputStream>
  GetInputStream(nsIDOMBlob* aValue, uint64_t* aInputLength, ErrorResult& aRv);

  static already_AddRefed<nsIInputStream>
  GetInputStream(const nsAString& aValue, uint64_t* aInputLength,
                 ErrorResult& aRv);
};

class FinishHelper MOZ_FINAL : public nsIRunnable
{
  friend class FileHandleBase;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

private:
  explicit FinishHelper(FileHandleBase* aFileHandle);
  ~FinishHelper()
  { }

  nsRefPtr<FileHandleBase> mFileHandle;
  nsTArray<nsCOMPtr<nsISupports>> mParallelStreams;
  nsCOMPtr<nsISupports> mStream;

  bool mAborted;
};

} 
} 

#endif
