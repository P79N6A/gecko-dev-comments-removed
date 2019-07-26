






#include "mozilla/net/RemoteOpenFileParent.h"
#include "mozilla/unused.h"
#include "nsEscape.h"

#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace mozilla {
namespace net {

bool
RemoteOpenFileParent::OpenSendCloseDelete()
{
#if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
  MOZ_NOT_REACHED("OS X and Windows shouldn't be doing IPDL here");
#else

  

  FileDescriptor fileDescriptor;

  nsAutoCString path;
  nsresult rv = mURI->GetFilePath(path);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "GetFilePath failed!");

  NS_UnescapeURL(path);

  if (NS_SUCCEEDED(rv)) {
    int fd = open(path.get(), O_RDONLY);
    if (fd == -1) {
      printf_stderr("RemoteOpenFileParent: file '%s' was not found!\n",
                    path.get());
    } else {
      fileDescriptor = FileDescriptor(fd);
    }
  }

  
  unused << Send__delete__(this, fileDescriptor);

  if (fileDescriptor.IsValid()) {
    
    
    close(fileDescriptor.PlatformHandle());
  }

#endif 

  return true;
}

} 
} 
