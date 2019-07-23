















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jchuff.h"		








typedef struct {
  INT32 put_buffer;		
  int put_bits;			
  int last_dc_val[MAX_COMPS_IN_SCAN]; 
} savable_state;






#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).put_buffer = (src).put_buffer, \
	 (dest).put_bits = (src).put_bits, \
	 (dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif


typedef struct {
  struct jpeg_entropy_encoder pub; 

  savable_state saved;		

  
  unsigned int restarts_to_go;	
  int next_restart_num;		

  
  c_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
  c_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];

#ifdef ENTROPY_OPT_SUPPORTED	
  long * dc_count_ptrs[NUM_HUFF_TBLS];
  long * ac_count_ptrs[NUM_HUFF_TBLS];
#endif
} huff_entropy_encoder;

typedef huff_entropy_encoder * huff_entropy_ptr;





typedef struct {
  JOCTET * next_output_byte;	
  size_t free_in_buffer;	
  savable_state cur;		
  j_compress_ptr cinfo;		
} working_state;



METHODDEF(boolean) encode_mcu_huff JPP((j_compress_ptr cinfo,
					JBLOCKROW *MCU_data));
METHODDEF(void) finish_pass_huff JPP((j_compress_ptr cinfo));
#ifdef ENTROPY_OPT_SUPPORTED
METHODDEF(boolean) encode_mcu_gather JPP((j_compress_ptr cinfo,
					  JBLOCKROW *MCU_data));
METHODDEF(void) finish_pass_gather JPP((j_compress_ptr cinfo));
#endif








METHODDEF(void)
start_pass_huff (j_compress_ptr cinfo, boolean gather_statistics)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;

  if (gather_statistics) {
#ifdef ENTROPY_OPT_SUPPORTED
    entropy->pub.encode_mcu = encode_mcu_gather;
    entropy->pub.finish_pass = finish_pass_gather;
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    entropy->pub.encode_mcu = encode_mcu_huff;
    entropy->pub.finish_pass = finish_pass_huff;
  }

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    if (gather_statistics) {
#ifdef ENTROPY_OPT_SUPPORTED
      
      
      if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS)
	ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
      if (actbl < 0 || actbl >= NUM_HUFF_TBLS)
	ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, actbl);
      
      
      if (entropy->dc_count_ptrs[dctbl] == NULL)
	entropy->dc_count_ptrs[dctbl] = (long *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->dc_count_ptrs[dctbl], 257 * SIZEOF(long));
      if (entropy->ac_count_ptrs[actbl] == NULL)
	entropy->ac_count_ptrs[actbl] = (long *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->ac_count_ptrs[actbl], 257 * SIZEOF(long));
#endif
    } else {
      
      
      jpeg_make_c_derived_tbl(cinfo, TRUE, dctbl,
			      & entropy->dc_derived_tbls[dctbl]);
      jpeg_make_c_derived_tbl(cinfo, FALSE, actbl,
			      & entropy->ac_derived_tbls[actbl]);
    }
    
    entropy->saved.last_dc_val[ci] = 0;
  }

  
  entropy->saved.put_buffer = 0;
  entropy->saved.put_bits = 0;

  
  entropy->restarts_to_go = cinfo->restart_interval;
  entropy->next_restart_num = 0;
}









GLOBAL(void)
jpeg_make_c_derived_tbl (j_compress_ptr cinfo, boolean isDC, int tblno,
			 c_derived_tbl ** pdtbl)
{
  JHUFF_TBL *htbl;
  c_derived_tbl *dtbl;
  int p, i, l, lastp, si, maxsymbol;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  



  
  if (tblno < 0 || tblno >= NUM_HUFF_TBLS)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);
  htbl =
    isDC ? cinfo->dc_huff_tbl_ptrs[tblno] : cinfo->ac_huff_tbl_ptrs[tblno];
  if (htbl == NULL)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);

  
  if (*pdtbl == NULL)
    *pdtbl = (c_derived_tbl *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(c_derived_tbl));
  dtbl = *pdtbl;
  
  

  p = 0;
  for (l = 1; l <= 16; l++) {
    i = (int) htbl->bits[l];
    if (i < 0 || p + i > 256)	
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    while (i--)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  lastp = p;
  
  
  

  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    


    if (((INT32) code) >= (((INT32) 1) << si))
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    code <<= 1;
    si++;
  }
  
  
  

  



  MEMZERO(dtbl->ehufsi, SIZEOF(dtbl->ehufsi));

  




  maxsymbol = isDC ? 15 : 255;

  for (p = 0; p < lastp; p++) {
    i = htbl->huffval[p];
    if (i < 0 || i > maxsymbol || dtbl->ehufsi[i])
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    dtbl->ehufco[i] = huffcode[p];
    dtbl->ehufsi[i] = huffsize[p];
  }
}





