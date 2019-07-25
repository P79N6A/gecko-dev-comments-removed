















#ifndef ANDROID_OMX_H_
#define ANDROID_OMX_H_

#include <media/IOMX.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>

#if !defined(STAGEFRIGHT_EXPORT)
#define STAGEFRIGHT_EXPORT
#endif

namespace android {

struct OMXMaster;
class OMXNodeInstance;
class STAGEFRIGHT_EXPORT OMX {
public:
  OMX();
  virtual ~OMX();

private:
  char reserved[96];
};
}  

#endif  
