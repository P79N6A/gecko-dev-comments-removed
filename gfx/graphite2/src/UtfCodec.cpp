

























#include "inc/UtfCodec.h"


namespace graphite2 {

}

using namespace graphite2;

const int8 _utf_codec<8>::sz_lut[16] =
{
		1,1,1,1,1,1,1,1,	
		0,0,0,0,  			
		2,2,				
		3,					
		4					
};

const byte  _utf_codec<8>::mask_lut[5] = {0x7f, 0xff, 0x3f, 0x1f, 0x0f};