#define emit_byte(state,val,action)  \
	{ *(state)->next_output_byte++ = (JOCTET) (val);  \
	  if (--(state)->free_in_buffer == 0)  \
	    if (! dump_buffer(state))  \
	      { action; } }


LOCAL(boolean)
dump_buffer (working_state * state)

{
  struct jpeg_destination_mgr * dest = state->cinfo->dest;

  if (! (*dest->empty_output_buffer) (state->cinfo))
    return FALSE;
  
  state->next_output_byte = dest->next_output_byte;
  state->free_in_buffer = dest->free_in_buffer;
  return TRUE;
}










INLINE
LOCAL(boolean)
emit_bits (working_state * state, unsigned int code, int size)

{
  
  register INT32 put_buffer = (INT32) code;
  register int put_bits = state->cur.put_bits;

  
  if (size == 0)
    ERREXIT(state->cinfo, JERR_HUFF_MISSING_CODE);

  put_buffer &= (((INT32) 1)<<size) - 1; 
  
  put_bits += size;		
  
  put_buffer <<= 24 - put_bits; 

  put_buffer |= state->cur.put_buffer; 
  
  while (put_bits >= 8) {
    int c = (int) ((put_buffer >> 16) & 0xFF);
    
    emit_byte(state, c, return FALSE);
    if (c == 0xFF) {		
      emit_byte(state, 0, return FALSE);
    }
    put_buffer <<= 8;
    put_bits -= 8;
  }

  state->cur.put_buffer = put_buffer; 
  state->cur.put_bits = put_bits;

  return TRUE;
}


LOCAL(boolean)
flush_bits (working_state * state)
{
  if (! emit_bits(state, 0x7F, 7)) 
    return FALSE;
  state->cur.put_buffer = 0;	
  state->cur.put_bits = 0;
  return TRUE;
}




LOCAL(boolean)
encode_one_block (working_state * state, JCOEFPTR block, int last_dc_val,
		  c_derived_tbl *dctbl, c_derived_tbl *actbl)
{
  register int temp, temp2;
  register int nbits;
  register int k, r, i;
  
  
  
  temp = temp2 = block[0] - last_dc_val;

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
    ERREXIT(state->cinfo, JERR_BAD_DCT_COEF);
  
  
  if (! emit_bits(state, dctbl->ehufco[nbits], dctbl->ehufsi[nbits]))
    return FALSE;

  
  
  if (nbits)			
    if (! emit_bits(state, (unsigned int) temp2, nbits))
      return FALSE;

  
  
  r = 0;			
  
  for (k = 1; k < DCTSIZE2; k++) {
    if ((temp = block[jpeg_natural_order[k]]) == 0) {
      r++;
    } else {
      
      while (r > 15) {
	if (! emit_bits(state, actbl->ehufco[0xF0], actbl->ehufsi[0xF0]))
	  return FALSE;
	r -= 16;
      }

      temp2 = temp;
      if (temp < 0) {
	temp = -temp;		
	
	temp2--;
      }
      
      
      nbits = 1;		
      while ((temp >>= 1))
	nbits++;
      
      if (nbits > MAX_COEF_BITS)
	ERREXIT(state->cinfo, JERR_BAD_DCT_COEF);
      
      
      i = (r << 4) + nbits;
      if (! emit_bits(state, actbl->ehufco[i], actbl->ehufsi[i]))
	return FALSE;

      
      
      if (! emit_bits(state, (unsigned int) temp2, nbits))
	return FALSE;
      
      r = 0;
    }
  }

  
  if (r > 0)
    if (! emit_bits(state, actbl->ehufco[0], actbl->ehufsi[0]))
      return FALSE;

  return TRUE;
}






