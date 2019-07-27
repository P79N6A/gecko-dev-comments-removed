





#ifndef mozilla_dom_OSFileSystem_h
#define mozilla_dom_OSFileSystem_h

#include "mozilla/dom/FileSystemBase.h"

namespace mozilla {
namespace dom {

class OSFileSystem : public FileSystemBase
{
public:
  explicit OSFileSystem(const nsAString& aRootDir);

  void
  Init(nsPIDOMWindow* aWindow);

  

  virtual nsPIDOMWindow*
  GetWindow() const override;

  virtual void
  GetRootName(nsAString& aRetval) const override;

  virtual bool
  IsSafeFile(nsIFile* aFile) const override;

  virtual bool
  IsSafeDirectory(Directory* aDir) const override;

private:
  virtual ~OSFileSystem() {}

   nsCOMPtr<nsPIDOMWindow> mWindow;
};

} 
} 

#endif 
