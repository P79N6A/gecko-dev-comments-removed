







#ifndef BASE_DEBUG_ON_START_H_
#define BASE_DEBUG_ON_START_H_

#include "base/basictypes.h"


#if defined(OS_WIN)

#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY  __declspec(selectany)
#endif


class DebugOnStart {
 public:
  
  
  typedef int  (__cdecl *PIFV)(void);

  
  
  static int __cdecl Init();

  
  
  static bool FindArgument(wchar_t* command_line, const wchar_t* argument);
};






#ifdef _WIN64



#pragma const_seg(push, ".CRT$XIB")

extern const DebugOnStart::PIFV debug_on_start;
DECLSPEC_SELECTANY const DebugOnStart::PIFV debug_on_start =
    &DebugOnStart::Init;

#pragma const_seg(pop)

#else  



#pragma data_seg(push, ".CRT$XIB")

DECLSPEC_SELECTANY DebugOnStart::PIFV debug_on_start = &DebugOnStart::Init;

#pragma data_seg(pop)

#endif  
#endif  

#endif  
