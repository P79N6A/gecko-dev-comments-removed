



#ifndef mozilla_dom_GamepadFunctions_h_
#define mozilla_dom_GamepadFunctions_h_

#include "mozilla/dom/GamepadBinding.h"

namespace mozilla {
namespace dom {
namespace GamepadFunctions {





uint32_t AddGamepad(const char* aID, GamepadMappingType aMapping,
                    uint32_t aNumButtons, uint32_t aNumAxes);

void RemoveGamepad(uint32_t aIndex);





void NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed,
                    double aValue);

void NewButtonEvent(uint32_t aIndex, uint32_t aButton, bool aPressed);




void NewAxisMoveEvent(uint32_t aIndex, uint32_t aAxis, double aValue);



void ResetGamepadIndexes();

} 
} 
} 

#endif
