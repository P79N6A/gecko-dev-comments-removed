













#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jchuff.h"		

#ifdef C_PROGRESSIVE_SUPPORTED



typedef struct {
  struct jpeg_entropy_encoder pub; 

  
  boolean gather_statistics;

  


  JOCTET * next_output_byte;	
  size_t free_in_buffer;	
  INT32 put_buffer;		
  int put_bits;			
  j_compress_ptr cinfo;		

  
  int last_dc_val[MAX_COMPS_IN_SCAN]; 

  
  int ac_tbl_no;		
  unsigned int EOBRUN;		
  unsigned int BE;		
  char * bit_buffer;		
  

  unsigned int restarts_to_go;	
  int next_restart_num;		

  



  c_derived_tbl * derived_tbls[NUM_HUFF_TBLS];

  
  long * count_ptrs[NUM_HUFF_TBLS];
} phuff_entropy_encoder;

typedef phuff_entropy_encoder * phuff_entropy_ptr;







#define MAX_CORR_BITS  1000	/* Max # of correction bits I can buffer */






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


METHODDEF(boolean) encode_mcu_DC_first JPP((j_compress_ptr cinfo,
					    JBLOCKROW *MCU_data));
METHODDEF(boolean) encode_mcu_AC_first JPP((j_compress_ptr cinfo,
					    JBLOCKROW *MCU_data));
METHODDEF(boolean) encode_mcu_DC_refine JPP((j_compress_ptr cinfo,
					     JBLOCKROW *MCU_data));
METHODDEF(boolean) encode_mcu_AC_refine JPP((j_compress_ptr cinfo,
					     JBLOCKROW *MCU_data));
METHODDEF(void) finish_pass_phuff JPP((j_compress_ptr cinfo));
METHODDEF(void) finish_pass_gather_phuff JPP((j_compress_ptr cinfo));






METHODDEF(void)
start_pass_phuff (j_compress_ptr cinfo, boolean gather_statistics)
{  
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  boolean is_DC_band;
  int ci, tbl;
  jpeg_component_info * compptr;

  entropy->cinfo = cinfo;
  entropy->gather_statistics = gather_statistics;

  is_DC_band = (cinfo->Ss == 0);

  

  
  if (cinfo->Ah == 0) {
    if (is_DC_band)
      entropy->pub.encode_mcu = encode_mcu_DC_first;
    else
      entropy->pub.encode_mcu = encode_mcu_AC_first;
  } else {
    if (is_DC_band)
      entropy->pub.encode_mcu = encode_mcu_DC_refine;
    else {
      entropy->pub.encode_mcu = encode_mcu_AC_refine;
      
      if (entropy->bit_buffer == NULL)
	entropy->bit_buffer = (char *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      MAX_CORR_BITS * SIZEOF(char));
    }
  }
  if (gather_statistics)
    entropy->pub.finish_pass = finish_pass_gather_phuff;
  else
    entropy->pub.finish_pass = finish_pass_phuff;

  


  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    
    entropy->last_dc_val[ci] = 0;
    
    if (is_DC_band) {
      if (cinfo->Ah != 0)	
	continue;
      tbl = compptr->dc_tbl_no;
    } else {
      entropy->ac_tbl_no = tbl = compptr->ac_tbl_no;
    }
    if (gather_statistics) {
      
      
      if (tbl < 0 || tbl >= NUM_HUFF_TBLS)
        ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tbl);
      
      
      if (entropy->count_ptrs[tbl] == NULL)
	entropy->count_ptrs[tbl] = (long *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->count_ptrs[tbl], 257 * SIZEOF(long));
    } else {
      
      
      jpeg_make_c_derived_tbl(cinfo, is_DC_band, tbl,
			      & entropy->derived_tbls[tbl]);
    }
  }

  
  entropy->EOBRUN = 0;
  entropy->BE = 0;

  
  entropy->put_buffer = 0;
  entropy->put_bits = 0;

  
  entropy->restarts_to_go = cinfo->restart_interval;
  entropy->next_restart_num = 0;
}








#define emit_byte(entropy,val)  \
	{ *(entropy)->next_output_byte++ = (JOCTET) (val);  \
	  if (--(entropy)->free_in_buffer == 0)  \
	    dump_buffer(entropy); }


LOCAL(void)
dump_buffer (phuff_entropy_ptr entropy)

