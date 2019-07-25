




































#ifndef mozilla_jetpack_JetpackChild_h
#define mozilla_jetpack_JetpackChild_h

#include "mozilla/jetpack/PJetpackChild.h"

namespace mozilla {
namespace jetpack {

class JetpackChild : public PJetpackChild
{
public:
  JetpackChild();
  ~JetpackChild();

  static JetpackChild* current();

  bool Init(base::ProcessHandle aParentProcessHandle,
            MessageLoop* aIOLoop,
            IPC::Channel* aChannel);

  void CleanUp();

protected:
  NS_OVERRIDE virtual bool RecvLoadImplementation(const nsCString& script);

private:
  static JetpackChild* gInstance;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackChild);
};

} 
} 


#endif 
