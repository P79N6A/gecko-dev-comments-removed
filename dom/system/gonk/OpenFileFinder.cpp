



#include "OpenFileFinder.h"

#include "mozilla/FileUtils.h"
#include "nsPrintfCString.h"

#include <sys/stat.h>
#include <errno.h>

namespace mozilla {
namespace system {

OpenFileFinder::OpenFileFinder(const nsACString& aPath)
  : mPath(aPath),
    mProcDir(nullptr),
    mFdDir(nullptr),
    mPid(0)
{
}

OpenFileFinder::~OpenFileFinder()
{
  Close();
}

bool
OpenFileFinder::First(OpenFileFinder::Info* aInfo)
{
  Close();

  mProcDir = opendir("/proc");
  if (!mProcDir) {
    return false;
  }
  mState = NEXT_PID;
  return Next(aInfo);
}

bool
OpenFileFinder::Next(OpenFileFinder::Info* aInfo)
{
  
  
  
  
  while (mState != DONE) {
    switch (mState) {
      case NEXT_PID: {
        struct dirent *pidEntry;
        pidEntry = readdir(mProcDir);
        if (!pidEntry) {
          mState = DONE;
          break;
        }
        char *endPtr;
        mPid = strtol(pidEntry->d_name, &endPtr, 10);
        if (mPid == 0 || *endPtr != '\0') {
          
          continue;
        }
        
        if (mFdDir) {
          closedir(mFdDir);
        }
        nsPrintfCString fdDirPath("/proc/%d/fd", mPid);
        mFdDir = opendir(fdDirPath.get());
        if (!mFdDir) {
          continue;
        }
        mState = CHECK_FDS;
      }
      
      case CHECK_FDS: {
        struct dirent *fdEntry;
        while((fdEntry = readdir(mFdDir))) {
          if (!strcmp(fdEntry->d_name, ".") ||
              !strcmp(fdEntry->d_name, "..")) {
            continue;
          }
          nsPrintfCString fdSymLink("/proc/%d/fd/%s", mPid, fdEntry->d_name);
          nsCString resolvedPath;
          if (ReadSymLink(fdSymLink, resolvedPath) && PathMatches(resolvedPath)) {
            
            
            FillInfo(aInfo, resolvedPath);
            return true;
          }
        }
        
        mState = NEXT_PID;
        continue;
      }
      case DONE:
      default:
        mState = DONE;  
        break;
    }
  }
  return false;
}

void
OpenFileFinder::Close()
{
  if (mFdDir) {
    closedir(mFdDir);
  }
  if (mProcDir) {
    closedir(mProcDir);
  }
}

void
OpenFileFinder::FillInfo(OpenFileFinder::Info* aInfo, const nsACString& aPath)
{
  aInfo->mFileName = aPath;
  aInfo->mPid = mPid;
  nsPrintfCString exePath("/proc/%d/exe", mPid);
  ReadSymLink(exePath, aInfo->mExe);
  aInfo->mComm.Truncate();
  aInfo->mAppName.Truncate();
  nsPrintfCString statPath("/proc/%d/stat", mPid);
  nsCString statString;
  statString.SetLength(200);
  char *stat = statString.BeginWriting();
  if (!stat) {
    return;
  }
  ReadSysFile(statPath.get(), stat, statString.Length());
  
  
  
  
  char *closeParen = strrchr(stat, ')');
  if (!closeParen) {
    return;
  }
  char *openParen = strchr(stat, '(');
  if (!openParen) {
    return;
  }
  if (openParen >= closeParen) {
    return;
  }
  nsDependentCSubstring comm(&openParen[1], closeParen - openParen - 1);
  aInfo->mComm = comm;
  
  
  
  
  int ppid = atoi(&closeParen[4]);
  
  if (ppid != getpid()) {
    return;
  }
  
  
  aInfo->mAppName = aInfo->mComm;
}

bool
OpenFileFinder::ReadSymLink(const nsACString& aSymLink, nsACString& aOutPath)
{
  aOutPath.Truncate();
  const char *symLink = aSymLink.BeginReading();

  
  struct stat st;
  if (lstat(symLink, &st)) {
      return false;
  }
  if ((st.st_mode & S_IFMT) != S_IFLNK) {
      return false;
  }

  
  
  

  char resolvedSymLink[PATH_MAX];
  ssize_t pathLength = readlink(symLink, resolvedSymLink,
                                sizeof(resolvedSymLink) - 1);
  if (pathLength <= 0) {
      return false;
  }
  resolvedSymLink[pathLength] = '\0';
  aOutPath.Assign(resolvedSymLink);
  return true;
}

} 
} 
