





#ifndef mozilla_dom_indexeddb_indexeddatabase_h__
#define mozilla_dom_indexeddb_indexeddatabase_h__

#include "nsIProgrammingLanguage.h"

#include "mozilla/Attributes.h"
#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsDOMError.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

#define BEGIN_INDEXEDDB_NAMESPACE \
  namespace mozilla { namespace dom { namespace indexedDB {

#define END_INDEXEDDB_NAMESPACE \
  } /* namespace indexedDB */ } /* namepsace dom */ } /* namespace mozilla */

#define USING_INDEXEDDB_NAMESPACE \
  using namespace mozilla::dom::indexedDB;

class nsIDOMBlob;

BEGIN_INDEXEDDB_NAMESPACE

class FileInfo;

struct SerializedStructuredCloneReadInfo;

struct StructuredCloneReadInfo
{
  void Swap(StructuredCloneReadInfo& aCloneReadInfo)
  {
    mCloneBuffer.swap(aCloneReadInfo.mCloneBuffer);
    mFileInfos.SwapElements(aCloneReadInfo.mFileInfos);
  }

  
  inline bool
  SetFromSerialized(const SerializedStructuredCloneReadInfo& aOther);

  JSAutoStructuredCloneBuffer mCloneBuffer;
  nsTArray<nsRefPtr<FileInfo> > mFileInfos;
};

struct SerializedStructuredCloneReadInfo
{
  SerializedStructuredCloneReadInfo()
  : data(nsnull), dataLength(0)
  { }

  bool
  operator==(const SerializedStructuredCloneReadInfo& aOther) const
  {
    return this->data == aOther.data &&
           this->dataLength == aOther.dataLength;
  }

  SerializedStructuredCloneReadInfo&
  operator=(const StructuredCloneReadInfo& aOther)
  {
    data = aOther.mCloneBuffer.data();
    dataLength = aOther.mCloneBuffer.nbytes();
    return *this;
  }

  
  uint64_t* data;
  size_t dataLength;
};

struct SerializedStructuredCloneWriteInfo;

struct StructuredCloneWriteInfo
{
  void Swap(StructuredCloneWriteInfo& aCloneWriteInfo)
  {
    mCloneBuffer.swap(aCloneWriteInfo.mCloneBuffer);
    mBlobs.SwapElements(aCloneWriteInfo.mBlobs);
    mOffsetToKeyProp = aCloneWriteInfo.mOffsetToKeyProp;
  }

  bool operator==(const StructuredCloneWriteInfo& aOther) const
  {
    return this->mCloneBuffer.nbytes() == aOther.mCloneBuffer.nbytes() &&
           this->mCloneBuffer.data() == aOther.mCloneBuffer.data() &&
           this->mBlobs == aOther.mBlobs &&
           this->mOffsetToKeyProp == aOther.mOffsetToKeyProp;
  }

  
  inline bool
  SetFromSerialized(const SerializedStructuredCloneWriteInfo& aOther);

  JSAutoStructuredCloneBuffer mCloneBuffer;
  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
  PRUint64 mOffsetToKeyProp;
};

struct SerializedStructuredCloneWriteInfo
{
  SerializedStructuredCloneWriteInfo()
  : data(nsnull), dataLength(0), offsetToKeyProp(0)
  { }

  bool
  operator==(const SerializedStructuredCloneWriteInfo& aOther) const
  {
    return this->data == aOther.data &&
           this->dataLength == aOther.dataLength &&
           this->offsetToKeyProp == aOther.offsetToKeyProp;
  }

  SerializedStructuredCloneWriteInfo&
  operator=(const StructuredCloneWriteInfo& aOther)
  {
    data = aOther.mCloneBuffer.data();
    dataLength = aOther.mCloneBuffer.nbytes();
    offsetToKeyProp = aOther.mOffsetToKeyProp;
    return *this;
  }

  
  uint64_t* data;
  size_t dataLength;
  uint64_t offsetToKeyProp;
};

END_INDEXEDDB_NAMESPACE

#endif 