LOCAL(boolean)
emit_restart (working_state * state, int restart_num)
{
  int ci;

  if (! flush_bits(state))
    return FALSE;

  emit_byte(state, 0xFF, return FALSE);
  emit_byte(state, JPEG_RST0 + restart_num, return FALSE);

  
  for (ci = 0; ci < state->cinfo->comps_in_scan; ci++)
    state->cur.last_dc_val[ci] = 0;

  

  return TRUE;
}






METHODDEF(boolean)
encode_mcu_huff (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  working_state state;
  int blkn, ci;
  jpeg_component_info * compptr;

  
  state.next_output_byte = cinfo->dest->next_output_byte;
  state.free_in_buffer = cinfo->dest->free_in_buffer;
  ASSIGN_STATE(state.cur, entropy->saved);
  state.cinfo = cinfo;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! emit_restart(&state, entropy->next_restart_num))
	return FALSE;
  }

  
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    if (! encode_one_block(&state,
			   MCU_data[blkn][0], state.cur.last_dc_val[ci],
			   entropy->dc_derived_tbls[compptr->dc_tbl_no],
			   entropy->ac_derived_tbls[compptr->ac_tbl_no]))
      return FALSE;
    
    state.cur.last_dc_val[ci] = MCU_data[blkn][0][0];
  }

  
  cinfo->dest->next_output_byte = state.next_output_byte;
  cinfo->dest->free_in_buffer = state.free_in_buffer;
  ASSIGN_STATE(entropy->saved, state.cur);

  
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
finish_pass_huff (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  working_state state;

  
  state.next_output_byte = cinfo->dest->next_output_byte;
  state.free_in_buffer = cinfo->dest->free_in_buffer;
  ASSIGN_STATE(state.cur, entropy->saved);
  state.cinfo = cinfo;

  
  if (! flush_bits(&state))
    ERREXIT(cinfo, JERR_CANT_SUSPEND);

  
  cinfo->dest->next_output_byte = state.next_output_byte;
  cinfo->dest->free_in_buffer = state.free_in_buffer;
  ASSIGN_STATE(entropy->saved, state.cur);
}













#ifdef ENTROPY_OPT_SUPPORTED




LOCAL(void)
htest_one_block (j_compress_ptr cinfo, JCOEFPTR block, int last_dc_val,
		 long dc_counts[], long ac_counts[])
{
  register int temp;
  register int nbits;
  register int k, r;
  
  
  
  temp = block[0] - last_dc_val;
  if (temp < 0)
    temp = -temp;
  
  
  nbits = 0;
  while (temp) {
    nbits++;
    temp >>= 1;
  }
  


  if (nbits > MAX_COEF_BITS+1)
    ERREXIT(cinfo, JERR_BAD_DCT_COEF);

  
  dc_counts[nbits]++;
  
  
  
  r = 0;			
  
  for (k = 1; k < DCTSIZE2; k++) {
    if ((temp = block[jpeg_natural_order[k]]) == 0) {
      r++;
    } else {
      
      while (r > 15) {
	ac_counts[0xF0]++;
	r -= 16;
      }
      
      
      if (temp < 0)
	temp = -temp;
      
      
      nbits = 1;		
      while ((temp >>= 1))
	nbits++;
      
      if (nbits > MAX_COEF_BITS)
	ERREXIT(cinfo, JERR_BAD_DCT_COEF);
      
      
      ac_counts[(r << 4) + nbits]++;
      
      r = 0;
    }
  }

  
  if (r > 0)
    ac_counts[0]++;
}







METHODDEF(boolean)
encode_mcu_gather (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int blkn, ci;
  jpeg_component_info * compptr;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      
      for (ci = 0; ci < cinfo->comps_in_scan; ci++)
	entropy->saved.last_dc_val[ci] = 0;
      
      entropy->restarts_to_go = cinfo->restart_interval;
    }
    entropy->restarts_to_go--;
  }

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    htest_one_block(cinfo, MCU_data[blkn][0], entropy->saved.last_dc_val[ci],
		    entropy->dc_count_ptrs[compptr->dc_tbl_no],
		    entropy->ac_count_ptrs[compptr->ac_tbl_no]);
    entropy->saved.last_dc_val[ci] = MCU_data[blkn][0][0];
  }

  return TRUE;
}






























