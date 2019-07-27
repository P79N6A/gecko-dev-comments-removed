





#ifndef mozilla_dom_Directory_h
#define mozilla_dom_Directory_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/File.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"




#ifdef CreateDirectory
#undef CreateDirectory
#endif


#ifdef CreateFile
#undef CreateFile
#endif

namespace mozilla {
namespace dom {

struct CreateFileOptions;
class FileSystemBase;
class Promise;
class StringOrFileOrDirectory;

class Directory final
  : public nsISupports
  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Directory)

public:
  static already_AddRefed<Promise>
  GetRoot(FileSystemBase* aFileSystem, ErrorResult& aRv);

  Directory(FileSystemBase* aFileSystem, const nsAString& aPath);

  

  nsPIDOMWindow*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void
  GetName(nsAString& aRetval) const;

  already_AddRefed<Promise>
  CreateFile(const nsAString& aPath, const CreateFileOptions& aOptions,
             ErrorResult& aRv);

  already_AddRefed<Promise>
  CreateDirectory(const nsAString& aPath, ErrorResult& aRv);

  already_AddRefed<Promise>
  Get(const nsAString& aPath, ErrorResult& aRv);

  already_AddRefed<Promise>
  Remove(const StringOrFileOrDirectory& aPath, ErrorResult& aRv);

  already_AddRefed<Promise>
  RemoveDeep(const StringOrFileOrDirectory& aPath, ErrorResult& aRv);

  

  void
  GetPath(nsAString& aRetval) const;

  already_AddRefed<Promise>
  GetFilesAndDirectories();

  

  FileSystemBase*
  GetFileSystem() const;
private:
  ~Directory();

  static bool
  IsValidRelativePath(const nsString& aPath);

  



  bool
  DOMPathToRealPath(const nsAString& aPath, nsAString& aRealPath) const;

  already_AddRefed<Promise>
  RemoveInternal(const StringOrFileOrDirectory& aPath, bool aRecursive,
                 ErrorResult& aRv);

  nsRefPtr<FileSystemBase> mFileSystem;
  nsString mPath;
};

} 
} 

#endif 
