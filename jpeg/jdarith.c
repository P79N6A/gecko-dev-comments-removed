














#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"




typedef struct {
  struct jpeg_entropy_decoder pub; 

  INT32 c;       
  INT32 a;               
  int ct;     
                                                         
                                                         
                                                         
  int last_dc_val[MAX_COMPS_IN_SCAN]; 
  int dc_context[MAX_COMPS_IN_SCAN]; 

  unsigned int restarts_to_go;	

  
  unsigned char * dc_stats[NUM_ARITH_TBLS];
  unsigned char * ac_stats[NUM_ARITH_TBLS];

  
  unsigned char fixed_bin[4];
} arith_entropy_decoder;

typedef arith_entropy_decoder * arith_entropy_ptr;














#define DC_STAT_BINS 64
#define AC_STAT_BINS 256


LOCAL(int)
get_byte (j_decompress_ptr cinfo)

{
  struct jpeg_source_mgr * src = cinfo->src;

  if (src->bytes_in_buffer == 0)
    if (! (*src->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  src->bytes_in_buffer--;
  return GETJOCTET(*src->next_input_byte++);
}





























LOCAL(int)
arith_decode (j_decompress_ptr cinfo, unsigned char *st)
{
  register arith_entropy_ptr e = (arith_entropy_ptr) cinfo->entropy;
  register unsigned char nl, nm;
  register INT32 qe, temp;
  register int sv, data;

  
  while (e->a < 0x8000L) {
    if (--e->ct < 0) {
      
      if (cinfo->unread_marker)
	data = 0;		
      else {
	data = get_byte(cinfo);	
	if (data == 0xFF) {	
	  do data = get_byte(cinfo);
	  while (data == 0xFF);	
	  if (data == 0)
	    data = 0xFF;	
	  else {
	    





	    cinfo->unread_marker = data;
	    data = 0;
	  }
	}
      }
      e->c = (e->c << 8) | data; 
      if ((e->ct += 8) < 0)	 
	
	if (++e->ct == 0)
	  
	  e->a = 0x8000L; 
    }
    e->a <<= 1;
  }

  


  sv = *st;
  qe = jpeg_aritab[sv & 0x7F];	
  nl = qe & 0xFF; qe >>= 8;	
  nm = qe & 0xFF; qe >>= 8;	

  
  temp = e->a - qe;
  e->a = temp;
  temp <<= e->ct;
  if (e->c >= temp) {
    e->c -= temp;
    
    if (e->a < qe) {
      e->a = qe;
      *st = (sv & 0x80) ^ nm;	
    } else {
      e->a = qe;
      *st = (sv & 0x80) ^ nl;	
      sv ^= 0x80;		
    }
  } else if (e->a < 0x8000L) {
    
    if (e->a < qe) {
      *st = (sv & 0x80) ^ nl;	
      sv ^= 0x80;		
    } else {
      *st = (sv & 0x80) ^ nm;	
    }
  }

  return sv >> 7;
}






LOCAL(void)
process_restart (j_decompress_ptr cinfo)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  int ci;
  jpeg_component_info * compptr;

  
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    ERREXIT(cinfo, JERR_CANT_SUSPEND);

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (! cinfo->progressive_mode || (cinfo->Ss == 0 && cinfo->Ah == 0)) {
      MEMZERO(entropy->dc_stats[compptr->dc_tbl_no], DC_STAT_BINS);
      
      entropy->last_dc_val[ci] = 0;
      entropy->dc_context[ci] = 0;
    }
    if (! cinfo->progressive_mode || cinfo->Ss) {
      MEMZERO(entropy->ac_stats[compptr->ac_tbl_no], AC_STAT_BINS);
    }
  }

  
  entropy->c = 0;
  entropy->a = 0;
  entropy->ct = -16;	

  
  entropy->restarts_to_go = cinfo->restart_interval;
}


















