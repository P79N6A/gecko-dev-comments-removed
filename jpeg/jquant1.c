











#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef QUANT_1PASS_SUPPORTED





















































#define ODITHER_SIZE  16	/* dimension of dither matrix */

#define ODITHER_CELLS (ODITHER_SIZE*ODITHER_SIZE)	/* # cells in matrix */
#define ODITHER_MASK  (ODITHER_SIZE-1) /* mask for wrapping around counters */

typedef int ODITHER_MATRIX[ODITHER_SIZE][ODITHER_SIZE];
typedef int (*ODITHER_MATRIX_PTR)[ODITHER_SIZE];

static const UINT8 base_dither_matrix[ODITHER_SIZE][ODITHER_SIZE] = {
  



  {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
  { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
  {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
  { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
  {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
  { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
  {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
  { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
  {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
  { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
  {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
  { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
  {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
  { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
  {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
  { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
};


























#if BITS_IN_JSAMPLE == 8
typedef INT16 FSERROR;		
typedef int LOCFSERROR;		
#else
typedef INT32 FSERROR;		
typedef INT32 LOCFSERROR;	
#endif

typedef FSERROR FAR *FSERRPTR;	




#define MAX_Q_COMPS 4		/* max components I can handle */

typedef struct {
  struct jpeg_color_quantizer pub; 

  
  JSAMPARRAY sv_colormap;	
  int sv_actual;		

  JSAMPARRAY colorindex;	
  



  boolean is_padded;		

  int Ncolors[MAX_Q_COMPS];	

  
  int row_index;		
  ODITHER_MATRIX_PTR odither[MAX_Q_COMPS]; 

  
  FSERRPTR fserrors[MAX_Q_COMPS]; 
  boolean on_odd_row;		
} my_cquantizer;

typedef my_cquantizer * my_cquantize_ptr;

















LOCAL(int)
select_ncolors (j_decompress_ptr cinfo, int Ncolors[])



{
  int nc = cinfo->out_color_components; 
  int max_colors = cinfo->desired_number_of_colors;
  int total_colors, iroot, i, j;
  boolean changed;
  long temp;
  static const int RGB_order[3] = { RGB_GREEN, RGB_RED, RGB_BLUE };

  
  
  iroot = 1;
  do {
    iroot++;
    temp = iroot;		
    for (i = 1; i < nc; i++)
      temp *= iroot;
  } while (temp <= (long) max_colors); 
  iroot--;			

  
  if (iroot < 2)
    ERREXIT1(cinfo, JERR_QUANT_FEW_COLORS, (int) temp);

  
  total_colors = 1;
  for (i = 0; i < nc; i++) {
    Ncolors[i] = iroot;
    total_colors *= iroot;
  }
  





  do {
    changed = FALSE;
    for (i = 0; i < nc; i++) {
      j = (cinfo->out_color_space == JCS_RGB ? RGB_order[i] : i);
      
      temp = total_colors / Ncolors[j];
      temp *= Ncolors[j]+1;	
      if (temp > (long) max_colors)
	break;			
      Ncolors[j]++;		
      total_colors = (int) temp;
      changed = TRUE;
    }
  } while (changed);

  return total_colors;
}


LOCAL(int)
output_value (j_decompress_ptr cinfo, int ci, int j, int maxj)


{
  




  return (int) (((INT32) j * MAXJSAMPLE + maxj/2) / maxj);
}


LOCAL(int)
largest_input_value (j_decompress_ptr cinfo, int ci, int j, int maxj)


{
  
  return (int) (((INT32) (2*j + 1) * MAXJSAMPLE + maxj) / (2*maxj));
}






LOCAL(void)
create_colormap (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  JSAMPARRAY colormap;		
  int total_colors;		
  int i,j,k, nci, blksize, blkdist, ptr, val;

  
  total_colors = select_ncolors(cinfo, cquantize->Ncolors);

  
  if (cinfo->out_color_components == 3)
    TRACEMS4(cinfo, 1, JTRC_QUANT_3_NCOLORS,
	     total_colors, cquantize->Ncolors[0],
	     cquantize->Ncolors[1], cquantize->Ncolors[2]);
  else
    TRACEMS1(cinfo, 1, JTRC_QUANT_NCOLORS, total_colors);

  
  
  

  colormap = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     (JDIMENSION) total_colors, (JDIMENSION) cinfo->out_color_components);

  
  
  blkdist = total_colors;

  for (i = 0; i < cinfo->out_color_components; i++) {
    
    nci = cquantize->Ncolors[i]; 
    blksize = blkdist / nci;
    for (j = 0; j < nci; j++) {
      
      val = output_value(cinfo, i, j, nci-1);
      
      for (ptr = j * blksize; ptr < total_colors; ptr += blkdist) {
	
	for (k = 0; k < blksize; k++)
	  colormap[i][ptr+k] = (JSAMPLE) val;
      }
    }
    blkdist = blksize;		
  }

  


  cquantize->sv_colormap = colormap;
  cquantize->sv_actual = total_colors;
}






LOCAL(void)
create_colorindex (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  JSAMPROW indexptr;
  int i,j,k, nci, blksize, val, pad;

  




  if (cinfo->dither_mode == JDITHER_ORDERED) {
    pad = MAXJSAMPLE*2;
    cquantize->is_padded = TRUE;
  } else {
    pad = 0;
    cquantize->is_padded = FALSE;
  }

  cquantize->colorindex = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     (JDIMENSION) (MAXJSAMPLE+1 + pad),
     (JDIMENSION) cinfo->out_color_components);

  
  blksize = cquantize->sv_actual;

  for (i = 0; i < cinfo->out_color_components; i++) {
    
    nci = cquantize->Ncolors[i]; 
    blksize = blksize / nci;

    
    if (pad)
      cquantize->colorindex[i] += MAXJSAMPLE;

    
    
    indexptr = cquantize->colorindex[i];
    val = 0;
    k = largest_input_value(cinfo, i, 0, nci-1);
    for (j = 0; j <= MAXJSAMPLE; j++) {
      while (j > k)		
	k = largest_input_value(cinfo, i, ++val, nci-1);
      
      indexptr[j] = (JSAMPLE) (val * blksize);
    }
    
    if (pad)
      for (j = 1; j <= MAXJSAMPLE; j++) {
	indexptr[-j] = indexptr[0];
	indexptr[MAXJSAMPLE+j] = indexptr[MAXJSAMPLE];
      }
  }
}







LOCAL(ODITHER_MATRIX_PTR)
make_odither_array (j_decompress_ptr cinfo, int ncolors)
{
  ODITHER_MATRIX_PTR odither;
  int j,k;
  INT32 num,den;

  odither = (ODITHER_MATRIX_PTR)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(ODITHER_MATRIX));
  




  den = 2 * ODITHER_CELLS * ((INT32) (ncolors - 1));
  for (j = 0; j < ODITHER_SIZE; j++) {
    for (k = 0; k < ODITHER_SIZE; k++) {
      num = ((INT32) (ODITHER_CELLS-1 - 2*((int)base_dither_matrix[j][k])))
	    * MAXJSAMPLE;
      


      odither[j][k] = (int) (num<0 ? -((-num)/den) : num/den);
    }
  }
  return odither;
}








LOCAL(void)
create_odither_tables (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  ODITHER_MATRIX_PTR odither;
  int i, j, nci;

  for (i = 0; i < cinfo->out_color_components; i++) {
    nci = cquantize->Ncolors[i]; 
    odither = NULL;		
    for (j = 0; j < i; j++) {
      if (nci == cquantize->Ncolors[j]) {
	odither = cquantize->odither[j];
	break;
      }
    }
    if (odither == NULL)	
      odither = make_odither_array(cinfo, nci);
    cquantize->odither[i] = odither;
  }
}






METHODDEF(void)
color_quantize (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		JSAMPARRAY output_buf, int num_rows)

{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  JSAMPARRAY colorindex = cquantize->colorindex;
  register int pixcode, ci;
  register JSAMPROW ptrin, ptrout;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  register int nc = cinfo->out_color_components;

  for (row = 0; row < num_rows; row++) {
    ptrin = input_buf[row];
    ptrout = output_buf[row];
    for (col = width; col > 0; col--) {
      pixcode = 0;
      for (ci = 0; ci < nc; ci++) {
	pixcode += GETJSAMPLE(colorindex[ci][GETJSAMPLE(*ptrin++)]);
      }
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF(void)
color_quantize3 (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		 JSAMPARRAY output_buf, int num_rows)

{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register int pixcode;
  register JSAMPROW ptrin, ptrout;
  JSAMPROW colorindex0 = cquantize->colorindex[0];
  JSAMPROW colorindex1 = cquantize->colorindex[1];
  JSAMPROW colorindex2 = cquantize->colorindex[2];
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    ptrin = input_buf[row];
    ptrout = output_buf[row];
    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex0[GETJSAMPLE(*ptrin++)]);
      pixcode += GETJSAMPLE(colorindex1[GETJSAMPLE(*ptrin++)]);
      pixcode += GETJSAMPLE(colorindex2[GETJSAMPLE(*ptrin++)]);
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF(void)
quantize_ord_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		     JSAMPARRAY output_buf, int num_rows)

{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex_ci;
  int * dither;			
  int row_index, col_index;	
  int nc = cinfo->out_color_components;
  int ci;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    
    jzero_far((void FAR *) output_buf[row],
	      (size_t) (width * SIZEOF(JSAMPLE)));
    row_index = cquantize->row_index;
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buf[row] + ci;
      output_ptr = output_buf[row];
      colorindex_ci = cquantize->colorindex[ci];
      dither = cquantize->odither[ci][row_index];
      col_index = 0;

      for (col = width; col > 0; col--) {
	






	*output_ptr += colorindex_ci[GETJSAMPLE(*input_ptr)+dither[col_index]];
	input_ptr += nc;
	output_ptr++;
	col_index = (col_index + 1) & ODITHER_MASK;
      }
    }
    
    row_index = (row_index + 1) & ODITHER_MASK;
    cquantize->row_index = row_index;
  }
}


METHODDEF(void)
quantize3_ord_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		      JSAMPARRAY output_buf, int num_rows)

{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register int pixcode;
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex0 = cquantize->colorindex[0];
  JSAMPROW colorindex1 = cquantize->colorindex[1];
  JSAMPROW colorindex2 = cquantize->colorindex[2];
  int * dither0;		
  int * dither1;
  int * dither2;
  int row_index, col_index;	
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    row_index = cquantize->row_index;
    input_ptr = input_buf[row];
    output_ptr = output_buf[row];
    dither0 = cquantize->odither[0][row_index];
    dither1 = cquantize->odither[1][row_index];
    dither2 = cquantize->odither[2][row_index];
    col_index = 0;

    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex0[GETJSAMPLE(*input_ptr++) +
					dither0[col_index]]);
      pixcode += GETJSAMPLE(colorindex1[GETJSAMPLE(*input_ptr++) +
					dither1[col_index]]);
      pixcode += GETJSAMPLE(colorindex2[GETJSAMPLE(*input_ptr++) +
					dither2[col_index]]);
      *output_ptr++ = (JSAMPLE) pixcode;
      col_index = (col_index + 1) & ODITHER_MASK;
    }
    row_index = (row_index + 1) & ODITHER_MASK;
    cquantize->row_index = row_index;
  }
}


METHODDEF(void)
quantize_fs_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		    JSAMPARRAY output_buf, int num_rows)

{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register LOCFSERROR cur;	
  LOCFSERROR belowerr;		
  LOCFSERROR bpreverr;		
  LOCFSERROR bnexterr;		
  LOCFSERROR delta;
  register FSERRPTR errorptr;	
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex_ci;
  JSAMPROW colormap_ci;
  int pixcode;
  int nc = cinfo->out_color_components;
  int dir;			
  int dirnc;			
  int ci;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  SHIFT_TEMPS

  for (row = 0; row < num_rows; row++) {
    
    jzero_far((void FAR *) output_buf[row],
	      (size_t) (width * SIZEOF(JSAMPLE)));
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buf[row] + ci;
      output_ptr = output_buf[row];
      if (cquantize->on_odd_row) {
	
	input_ptr += (width-1) * nc; 
	output_ptr += width-1;
	dir = -1;
	dirnc = -nc;
	errorptr = cquantize->fserrors[ci] + (width+1); 
      } else {
	
	dir = 1;
	dirnc = nc;
	errorptr = cquantize->fserrors[ci]; 
      }
      colorindex_ci = cquantize->colorindex[ci];
      colormap_ci = cquantize->sv_colormap[ci];
      
      cur = 0;
      
      belowerr = bpreverr = 0;

      for (col = width; col > 0; col--) {
	







	cur = RIGHT_SHIFT(cur + errorptr[dir] + 8, 4);
	



	cur += GETJSAMPLE(*input_ptr);
	cur = GETJSAMPLE(range_limit[cur]);
	
	pixcode = GETJSAMPLE(colorindex_ci[cur]);
	*output_ptr += (JSAMPLE) pixcode;
	
	
	
	cur -= GETJSAMPLE(colormap_ci[pixcode]);
	



	bnexterr = cur;
	delta = cur * 2;
	cur += delta;		
	errorptr[0] = (FSERROR) (bpreverr + cur);
	cur += delta;		
	bpreverr = belowerr + cur;
	belowerr = bnexterr;
	cur += delta;		
	



	input_ptr += dirnc;	
	output_ptr += dir;	
	errorptr += dir;	
      }
      



      errorptr[0] = (FSERROR) bpreverr; 
    }
    cquantize->on_odd_row = (cquantize->on_odd_row ? FALSE : TRUE);
  }
}






LOCAL(void)
alloc_fs_workspace (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  size_t arraysize;
  int i;

  arraysize = (size_t) ((cinfo->output_width + 2) * SIZEOF(FSERROR));
  for (i = 0; i < cinfo->out_color_components; i++) {
    cquantize->fserrors[i] = (FSERRPTR)
      (*cinfo->mem->alloc_large)((j_common_ptr) cinfo, JPOOL_IMAGE, arraysize);
  }
}






METHODDEF(void)
start_pass_1_quant (j_decompress_ptr cinfo, boolean is_pre_scan)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  size_t arraysize;
  int i;

  
  cinfo->colormap = cquantize->sv_colormap;
  cinfo->actual_number_of_colors = cquantize->sv_actual;

  
  switch (cinfo->dither_mode) {
  case JDITHER_NONE:
    if (cinfo->out_color_components == 3)
      cquantize->pub.color_quantize = color_quantize3;
    else
      cquantize->pub.color_quantize = color_quantize;
    break;
  case JDITHER_ORDERED:
    if (cinfo->out_color_components == 3)
      cquantize->pub.color_quantize = quantize3_ord_dither;
    else
      cquantize->pub.color_quantize = quantize_ord_dither;
    cquantize->row_index = 0;	
    



    if (! cquantize->is_padded)
      create_colorindex(cinfo);
    
    if (cquantize->odither[0] == NULL)
      create_odither_tables(cinfo);
    break;
  case JDITHER_FS:
    cquantize->pub.color_quantize = quantize_fs_dither;
    cquantize->on_odd_row = FALSE; 
    
    if (cquantize->fserrors[0] == NULL)
      alloc_fs_workspace(cinfo);
    
    arraysize = (size_t) ((cinfo->output_width + 2) * SIZEOF(FSERROR));
    for (i = 0; i < cinfo->out_color_components; i++)
      jzero_far((void FAR *) cquantize->fserrors[i], arraysize);
    break;
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }
}






METHODDEF(void)
finish_pass_1_quant (j_decompress_ptr cinfo)
{
  
}







METHODDEF(void)
new_color_map_1_quant (j_decompress_ptr cinfo)
{
  ERREXIT(cinfo, JERR_MODE_CHANGE);
}






GLOBAL(void)
jinit_1pass_quantizer (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize;

  cquantize = (my_cquantize_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_cquantizer));
  cinfo->cquantize = (struct jpeg_color_quantizer *) cquantize;
  cquantize->pub.start_pass = start_pass_1_quant;
  cquantize->pub.finish_pass = finish_pass_1_quant;
  cquantize->pub.new_color_map = new_color_map_1_quant;
  cquantize->fserrors[0] = NULL; 
  cquantize->odither[0] = NULL;	

  
  if (cinfo->out_color_components > MAX_Q_COMPS)
    ERREXIT1(cinfo, JERR_QUANT_COMPONENTS, MAX_Q_COMPS);
  
  if (cinfo->desired_number_of_colors > (MAXJSAMPLE+1))
    ERREXIT1(cinfo, JERR_QUANT_MANY_COLORS, MAXJSAMPLE+1);

  
  create_colormap(cinfo);
  create_colorindex(cinfo);

  





  if (cinfo->dither_mode == JDITHER_FS)
    alloc_fs_workspace(cinfo);
}

#endif 
