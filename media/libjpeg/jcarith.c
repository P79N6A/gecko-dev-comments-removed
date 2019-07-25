














#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"




typedef struct {
  struct jpeg_entropy_encoder pub; 

  INT32 c; 
  INT32 a;               
  INT32 sc;        
  INT32 zc;          

  int ct;  
  int buffer;                

  int last_dc_val[MAX_COMPS_IN_SCAN]; 
  int dc_context[MAX_COMPS_IN_SCAN]; 

  unsigned int restarts_to_go;	
  int next_restart_num;		

  
  unsigned char * dc_stats[NUM_ARITH_TBLS];
  unsigned char * ac_stats[NUM_ARITH_TBLS];

  
  unsigned char fixed_bin[4];
} arith_entropy_encoder;

typedef arith_entropy_encoder * arith_entropy_ptr;














#define DC_STAT_BINS 64
#define AC_STAT_BINS 256






































#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define ISHIFT_TEMPS	int ishift_temp;
#define IRIGHT_SHIFT(x,shft)  \
	((ishift_temp = (x)) < 0 ? \
	 (ishift_temp >> (shft)) | ((~0) << (16-(shft))) : \
	 (ishift_temp >> (shft)))
#else
#define ISHIFT_TEMPS
#define IRIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif


LOCAL(void)
emit_byte (int val, j_compress_ptr cinfo)

