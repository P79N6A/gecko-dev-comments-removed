

















#ifndef __AFRANGES_H__
#define __AFRANGES_H__


#include "aftypes.h"


FT_BEGIN_HEADER

#undef  SCRIPT
#define SCRIPT( s, S, d, h, sc1, sc2, sc3 )                             \
          extern const AF_Script_UniRangeRec  af_ ## s ## _uniranges[];

#include "afscript.h"

 

FT_END_HEADER

#endif 



