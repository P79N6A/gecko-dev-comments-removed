#if defined(__WATCOMC__) || defined(__WATCOM_CPLUSPLUS__)
#ifndef __WATCOM_FIX_H__
#define __WATCOM_FIX_H__ 1








#if defined(XP_WIN16) || defined(WIN16) 
#pragma aux default "_*" \
	parm caller [] \
	value struct float struct routine [ax] \
	modify [ax bx cx dx es]
#else
#pragma aux default "_*" \
	parm caller [] \
	value struct float struct routine [eax] \
	modify [eax ecx edx]
#endif
#pragma aux default far

#endif 
#endif 
