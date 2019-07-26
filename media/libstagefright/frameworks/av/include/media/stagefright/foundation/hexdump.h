















#ifndef HEXDUMP_H_

#define HEXDUMP_H_

#include <sys/types.h>

namespace stagefright {

struct AString;

void hexdump(
        const void *_data, size_t size,
        size_t indent = 0, AString *appendTo = NULL);

}  

#endif  
