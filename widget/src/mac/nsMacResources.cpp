




































#include "nsMacResources.h"
#include <Carbon/Carbon.h>

short nsMacResources::mRefNum      = kResFileNotOpened;
short nsMacResources::mSaveResFile = 0;


nsresult nsMacResources::OpenLocalResourceFile()
{
  if (mRefNum == kResFileNotOpened) {
    CFBundleRef appBundle = ::CFBundleGetMainBundle();
    if (appBundle) {
      CFURLRef executable = ::CFBundleCopyExecutableURL(appBundle);
      if (executable) {
        CFURLRef binDir = ::CFURLCreateCopyDeletingLastPathComponent(
                           kCFAllocatorDefault, executable);
        if (binDir) {
          CFURLRef resourceFile = ::CFURLCreateCopyAppendingPathComponent(
                                   kCFAllocatorDefault, binDir,
                                   CFSTR("libwidget.rsrc"), PR_FALSE);
          if (resourceFile) {
            FSRef resourceRef;
            if (::CFURLGetFSRef(resourceFile, &resourceRef))
              ::FSOpenResourceFile(&resourceRef, 0, NULL, fsRdPerm, &mRefNum);
            ::CFRelease(resourceFile);
          }
          ::CFRelease(binDir);
        }
        ::CFRelease(executable);
      }
    }
  }
  if (mRefNum == kResFileNotOpened)
    return NS_ERROR_NOT_INITIALIZED;

  mSaveResFile = ::CurResFile();
  ::UseResFile(mRefNum);

  return (::ResError() == noErr ? NS_OK : NS_ERROR_FAILURE);
}


nsresult nsMacResources::CloseLocalResourceFile()
{
  if (mRefNum == kResFileNotOpened)
    return NS_ERROR_NOT_INITIALIZED;

  ::UseResFile(mSaveResFile);

  return (::ResError() == noErr ? NS_OK : NS_ERROR_FAILURE);
}

