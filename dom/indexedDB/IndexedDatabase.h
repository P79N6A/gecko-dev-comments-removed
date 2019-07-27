





#ifndef mozilla_dom_indexeddb_indexeddatabase_h__
#define mozilla_dom_indexeddb_indexeddatabase_h__

#include "js/StructuredClone.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class File;

namespace indexedDB {

class FileInfo;
class IDBDatabase;
class SerializedStructuredCloneReadInfo;

struct StructuredCloneFile
{
  nsRefPtr<File> mFile;
  nsRefPtr<FileInfo> mFileInfo;

  
  inline
  StructuredCloneFile();

  
  inline
  ~StructuredCloneFile();

  
  inline bool
  operator==(const StructuredCloneFile& aOther) const;
};

struct StructuredCloneReadInfo
{
  nsTArray<uint8_t> mData;
  nsTArray<StructuredCloneFile> mFiles;
  IDBDatabase* mDatabase;

  
  JSAutoStructuredCloneBuffer mCloneBuffer;

  
  inline
  StructuredCloneReadInfo();

  
  inline
  ~StructuredCloneReadInfo();

  
  inline StructuredCloneReadInfo&
  operator=(StructuredCloneReadInfo&& aOther);

  
  inline
  MOZ_IMPLICIT StructuredCloneReadInfo(SerializedStructuredCloneReadInfo&& aOther);
};

} 
} 
} 

#endif 
