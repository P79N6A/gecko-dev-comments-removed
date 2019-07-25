















#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		
#include "jsimddct.h"




typedef JMETHOD(void, forward_DCT_method_ptr, (DCTELEM * data));
typedef JMETHOD(void, float_DCT_method_ptr, (FAST_FLOAT * data));

typedef JMETHOD(void, convsamp_method_ptr,
                (JSAMPARRAY sample_data, JDIMENSION start_col,
                 DCTELEM * workspace));
typedef JMETHOD(void, float_convsamp_method_ptr,
                (JSAMPARRAY sample_data, JDIMENSION start_col,
                 FAST_FLOAT *workspace));

typedef JMETHOD(void, quantize_method_ptr,
                (JCOEFPTR coef_block, DCTELEM * divisors,
                 DCTELEM * workspace));
typedef JMETHOD(void, float_quantize_method_ptr,
                (JCOEFPTR coef_block, FAST_FLOAT * divisors,
                 FAST_FLOAT * workspace));

METHODDEF(void) quantize (JCOEFPTR, DCTELEM *, DCTELEM *);

typedef struct {
  struct jpeg_forward_dct pub;	

  
  forward_DCT_method_ptr dct;
  convsamp_method_ptr convsamp;
  quantize_method_ptr quantize;

  



  DCTELEM * divisors[NUM_QUANT_TBLS];

  
  DCTELEM * workspace;

#ifdef DCT_FLOAT_SUPPORTED
  
  float_DCT_method_ptr float_dct;
  float_convsamp_method_ptr float_convsamp;
  float_quantize_method_ptr float_quantize;
  FAST_FLOAT * float_divisors[NUM_QUANT_TBLS];
  FAST_FLOAT * float_workspace;
#endif
} my_fdct_controller;

typedef my_fdct_controller * my_fdct_ptr;





LOCAL(int)
flss (UINT16 val)
{
  int bit;

  bit = 16;

  if (!val)
    return 0;

  if (!(val & 0xff00)) {
    bit -= 8;
    val <<= 8;
  }
  if (!(val & 0xf000)) {
    bit -= 4;
    val <<= 4;
  }
  if (!(val & 0xc000)) {
    bit -= 2;
    val <<= 2;
  }
  if (!(val & 0x8000)) {
    bit -= 1;
    val <<= 1;
  }

  return bit;
}



























































LOCAL(int)
compute_reciprocal (UINT16 divisor, DCTELEM * dtbl)
{
  UDCTELEM2 fq, fr;
  UDCTELEM c;
  int b, r;

  b = flss(divisor) - 1;
  r  = sizeof(DCTELEM) * 8 + b;

  fq = ((UDCTELEM2)1 << r) / divisor;
  fr = ((UDCTELEM2)1 << r) % divisor;

  c = divisor / 2; 

  if (fr == 0) { 
    
    fq >>= 1;
    r--;
  } else if (fr <= (divisor / 2)) { 
    c++;
  } else { 
    fq++;
  }

  dtbl[DCTSIZE2 * 0] = (DCTELEM) fq;      
  dtbl[DCTSIZE2 * 1] = (DCTELEM) c;       
  dtbl[DCTSIZE2 * 2] = (DCTELEM) (1 << (sizeof(DCTELEM)*8*2 - r));  
  dtbl[DCTSIZE2 * 3] = (DCTELEM) r - sizeof(DCTELEM)*8; 

  if(r <= 16) return 0;
  else return 1;
}










METHODDEF(void)
start_pass_fdctmgr (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  int ci, qtblno, i;
  jpeg_component_info *compptr;
  JQUANT_TBL * qtbl;
  DCTELEM * dtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    qtblno = compptr->quant_tbl_no;
    
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    qtbl = cinfo->quant_tbl_ptrs[qtblno];
    
    
    switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
    case JDCT_ISLOW:
      


      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      (DCTSIZE2 * 4) * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	if(!compute_reciprocal(qtbl->quantval[i] << 3, &dtbl[i])
	  && fdct->quantize == jsimd_quantize)
	  fdct->quantize = quantize;
      }
      break;
