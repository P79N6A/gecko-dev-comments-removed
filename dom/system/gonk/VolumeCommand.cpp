



#include "nsString.h"
#include "nsWhitespaceTokenizer.h"

#include "Volume.h"
#include "VolumeCommand.h"
#include "VolumeManager.h"
#include "VolumeManagerLog.h"

namespace mozilla {
namespace system {



























VolumeActionCommand::VolumeActionCommand(Volume* aVolume,
                                         const char* aAction,
                                         const char* aExtraArgs,
                                         VolumeResponseCallback* aCallback)
  : VolumeCommand(aCallback),
    mVolume(aVolume)
{
  nsAutoCString cmd;

  cmd = "volume ";
  cmd += aAction;
  cmd += " ";
  cmd += aVolume->Name().get();

  
  if (aExtraArgs && (*aExtraArgs != '\0')) {
    cmd += " ";
    cmd += aExtraArgs;
  }
  SetCmd(cmd);
}















VolumeListCommand::VolumeListCommand(VolumeResponseCallback* aCallback)
  : VolumeCommand(NS_LITERAL_CSTRING("volume list"), aCallback)
{
}

} 
} 

