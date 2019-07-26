



#ifndef mozilla_system_openfilefinder_h__
#define mozilla_system_openfilefinder_h__

#include "nsString.h"

#include <dirent.h>

#define USE_DEBUG 0

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "OpenFileFinder", ## args)
#define LOGW(args...) __android_log_print(ANDROID_LOG_WARN,  "OpenFileFinder", ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "OpenFileFinder", ## args)

#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "OpenFileFinder" , ## args)
#else
#define DBG(args...)
#endif

namespace mozilla {
namespace system {

class OpenFileFinder
{
public:
  enum State
  {
    NEXT_PID,
    CHECK_FDS,
    DONE
  };
  class Info
  {
  public:
    nsCString mFileName;  
    nsCString mAppName;   
    pid_t     mPid;       
    nsCString mComm;      
    nsCString mExe;       
    bool      mIsB2gOrDescendant; 
  };

  OpenFileFinder(const nsACString& aPath, bool aCheckIsB2gOrDescendant = true);
  ~OpenFileFinder();

  bool First(Info* aInfo);  
  bool Next(Info* aInfo);   
  void Close();

private:

  void FillInfo(Info *aInfo, const nsACString& aPath);
  bool ReadSymLink(const nsACString& aSymLink, nsACString& aOutPath);
  bool PathMatches(const nsACString& aPath)
  {
    return Substring(aPath, 0, mPath.Length()).Equals(mPath);
  }

  State     mState;   
  nsCString mPath;    
  DIR*      mProcDir; 
  DIR*      mFdDir;   
  int       mPid;     
  pid_t     mMyPid;   
  bool      mCheckIsB2gOrDescendant; 
};

} 
} 

#endif