GLOBAL(void)
jpeg_gen_optimal_table (j_compress_ptr cinfo, JHUFF_TBL * htbl, long freq[])
{
#define MAX_CLEN 32		/* assumed maximum initial code length */
  UINT8 bits[MAX_CLEN+1];	
  int codesize[257];		
  int others[257];		
  int c1, c2;
  int p, i, j;
  long v;

  

  MEMZERO(bits, SIZEOF(bits));
  MEMZERO(codesize, SIZEOF(codesize));
  for (i = 0; i < 257; i++)
    others[i] = -1;		
  
  freq[256] = 1;		
  




  

  for (;;) {
    
    
    c1 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v) {
	v = freq[i];
	c1 = i;
      }
    }

    
    
    c2 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v && i != c1) {
	v = freq[i];
	c2 = i;
      }
    }

    
    if (c2 < 0)
      break;
    
    
    freq[c1] += freq[c2];
    freq[c2] = 0;

    
    codesize[c1]++;
    while (others[c1] >= 0) {
      c1 = others[c1];
      codesize[c1]++;
    }
    
    others[c1] = c2;		
    
    
    codesize[c2]++;
    while (others[c2] >= 0) {
      c2 = others[c2];
      codesize[c2]++;
    }
  }

  
  for (i = 0; i <= 256; i++) {
    if (codesize[i]) {
      
      
      if (codesize[i] > MAX_CLEN)
	ERREXIT(cinfo, JERR_HUFF_CLEN_OVERFLOW);

      bits[codesize[i]]++;
    }
  }

  









  
  for (i = MAX_CLEN; i > 16; i--) {
    while (bits[i] > 0) {
      j = i - 2;		
      while (bits[j] == 0)
	j--;
      
      bits[i] -= 2;		
      bits[i-1]++;		
      bits[j+1] += 2;		
      bits[j]--;		
    }
  }

  
  while (bits[i] == 0)		
    i--;
  bits[i]--;
  
  
  MEMCOPY(htbl->bits, bits, SIZEOF(htbl->bits));
  
  
  


  p = 0;
  for (i = 1; i <= MAX_CLEN; i++) {
    for (j = 0; j <= 255; j++) {
      if (codesize[j] == i) {
	htbl->huffval[p] = (UINT8) j;
	p++;
      }
    }
  }

  
  htbl->sent_table = FALSE;
}






METHODDEF(void)
finish_pass_gather (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;
  JHUFF_TBL **htblptr;
  boolean did_dc[NUM_HUFF_TBLS];
  boolean did_ac[NUM_HUFF_TBLS];

  


  MEMZERO(did_dc, SIZEOF(did_dc));
  MEMZERO(did_ac, SIZEOF(did_ac));

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    if (! did_dc[dctbl]) {
      htblptr = & cinfo->dc_huff_tbl_ptrs[dctbl];
      if (*htblptr == NULL)
	*htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
      jpeg_gen_optimal_table(cinfo, *htblptr, entropy->dc_count_ptrs[dctbl]);
      did_dc[dctbl] = TRUE;
    }
    if (! did_ac[actbl]) {
      htblptr = & cinfo->ac_huff_tbl_ptrs[actbl];
      if (*htblptr == NULL)
	*htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
      jpeg_gen_optimal_table(cinfo, *htblptr, entropy->ac_count_ptrs[actbl]);
      did_ac[actbl] = TRUE;
    }
  }
}


#endif 






GLOBAL(void)
jinit_huff_encoder (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_encoder));
  cinfo->entropy = (struct jpeg_entropy_encoder *) entropy;
  entropy->pub.start_pass = start_pass_huff;

  
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
#ifdef ENTROPY_OPT_SUPPORTED
    entropy->dc_count_ptrs[i] = entropy->ac_count_ptrs[i] = NULL;
#endif
  }
}
