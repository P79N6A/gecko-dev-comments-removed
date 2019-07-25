




































#ifndef mozilla_jetpack_JetpackProcessChild_h
#define mozilla_jetpack_JetpackProcessChild_h

#include "mozilla/ipc/ProcessChild.h"

#include "mozilla/jetpack/JetpackChild.h"

namespace mozilla {
namespace jetpack {




class JetpackProcessChild : public mozilla::ipc::ProcessChild
{
  typedef mozilla::ipc::ProcessChild ProcessChild;

public:
  JetpackProcessChild(ProcessHandle aParentHandle)
    : ProcessChild(aParentHandle)
  { }

  virtual ~JetpackProcessChild()
  { }

  NS_OVERRIDE virtual bool Init();
  NS_OVERRIDE virtual void CleanUp();

  static JetpackProcessChild* current() {
    return static_cast<JetpackProcessChild*>(ProcessChild::current());
  }

private:
  JetpackChild mJetpack;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackProcessChild);
};

}
}

#endif 