METHODDEF(boolean)
decode_mcu_DC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  unsigned char *st;
  int blkn, ci, tbl, sign;
  int v, m;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      process_restart(cinfo);
    entropy->restarts_to_go--;
  }

  if (entropy->ct == -1) return TRUE;	

  

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    tbl = cinfo->cur_comp_info[ci]->dc_tbl_no;

    

    
    st = entropy->dc_stats[tbl] + entropy->dc_context[ci];

    
    if (arith_decode(cinfo, st) == 0)
      entropy->dc_context[ci] = 0;
    else {
      
      
      sign = arith_decode(cinfo, st + 1);
      st += 2; st += sign;
      
      if ((m = arith_decode(cinfo, st)) != 0) {
	st = entropy->dc_stats[tbl] + 20;	
	while (arith_decode(cinfo, st)) {
	  if ((m <<= 1) == 0x8000) {
	    WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	    entropy->ct = -1;			
	    return TRUE;
	  }
	  st += 1;
	}
      }
      
      if (m < (int) ((1L << cinfo->arith_dc_L[tbl]) >> 1))
	entropy->dc_context[ci] = 0;		   
      else if (m > (int) ((1L << cinfo->arith_dc_U[tbl]) >> 1))
	entropy->dc_context[ci] = 12 + (sign * 4); 
      else
	entropy->dc_context[ci] = 4 + (sign * 4);  
      v = m;
      
      st += 14;
      while (m >>= 1)
	if (arith_decode(cinfo, st)) v |= m;
      v += 1; if (sign) v = -v;
      entropy->last_dc_val[ci] += v;
    }

    
    (*block)[0] = (JCOEF) (entropy->last_dc_val[ci] << cinfo->Al);
  }

  return TRUE;
}







METHODDEF(boolean)
decode_mcu_AC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  unsigned char *st;
  int tbl, sign, k;
  int v, m;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      process_restart(cinfo);
    entropy->restarts_to_go--;
  }

  if (entropy->ct == -1) return TRUE;	

  
  block = MCU_data[0];
  tbl = cinfo->cur_comp_info[0]->ac_tbl_no;

  

  
  for (k = cinfo->Ss; k <= cinfo->Se; k++) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    if (arith_decode(cinfo, st)) break;		
    while (arith_decode(cinfo, st + 1) == 0) {
      st += 3; k++;
      if (k > cinfo->Se) {
	WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	entropy->ct = -1;			
	return TRUE;
      }
    }
    
    
    sign = arith_decode(cinfo, entropy->fixed_bin);
    st += 2;
    
    if ((m = arith_decode(cinfo, st)) != 0) {
      if (arith_decode(cinfo, st)) {
	m <<= 1;
	st = entropy->ac_stats[tbl] +
	     (k <= cinfo->arith_ac_K[tbl] ? 189 : 217);
	while (arith_decode(cinfo, st)) {
	  if ((m <<= 1) == 0x8000) {
	    WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	    entropy->ct = -1;			
	    return TRUE;
	  }
	  st += 1;
	}
      }
    }
    v = m;
    
    st += 14;
    while (m >>= 1)
      if (arith_decode(cinfo, st)) v |= m;
    v += 1; if (sign) v = -v;
    
    (*block)[jpeg_natural_order[k]] = (JCOEF) (v << cinfo->Al);
  }

  return TRUE;
}






METHODDEF(boolean)
decode_mcu_DC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  unsigned char *st;
  int p1, blkn;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      process_restart(cinfo);
    entropy->restarts_to_go--;
  }

  st = entropy->fixed_bin;	
  p1 = 1 << cinfo->Al;		

  

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    
    if (arith_decode(cinfo, st))
      MCU_data[blkn][0][0] |= p1;
  }

  return TRUE;
}






