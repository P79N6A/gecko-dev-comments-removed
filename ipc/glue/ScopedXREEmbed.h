



#ifndef __IPC_GLUE_SCOPEDXREEMBED_H__
#define __IPC_GLUE_SCOPEDXREEMBED_H__

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsIFile.h"

namespace mozilla {
namespace ipc {

class ScopedXREEmbed
{
public:
  ScopedXREEmbed();
  ~ScopedXREEmbed();

  void Start();
  void Stop();
  void SetAppDir(const nsACString& aPath);

private:
  bool mShouldKillEmbedding;
  nsCOMPtr<nsIFile> mAppDir;
};

} 
} 

#endif 