{
  struct jpeg_destination_mgr * dest = entropy->cinfo->dest;

  if (! (*dest->empty_output_buffer) (entropy->cinfo))
    ERREXIT(entropy->cinfo, JERR_CANT_SUSPEND);
  
  entropy->next_output_byte = dest->next_output_byte;
  entropy->free_in_buffer = dest->free_in_buffer;
}










INLINE
LOCAL(void)
emit_bits (phuff_entropy_ptr entropy, unsigned int code, int size)

{
  
  register INT32 put_buffer = (INT32) code;
  register int put_bits = entropy->put_bits;

  
  if (size == 0)
    ERREXIT(entropy->cinfo, JERR_HUFF_MISSING_CODE);

  if (entropy->gather_statistics)
    return;			

  put_buffer &= (((INT32) 1)<<size) - 1; 
  
  put_bits += size;		
  
  put_buffer <<= 24 - put_bits; 

  put_buffer |= entropy->put_buffer; 

  while (put_bits >= 8) {
    int c = (int) ((put_buffer >> 16) & 0xFF);
    
    emit_byte(entropy, c);
    if (c == 0xFF) {		
      emit_byte(entropy, 0);
    }
    put_buffer <<= 8;
    put_bits -= 8;
  }

  entropy->put_buffer = put_buffer; 
  entropy->put_bits = put_bits;
}


LOCAL(void)
flush_bits (phuff_entropy_ptr entropy)
{
  emit_bits(entropy, 0x7F, 7); 
  entropy->put_buffer = 0;     
  entropy->put_bits = 0;
}






INLINE
LOCAL(void)
emit_symbol (phuff_entropy_ptr entropy, int tbl_no, int symbol)
{
  if (entropy->gather_statistics)
    entropy->count_ptrs[tbl_no][symbol]++;
  else {
    c_derived_tbl * tbl = entropy->derived_tbls[tbl_no];
    emit_bits(entropy, tbl->ehufco[symbol], tbl->ehufsi[symbol]);
  }
}






LOCAL(void)
emit_buffered_bits (phuff_entropy_ptr entropy, char * bufstart,
		    unsigned int nbits)
{
  if (entropy->gather_statistics)
    return;			

  while (nbits > 0) {
    emit_bits(entropy, (unsigned int) (*bufstart), 1);
    bufstart++;
    nbits--;
  }
}






LOCAL(void)
emit_eobrun (phuff_entropy_ptr entropy)
{
  register int temp, nbits;

  if (entropy->EOBRUN > 0) {	
    temp = entropy->EOBRUN;
    nbits = 0;
    while ((temp >>= 1))
      nbits++;
    
    if (nbits > 14)
      ERREXIT(entropy->cinfo, JERR_HUFF_MISSING_CODE);

    emit_symbol(entropy, entropy->ac_tbl_no, nbits << 4);
    if (nbits)
      emit_bits(entropy, entropy->EOBRUN, nbits);

    entropy->EOBRUN = 0;

    
    emit_buffered_bits(entropy, entropy->bit_buffer, entropy->BE);
    entropy->BE = 0;
  }
}






LOCAL(void)
emit_restart (phuff_entropy_ptr entropy, int restart_num)
{
  int ci;

  emit_eobrun(entropy);

  if (! entropy->gather_statistics) {
    flush_bits(entropy);
    emit_byte(entropy, 0xFF);
    emit_byte(entropy, JPEG_RST0 + restart_num);
  }

  if (entropy->cinfo->Ss == 0) {
    
    for (ci = 0; ci < entropy->cinfo->comps_in_scan; ci++)
      entropy->last_dc_val[ci] = 0;
  } else {
    
    entropy->EOBRUN = 0;
    entropy->BE = 0;
  }
}







