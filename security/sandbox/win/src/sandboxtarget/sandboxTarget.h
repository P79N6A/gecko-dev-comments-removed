





#ifndef __SECURITY_SANDBOX_SANDBOXTARGET_H__
#define __SECURITY_SANDBOX_SANDBOXTARGET_H__

#ifdef TARGET_SANDBOX_EXPORTS
#define TARGET_SANDBOX_EXPORT __declspec(dllexport)
#else
#define TARGET_SANDBOX_EXPORT __declspec(dllimport)
#endif
namespace mozilla {


class TARGET_SANDBOX_EXPORT SandboxTarget
{
public:
  typedef void (*StartSandboxPtr)();

  


  static SandboxTarget* Instance()
  {
    static SandboxTarget sb;
    return &sb;
  }

  




  void SetStartSandboxCallback(StartSandboxPtr aStartSandboxCallback)
  {
    mStartSandboxCallback = aStartSandboxCallback;
  }

  



  void StartSandbox()
  {
    if (mStartSandboxCallback) {
      mStartSandboxCallback();
    }
  }

protected:
  SandboxTarget() :
    mStartSandboxCallback(nullptr)
  {
  }

  StartSandboxPtr mStartSandboxCallback;
};


} 

#endif
