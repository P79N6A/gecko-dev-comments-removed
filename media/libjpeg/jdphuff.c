















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdhuff.h"		


#ifdef D_PROGRESSIVE_SUPPORTED








typedef struct {
  unsigned int EOBRUN;			
  int last_dc_val[MAX_COMPS_IN_SCAN];	
} savable_state;






#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).EOBRUN = (src).EOBRUN, \
	 (dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif


typedef struct {
  struct jpeg_entropy_decoder pub; 

  


  bitread_perm_state bitstate;	
  savable_state saved;		

  
  unsigned int restarts_to_go;	

  
  d_derived_tbl * derived_tbls[NUM_HUFF_TBLS];

  d_derived_tbl * ac_derived_tbl; 
} phuff_entropy_decoder;

typedef phuff_entropy_decoder * phuff_entropy_ptr;


METHODDEF(boolean) decode_mcu_DC_first JPP((j_decompress_ptr cinfo,
					    JBLOCKROW *MCU_data));
METHODDEF(boolean) decode_mcu_AC_first JPP((j_decompress_ptr cinfo,
					    JBLOCKROW *MCU_data));
METHODDEF(boolean) decode_mcu_DC_refine JPP((j_decompress_ptr cinfo,
					     JBLOCKROW *MCU_data));
METHODDEF(boolean) decode_mcu_AC_refine JPP((j_decompress_ptr cinfo,
					     JBLOCKROW *MCU_data));






METHODDEF(void)
start_pass_phuff_decoder (j_decompress_ptr cinfo)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  boolean is_DC_band, bad;
  int ci, coefi, tbl;
  int *coef_bit_ptr;
  jpeg_component_info * compptr;

  is_DC_band = (cinfo->Ss == 0);

  
  bad = FALSE;
  if (is_DC_band) {
    if (cinfo->Se != 0)
      bad = TRUE;
  } else {
    
    if (cinfo->Ss > cinfo->Se || cinfo->Se >= DCTSIZE2)
      bad = TRUE;
    
    if (cinfo->comps_in_scan != 1)
      bad = TRUE;
  }
  if (cinfo->Ah != 0) {
    
    if (cinfo->Al != cinfo->Ah-1)
      bad = TRUE;
  }
  if (cinfo->Al > 13)		
    bad = TRUE;
  





  if (bad)
    ERREXIT4(cinfo, JERR_BAD_PROGRESSION,
	     cinfo->Ss, cinfo->Se, cinfo->Ah, cinfo->Al);
  



  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    int cindex = cinfo->cur_comp_info[ci]->component_index;
    coef_bit_ptr = & cinfo->coef_bits[cindex][0];
    if (!is_DC_band && coef_bit_ptr[0] < 0) 
      WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, 0);
    for (coefi = cinfo->Ss; coefi <= cinfo->Se; coefi++) {
      int expected = (coef_bit_ptr[coefi] < 0) ? 0 : coef_bit_ptr[coefi];
      if (cinfo->Ah != expected)
	WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, coefi);
      coef_bit_ptr[coefi] = cinfo->Al;
    }
  }

  
  if (cinfo->Ah == 0) {
    if (is_DC_band)
      entropy->pub.decode_mcu = decode_mcu_DC_first;
    else
      entropy->pub.decode_mcu = decode_mcu_AC_first;
  } else {
    if (is_DC_band)
      entropy->pub.decode_mcu = decode_mcu_DC_refine;
    else
      entropy->pub.decode_mcu = decode_mcu_AC_refine;
  }

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    


    if (is_DC_band) {
      if (cinfo->Ah == 0) {	
	tbl = compptr->dc_tbl_no;
	jpeg_make_d_derived_tbl(cinfo, TRUE, tbl,
				& entropy->derived_tbls[tbl]);
      }
    } else {
      tbl = compptr->ac_tbl_no;
      jpeg_make_d_derived_tbl(cinfo, FALSE, tbl,
			      & entropy->derived_tbls[tbl]);
      
      entropy->ac_derived_tbl = entropy->derived_tbls[tbl];
    }
    
    entropy->saved.last_dc_val[ci] = 0;
  }

  
  entropy->bitstate.bits_left = 0;
  entropy->bitstate.get_buffer = 0; 
  entropy->pub.insufficient_data = FALSE;

  
  entropy->saved.EOBRUN = 0;

  
  entropy->restarts_to_go = cinfo->restart_interval;
}







#ifdef AVOID_TABLES

#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = 
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

#endif 







LOCAL(boolean)
process_restart (j_decompress_ptr cinfo)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  int ci;

  
  
  cinfo->marker->discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;
  
  entropy->saved.EOBRUN = 0;

  
  entropy->restarts_to_go = cinfo->restart_interval;

  




  if (cinfo->unread_marker == 0)
    entropy->pub.insufficient_data = FALSE;

  return TRUE;
}
























METHODDEF(boolean)
decode_mcu_DC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  int Al = cinfo->Al;
  register int s, r;
  int blkn, ci;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  savable_state state;
  d_derived_tbl * tbl;
  jpeg_component_info * compptr;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  


  if (! entropy->pub.insufficient_data) {

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(state, entropy->saved);

    

    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      block = MCU_data[blkn];
      ci = cinfo->MCU_membership[blkn];
      compptr = cinfo->cur_comp_info[ci];
      tbl = entropy->derived_tbls[compptr->dc_tbl_no];

      

      
      HUFF_DECODE(s, br_state, tbl, return FALSE, label1);
      if (s) {
	CHECK_BIT_BUFFER(br_state, s, return FALSE);
	r = GET_BITS(s);
	s = HUFF_EXTEND(r, s);
      }

      
      s += state.last_dc_val[ci];
      state.last_dc_val[ci] = s;
      
      (*block)[0] = (JCOEF) (s << Al);
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(entropy->saved, state);
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}







