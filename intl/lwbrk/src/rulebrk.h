









#ifndef __RULEBRK_H__
#define __RULEBRK_H__
#include "th_char.h"

#ifdef __cplusplus
extern "C" {
#endif

int TrbWordBreakPos(const th_char *pstr, int left, 
                    const th_char *rstr, int right);
int TrbFollowing(const th_char *begin, int length, int offset);

#ifdef __cplusplus
}
#endif
#endif
