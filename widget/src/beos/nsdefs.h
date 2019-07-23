




































#ifndef NSDEFS_H
#define NSDEFS_H

#ifdef _DEBUG
  #define BREAK_TO_DEBUGGER           DebugBreak()
#else   
  #define BREAK_TO_DEBUGGER
#endif  

#ifdef _DEBUG
  #define VERIFY(exp)                 ((exp) ? 0: (GetLastError(), BREAK_TO_DEBUGGER))
#else   
  #define VERIFY(exp)                 (exp)
#endif  

#endif  


