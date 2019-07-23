



































#ifndef SYSMACROS_MD_H_
#define SYSMACROS_MD_H_

#if defined (XP_WIN) && !defined (_WIN32)
#ifndef HUGEP
#define HUGEP __huge
#endif
#else
#ifndef HUGEP
#define HUGEP
#endif
#endif 


#endif 
