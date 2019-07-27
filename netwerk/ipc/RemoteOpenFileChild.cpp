





#include "RemoteOpenFileChild.h"

#include "mozilla/unused.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/net/NeckoChild.h"
#include "nsThreadUtils.h"
#include "nsJARProtocolHandler.h"
#include "nsIRemoteOpenFileListener.h"
#include "nsProxyRelease.h"
#include "SerializedLoadContext.h"


#include "private/pprio.h"

#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
#include <unistd.h>
#endif

using namespace mozilla::ipc;

namespace mozilla {
namespace net {





class CallsListenerInNewEvent : public nsRunnable
{
public:
    CallsListenerInNewEvent(nsIRemoteOpenFileListener *aListener, nsresult aRv)
      : mListener(aListener), mRV(aRv)
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(aListener);
    }

    void Dispatch()
    {
        MOZ_ASSERT(NS_IsMainThread());

        nsresult rv = NS_DispatchToCurrentThread(this);
        NS_ENSURE_SUCCESS_VOID(rv);
    }

private:
    NS_IMETHOD Run()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mListener);

        mListener->OnRemoteFileOpenComplete(mRV);
        return NS_OK;
    }

    nsCOMPtr<nsIRemoteOpenFileListener> mListener;
    nsresult mRV;
};





NS_IMPL_ISUPPORTS(RemoteOpenFileChild,
                  nsIFile,
                  nsIHashable,
                  nsICachedFileDescriptorListener)

RemoteOpenFileChild::RemoteOpenFileChild(const RemoteOpenFileChild& other)
  : mTabChild(other.mTabChild)
  , mNSPRFileDesc(nullptr)
  , mAsyncOpenCalled(other.mAsyncOpenCalled)
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  
  
  MOZ_ASSERT(!other.mNSPRFileDesc);
#else
  if (other.mNSPRFileDesc) {
    PROsfd osfd = dup(PR_FileDesc2NativeHandle(other.mNSPRFileDesc));
    mNSPRFileDesc = PR_ImportFile(osfd);
  }
#endif

  
  other.mURI->Clone(getter_AddRefs(mURI));
  if (other.mAppURI) {
    other.mAppURI->Clone(getter_AddRefs(mAppURI));
  }
  other.mFile->Clone(getter_AddRefs(mFile));
}

RemoteOpenFileChild::~RemoteOpenFileChild()
{
  if (NS_IsMainThread()) {
    if (mListener) {
      NotifyListener(NS_ERROR_UNEXPECTED);
    }
  } else {
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    if (mainThread) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_ProxyRelease(mainThread, mURI, true)));
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_ProxyRelease(mainThread, mAppURI, true)));
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_ProxyRelease(mainThread, mListener,
                                                   true)));

      TabChild* tabChild;
      mTabChild.forget(&tabChild);

      if (tabChild) {
        nsCOMPtr<nsIRunnable> runnable =
          NS_NewNonOwningRunnableMethod(tabChild, &TabChild::Release);
        MOZ_ASSERT(runnable);

        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mainThread->Dispatch(runnable,
                                                          NS_DISPATCH_NORMAL)));
      }
    } else {
      using mozilla::unused;

      NS_WARNING("RemoteOpenFileChild released after thread shutdown, leaking "
                 "its members!");

      unused << mURI.forget();
      unused << mAppURI.forget();
      unused << mListener.forget();
      unused << mTabChild.forget();
    }
  }

  if (mNSPRFileDesc) {
    
    PR_Close(mNSPRFileDesc);
  }
}

nsresult
RemoteOpenFileChild::Init(nsIURI* aRemoteOpenUri, nsIURI* aAppUri)
{
  if (!aRemoteOpenUri) {
    return NS_ERROR_INVALID_ARG;
  }

  if (aAppUri) {
    aAppUri->Clone(getter_AddRefs(mAppURI));
  }

  nsAutoCString scheme;
  nsresult rv = aRemoteOpenUri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!scheme.EqualsLiteral("remoteopenfile")) {
    return NS_ERROR_INVALID_ARG;
  }

  
  nsCOMPtr<nsIURI> clonedURI;
  rv = aRemoteOpenUri->Clone(getter_AddRefs(clonedURI));
  NS_ENSURE_SUCCESS(rv, rv);

  clonedURI->SetScheme(NS_LITERAL_CSTRING("file"));
  nsAutoCString spec;
  clonedURI->GetSpec(spec);

  rv = NS_NewURI(getter_AddRefs(mURI), spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mURI);
  if (!fileURL) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = fileURL->GetFile(getter_AddRefs(mFile));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