{
  struct jpeg_destination_mgr * dest = cinfo->dest;

  *dest->next_output_byte++ = (JOCTET) val;
  if (--dest->free_in_buffer == 0)
    if (! (*dest->empty_output_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
}






METHODDEF(void)
finish_pass (j_compress_ptr cinfo)
{
  arith_entropy_ptr e = (arith_entropy_ptr) cinfo->entropy;
  INT32 temp;

  

  

  if ((temp = (e->a - 1 + e->c) & 0xFFFF0000L) < e->c)
    e->c = temp + 0x8000L;
  else
    e->c = temp;
  
  e->c <<= e->ct;
  if (e->c & 0xF8000000L) {
    
    if (e->buffer >= 0) {
      if (e->zc)
	do emit_byte(0x00, cinfo);
	while (--e->zc);
      emit_byte(e->buffer + 1, cinfo);
      if (e->buffer + 1 == 0xFF)
	emit_byte(0x00, cinfo);
    }
    e->zc += e->sc;  
    e->sc = 0;
  } else {
    if (e->buffer == 0)
      ++e->zc;
    else if (e->buffer >= 0) {
      if (e->zc)
	do emit_byte(0x00, cinfo);
	while (--e->zc);
      emit_byte(e->buffer, cinfo);
    }
    if (e->sc) {
      if (e->zc)
	do emit_byte(0x00, cinfo);
	while (--e->zc);
      do {
	emit_byte(0xFF, cinfo);
	emit_byte(0x00, cinfo);
      } while (--e->sc);
    }
  }
  
  if (e->c & 0x7FFF800L) {
    if (e->zc)  
      do emit_byte(0x00, cinfo);
      while (--e->zc);
    emit_byte((e->c >> 19) & 0xFF, cinfo);
    if (((e->c >> 19) & 0xFF) == 0xFF)
      emit_byte(0x00, cinfo);
    if (e->c & 0x7F800L) {
      emit_byte((e->c >> 11) & 0xFF, cinfo);
      if (((e->c >> 11) & 0xFF) == 0xFF)
	emit_byte(0x00, cinfo);
    }
  }
}
























LOCAL(void)
arith_encode (j_compress_ptr cinfo, unsigned char *st, int val) 
{
  register arith_entropy_ptr e = (arith_entropy_ptr) cinfo->entropy;
  register unsigned char nl, nm;
  register INT32 qe, temp;
  register int sv;

  


  sv = *st;
  qe = jpeg_aritab[sv & 0x7F];	
  nl = qe & 0xFF; qe >>= 8;	
  nm = qe & 0xFF; qe >>= 8;	

  
  e->a -= qe;
  if (val != (sv >> 7)) {
    
    if (e->a >= qe) {
      



      e->c += e->a;
      e->a = qe;
    }
    *st = (sv & 0x80) ^ nl;	
  } else {
    
    if (e->a >= 0x8000L)
      return;  
    if (e->a < qe) {
      


      e->c += e->a;
      e->a = qe;
    }
    *st = (sv & 0x80) ^ nm;	
  }

  
  do {
    e->a <<= 1;
    e->c <<= 1;
    if (--e->ct == 0) {
      
      temp = e->c >> 19;
      if (temp > 0xFF) {
	
	if (e->buffer >= 0) {
	  if (e->zc)
	    do emit_byte(0x00, cinfo);
	    while (--e->zc);
	  emit_byte(e->buffer + 1, cinfo);
	  if (e->buffer + 1 == 0xFF)
	    emit_byte(0x00, cinfo);
	}
	e->zc += e->sc;  
	e->sc = 0;
	


	e->buffer = temp & 0xFF;  
      } else if (temp == 0xFF) {
	++e->sc;  
      } else {
	
	if (e->buffer == 0)
	  ++e->zc;
	else if (e->buffer >= 0) {
	  if (e->zc)
	    do emit_byte(0x00, cinfo);
	    while (--e->zc);
	  emit_byte(e->buffer, cinfo);
	}
	if (e->sc) {
	  if (e->zc)
	    do emit_byte(0x00, cinfo);
	    while (--e->zc);
	  do {
	    emit_byte(0xFF, cinfo);
	    emit_byte(0x00, cinfo);
	  } while (--e->sc);
	}
	e->buffer = temp & 0xFF;  
      }
      e->c &= 0x7FFFFL;
      e->ct += 8;
    }
  } while (e->a < 0x8000L);
}






LOCAL(void)
emit_restart (j_compress_ptr cinfo, int restart_num)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  int ci;
  jpeg_component_info * compptr;

  finish_pass(cinfo);

  emit_byte(0xFF, cinfo);
  emit_byte(JPEG_RST0 + restart_num, cinfo);

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    
    if (cinfo->progressive_mode == 0 || (cinfo->Ss == 0 && cinfo->Ah == 0)) {
      MEMZERO(entropy->dc_stats[compptr->dc_tbl_no], DC_STAT_BINS);
      
      entropy->last_dc_val[ci] = 0;
      entropy->dc_context[ci] = 0;
    }
    
    if (cinfo->progressive_mode == 0 || cinfo->Se) {
      MEMZERO(entropy->ac_stats[compptr->ac_tbl_no], AC_STAT_BINS);
    }
  }

  
  entropy->c = 0;
  entropy->a = 0x10000L;
  entropy->sc = 0;
  entropy->zc = 0;
  entropy->ct = 11;
  entropy->buffer = -1;  
}







METHODDEF(boolean)
encode_mcu_DC_first (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  unsigned char *st;
  int blkn, ci, tbl;
  int v, v2, m;
  ISHIFT_TEMPS

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      emit_restart(cinfo, entropy->next_restart_num);
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    tbl = cinfo->cur_comp_info[ci]->dc_tbl_no;

    


    m = IRIGHT_SHIFT((int) ((*block)[0]), cinfo->Al);

    

    
    st = entropy->dc_stats[tbl] + entropy->dc_context[ci];

    
    if ((v = m - entropy->last_dc_val[ci]) == 0) {
      arith_encode(cinfo, st, 0);
      entropy->dc_context[ci] = 0;	
    } else {
      entropy->last_dc_val[ci] = m;
      arith_encode(cinfo, st, 1);
      
      
      if (v > 0) {
	arith_encode(cinfo, st + 1, 0);	
	st += 2;			
	entropy->dc_context[ci] = 4;	
      } else {
	v = -v;
	arith_encode(cinfo, st + 1, 1);	
	st += 3;			
	entropy->dc_context[ci] = 8;	
      }
      
      m = 0;
      if (v -= 1) {
	arith_encode(cinfo, st, 1);
	m = 1;
	v2 = v;
	st = entropy->dc_stats[tbl] + 20; 
	while (v2 >>= 1) {
	  arith_encode(cinfo, st, 1);
	  m <<= 1;
	  st += 1;
	}
      }
      arith_encode(cinfo, st, 0);
      
      if (m < (int) ((1L << cinfo->arith_dc_L[tbl]) >> 1))
	entropy->dc_context[ci] = 0;	
      else if (m > (int) ((1L << cinfo->arith_dc_U[tbl]) >> 1))
	entropy->dc_context[ci] += 8;	
      
      st += 14;
      while (m >>= 1)
	arith_encode(cinfo, st, (m & v) ? 1 : 0);
    }
  }

  return TRUE;
}







