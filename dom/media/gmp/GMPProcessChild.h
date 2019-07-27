




#ifndef GMPProcessChild_h_
#define GMPProcessChild_h_

#include "mozilla/ipc/ProcessChild.h"
#include "GMPChild.h"

namespace mozilla {
namespace gmp {

class GMPLoader;

class GMPProcessChild MOZ_FINAL : public mozilla::ipc::ProcessChild {
protected:
  typedef mozilla::ipc::ProcessChild ProcessChild;

public:
  explicit GMPProcessChild(ProcessHandle parentHandle);
  ~GMPProcessChild();

  virtual bool Init() MOZ_OVERRIDE;
  virtual void CleanUp() MOZ_OVERRIDE;

  
  
  
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
