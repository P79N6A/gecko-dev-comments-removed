





#ifndef mozilla_dom_FileSystemRequestParent_h
#define mozilla_dom_FileSystemRequestParent_h

#include "mozilla/dom/PFileSystemRequestParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"

namespace mozilla {
namespace dom {

class FileSystemBase;

class FileSystemRequestParent MOZ_FINAL
  : public PFileSystemRequestParent
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileSystemRequestParent)
public:
  FileSystemRequestParent();

  bool
  IsRunning()
  {
    return state() == PFileSystemRequest::__Start;
  }

  bool
  Dispatch(ContentParent* aParent, const FileSystemParams& aParams);

  virtual void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

private:
  
  virtual
  ~FileSystemRequestParent();

  nsRefPtr<FileSystemBase> mFileSystem;
};

} 
} 

#endif 