METHODDEF(boolean)
encode_mcu_AC_first (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  unsigned char *st;
  int tbl, k, ke;
  int v, v2, m;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      emit_restart(cinfo, entropy->next_restart_num);
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  
  block = MCU_data[0];
  tbl = cinfo->cur_comp_info[0]->ac_tbl_no;

  

  
  for (ke = cinfo->Se; ke > 0; ke--)
    



    if ((v = (*block)[jpeg_natural_order[ke]]) >= 0) {
      if (v >>= cinfo->Al) break;
    } else {
      v = -v;
      if (v >>= cinfo->Al) break;
    }

  
  for (k = cinfo->Ss; k <= ke; k++) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    arith_encode(cinfo, st, 0);		
    for (;;) {
      if ((v = (*block)[jpeg_natural_order[k]]) >= 0) {
	if (v >>= cinfo->Al) {
	  arith_encode(cinfo, st + 1, 1);
	  arith_encode(cinfo, entropy->fixed_bin, 0);
	  break;
	}
      } else {
	v = -v;
	if (v >>= cinfo->Al) {
	  arith_encode(cinfo, st + 1, 1);
	  arith_encode(cinfo, entropy->fixed_bin, 1);
	  break;
	}
      }
      arith_encode(cinfo, st + 1, 0); st += 3; k++;
    }
    st += 2;
    
    m = 0;
    if (v -= 1) {
      arith_encode(cinfo, st, 1);
      m = 1;
      v2 = v;
      if (v2 >>= 1) {
	arith_encode(cinfo, st, 1);
	m <<= 1;
	st = entropy->ac_stats[tbl] +
	     (k <= cinfo->arith_ac_K[tbl] ? 189 : 217);
	while (v2 >>= 1) {
	  arith_encode(cinfo, st, 1);
	  m <<= 1;
	  st += 1;
	}
      }
    }
    arith_encode(cinfo, st, 0);
    
    st += 14;
    while (m >>= 1)
      arith_encode(cinfo, st, (m & v) ? 1 : 0);
  }
  
  if (k <= cinfo->Se) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    arith_encode(cinfo, st, 1);
  }

  return TRUE;
}






METHODDEF(boolean)
encode_mcu_DC_refine (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  unsigned char *st;
  int Al, blkn;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      emit_restart(cinfo, entropy->next_restart_num);
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  st = entropy->fixed_bin;	
  Al = cinfo->Al;

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    
    arith_encode(cinfo, st, (MCU_data[blkn][0][0] >> Al) & 1);
  }

  return TRUE;
}






