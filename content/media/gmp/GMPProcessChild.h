




#ifndef GMPProcessChild_h_
#define GMPProcessChild_h_

#include "mozilla/ipc/ProcessChild.h"
#include "GMPChild.h"

namespace mozilla {
namespace gmp {

class GMPProcessChild MOZ_FINAL : public mozilla::ipc::ProcessChild {
protected:
  typedef mozilla::ipc::ProcessChild ProcessChild;

public:
  explicit GMPProcessChild(ProcessHandle parentHandle);
  ~GMPProcessChild();

  virtual bool Init() MOZ_OVERRIDE;
  virtual void CleanUp() MOZ_OVERRIDE;

private:
  GMPChild mPlugin;

  DISALLOW_COPY_AND_ASSIGN(GMPProcessChild);
};

} 
} 

#endif 