METHODDEF(boolean)
decode_mcu_AC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  JBLOCKROW block;
  JCOEFPTR thiscoef;
  unsigned char *st;
  int tbl, k, kex;
  int p1, m1;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      process_restart(cinfo);
    entropy->restarts_to_go--;
  }

  if (entropy->ct == -1) return TRUE;	

  
  block = MCU_data[0];
  tbl = cinfo->cur_comp_info[0]->ac_tbl_no;

  p1 = 1 << cinfo->Al;		
  m1 = (-1) << cinfo->Al;	

  
  for (kex = cinfo->Se; kex > 0; kex--)
    if ((*block)[jpeg_natural_order[kex]]) break;

  for (k = cinfo->Ss; k <= cinfo->Se; k++) {
    st = entropy->ac_stats[tbl] + 3 * (k - 1);
    if (k > kex)
      if (arith_decode(cinfo, st)) break;	
    for (;;) {
      thiscoef = *block + jpeg_natural_order[k];
      if (*thiscoef) {				
	if (arith_decode(cinfo, st + 2)) {
	  if (*thiscoef < 0)
	    *thiscoef += m1;
	  else
	    *thiscoef += p1;
	}
	break;
      }
      if (arith_decode(cinfo, st + 1)) {	
	if (arith_decode(cinfo, entropy->fixed_bin))
	  *thiscoef = m1;
	else
	  *thiscoef = p1;
	break;
      }
      st += 3; k++;
      if (k > cinfo->Se) {
	WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	entropy->ct = -1;			
	return TRUE;
      }
    }
  }

  return TRUE;
}






METHODDEF(boolean)
decode_mcu (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  jpeg_component_info * compptr;
  JBLOCKROW block;
  unsigned char *st;
  int blkn, ci, tbl, sign, k;
  int v, m;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      process_restart(cinfo);
    entropy->restarts_to_go--;
  }

  if (entropy->ct == -1) return TRUE;	

  

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];

    

    tbl = compptr->dc_tbl_no;

    
    st = entropy->dc_stats[tbl] + entropy->dc_context[ci];

    
    if (arith_decode(cinfo, st) == 0)
      entropy->dc_context[ci] = 0;
    else {
      
      
      sign = arith_decode(cinfo, st + 1);
      st += 2; st += sign;
      
      if ((m = arith_decode(cinfo, st)) != 0) {
	st = entropy->dc_stats[tbl] + 20;	
	while (arith_decode(cinfo, st)) {
	  if ((m <<= 1) == 0x8000) {
	    WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	    entropy->ct = -1;			
	    return TRUE;
	  }
	  st += 1;
	}
      }
      
      if (m < (int) ((1L << cinfo->arith_dc_L[tbl]) >> 1))
	entropy->dc_context[ci] = 0;		   
      else if (m > (int) ((1L << cinfo->arith_dc_U[tbl]) >> 1))
	entropy->dc_context[ci] = 12 + (sign * 4); 
      else
	entropy->dc_context[ci] = 4 + (sign * 4);  
      v = m;
      
      st += 14;
      while (m >>= 1)
	if (arith_decode(cinfo, st)) v |= m;
      v += 1; if (sign) v = -v;
      entropy->last_dc_val[ci] += v;
    }

    (*block)[0] = (JCOEF) entropy->last_dc_val[ci];

    

    tbl = compptr->ac_tbl_no;

    
    for (k = 1; k <= DCTSIZE2 - 1; k++) {
      st = entropy->ac_stats[tbl] + 3 * (k - 1);
      if (arith_decode(cinfo, st)) break;	
      while (arith_decode(cinfo, st + 1) == 0) {
	st += 3; k++;
	if (k > DCTSIZE2 - 1) {
	  WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	  entropy->ct = -1;			
	  return TRUE;
	}
      }
      
      
      sign = arith_decode(cinfo, entropy->fixed_bin);
      st += 2;
      
      if ((m = arith_decode(cinfo, st)) != 0) {
	if (arith_decode(cinfo, st)) {
	  m <<= 1;
	  st = entropy->ac_stats[tbl] +
	       (k <= cinfo->arith_ac_K[tbl] ? 189 : 217);
	  while (arith_decode(cinfo, st)) {
	    if ((m <<= 1) == 0x8000) {
	      WARNMS(cinfo, JWRN_ARITH_BAD_CODE);
	      entropy->ct = -1;			
	      return TRUE;
	    }
	    st += 1;
	  }
	}
      }
      v = m;
      
      st += 14;
      while (m >>= 1)
	if (arith_decode(cinfo, st)) v |= m;
      v += 1; if (sign) v = -v;
      (*block)[jpeg_natural_order[k]] = (JCOEF) v;
    }
  }

  return TRUE;
}






