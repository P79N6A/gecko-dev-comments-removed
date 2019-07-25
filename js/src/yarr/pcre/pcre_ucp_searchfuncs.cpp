










































#include "pcre_internal.h"

#include "ucpinternal.h"       
#include "ucptable.cpp"        














int jsc_pcre_ucp_othercase(unsigned c)
{
    int bot = 0;
    int top = sizeof(ucp_table) / sizeof(cnode);
    int mid;
    
    



    
    for (;;) {
        if (top <= bot)
            return -1;
        mid = (bot + top) >> 1;
        if (c == (ucp_table[mid].f0 & f0_charmask))
            break;
        if (c < (ucp_table[mid].f0 & f0_charmask))
            top = mid;
        else {
            if ((ucp_table[mid].f0 & f0_rangeflag) && (c <= (ucp_table[mid].f0 & f0_charmask) + (ucp_table[mid].f1 & f1_rangemask)))
                break;
            bot = mid + 1;
        }
    }
    
    

    
    if (ucp_table[mid].f0 & f0_rangeflag)
        return -1;
    
    int offset = ucp_table[mid].f1 & f1_casemask;
    if (offset & f1_caseneg)
        offset |= f1_caseneg;
    return !offset ? -1 : c + offset;
}
