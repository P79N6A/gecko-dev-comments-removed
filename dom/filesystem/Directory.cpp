





#include "mozilla/dom/Directory.h"

#include "CreateDirectoryTask.h"
#include "GetFileOrDirectoryTask.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsString.h"
#include "mozilla/dom/DirectoryBinding.h"
#include "mozilla/dom/FileSystemBase.h"
#include "mozilla/dom/FileSystemUtils.h"




#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Directory)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Directory)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Directory)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Directory)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


already_AddRefed<Promise>
Directory::GetRoot(FileSystemBase* aFileSystem)
{
  nsRefPtr<GetFileOrDirectoryTask> task = new GetFileOrDirectoryTask(
    aFileSystem, EmptyString(), true);
  task->Start();
  return task->GetPromise();
}

Directory::Directory(FileSystemBase* aFileSystem,
                     const nsAString& aPath)
  : mFileSystem(aFileSystem)
  , mPath(aPath)
{
  MOZ_ASSERT(aFileSystem, "aFileSystem should not be null.");
  
  mPath.Trim(FILESYSTEM_DOM_PATH_SEPARATOR, false, true);

  SetIsDOMBinding();
}

Directory::~Directory()
{
}

nsPIDOMWindow*
Directory::GetParentObject() const
{
  return mFileSystem->GetWindow();
}

JSObject*
Directory::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DirectoryBinding::Wrap(aCx, aScope, this);
}

void
Directory::GetName(nsString& aRetval) const
{
  aRetval.Truncate();

  if (mPath.IsEmpty()) {
    aRetval = mFileSystem->GetRootName();
    return;
  }

  aRetval = Substring(mPath,
                      mPath.RFindChar(FileSystemUtils::kSeparatorChar) + 1);
}

already_AddRefed<Promise>
Directory::CreateDirectory(const nsAString& aPath)
{
  nsresult error = NS_OK;
  nsString realPath;
  if (!DOMPathToRealPath(aPath, realPath)) {
    error = NS_ERROR_DOM_FILESYSTEM_INVALID_PATH_ERR;
  }
  nsRefPtr<CreateDirectoryTask> task = new CreateDirectoryTask(
    mFileSystem, realPath);
  task->SetError(error);
  task->Start();
  return task->GetPromise();
}

already_AddRefed<Promise>
Directory::Get(const nsAString& aPath)
{
  nsresult error = NS_OK;
  nsString realPath;
  if (!DOMPathToRealPath(aPath, realPath)) {
    error = NS_ERROR_DOM_FILESYSTEM_INVALID_PATH_ERR;
  }
  nsRefPtr<GetFileOrDirectoryTask> task = new GetFileOrDirectoryTask(
    mFileSystem, realPath, false);
  task->SetError(error);
  task->Start();
  return task->GetPromise();
}

bool
Directory::DOMPathToRealPath(const nsAString& aPath, nsAString& aRealPath) const
{
  aRealPath.Truncate();

  nsString relativePath;
  relativePath = aPath;

  
  static const char kWhitespace[] = "\b\t\r\n ";
  relativePath.Trim(kWhitespace);

  if (!IsValidRelativePath(relativePath)) {
    return false;
  }

  aRealPath = mPath + NS_LITERAL_STRING(FILESYSTEM_DOM_PATH_SEPARATOR) +
    relativePath;

  return true;
}


bool
Directory::IsValidRelativePath(const nsString& aPath)
{
  
  if (aPath.IsEmpty()) {
    return false;
  }

  
  if (aPath.First() == FileSystemUtils::kSeparatorChar ||
      aPath.Last() == FileSystemUtils::kSeparatorChar) {
    return false;
  }

  NS_NAMED_LITERAL_STRING(kCurrentDir, ".");
  NS_NAMED_LITERAL_STRING(kParentDir, "..");

  
  nsCharSeparatedTokenizer tokenizer(aPath, FileSystemUtils::kSeparatorChar);
  while (tokenizer.hasMoreTokens()) {
    nsDependentSubstring pathComponent = tokenizer.nextToken();
    
    
    
    if (pathComponent.IsEmpty() ||
        pathComponent.Equals(kCurrentDir) ||
        pathComponent.Equals(kParentDir)) {
      return false;
    }
  }

  return true;
}

} 
} 