METHODDEF(boolean)
decode_mcu_AC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  register int s, k, r;
  unsigned int EOBRUN;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  


  if (! entropy->pub.insufficient_data) {

    


    EOBRUN = entropy->saved.EOBRUN;	

    

    if (EOBRUN > 0)		
      EOBRUN--;			
    else {
      BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
      block = MCU_data[0];
      tbl = entropy->ac_derived_tbl;

      for (k = cinfo->Ss; k <= Se; k++) {
	HUFF_DECODE(s, br_state, tbl, return FALSE, label2);
	r = s >> 4;
	s &= 15;
	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  r = GET_BITS(s);
	  s = HUFF_EXTEND(r, s);
	  
	  (*block)[jpeg_natural_order[k]] = (JCOEF) (s << Al);
	} else {
	  if (r == 15) {	
	    k += 15;		
	  } else {		
	    EOBRUN = 1 << r;
	    if (r) {		
	      CHECK_BIT_BUFFER(br_state, r, return FALSE);
	      r = GET_BITS(r);
	      EOBRUN += r;
	    }
	    EOBRUN--;		
	    break;		
	  }
	}
      }

      BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    }

    
    entropy->saved.EOBRUN = EOBRUN;	
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}








METHODDEF(boolean)
decode_mcu_DC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  int p1 = 1 << cinfo->Al;	
  int blkn;
  JBLOCKROW block;
  BITREAD_STATE_VARS;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  



  
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);

  

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];

    
    CHECK_BIT_BUFFER(br_state, 1, return FALSE);
    if (GET_BITS(1))
      (*block)[0] |= p1;
    
  }

  
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);

  
  entropy->restarts_to_go--;

  return TRUE;
}






METHODDEF(boolean)
decode_mcu_AC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  int Se = cinfo->Se;
  int p1 = 1 << cinfo->Al;	
  int m1 = (-1) << cinfo->Al;	
  register int s, k, r;
  unsigned int EOBRUN;
  JBLOCKROW block;
  JCOEFPTR thiscoef;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;
  int num_newnz;
  int newnz_pos[DCTSIZE2];

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  

  if (! entropy->pub.insufficient_data) {

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    EOBRUN = entropy->saved.EOBRUN; 

    
    block = MCU_data[0];
    tbl = entropy->ac_derived_tbl;

    





    num_newnz = 0;

    
    k = cinfo->Ss;

    if (EOBRUN == 0) {
      for (; k <= Se; k++) {
	HUFF_DECODE(s, br_state, tbl, goto undoit, label3);
	r = s >> 4;
	s &= 15;
	if (s) {
	  if (s != 1)		
	    WARNMS(cinfo, JWRN_HUFF_BAD_CODE);
	  CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	  if (GET_BITS(1))
	    s = p1;		
	  else
	    s = m1;		
	} else {
	  if (r != 15) {
	    EOBRUN = 1 << r;	
	    if (r) {
	      CHECK_BIT_BUFFER(br_state, r, goto undoit);
	      r = GET_BITS(r);
	      EOBRUN += r;
	    }
	    break;		
	  }
	  
	}
	



	do {
	  thiscoef = *block + jpeg_natural_order[k];
	  if (*thiscoef != 0) {
	    CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	    if (GET_BITS(1)) {
	      if ((*thiscoef & p1) == 0) { 
		if (*thiscoef >= 0)
		  *thiscoef += p1;
		else
		  *thiscoef += m1;
	      }
	    }
	  } else {
	    if (--r < 0)
	      break;		
	  }
	  k++;
	} while (k <= Se);
	if (s) {
	  int pos = jpeg_natural_order[k];
	  
	  (*block)[pos] = (JCOEF) s;
	  
	  newnz_pos[num_newnz++] = pos;
	}
      }
    }

    if (EOBRUN > 0) {
      




      for (; k <= Se; k++) {
	thiscoef = *block + jpeg_natural_order[k];
	if (*thiscoef != 0) {
	  CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	  if (GET_BITS(1)) {
	    if ((*thiscoef & p1) == 0) { 
	      if (*thiscoef >= 0)
		*thiscoef += p1;
	      else
		*thiscoef += m1;
	    }
	  }
	}
      }
      
      EOBRUN--;
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    entropy->saved.EOBRUN = EOBRUN; 
  }

  
  entropy->restarts_to_go--;

  return TRUE;

undoit:
  
  while (num_newnz > 0)
    (*block)[newnz_pos[--num_newnz]] = 0;

  return FALSE;
}






GLOBAL(void)
jinit_phuff_decoder (j_decompress_ptr cinfo)
{
  phuff_entropy_ptr entropy;
  int *coef_bit_ptr;
  int ci, i;

  entropy = (phuff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(phuff_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass_phuff_decoder;

  
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->derived_tbls[i] = NULL;
  }

  
  cinfo->coef_bits = (int (*)[DCTSIZE2])
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				cinfo->num_components*DCTSIZE2*SIZEOF(int));
  coef_bit_ptr = & cinfo->coef_bits[0][0];
  for (ci = 0; ci < cinfo->num_components; ci++) 
    for (i = 0; i < DCTSIZE2; i++)
      *coef_bit_ptr++ = -1;
}

#endif 
