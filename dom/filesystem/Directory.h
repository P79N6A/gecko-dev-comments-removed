





#ifndef mozilla_dom_Directory_h
#define mozilla_dom_Directory_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMFile.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"




#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace mozilla {
namespace dom {

class FileSystemBase;
class Promise;
class StringOrFileOrDirectory;

class Directory MOZ_FINAL
  : public nsISupports
  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Directory)

public:
  static already_AddRefed<Promise>
  GetRoot(FileSystemBase* aFileSystem);

  Directory(FileSystemBase* aFileSystem, const nsAString& aPath);
  ~Directory();

  

  nsPIDOMWindow*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void
  GetName(nsString& aRetval) const;

  already_AddRefed<Promise>
  CreateDirectory(const nsAString& aPath);

  already_AddRefed<Promise>
  Get(const nsAString& aPath);

  already_AddRefed<Promise>
  Remove(const StringOrFileOrDirectory& aPath);

  already_AddRefed<Promise>
  RemoveDeep(const StringOrFileOrDirectory& aPath);

  

  FileSystemBase*
  GetFileSystem() const;
private:
  static bool
  IsValidRelativePath(const nsString& aPath);

  



  bool
  DOMPathToRealPath(const nsAString& aPath, nsAString& aRealPath) const;

  already_AddRefed<Promise>
  RemoveInternal(const StringOrFileOrDirectory& aPath, bool aRecursive);

  nsRefPtr<FileSystemBase> mFileSystem;
  nsString mPath;
};

} 
} 

#endif 