METHODDEF(boolean)
encode_mcu_DC_first (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  register int temp, temp2;
  register int nbits;
  int blkn, ci;
  int Al = cinfo->Al;
  JBLOCKROW block;
  jpeg_component_info * compptr;
  ISHIFT_TEMPS

  entropy->next_output_byte = cinfo->dest->next_output_byte;
  entropy->free_in_buffer = cinfo->dest->free_in_buffer;

  
  if (cinfo->restart_interval)
    if (entropy->restarts_to_go == 0)
      emit_restart(entropy, entropy->next_restart_num);

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];

    


    temp2 = IRIGHT_SHIFT((int) ((*block)[0]), Al);

    
    temp = temp2 - entropy->last_dc_val[ci];
    entropy->last_dc_val[ci] = temp2;

    
    temp2 = temp;
    if (temp < 0) {
      temp = -temp;		
      
      
      temp2--;
    }
    
    
    nbits = 0;
    while (temp) {
      nbits++;
      temp >>= 1;
    }
    


    if (nbits > MAX_COEF_BITS+1)
      ERREXIT(cinfo, JERR_BAD_DCT_COEF);
    
    
    emit_symbol(entropy, compptr->dc_tbl_no, nbits);
    
    
    
    if (nbits)			
      emit_bits(entropy, (unsigned int) temp2, nbits);
  }

  cinfo->dest->next_output_byte = entropy->next_output_byte;
  cinfo->dest->free_in_buffer = entropy->free_in_buffer;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  return TRUE;
}







METHODDEF(boolean)
encode_mcu_AC_first (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  register int temp, temp2;
  register int nbits;
  register int r, k;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  JBLOCKROW block;

  entropy->next_output_byte = cinfo->dest->next_output_byte;
  entropy->free_in_buffer = cinfo->dest->free_in_buffer;

  
  if (cinfo->restart_interval)
    if (entropy->restarts_to_go == 0)
      emit_restart(entropy, entropy->next_restart_num);

  
  block = MCU_data[0];

  
  
  r = 0;			
   
  for (k = cinfo->Ss; k <= Se; k++) {
    if ((temp = (*block)[jpeg_natural_order[k]]) == 0) {
      r++;
      continue;
    }
    




    if (temp < 0) {
      temp = -temp;		
      temp >>= Al;		
      
      temp2 = ~temp;
    } else {
      temp >>= Al;		
      temp2 = temp;
    }
    
    if (temp == 0) {
      r++;
      continue;
    }

    
    if (entropy->EOBRUN > 0)
      emit_eobrun(entropy);
    
    while (r > 15) {
      emit_symbol(entropy, entropy->ac_tbl_no, 0xF0);
      r -= 16;
    }

    
    nbits = 1;			
    while ((temp >>= 1))
      nbits++;
    
    if (nbits > MAX_COEF_BITS)
      ERREXIT(cinfo, JERR_BAD_DCT_COEF);

    
    emit_symbol(entropy, entropy->ac_tbl_no, (r << 4) + nbits);

    
    
    emit_bits(entropy, (unsigned int) temp2, nbits);

    r = 0;			
  }

  if (r > 0) {			
    entropy->EOBRUN++;		
    if (entropy->EOBRUN == 0x7FFF)
      emit_eobrun(entropy);	
  }

  cinfo->dest->next_output_byte = entropy->next_output_byte;
  cinfo->dest->free_in_buffer = entropy->free_in_buffer;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  return TRUE;
}








METHODDEF(boolean)
encode_mcu_DC_refine (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  register int temp;
  int blkn;
  int Al = cinfo->Al;
  JBLOCKROW block;

  entropy->next_output_byte = cinfo->dest->next_output_byte;
  entropy->free_in_buffer = cinfo->dest->free_in_buffer;

  
  if (cinfo->restart_interval)
    if (entropy->restarts_to_go == 0)
      emit_restart(entropy, entropy->next_restart_num);

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];

    
    temp = (*block)[0];
    emit_bits(entropy, (unsigned int) (temp >> Al), 1);
  }

  cinfo->dest->next_output_byte = entropy->next_output_byte;
  cinfo->dest->free_in_buffer = entropy->free_in_buffer;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  return TRUE;
}






