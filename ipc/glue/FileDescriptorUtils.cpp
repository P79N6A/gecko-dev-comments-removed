



#include "FileDescriptorUtils.h"

#include "nsIEventTarget.h"

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "prio.h"
#include "private/pprio.h"

using mozilla::ipc::CloseFileRunnable;

#ifdef DEBUG

CloseFileRunnable::CloseFileRunnable(const FileDescriptor& aFileDescriptor)
: mFileDescriptor(aFileDescriptor)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aFileDescriptor.IsValid());
}

#endif 

CloseFileRunnable::~CloseFileRunnable()
{
  if (mFileDescriptor.IsValid()) {
    
    
    CloseFile();
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(CloseFileRunnable, nsIRunnable)

void
CloseFileRunnable::Dispatch()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIEventTarget> eventTarget =
    do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
  NS_ENSURE_TRUE_VOID(eventTarget);

  nsresult rv = eventTarget->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS_VOID(rv);
}

void
CloseFileRunnable::CloseFile()
{
  
  

  MOZ_ASSERT(mFileDescriptor.IsValid());

  PRFileDesc* fd =
    PR_ImportFile(PROsfd(mFileDescriptor.PlatformHandle()));
  NS_WARN_IF_FALSE(fd, "Failed to import file handle!");

  mFileDescriptor = FileDescriptor();

  if (fd) {
    PR_Close(fd);
    fd = nullptr;
  }
}

NS_IMETHODIMP
CloseFileRunnable::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  CloseFile();
  return NS_OK;
}
