





































#ifndef mozilla_plugins_StreamNotifyParent_h
#define mozilla_plugins_StreamNotifyParent_h

#include "mozilla/plugins/PStreamNotifyParent.h"

namespace mozilla {
namespace plugins {

class StreamNotifyParent : public PStreamNotifyParent
{
  friend class PluginInstanceParent;

  StreamNotifyParent() { }
};

} 
} 

#endif