METHODDEF(boolean)
encode_mcu_AC_refine (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  register int temp;
  register int r, k;
  int EOB;
  char *BR_buffer;
  unsigned int BR;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  JBLOCKROW block;
  int absvalues[DCTSIZE2];

  entropy->next_output_byte = cinfo->dest->next_output_byte;
  entropy->free_in_buffer = cinfo->dest->free_in_buffer;

  
  if (cinfo->restart_interval)
    if (entropy->restarts_to_go == 0)
      emit_restart(entropy, entropy->next_restart_num);

  
  block = MCU_data[0];

  


  EOB = 0;
  for (k = cinfo->Ss; k <= Se; k++) {
    temp = (*block)[jpeg_natural_order[k]];
    



    if (temp < 0)
      temp = -temp;		
    temp >>= Al;		
    absvalues[k] = temp;	
    if (temp == 1)
      EOB = k;			
  }

  
  
  r = 0;			
  BR = 0;			
  BR_buffer = entropy->bit_buffer + entropy->BE; 

  for (k = cinfo->Ss; k <= Se; k++) {
    if ((temp = absvalues[k]) == 0) {
      r++;
      continue;
    }

    
    while (r > 15 && k <= EOB) {
      
      emit_eobrun(entropy);
      
      emit_symbol(entropy, entropy->ac_tbl_no, 0xF0);
      r -= 16;
      
      emit_buffered_bits(entropy, BR_buffer, BR);
      BR_buffer = entropy->bit_buffer; 
      BR = 0;
    }

    




    if (temp > 1) {
      
      BR_buffer[BR++] = (char) (temp & 1);
      continue;
    }

    
    emit_eobrun(entropy);

    
    emit_symbol(entropy, entropy->ac_tbl_no, (r << 4) + 1);

    
    temp = ((*block)[jpeg_natural_order[k]] < 0) ? 0 : 1;
    emit_bits(entropy, (unsigned int) temp, 1);

    
    emit_buffered_bits(entropy, BR_buffer, BR);
    BR_buffer = entropy->bit_buffer; 
    BR = 0;
    r = 0;			
  }

  if (r > 0 || BR > 0) {	
    entropy->EOBRUN++;		
    entropy->BE += BR;		
    



    if (entropy->EOBRUN == 0x7FFF || entropy->BE > (MAX_CORR_BITS-DCTSIZE2+1))
      emit_eobrun(entropy);
  }

  cinfo->dest->next_output_byte = entropy->next_output_byte;
  cinfo->dest->free_in_buffer = entropy->free_in_buffer;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      entropy->restarts_to_go = cinfo->restart_interval;
      entropy->next_restart_num++;
      entropy->next_restart_num &= 7;
    }
    entropy->restarts_to_go--;
  }

  return TRUE;
}






METHODDEF(void)
finish_pass_phuff (j_compress_ptr cinfo)
{   
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;

  entropy->next_output_byte = cinfo->dest->next_output_byte;
  entropy->free_in_buffer = cinfo->dest->free_in_buffer;

  
  emit_eobrun(entropy);
  flush_bits(entropy);

  cinfo->dest->next_output_byte = entropy->next_output_byte;
  cinfo->dest->free_in_buffer = entropy->free_in_buffer;
}






METHODDEF(void)
finish_pass_gather_phuff (j_compress_ptr cinfo)
{
  phuff_entropy_ptr entropy = (phuff_entropy_ptr) cinfo->entropy;
  boolean is_DC_band;
  int ci, tbl;
  jpeg_component_info * compptr;
  JHUFF_TBL **htblptr;
  boolean did[NUM_HUFF_TBLS];

  
  emit_eobrun(entropy);

  is_DC_band = (cinfo->Ss == 0);

  


  MEMZERO(did, SIZEOF(did));

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (is_DC_band) {
      if (cinfo->Ah != 0)	
	continue;
      tbl = compptr->dc_tbl_no;
    } else {
      tbl = compptr->ac_tbl_no;
    }
    if (! did[tbl]) {
      if (is_DC_band)
        htblptr = & cinfo->dc_huff_tbl_ptrs[tbl];
      else
        htblptr = & cinfo->ac_huff_tbl_ptrs[tbl];
      if (*htblptr == NULL)
        *htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
      jpeg_gen_optimal_table(cinfo, *htblptr, entropy->count_ptrs[tbl]);
      did[tbl] = TRUE;
    }
  }
}






GLOBAL(void)
jinit_phuff_encoder (j_compress_ptr cinfo)
{
  phuff_entropy_ptr entropy;
  int i;

  entropy = (phuff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(phuff_entropy_encoder));
  cinfo->entropy = (struct jpeg_entropy_encoder *) entropy;
  entropy->pub.start_pass = start_pass_phuff;

  
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->derived_tbls[i] = NULL;
    entropy->count_ptrs[i] = NULL;
  }
  entropy->bit_buffer = NULL;	
}

#endif 
