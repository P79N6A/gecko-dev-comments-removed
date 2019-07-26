






#include "mozilla/net/RemoteOpenFileParent.h"
#include "mozilla/unused.h"
#include "nsEscape.h"

#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
#include <fcntl.h>
#endif

namespace mozilla {
namespace net {

RemoteOpenFileParent::RemoteOpenFileParent(nsIFileURL *aURI)
  : mURI(aURI)
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
  , mFd(-1)
#endif
{}

RemoteOpenFileParent::~RemoteOpenFileParent()
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
  if (mFd != -1) {
    
    
    close(mFd);
  }
#endif
}

bool
RemoteOpenFileParent::RecvAsyncOpenFile()
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  NS_NOTREACHED("osX and Windows shouldn't be doing IPDL here");
#else

  

  nsAutoCString path;
  nsresult rv = mURI->GetFilePath(path);
  NS_UnescapeURL(path);
  if (NS_SUCCEEDED(rv)) {
    int fd = open(path.get(), O_RDONLY);
    if (fd != -1) {
      unused << SendFileOpened(FileDescriptor(fd), NS_OK);
      
      
      mFd = fd;
      return true;
    }
  }

  
  
  unused << SendFileOpened(FileDescriptor(mFd), NS_ERROR_NOT_AVAILABLE);
#endif 

  return true;
}

} 
} 
