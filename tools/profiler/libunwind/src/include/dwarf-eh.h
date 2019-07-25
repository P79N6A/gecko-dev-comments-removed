
























#ifndef dwarf_eh_h
#define dwarf_eh_h

#include "dwarf.h"













































































#define DW_EH_VERSION		1	/* The version we're implementing */

struct dwarf_eh_frame_hdr
  {
    unsigned char version;
    unsigned char eh_frame_ptr_enc;
    unsigned char fde_count_enc;
    unsigned char table_enc;
    










  };

#endif 
