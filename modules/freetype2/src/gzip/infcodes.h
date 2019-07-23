









#ifndef _INFCODES_H
#define _INFCODES_H

struct inflate_codes_state;
typedef struct inflate_codes_state FAR inflate_codes_statef;

local inflate_codes_statef *inflate_codes_new OF((
    uInt, uInt,
    inflate_huft *, inflate_huft *,
    z_streamp ));

local int inflate_codes OF((
    inflate_blocks_statef *,
    z_streamp ,
    int));

local void inflate_codes_free OF((
    inflate_codes_statef *,
    z_streamp ));

#endif 