#endif
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	





#define CONST_BITS 14
	static const INT16 aanscales[DCTSIZE2] = {
	  
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT_TEMPS

	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					(DCTSIZE2 * 4) * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  if(!compute_reciprocal(
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-3), &dtbl[i])
	    && fdct->quantize == jsimd_quantize)
	    fdct->quantize = quantize;
	}
      }
      break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	







	FAST_FLOAT * fdtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	if (fdct->float_divisors[qtblno] == NULL) {
	  fdct->float_divisors[qtblno] = (FAST_FLOAT *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(FAST_FLOAT));
	}
	fdtbl = fdct->float_divisors[qtblno];
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fdtbl[i] = (FAST_FLOAT)
	      (1.0 / (((double) qtbl->quantval[i] *
		       aanscalefactor[row] * aanscalefactor[col] * 8.0)));
	    i++;
	  }
	}
      }
      break;
#endif
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}






METHODDEF(void)
convsamp (JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM * workspace)
{
  register DCTELEM *workspaceptr;
  register JSAMPROW elemptr;
  register int elemr;

  workspaceptr = workspace;
  for (elemr = 0; elemr < DCTSIZE; elemr++) {
    elemptr = sample_data[elemr] + start_col;

#if DCTSIZE == 8		
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
#else
    {
      register int elemc;
      for (elemc = DCTSIZE; elemc > 0; elemc--)
        *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
    }
#endif
  }
}






METHODDEF(void)
quantize (JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace)
{
  int i;
  DCTELEM temp;
  UDCTELEM recip, corr, shift;
  UDCTELEM2 product;
  JCOEFPTR output_ptr = coef_block;

  for (i = 0; i < DCTSIZE2; i++) {
    temp = workspace[i];
    recip = divisors[i + DCTSIZE2 * 0];
    corr =  divisors[i + DCTSIZE2 * 1];
    shift = divisors[i + DCTSIZE2 * 3];

    if (temp < 0) {
      temp = -temp;
      product = (UDCTELEM2)(temp + corr) * recip;
      product >>= shift + sizeof(DCTELEM)*8;
      temp = product;
      temp = -temp;
    } else {
      product = (UDCTELEM2)(temp + corr) * recip;
      product >>= shift + sizeof(DCTELEM)*8;
      temp = product;
    }

    output_ptr[i] = (JCOEF) temp;
  }
}










METHODDEF(void)
forward_DCT (j_compress_ptr cinfo, jpeg_component_info * compptr,
	     JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
	     JDIMENSION start_row, JDIMENSION start_col,
	     JDIMENSION num_blocks)

{
  
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
  DCTELEM * workspace;
  JDIMENSION bi;

  
  forward_DCT_method_ptr do_dct = fdct->dct;
  convsamp_method_ptr do_convsamp = fdct->convsamp;
  quantize_method_ptr do_quantize = fdct->quantize;
  workspace = fdct->workspace;

  sample_data += start_row;	

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    
    (*do_convsamp) (sample_data, start_col, workspace);

    
    (*do_dct) (workspace);

    
    (*do_quantize) (coef_blocks[bi], divisors, workspace);
  }
}


#ifdef DCT_FLOAT_SUPPORTED