METHODDEF(boolean)
encode_mcu_AC_refine (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  unsigned char *st;
  int tbl, k, ke, kex;
  int v;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      emit_restart(cinfo, entropy->next_restart_num);
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  
  block = MCU_data[0];
  tbl = cinfo->cur_comp_info[0]->ac_tbl_no;

  

  
  for (ke = cinfo->Se; ke > 0; ke--)
    



    if ((v = (*block)[jpeg_natural_order[ke]]) >= 0) {
      if (v >>= cinfo->Al) break;
    } else {
      v = -v;
      if (v >>= cinfo->Al) break;
    }

  
  for (kex = ke; kex > 0; kex--)
    if ((v = (*block)[jpeg_natural_order[kex]]) >= 0) {
      if (v >>= cinfo->Ah) break;
    } else {
      v = -v;
      if (v >>= cinfo->Ah) break;
    }

  
  for (k = cinfo->Ss; k <= ke; k++) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    if (k > kex)
      arith_encode(cinfo, st, 0);	
    for (;;) {
      if ((v = (*block)[jpeg_natural_order[k]]) >= 0) {
	if (v >>= cinfo->Al) {
	  if (v >> 1)			
	    arith_encode(cinfo, st + 2, (v & 1));
	  else {			
	    arith_encode(cinfo, st + 1, 1);
	    arith_encode(cinfo, entropy->fixed_bin, 0);
	  }
	  break;
	}
      } else {
	v = -v;
	if (v >>= cinfo->Al) {
	  if (v >> 1)			
	    arith_encode(cinfo, st + 2, (v & 1));
	  else {			
	    arith_encode(cinfo, st + 1, 1);
	    arith_encode(cinfo, entropy->fixed_bin, 1);
	  }
	  break;
	}
      }
      arith_encode(cinfo, st + 1, 0); st += 3; k++;
    }
  }
  
  if (k <= cinfo->Se) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    arith_encode(cinfo, st, 1);
  }

  return TRUE;
}






METHODDEF(boolean)
encode_mcu (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  jpeg_component_info * compptr;
  JBLOCKROW block;
  unsigned char *st;
  int blkn, ci, tbl, k, ke;
  int v, v2, m;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      emit_restart(cinfo, entropy->next_restart_num);
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];

    

    tbl = compptr->dc_tbl_no;

    
    st = entropy->dc_stats[tbl] + entropy->dc_context[ci];

    
    if ((v = (*block)[0] - entropy->last_dc_val[ci]) == 0) {
      arith_encode(cinfo, st, 0);
      entropy->dc_context[ci] = 0;	
    } else {
      entropy->last_dc_val[ci] = (*block)[0];
      arith_encode(cinfo, st, 1);
      
      
      if (v > 0) {
	arith_encode(cinfo, st + 1, 0);	
	st += 2;			
	entropy->dc_context[ci] = 4;	
      } else {
	v = -v;
	arith_encode(cinfo, st + 1, 1);	
	st += 3;			
	entropy->dc_context[ci] = 8;	
      }
      
      m = 0;
      if (v -= 1) {
	arith_encode(cinfo, st, 1);
	m = 1;
	v2 = v;
	st = entropy->dc_stats[tbl] + 20; 
	while (v2 >>= 1) {
	  arith_encode(cinfo, st, 1);
	  m <<= 1;
	  st += 1;
	}
      }
      arith_encode(cinfo, st, 0);
      
      if (m < (int) ((1L << cinfo->arith_dc_L[tbl]) >> 1))
	entropy->dc_context[ci] = 0;	
      else if (m > (int) ((1L << cinfo->arith_dc_U[tbl]) >> 1))
	entropy->dc_context[ci] += 8;	
      
      st += 14;
      while (m >>= 1)
	arith_encode(cinfo, st, (m & v) ? 1 : 0);
    }

    

    tbl = compptr->ac_tbl_no;

    
    for (ke = DCTSIZE2 - 1; ke > 0; ke--)
      if ((*block)[jpeg_natural_order[ke]]) break;

    
    for (k = 1; k <= ke; k++) {
      st = entropy->ac_stats[tbl] + 3 * (k - 1);
      arith_encode(cinfo, st, 0);	
      while ((v = (*block)[jpeg_natural_order[k]]) == 0) {
	arith_encode(cinfo, st + 1, 0); st += 3; k++;
      }
      arith_encode(cinfo, st + 1, 1);
      
      
      if (v > 0) {
	arith_encode(cinfo, entropy->fixed_bin, 0);
      } else {
	v = -v;
	arith_encode(cinfo, entropy->fixed_bin, 1);
      }
      st += 2;
      
      m = 0;
      if (v -= 1) {
	arith_encode(cinfo, st, 1);
	m = 1;
	v2 = v;
	if (v2 >>= 1) {
	  arith_encode(cinfo, st, 1);
	  m <<= 1;
	  st = entropy->ac_stats[tbl] +
	       (k <= cinfo->arith_ac_K[tbl] ? 189 : 217);
	  while (v2 >>= 1) {
	    arith_encode(cinfo, st, 1);
	    m <<= 1;
	    st += 1;
	  }
	}
      }
      arith_encode(cinfo, st, 0);
      
      st += 14;
      while (m >>= 1)
	arith_encode(cinfo, st, (m & v) ? 1 : 0);
    }
    
    if (k <= DCTSIZE2 - 1) {
      st = entropy->ac_stats[tbl] + 3 * (k - 1);
      arith_encode(cinfo, st, 1);
    }
  }

  return TRUE;
}






