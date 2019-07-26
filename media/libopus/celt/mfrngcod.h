



































#if !defined(_mfrngcode_H)
# define _mfrngcode_H (1)
# include "entcode.h"




# define EC_SYM_BITS   (8)

# define EC_CODE_BITS  (32)

# define EC_SYM_MAX    ((1U<<EC_SYM_BITS)-1)

# define EC_CODE_SHIFT (EC_CODE_BITS-EC_SYM_BITS-1)

# define EC_CODE_TOP   (((opus_uint32)1U)<<(EC_CODE_BITS-1))

# define EC_CODE_BOT   (EC_CODE_TOP>>EC_SYM_BITS)

# define EC_CODE_EXTRA ((EC_CODE_BITS-2)%EC_SYM_BITS+1)
#endif
