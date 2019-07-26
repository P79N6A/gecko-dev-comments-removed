



#include "FileDescriptorUtils.h"

#include "nsIEventTarget.h"

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "prio.h"
#include "private/pprio.h"

#include <errno.h>
#ifdef XP_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

using mozilla::ipc::CloseFileRunnable;

#ifdef DEBUG

CloseFileRunnable::CloseFileRunnable(const FileDescriptor& aFileDescriptor)
: mFileDescriptor(aFileDescriptor)
{
  MOZ_ASSERT(aFileDescriptor.IsValid());
}

#endif 

CloseFileRunnable::~CloseFileRunnable()
{
  if (mFileDescriptor.IsValid()) {
    
    
    CloseFile();
  }
}

NS_IMPL_ISUPPORTS(CloseFileRunnable, nsIRunnable)

void
CloseFileRunnable::Dispatch()
{
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

namespace mozilla {
namespace ipc {

FILE*
FileDescriptorToFILE(const FileDescriptor& aDesc,
                     const char* aOpenMode)
{
  if (!aDesc.IsValid()) {
    errno = EBADF;
    return nullptr;
  }
  FileDescriptor::PlatformHandleType handle = aDesc.PlatformHandle();
#ifdef XP_WIN
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
  if (fd == -1) {
    CloseHandle(handle);
    return nullptr;
  }
#else
  int fd = handle;
#endif
  FILE* file = fdopen(fd, aOpenMode);
  if (!file) {
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
  }
  return file;
}

FileDescriptor
FILEToFileDescriptor(FILE* aStream)
{
  if (!aStream) {
    errno = EBADF;
    return FileDescriptor();
  }
#ifdef XP_WIN
  int fd = _fileno(aStream);
  if (fd == -1) {
    return FileDescriptor();
  }
  return FileDescriptor(reinterpret_cast<HANDLE>(_get_osfhandle(fd)));
#else
  return FileDescriptor(fileno(aStream));
#endif
}

} 
} 
