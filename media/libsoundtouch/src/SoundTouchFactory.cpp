





#include <soundtouch/SoundTouch.h>

namespace soundtouch
{

EXPORT
soundtouch::SoundTouch*
createSoundTouchObj()
{
  return new soundtouch::SoundTouch();
}

EXPORT
void
destroySoundTouchObj(soundtouch::SoundTouch* aObj)
{
  
  
  
  if (aObj) {
    delete aObj;
  }
}

}