METHODDEF(void)
convsamp_float (JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT * workspace)
{
  register FAST_FLOAT *workspaceptr;
  register JSAMPROW elemptr;
  register int elemr;

  workspaceptr = workspace;
  for (elemr = 0; elemr < DCTSIZE; elemr++) {
    elemptr = sample_data[elemr] + start_col;
#if DCTSIZE == 8		
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    *workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
#else
    {
      register int elemc;
      for (elemc = DCTSIZE; elemc > 0; elemc--)
        *workspaceptr++ = (FAST_FLOAT)
                          (GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
    }
#endif
  }
}


METHODDEF(void)
quantize_float (JCOEFPTR coef_block, FAST_FLOAT * divisors, FAST_FLOAT * workspace)
{
  register FAST_FLOAT temp;
  register int i;
  register JCOEFPTR output_ptr = coef_block;

  for (i = 0; i < DCTSIZE2; i++) {
    
    temp = workspace[i] * divisors[i];

    





    output_ptr[i] = (JCOEF) ((int) (temp + (FAST_FLOAT) 16384.5) - 16384);
  }
}


METHODDEF(void)
forward_DCT_float (j_compress_ptr cinfo, jpeg_component_info * compptr,
		   JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
		   JDIMENSION start_row, JDIMENSION start_col,
		   JDIMENSION num_blocks)

{
  
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  FAST_FLOAT * divisors = fdct->float_divisors[compptr->quant_tbl_no];
  FAST_FLOAT * workspace;
  JDIMENSION bi;


  
  float_DCT_method_ptr do_dct = fdct->float_dct;
  float_convsamp_method_ptr do_convsamp = fdct->float_convsamp;
  float_quantize_method_ptr do_quantize = fdct->float_quantize;
  workspace = fdct->float_workspace;

  sample_data += start_row;	

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    
    (*do_convsamp) (sample_data, start_col, workspace);

    
    (*do_dct) (workspace);

    
    (*do_quantize) (coef_blocks[bi], divisors, workspace);
  }
}

#endif 






GLOBAL(void)
jinit_forward_dct (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct;
  int i;

  fdct = (my_fdct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_fdct_controller));
  cinfo->fdct = (struct jpeg_forward_dct *) fdct;
  fdct->pub.start_pass = start_pass_fdctmgr;

  
  switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
  case JDCT_ISLOW:
    fdct->pub.forward_DCT = forward_DCT;
    if (jsimd_can_fdct_islow())
      fdct->dct = jsimd_fdct_islow;
    else
      fdct->dct = jpeg_fdct_islow;
    break;
#endif
#ifdef DCT_IFAST_SUPPORTED
  case JDCT_IFAST:
    fdct->pub.forward_DCT = forward_DCT;
    if (jsimd_can_fdct_ifast())
      fdct->dct = jsimd_fdct_ifast;
    else
      fdct->dct = jpeg_fdct_ifast;
    break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
  case JDCT_FLOAT:
    fdct->pub.forward_DCT = forward_DCT_float;
    if (jsimd_can_fdct_float())
      fdct->float_dct = jsimd_fdct_float;
    else
      fdct->float_dct = jpeg_fdct_float;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }

  
  switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
  case JDCT_ISLOW:
#endif
#ifdef DCT_IFAST_SUPPORTED
  case JDCT_IFAST:
#endif
#if defined(DCT_ISLOW_SUPPORTED) || defined(DCT_IFAST_SUPPORTED)
    if (jsimd_can_convsamp())
      fdct->convsamp = jsimd_convsamp;
    else
      fdct->convsamp = convsamp;
    if (jsimd_can_quantize())
      fdct->quantize = jsimd_quantize;
    else
      fdct->quantize = quantize;
    break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
  case JDCT_FLOAT:
    if (jsimd_can_convsamp_float())
      fdct->float_convsamp = jsimd_convsamp_float;
    else
      fdct->float_convsamp = convsamp_float;
    if (jsimd_can_quantize_float())
      fdct->float_quantize = jsimd_quantize_float;
    else
      fdct->float_quantize = quantize_float;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }

  
#ifdef DCT_FLOAT_SUPPORTED
  if (cinfo->dct_method == JDCT_FLOAT)
    fdct->float_workspace = (FAST_FLOAT *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(FAST_FLOAT) * DCTSIZE2);
  else
#endif
    fdct->workspace = (DCTELEM *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(DCTELEM) * DCTSIZE2);

  
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    fdct->divisors[i] = NULL;
#ifdef DCT_FLOAT_SUPPORTED
    fdct->float_divisors[i] = NULL;
#endif
  }
}