METHODDEF(void)
start_pass (j_compress_ptr cinfo, boolean gather_statistics)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  int ci, tbl;
  jpeg_component_info * compptr;

  if (gather_statistics)
    



    ERREXIT(cinfo, JERR_NOT_COMPILED);

  

  
  if (cinfo->progressive_mode) {
    if (cinfo->Ah == 0) {
      if (cinfo->Ss == 0)
	entropy->pub.encode_mcu = encode_mcu_DC_first;
      else
	entropy->pub.encode_mcu = encode_mcu_AC_first;
    } else {
      if (cinfo->Ss == 0)
	entropy->pub.encode_mcu = encode_mcu_DC_refine;
      else
	entropy->pub.encode_mcu = encode_mcu_AC_refine;
    }
  } else
    entropy->pub.encode_mcu = encode_mcu;

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    
    if (cinfo->progressive_mode == 0 || (cinfo->Ss == 0 && cinfo->Ah == 0)) {
      tbl = compptr->dc_tbl_no;
      if (tbl < 0 || tbl >= NUM_ARITH_TBLS)
	ERREXIT1(cinfo, JERR_NO_ARITH_TABLE, tbl);
      if (entropy->dc_stats[tbl] == NULL)
	entropy->dc_stats[tbl] = (unsigned char *) (*cinfo->mem->alloc_small)
	  ((j_common_ptr) cinfo, JPOOL_IMAGE, DC_STAT_BINS);
      MEMZERO(entropy->dc_stats[tbl], DC_STAT_BINS);
      
      entropy->last_dc_val[ci] = 0;
      entropy->dc_context[ci] = 0;
    }
    
    if (cinfo->progressive_mode == 0 || cinfo->Se) {
      tbl = compptr->ac_tbl_no;
      if (tbl < 0 || tbl >= NUM_ARITH_TBLS)
	ERREXIT1(cinfo, JERR_NO_ARITH_TABLE, tbl);
      if (entropy->ac_stats[tbl] == NULL)
	entropy->ac_stats[tbl] = (unsigned char *) (*cinfo->mem->alloc_small)
	  ((j_common_ptr) cinfo, JPOOL_IMAGE, AC_STAT_BINS);
      MEMZERO(entropy->ac_stats[tbl], AC_STAT_BINS);
#ifdef CALCULATE_SPECTRAL_CONDITIONING
      if (cinfo->progressive_mode)
	
	cinfo->arith_ac_K[tbl] = cinfo->Ss + ((8 + cinfo->Se - cinfo->Ss) >> 4);
#endif
    }
  }

  
  entropy->c = 0;
  entropy->a = 0x10000L;
  entropy->sc = 0;
  entropy->zc = 0;
  entropy->ct = 11;
  entropy->buffer = -1;  

  
  entropy->restarts_to_go = cinfo->restart_interval;
  entropy->next_restart_num = 0;
}






GLOBAL(void)
jinit_arith_encoder (j_compress_ptr cinfo)
{
  arith_entropy_ptr entropy;
  int i;

  entropy = (arith_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(arith_entropy_encoder));
  cinfo->entropy = (struct jpeg_entropy_encoder *) entropy;
  entropy->pub.start_pass = start_pass;
  entropy->pub.finish_pass = finish_pass;

  
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    entropy->dc_stats[i] = NULL;
    entropy->ac_stats[i] = NULL;
  }

  
  entropy->fixed_bin[0] = 113;
}
