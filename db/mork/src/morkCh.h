



































 
#ifndef _MORKCH_
#define _MORKCH_ 1

#ifndef _MORK_
#include "mork.h"
#endif







#define morkCh_kW      (1 << 0)
#define morkCh_kD      (1 << 1)
#define morkCh_kV      (1 << 2)
#define morkCh_kU      (1 << 3)
#define morkCh_kL      (1 << 4)
#define morkCh_kX      (1 << 5)
#define morkCh_kN      (1 << 6)
#define morkCh_kM      (1 << 7)

extern const mork_flags morkCh_Type[]; 



#define morkCh_IsDigit(c)    ( ((mork_ch) c) >= '0' && ((mork_ch) c) <= '9' )


#define morkCh_IsOctal(c)    ( ((mork_ch) c) >= '0' && ((mork_ch) c) <= '7' )


#define morkCh_IsHex(c) ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kX )


#define morkCh_IsValue(c)    ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kV )


#define morkCh_IsWhite(c)    ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kW )


#define morkCh_IsName(c)     ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kN )


#define morkCh_IsMore(c) ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kM )


#define morkCh_IsAlpha(c)    \
  ( morkCh_Type[ (mork_ch)(c) ] & (morkCh_kL|morkCh_kU) )


#define morkCh_IsAlphaNum(c) \
  (morkCh_Type[ (mork_ch)(c) ]&(morkCh_kL|morkCh_kU|morkCh_kD))



#define morkCh_GetFlags(c) ( morkCh_Type[ (mork_ch)(c) ] )

#define morkFlags_IsDigit(f)    ( (f) & morkCh_kD )
#define morkFlags_IsHex(f)      ( (f) & morkCh_kX )
#define morkFlags_IsValue(f)    ( (f) & morkCh_kV )
#define morkFlags_IsWhite(f)    ( (f) & morkCh_kW )
#define morkFlags_IsName(f)     ( (f) & morkCh_kN )
#define morkFlags_IsMore(f)     ( (f) & morkCh_kM )
#define morkFlags_IsAlpha(f)    ( (f) & (morkCh_kL|morkCh_kU) )
#define morkFlags_IsAlphaNum(f) ( (f) & (morkCh_kL|morkCh_kU|morkCh_kD) )

#define morkFlags_IsUpper(f)    ( (f) & morkCh_kU )
#define morkFlags_IsLower(f)    ( (f) & morkCh_kL )



  
#define morkCh_IsAscii(c)         ( ((mork_u1) c) <= 0x7F )
#define morkCh_IsSevenBitChar(c)  ( ((mork_u1) c) <= 0x7F )



#define morkCh_ToLower(c)    ((c)-'A'+'a')
#define morkCh_ToUpper(c)    ((c)-'a'+'A')


#define morkCh_IsUpper(c)    ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kU )


#define morkCh_IsLower(c)    ( morkCh_Type[ (mork_ch)(c) ] & morkCh_kL )

#endif

