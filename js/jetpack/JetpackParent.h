




































#ifndef mozilla_jetpack_JetpackParent_h
#define mozilla_jetpack_JetpackParent_h

#include "mozilla/jetpack/PJetpackParent.h"
#include "mozilla/jetpack/JetpackProcessParent.h"
#include "nsIJetpack.h"

namespace mozilla {
namespace jetpack {

class JetpackParent
  : public PJetpackParent
  , public nsIJetpack
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJETPACK

  JetpackParent();
  ~JetpackParent();

private:
  JetpackProcessParent* mSubprocess;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackParent);
};

} 
} 

#endif 