RemoteOpenFileChild::AsyncRemoteFileOpen(int32_t aFlags,
                                         nsIRemoteOpenFileListener* aListener,
                                         nsITabChild* aTabChild,
                                         nsILoadContext *aLoadContext)
{
  if (!mFile) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (!aListener) {
    return NS_ERROR_INVALID_ARG;
  }

  if (mAsyncOpenCalled) {
    return NS_ERROR_ALREADY_OPENED;
  }

  if (aFlags != PR_RDONLY) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  mTabChild = static_cast<TabChild*>(aTabChild);

  if (MissingRequiredTabChild(mTabChild, "remoteopenfile")) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  
  
  nsRefPtr<CallsListenerInNewEvent> runnable =
    new CallsListenerInNewEvent(aListener, NS_OK);
  runnable->Dispatch();

  mAsyncOpenCalled = true;
  return NS_OK;
#else
  nsString path;
  if (NS_FAILED(mFile->GetPath(path))) {
    MOZ_CRASH("Couldn't get path from file!");
  }

  if (mTabChild) {
    if (mTabChild->GetCachedFileDescriptor(path, this)) {
      
      
      return NS_OK;
    }
  }

  URIParams uri;
  SerializeURI(mURI, uri);
  OptionalURIParams appUri;
  SerializeURI(mAppURI, appUri);

  IPC::SerializedLoadContext loadContext(aLoadContext);
  gNeckoChild->SendPRemoteOpenFileConstructor(this, loadContext, uri, appUri);

  
  AddIPDLReference();

  mListener = aListener;
  mAsyncOpenCalled = true;
  return NS_OK;
#endif
}

nsresult
RemoteOpenFileChild::SetNSPRFileDesc(PRFileDesc* aNSPRFileDesc)
{
  MOZ_ASSERT(!mNSPRFileDesc);
  if (mNSPRFileDesc) {
    return NS_ERROR_ALREADY_OPENED;
  }

  mNSPRFileDesc = aNSPRFileDesc;
  return NS_OK;
}

void
RemoteOpenFileChild::OnCachedFileDescriptor(const nsAString& aPath,
                                            const FileDescriptor& aFD)
{
#ifdef DEBUG
  if (!aPath.IsEmpty()) {
    MOZ_ASSERT(mFile);

    nsString path;
    MOZ_ASSERT(NS_SUCCEEDED(mFile->GetPath(path)));
    MOZ_ASSERT(path == aPath, "Paths don't match!");
  }
#endif

  HandleFileDescriptorAndNotifyListener(aFD,  false);
}

void
RemoteOpenFileChild::HandleFileDescriptorAndNotifyListener(
                                                      const FileDescriptor& aFD,
                                                      bool aFromRecvDelete)
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  MOZ_CRASH("OS X and Windows shouldn't be doing IPDL here");
#else
  if (!mListener) {
    
    
    
    if (aFD.IsValid()) {
      nsRefPtr<CloseFileRunnable> runnable = new CloseFileRunnable(aFD);
      runnable->Dispatch();
    }
    return;
  }

  MOZ_ASSERT(!mNSPRFileDesc);

  nsRefPtr<TabChild> tabChild;
  mTabChild.swap(tabChild);

  
  
  
  
  if (tabChild && aFromRecvDelete) {
    nsString path;
    if (NS_FAILED(mFile->GetPath(path))) {
      MOZ_CRASH("Couldn't get path from file!");
    }

    tabChild->CancelCachedFileDescriptorCallback(path, this);
  }

  if (aFD.IsValid()) {
    mNSPRFileDesc = PR_ImportFile(aFD.PlatformHandle());
    if (!mNSPRFileDesc) {
      NS_WARNING("Failed to import file handle!");
    }
  }

  NotifyListener(mNSPRFileDesc ? NS_OK : NS_ERROR_FILE_NOT_FOUND);
#endif
}

void
RemoteOpenFileChild::NotifyListener(nsresult aResult)
{
  MOZ_ASSERT(mListener);
  mListener->OnRemoteFileOpenComplete(aResult);
  mListener = nullptr;     

  nsRefPtr<nsJARProtocolHandler> handler(gJarHandler);
  NS_WARN_IF_FALSE(handler, "nsJARProtocolHandler is already gone!");

  if (handler) {
    handler->RemoteOpenFileComplete(this, aResult);
  }
}





bool
RemoteOpenFileChild::Recv__delete__(const FileDescriptor& aFD)
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  NS_NOTREACHED("OS X and Windows shouldn't be doing IPDL here");
#else
  HandleFileDescriptorAndNotifyListener(aFD,  true);
#endif

  return true;
}





NS_IMETHODIMP
RemoteOpenFileChild::Clone(nsIFile **file)
{
  *file = new RemoteOpenFileChild(*this);
  NS_ADDREF(*file);

  return NS_OK;
}



NS_IMETHODIMP
RemoteOpenFileChild::OpenNSPRFileDesc(int32_t aFlags, int32_t aMode,
                                      PRFileDesc **aRetval)
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  
  return mFile->OpenNSPRFileDesc(aFlags, aMode, aRetval);

#else
  if (aFlags != PR_RDONLY) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mNSPRFileDesc) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  PROsfd osfd = dup(PR_FileDesc2NativeHandle(mNSPRFileDesc));
  *aRetval = PR_ImportFile(osfd);

  return NS_OK;
#endif
}






