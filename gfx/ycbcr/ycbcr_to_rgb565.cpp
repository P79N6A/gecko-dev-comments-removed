





































#include "ycbcr_to_rgb565.h"

#if !defined (MOZILLA_MAY_SUPPORT_NEON)

int have_ycbcr_to_rgb565 ()
{
    return 0;
}

#else

int have_ycbcr_to_rgb565 ()
{
    return mozilla::supports_neon();
}

#endif 
