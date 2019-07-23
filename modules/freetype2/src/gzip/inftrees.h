












#ifndef _INFTREES_H
#define _INFTREES_H

typedef struct inflate_huft_s FAR inflate_huft;

struct inflate_huft_s {
  union {
    struct {
      Byte Exop;        
      Byte Bits;        
    } what;
    uInt pad;           
  } word;               
  uInt base;            

};






#define MANY 1440

local  int inflate_trees_bits OF((
    uIntf *,                    
    uIntf *,                    
    inflate_huft * FAR *,       
    inflate_huft *,             
    z_streamp));                

local  int inflate_trees_dynamic OF((
    uInt,                       
    uInt,                       
    uIntf *,                    
    uIntf *,                    
    uIntf *,                    
    inflate_huft * FAR *,       
    inflate_huft * FAR *,       
    inflate_huft *,             
    z_streamp));                

local  int inflate_trees_fixed OF((
    uIntf *,                    
    uIntf *,                    
    const inflate_huft * FAR *, 
    const inflate_huft * FAR *, 
    z_streamp));                

#endif 
