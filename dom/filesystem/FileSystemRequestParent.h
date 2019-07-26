





#ifndef mozilla_dom_FileSystemRequestParent_h
#define mozilla_dom_FileSystemRequestParent_h

#include "mozilla/dom/PFileSystemRequestParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"

namespace mozilla {
namespace dom {

class FileSystemBase;

class FileSystemRequestParent
  : public PFileSystemRequestParent
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileSystemRequestParent)
public:
  FileSystemRequestParent();

  virtual
  ~FileSystemRequestParent();

  bool
  IsRunning()
  {
    return state() == PFileSystemRequest::__Start;
  }

  bool
  Dispatch(ContentParent* aParent, const FileSystemParams& aParams);
private:
  nsRefPtr<FileSystemBase> mFileSystem;
};

} 
} 

#endif 
