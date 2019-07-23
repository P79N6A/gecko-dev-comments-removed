









#ifndef _INFBLOCK_H
#define _INFBLOCK_H

struct inflate_blocks_state;
typedef struct inflate_blocks_state FAR inflate_blocks_statef;

local  inflate_blocks_statef * inflate_blocks_new OF((
    z_streamp z,
    check_func c,               
    uInt w));                   

local  int inflate_blocks OF((
    inflate_blocks_statef *,
    z_streamp ,
    int));                      

local  void inflate_blocks_reset OF((
    inflate_blocks_statef *,
    z_streamp ,
    uLongf *));                  

local  int inflate_blocks_free OF((
    inflate_blocks_statef *,
    z_streamp));

#endif 
