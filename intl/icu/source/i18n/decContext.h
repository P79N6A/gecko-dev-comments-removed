




























#include "unicode/utypes.h"
#include "putilimp.h"




















#if !defined(DECCONTEXT)
  #define DECCONTEXT
  #define DECCNAME     "decContext"                     /* Short name */
  #define DECCFULLNAME "Decimal Context Descriptor"   /* Verbose name */
  #define DECCAUTHOR   "Mike Cowlishaw"               /* Who to blame */

  #if !defined(int32_t)
         
  #endif
  #include <stdio.h>               
  #include <signal.h>              

  
  #if !defined(DECEXTFLAG)
  #define DECEXTFLAG 1             /* 1=enable extended flags         */
  #endif

  
  #if !defined(DECSUBSET)
  #define DECSUBSET  0             /* 1=enable subset arithmetic      */
  #endif

  
  enum rounding {
    DEC_ROUND_CEILING,             
    DEC_ROUND_UP,                  
    DEC_ROUND_HALF_UP,             
    DEC_ROUND_HALF_EVEN,           
    DEC_ROUND_HALF_DOWN,           
    DEC_ROUND_DOWN,                
    DEC_ROUND_FLOOR,               
    DEC_ROUND_05UP,                
    DEC_ROUND_MAX                  
    };
  #define DEC_ROUND_DEFAULT DEC_ROUND_HALF_EVEN;

  typedef struct {
    int32_t  digits;               
    int32_t  emax;                 
    int32_t  emin;                 
    enum     rounding round;       
    uint32_t traps;                
    uint32_t status;               
    uint8_t  clamp;                
    #if DECSUBSET
    uint8_t  extended;             
    #endif
    } decContext;

  
  #define DEC_MAX_DIGITS 999999999
  #define DEC_MIN_DIGITS         1
  #define DEC_MAX_EMAX   999999999
  #define DEC_MIN_EMAX           0
  #define DEC_MAX_EMIN           0
  #define DEC_MIN_EMIN  -999999999
  #define DEC_MAX_MATH      999999 /* max emax, etc., for math funcs. */

  
  
  
  enum decClass {
    DEC_CLASS_SNAN,
    DEC_CLASS_QNAN,
    DEC_CLASS_NEG_INF,
    DEC_CLASS_NEG_NORMAL,
    DEC_CLASS_NEG_SUBNORMAL,
    DEC_CLASS_NEG_ZERO,
    DEC_CLASS_POS_ZERO,
    DEC_CLASS_POS_SUBNORMAL,
    DEC_CLASS_POS_NORMAL,
    DEC_CLASS_POS_INF
    };
  
  #define DEC_ClassString_SN  "sNaN"
  #define DEC_ClassString_QN  "NaN"
  #define DEC_ClassString_NI  "-Infinity"
  #define DEC_ClassString_NN  "-Normal"
  #define DEC_ClassString_NS  "-Subnormal"
  #define DEC_ClassString_NZ  "-Zero"
  #define DEC_ClassString_PZ  "+Zero"
  #define DEC_ClassString_PS  "+Subnormal"
  #define DEC_ClassString_PN  "+Normal"
  #define DEC_ClassString_PI  "+Infinity"
  #define DEC_ClassString_UN  "Invalid"

  
  
  #if DECEXTFLAG
    
    #define DEC_Conversion_syntax    0x00000001
    #define DEC_Division_by_zero     0x00000002
    #define DEC_Division_impossible  0x00000004
    #define DEC_Division_undefined   0x00000008
    #define DEC_Insufficient_storage 0x00000010 /* [when malloc fails]  */
    #define DEC_Inexact              0x00000020
    #define DEC_Invalid_context      0x00000040
    #define DEC_Invalid_operation    0x00000080
    #if DECSUBSET
    #define DEC_Lost_digits          0x00000100
    #endif
    #define DEC_Overflow             0x00000200
    #define DEC_Clamped              0x00000400
    #define DEC_Rounded              0x00000800
    #define DEC_Subnormal            0x00001000
    #define DEC_Underflow            0x00002000
  #else
    
    #define DEC_Conversion_syntax    0x00000010
    #define DEC_Division_by_zero     0x00000002
    #define DEC_Division_impossible  0x00000010
    #define DEC_Division_undefined   0x00000010
    #define DEC_Insufficient_storage 0x00000010 /* [when malloc fails]  */
    #define DEC_Inexact              0x00000001
    #define DEC_Invalid_context      0x00000010
    #define DEC_Invalid_operation    0x00000010
    #if DECSUBSET
    #define DEC_Lost_digits          0x00000000
    #endif
    #define DEC_Overflow             0x00000008
    #define DEC_Clamped              0x00000000
    #define DEC_Rounded              0x00000000
    #define DEC_Subnormal            0x00000000
    #define DEC_Underflow            0x00000004
  #endif

  
  
  
  #define DEC_IEEE_754_Division_by_zero  (DEC_Division_by_zero)
  #if DECSUBSET
  #define DEC_IEEE_754_Inexact           (DEC_Inexact | DEC_Lost_digits)
  #else
  #define DEC_IEEE_754_Inexact           (DEC_Inexact)
  #endif
  #define DEC_IEEE_754_Invalid_operation (DEC_Conversion_syntax |     \
                                          DEC_Division_impossible |   \
                                          DEC_Division_undefined |    \
                                          DEC_Insufficient_storage |  \
                                          DEC_Invalid_context |       \
                                          DEC_Invalid_operation)
  #define DEC_IEEE_754_Overflow          (DEC_Overflow)
  #define DEC_IEEE_754_Underflow         (DEC_Underflow)

  
  #define DEC_Errors (DEC_IEEE_754_Division_by_zero |                 \
                      DEC_IEEE_754_Invalid_operation |                \
                      DEC_IEEE_754_Overflow | DEC_IEEE_754_Underflow)
  
  #define DEC_NaNs    DEC_IEEE_754_Invalid_operation

  
  #if DECSUBSET
  #define DEC_Information (DEC_Clamped | DEC_Rounded | DEC_Inexact    \
                          | DEC_Lost_digits)
  #else
  #define DEC_Information (DEC_Clamped | DEC_Rounded | DEC_Inexact)
  #endif

  
  #define DEC_IEEE_854_Division_by_zero  DEC_IEEE_754_Division_by_zero
  #define DEC_IEEE_854_Inexact           DEC_IEEE_754_Inexact
  #define DEC_IEEE_854_Invalid_operation DEC_IEEE_754_Invalid_operation
  #define DEC_IEEE_854_Overflow          DEC_IEEE_754_Overflow
  #define DEC_IEEE_854_Underflow         DEC_IEEE_754_Underflow

  
  #define DEC_Condition_CS "Conversion syntax"
  #define DEC_Condition_DZ "Division by zero"
  #define DEC_Condition_DI "Division impossible"
  #define DEC_Condition_DU "Division undefined"
  #define DEC_Condition_IE "Inexact"
  #define DEC_Condition_IS "Insufficient storage"
  #define DEC_Condition_IC "Invalid context"
  #define DEC_Condition_IO "Invalid operation"
  #if DECSUBSET
  #define DEC_Condition_LD "Lost digits"
  #endif
  #define DEC_Condition_OV "Overflow"
  #define DEC_Condition_PA "Clamped"
  #define DEC_Condition_RO "Rounded"
  #define DEC_Condition_SU "Subnormal"
  #define DEC_Condition_UN "Underflow"
  #define DEC_Condition_ZE "No status"
  #define DEC_Condition_MU "Multiple status"
  #define DEC_Condition_Length 21  /* length of the longest string,   */
                                   

  
  #define DEC_INIT_BASE         0
  #define DEC_INIT_DECIMAL32   32
  #define DEC_INIT_DECIMAL64   64
  #define DEC_INIT_DECIMAL128 128
  
  #define DEC_INIT_DECSINGLE  DEC_INIT_DECIMAL32
  #define DEC_INIT_DECDOUBLE  DEC_INIT_DECIMAL64
  #define DEC_INIT_DECQUAD    DEC_INIT_DECIMAL128

  
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextClearStatus(decContext *, uint32_t);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextDefault(decContext *, int32_t);
  U_INTERNAL enum rounding U_EXPORT2 uprv_decContextGetRounding(decContext *);
  U_INTERNAL uint32_t      U_EXPORT2 uprv_decContextGetStatus(decContext *);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextRestoreStatus(decContext *, uint32_t, uint32_t);
  U_INTERNAL uint32_t      U_EXPORT2 uprv_decContextSaveStatus(decContext *, uint32_t);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextSetRounding(decContext *, enum rounding);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextSetStatus(decContext *, uint32_t);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextSetStatusFromString(decContext *, const char *);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextSetStatusFromStringQuiet(decContext *, const char *);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextSetStatusQuiet(decContext *, uint32_t);
  U_INTERNAL const char  * U_EXPORT2 uprv_decContextStatusToString(const decContext *);
  U_INTERNAL int32_t       U_EXPORT2 uprv_decContextTestEndian(uint8_t);
  U_INTERNAL uint32_t      U_EXPORT2 uprv_decContextTestSavedStatus(uint32_t, uint32_t);
  U_INTERNAL uint32_t      U_EXPORT2 uprv_decContextTestStatus(decContext *, uint32_t);
  U_INTERNAL decContext  * U_EXPORT2 uprv_decContextZeroStatus(decContext *);

#endif
