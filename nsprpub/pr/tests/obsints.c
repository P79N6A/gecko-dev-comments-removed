












































#include <stdio.h>

#ifdef NO_NSPR_10_SUPPORT


int main(int argc, char **argv)
{
    printf("PASS\n");
    return 0;
}

#else 

#include "prtypes.h"  

int main(int argc, char **argv)
{
    



    intn in;
    uintn uin;
    uint ui;
    int8 i8;
    uint8 ui8;
    int16 i16;
    uint16 ui16;
    int32 i32;
    uint32 ui32;
    int64 i64;
    uint64 ui64;

    printf("PASS\n");
    return 0;
}

#endif 