METHODDEF(void)
start_pass (j_decompress_ptr cinfo)
{
  arith_entropy_ptr entropy = (arith_entropy_ptr) cinfo->entropy;
  int ci, tbl;
  jpeg_component_info * compptr;

  if (cinfo->progressive_mode) {
    
    if (cinfo->Ss == 0) {
      if (cinfo->Se != 0)
	goto bad;
    } else {
      
      if (cinfo->Se < cinfo->Ss || cinfo->Se > DCTSIZE2 - 1)
	goto bad;
      
      if (cinfo->comps_in_scan != 1)
	goto bad;
    }
    if (cinfo->Ah != 0) {
      
      if (cinfo->Ah-1 != cinfo->Al)
	goto bad;
    }
    if (cinfo->Al > 13) {	
      bad:
      ERREXIT4(cinfo, JERR_BAD_PROGRESSION,
	       cinfo->Ss, cinfo->Se, cinfo->Ah, cinfo->Al);
    }
    



    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      int coefi, cindex = cinfo->cur_comp_info[ci]->component_index;
      int *coef_bit_ptr = & cinfo->coef_bits[cindex][0];
      if (cinfo->Ss && coef_bit_ptr[0] < 0) 
	WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, 0);
      for (coefi = cinfo->Ss; coefi <= cinfo->Se; coefi++) {
	int expected = (coef_bit_ptr[coefi] < 0) ? 0 : coef_bit_ptr[coefi];
	if (cinfo->Ah != expected)
	  WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, coefi);
	coef_bit_ptr[coefi] = cinfo->Al;
      }
    }
    
    if (cinfo->Ah == 0) {
      if (cinfo->Ss == 0)
	entropy->pub.decode_mcu = decode_mcu_DC_first;
      else
	entropy->pub.decode_mcu = decode_mcu_AC_first;
    } else {
      if (cinfo->Ss == 0)
	entropy->pub.decode_mcu = decode_mcu_DC_refine;
      else
	entropy->pub.decode_mcu = decode_mcu_AC_refine;
    }
  } else {
    


    if (cinfo->Ss != 0 || cinfo->Ah != 0 || cinfo->Al != 0 ||
	(cinfo->Se < DCTSIZE2 && cinfo->Se != DCTSIZE2 - 1))
      WARNMS(cinfo, JWRN_NOT_SEQUENTIAL);
    
    entropy->pub.decode_mcu = decode_mcu;
  }

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (! cinfo->progressive_mode || (cinfo->Ss == 0 && cinfo->Ah == 0)) {
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
    if (! cinfo->progressive_mode || cinfo->Ss) {
      tbl = compptr->ac_tbl_no;
      if (tbl < 0 || tbl >= NUM_ARITH_TBLS)
	ERREXIT1(cinfo, JERR_NO_ARITH_TABLE, tbl);
      if (entropy->ac_stats[tbl] == NULL)
	entropy->ac_stats[tbl] = (unsigned char *) (*cinfo->mem->alloc_small)
	  ((j_common_ptr) cinfo, JPOOL_IMAGE, AC_STAT_BINS);
      MEMZERO(entropy->ac_stats[tbl], AC_STAT_BINS);
    }
  }

  
  entropy->c = 0;
  entropy->a = 0;
  entropy->ct = -16;	

  
  entropy->restarts_to_go = cinfo->restart_interval;
}






GLOBAL(void)
jinit_arith_decoder (j_decompress_ptr cinfo)
{
  arith_entropy_ptr entropy;
  int i;

  entropy = (arith_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(arith_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass;

  
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    entropy->dc_stats[i] = NULL;
    entropy->ac_stats[i] = NULL;
  }

  
  entropy->fixed_bin[0] = 113;

  if (cinfo->progressive_mode) {
    
    int *coef_bit_ptr, ci;
    cinfo->coef_bits = (int (*)[DCTSIZE2])
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  cinfo->num_components*DCTSIZE2*SIZEOF(int));
    coef_bit_ptr = & cinfo->coef_bits[0][0];
    for (ci = 0; ci < cinfo->num_components; ci++) 
      for (i = 0; i < DCTSIZE2; i++)
	*coef_bit_ptr++ = -1;
  }
}
