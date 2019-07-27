














#ifndef BOOTANIMATION_H
#define BOOTANIMATION_H

namespace mozilla {

MOZ_EXPORT __attribute__ ((weak))
void StartBootAnimation();


MOZ_EXPORT __attribute__ ((weak))
void StopBootAnimation();

} 

#endif 
