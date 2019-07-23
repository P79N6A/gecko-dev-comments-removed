



#ifndef BASE_KEYBOARD_CODES_H_
#define BASE_KEYBOARD_CODES_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/keyboard_codes_win.h"
#elif defined(OS_POSIX)
#include "base/keyboard_codes_posix.h"
#endif

#endif  
