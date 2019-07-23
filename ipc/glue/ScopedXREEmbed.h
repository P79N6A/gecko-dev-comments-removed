



































#ifndef __IPC_GLUE_SCOPEDXREEMBED_H__
#define __IPC_GLUE_SCOPEDXREEMBED_H__

namespace mozilla {
namespace ipc {

class ScopedXREEmbed
{
public:
  ScopedXREEmbed();
  ~ScopedXREEmbed();

  void Start();
  void Stop();

private:
  bool mShouldKillEmbedding;
};

} 
} 

#endif 