nsresult
RemoteOpenFileChild::GetLeafName(nsAString &aLeafName)
{
  return mFile->GetLeafName(aLeafName);
}

NS_IMETHODIMP
RemoteOpenFileChild::GetNativeLeafName(nsACString &aLeafName)
{
  return mFile->GetNativeLeafName(aLeafName);
}

nsresult
RemoteOpenFileChild::GetTarget(nsAString &_retval)
{
  return mFile->GetTarget(_retval);
}

NS_IMETHODIMP
RemoteOpenFileChild::GetNativeTarget(nsACString &_retval)
{
  return mFile->GetNativeTarget(_retval);
}

nsresult
RemoteOpenFileChild::GetPath(nsAString &_retval)
{
  return mFile->GetPath(_retval);
}

NS_IMETHODIMP
RemoteOpenFileChild::GetNativePath(nsACString &_retval)
{
  return mFile->GetNativePath(_retval);
}

NS_IMETHODIMP
RemoteOpenFileChild::Equals(nsIFile *inFile, bool *_retval)
{
  return mFile->Equals(inFile, _retval);
}

NS_IMETHODIMP
RemoteOpenFileChild::Contains(nsIFile *inFile, bool *_retval)
{
  return mFile->Contains(inFile, _retval);
}

NS_IMETHODIMP
RemoteOpenFileChild::GetParent(nsIFile **aParent)
{
  return mFile->GetParent(aParent);
}

NS_IMETHODIMP
RemoteOpenFileChild::GetFollowLinks(bool *aFollowLinks)
{
  return mFile->GetFollowLinks(aFollowLinks);
}





nsresult
RemoteOpenFileChild::Append(const nsAString &node)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::AppendNative(const nsACString &fragment)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Normalize()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Create(uint32_t type, uint32_t permissions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
RemoteOpenFileChild::SetLeafName(const nsAString &aLeafName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetNativeLeafName(const nsACString &aLeafName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
RemoteOpenFileChild::InitWithPath(const nsAString &filePath)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::InitWithNativePath(const nsACString &filePath)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::InitWithFile(nsIFile *aFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetFollowLinks(bool aFollowLinks)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult  
RemoteOpenFileChild::AppendRelativePath(const nsAString &node)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::AppendRelativeNativePath(const nsACString &fragment)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetPersistentDescriptor(nsACString &aPersistentDescriptor)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetPersistentDescriptor(const nsACString &aPersistentDescriptor)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetRelativeDescriptor(nsIFile *fromFile, nsACString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetRelativeDescriptor(nsIFile *fromFile,
                                   const nsACString& relativeDesc)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
RemoteOpenFileChild::CopyTo(nsIFile *newParentDir, const nsAString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::CopyToNative(nsIFile *newParent, const nsACString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
RemoteOpenFileChild::CopyToFollowingLinks(nsIFile *newParentDir,
                                  const nsAString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::CopyToFollowingLinksNative(nsIFile *newParent,
                                        const nsACString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
RemoteOpenFileChild::MoveTo(nsIFile *newParentDir, const nsAString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::MoveToNative(nsIFile *newParent, const nsACString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::RenameTo(nsIFile *newParentDir, const nsAString &newName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Remove(bool recursive)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetPermissions(uint32_t *aPermissions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetPermissions(uint32_t aPermissions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetPermissionsOfLink(uint32_t *aPermissionsOfLink)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetPermissionsOfLink(uint32_t aPermissions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetLastModifiedTime(PRTime *aLastModTime)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetLastModifiedTime(PRTime aLastModTime)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetLastModifiedTimeOfLink(PRTime *aLastModTimeOfLink)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetLastModifiedTimeOfLink(PRTime aLastModTimeOfLink)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetFileSize(int64_t *aFileSize)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::SetFileSize(int64_t aFileSize)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetFileSizeOfLink(int64_t *aFileSize)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Exists(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsWritable(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsReadable(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsExecutable(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsHidden(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsDirectory(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsFile(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsSymlink(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::IsSpecial(bool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::CreateUnique(uint32_t type, uint32_t attributes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetDirectoryEntries(nsISimpleEnumerator **entries)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::OpenANSIFileDesc(const char *mode, FILE **_retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Load(PRLibrary **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetDiskSpaceAvailable(int64_t *aDiskSpaceAvailable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Reveal()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::Launch()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
RemoteOpenFileChild::Equals(nsIHashable* aOther, bool *aResult)
{
  nsCOMPtr<nsIHashable> hashable = do_QueryInterface(mFile);

  MOZ_ASSERT(hashable);

  if (hashable) {
    return hashable->Equals(aOther, aResult);
  }
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
RemoteOpenFileChild::GetHashCode(uint32_t *aResult)
{
  nsCOMPtr<nsIHashable> hashable = do_QueryInterface(mFile);

  MOZ_ASSERT(hashable);

  if (hashable) {
    return hashable->GetHashCode(aResult);
  }
  return NS_ERROR_UNEXPECTED;
}

} 
} 
