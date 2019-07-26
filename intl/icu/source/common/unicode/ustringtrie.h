













#ifndef __USTRINGTRIE_H__
#define __USTRINGTRIE_H__






#include "unicode/utypes.h"









enum UStringTrieResult {
    






    USTRINGTRIE_NO_MATCH,
    





    USTRINGTRIE_NO_VALUE,
    






    USTRINGTRIE_FINAL_VALUE,
    






    USTRINGTRIE_INTERMEDIATE_VALUE
};







#define USTRINGTRIE_MATCHES(result) ((result)!=USTRINGTRIE_NO_MATCH)










#define USTRINGTRIE_HAS_VALUE(result) ((result)>=USTRINGTRIE_FINAL_VALUE)








#define USTRINGTRIE_HAS_NEXT(result) ((result)&1)

#endif  
