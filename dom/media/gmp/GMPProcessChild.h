




#ifndef GMPProcessChild_h_
#define GMPProcessChild_h_

#include "mozilla/ipc/ProcessChild.h"
#include "GMPChild.h"

namespace mozilla {
namespace gmp {

class GMPLoader;

class GMPProcessChild final : public mozilla::ipc::ProcessChild {
protected:
  typedef mozilla::ipc::ProcessChild ProcessChild;

public:
  explicit GMPProcessChild(ProcessId aParentPid);
  ~GMPProcessChild();

  virtual bool Init() override;
  virtual void CleanUp() override;

  
  
  
  static void SetGMPLoader(GMPLoader* aHost);
  static GMPLoader* GetGMPLoader();

private:
  GMPChild mPlugin;
  static GMPLoader* mLoader;
  DISALLOW_COPY_AND_ASSIGN(GMPProcessChild);
};

} 
} 

#endif 
