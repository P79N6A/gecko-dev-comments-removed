











#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"






GLOBAL(void)
jpeg_add_quant_table (j_compress_ptr cinfo, int which_tbl,
		      const unsigned int *basic_table,
		      int scale_factor, boolean force_baseline)





{
  JQUANT_TBL ** qtblptr;
  int i;
  long temp;

  
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (which_tbl < 0 || which_tbl >= NUM_QUANT_TBLS)
    ERREXIT1(cinfo, JERR_DQT_INDEX, which_tbl);

  qtblptr = & cinfo->quant_tbl_ptrs[which_tbl];

  if (*qtblptr == NULL)
    *qtblptr = jpeg_alloc_quant_table((j_common_ptr) cinfo);

  for (i = 0; i < DCTSIZE2; i++) {
    temp = ((long) basic_table[i] * scale_factor + 50L) / 100L;
    
    if (temp <= 0L) temp = 1L;
    if (temp > 32767L) temp = 32767L; 
    if (force_baseline && temp > 255L)
      temp = 255L;		
    (*qtblptr)->quantval[i] = (UINT16) temp;
  }

  
  (*qtblptr)->sent_table = FALSE;
}


GLOBAL(void)
jpeg_set_linear_quality (j_compress_ptr cinfo, int scale_factor,
			 boolean force_baseline)





{
  



  static const unsigned int std_luminance_quant_tbl[DCTSIZE2] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };
  static const unsigned int std_chrominance_quant_tbl[DCTSIZE2] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
  };

  
  jpeg_add_quant_table(cinfo, 0, (const unsigned int *)std_luminance_quant_tbl,
		       scale_factor, force_baseline);
  jpeg_add_quant_table(cinfo, 1, (const unsigned int *)std_chrominance_quant_tbl,
		       scale_factor, force_baseline);
}


GLOBAL(int)
jpeg_quality_scaling (int quality)




{
  
  if (quality <= 0) quality = 1;
  if (quality > 100) quality = 100;

  





  if (quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - quality*2;

  return quality;
}


GLOBAL(void)
jpeg_set_quality (j_compress_ptr cinfo, int quality, boolean force_baseline)





{
  
  quality = jpeg_quality_scaling(quality);

  
  jpeg_set_linear_quality(cinfo, quality, force_baseline);
}






LOCAL(void)
add_huff_table (j_compress_ptr cinfo,
		JHUFF_TBL **htblptr, const UINT8 *bits, const UINT8 *val)

{
  int nsymbols, len;

  if (*htblptr == NULL)
    *htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);

  
  MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));

  



  nsymbols = 0;
  for (len = 1; len <= 16; len++)
    nsymbols += bits[len];
  if (nsymbols < 1 || nsymbols > 256)
    ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);

  MEMCOPY((*htblptr)->huffval, val, nsymbols * SIZEOF(UINT8));

  
  (*htblptr)->sent_table = FALSE;
}


LOCAL(void)
std_huff_tables (j_compress_ptr cinfo)


{
  static const UINT8 bits_dc_luminance[17] =
    {  0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_dc_chrominance[17] =
    {  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_ac_luminance[17] =
    {  0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  static const UINT8 bits_ac_chrominance[17] =
    {  0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[0],
		 bits_dc_luminance, val_dc_luminance);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[0],
		 bits_ac_luminance, val_ac_luminance);
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[1],
		 bits_dc_chrominance, val_dc_chrominance);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[1],
		 bits_ac_chrominance, val_ac_chrominance);
}












GLOBAL(void)
jpeg_set_defaults (j_compress_ptr cinfo)
{
  int i;

  
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  



  if (cinfo->comp_info == NULL)
    cinfo->comp_info = (jpeg_component_info *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  MAX_COMPONENTS * SIZEOF(jpeg_component_info));

  

  cinfo->data_precision = BITS_IN_JSAMPLE;
  
  jpeg_set_quality(cinfo, 75, TRUE);
  
  std_huff_tables(cinfo);

  
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }

  
  cinfo->scan_info = NULL;
  cinfo->num_scans = 0;

  
  cinfo->raw_data_in = FALSE;

  
  cinfo->arith_code = FALSE;

  
  cinfo->optimize_coding = FALSE;
  




  if (cinfo->data_precision > 8)
    cinfo->optimize_coding = TRUE;

  
  cinfo->CCIR601_sampling = FALSE;

  
  cinfo->smoothing_factor = 0;

  
  cinfo->dct_method = JDCT_DEFAULT;

  
  cinfo->restart_interval = 0;
  cinfo->restart_in_rows = 0;

  








  cinfo->JFIF_major_version = 1; 
  cinfo->JFIF_minor_version = 1;
  cinfo->density_unit = 0;	
  cinfo->X_density = 1;		
  cinfo->Y_density = 1;

  

  jpeg_default_colorspace(cinfo);
}






GLOBAL(void)
jpeg_default_colorspace (j_compress_ptr cinfo)
{
  switch (cinfo->in_color_space) {
  case JCS_GRAYSCALE:
    jpeg_set_colorspace(cinfo, JCS_GRAYSCALE);
    break;
  case JCS_RGB:
    jpeg_set_colorspace(cinfo, JCS_YCbCr);
    break;
  case JCS_YCbCr:
    jpeg_set_colorspace(cinfo, JCS_YCbCr);
    break;
  case JCS_CMYK:
    jpeg_set_colorspace(cinfo, JCS_CMYK); 
    break;
  case JCS_YCCK:
    jpeg_set_colorspace(cinfo, JCS_YCCK);
    break;
  case JCS_UNKNOWN:
    jpeg_set_colorspace(cinfo, JCS_UNKNOWN);
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
  }
}






GLOBAL(void)
jpeg_set_colorspace (j_compress_ptr cinfo, J_COLOR_SPACE colorspace)
{
  jpeg_component_info * compptr;
  int ci;

#define SET_COMP(index,id,hsamp,vsamp,quant,dctbl,actbl)  \
  (compptr = &cinfo->comp_info[index], \
   compptr->component_id = (id), \
   compptr->h_samp_factor = (hsamp), \
   compptr->v_samp_factor = (vsamp), \
   compptr->quant_tbl_no = (quant), \
   compptr->dc_tbl_no = (dctbl), \
   compptr->ac_tbl_no = (actbl) )

  
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  



  cinfo->jpeg_color_space = colorspace;

  cinfo->write_JFIF_header = FALSE; 
  cinfo->write_Adobe_marker = FALSE; 

  switch (colorspace) {
  case JCS_GRAYSCALE:
    cinfo->write_JFIF_header = TRUE; 
    cinfo->num_components = 1;
    
    SET_COMP(0, 1, 1,1, 0, 0,0);
    break;
  case JCS_RGB:
    cinfo->write_Adobe_marker = TRUE; 
    cinfo->num_components = 3;
    SET_COMP(0, 0x52 , 1,1, 0, 0,0);
    SET_COMP(1, 0x47 , 1,1, 0, 0,0);
    SET_COMP(2, 0x42 , 1,1, 0, 0,0);
    break;
  case JCS_YCbCr:
    cinfo->write_JFIF_header = TRUE; 
    cinfo->num_components = 3;
    
    
    SET_COMP(0, 1, 2,2, 0, 0,0);
    SET_COMP(1, 2, 1,1, 1, 1,1);
    SET_COMP(2, 3, 1,1, 1, 1,1);
    break;
  case JCS_CMYK:
    cinfo->write_Adobe_marker = TRUE; 
    cinfo->num_components = 4;
    SET_COMP(0, 0x43 , 1,1, 0, 0,0);
    SET_COMP(1, 0x4D , 1,1, 0, 0,0);
    SET_COMP(2, 0x59 , 1,1, 0, 0,0);
    SET_COMP(3, 0x4B , 1,1, 0, 0,0);
    break;
  case JCS_YCCK:
    cinfo->write_Adobe_marker = TRUE; 
    cinfo->num_components = 4;
    SET_COMP(0, 1, 2,2, 0, 0,0);
    SET_COMP(1, 2, 1,1, 1, 1,1);
    SET_COMP(2, 3, 1,1, 1, 1,1);
    SET_COMP(3, 4, 2,2, 0, 0,0);
    break;
  case JCS_UNKNOWN:
    cinfo->num_components = cinfo->input_components;
    if (cinfo->num_components < 1 || cinfo->num_components > MAX_COMPONENTS)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
	       MAX_COMPONENTS);
    for (ci = 0; ci < cinfo->num_components; ci++) {
      SET_COMP(ci, ci, 1,1, 0, 0,0);
    }
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
  }
}


#ifdef C_PROGRESSIVE_SUPPORTED

LOCAL(jpeg_scan_info *)
fill_a_scan (jpeg_scan_info * scanptr, int ci,
	     int Ss, int Se, int Ah, int Al)

{
  scanptr->comps_in_scan = 1;
  scanptr->component_index[0] = ci;
  scanptr->Ss = Ss;
  scanptr->Se = Se;
  scanptr->Ah = Ah;
  scanptr->Al = Al;
  scanptr++;
  return scanptr;
}

LOCAL(jpeg_scan_info *)
fill_scans (jpeg_scan_info * scanptr, int ncomps,
	    int Ss, int Se, int Ah, int Al)

{
  int ci;

  for (ci = 0; ci < ncomps; ci++) {
    scanptr->comps_in_scan = 1;
    scanptr->component_index[0] = ci;
    scanptr->Ss = Ss;
    scanptr->Se = Se;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  }
  return scanptr;
}

LOCAL(jpeg_scan_info *)
fill_dc_scans (jpeg_scan_info * scanptr, int ncomps, int Ah, int Al)

{
  int ci;

  if (ncomps <= MAX_COMPS_IN_SCAN) {
    
    scanptr->comps_in_scan = ncomps;
    for (ci = 0; ci < ncomps; ci++)
      scanptr->component_index[ci] = ci;
    scanptr->Ss = scanptr->Se = 0;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  } else {
    
    scanptr = fill_scans(scanptr, ncomps, 0, 0, Ah, Al);
  }
  return scanptr;
}







GLOBAL(void)
jpeg_simple_progression (j_compress_ptr cinfo)
{
  int ncomps = cinfo->num_components;
  int nscans;
  jpeg_scan_info * scanptr;

  
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  
  if (ncomps == 3 && cinfo->jpeg_color_space == JCS_YCbCr) {
    
    nscans = 10;
  } else {
    
    if (ncomps > MAX_COMPS_IN_SCAN)
      nscans = 6 * ncomps;	
    else
      nscans = 2 + 4 * ncomps;	
  }

  






  if (cinfo->script_space == NULL || cinfo->script_space_size < nscans) {
    cinfo->script_space_size = MAX(nscans, 10);
    cinfo->script_space = (jpeg_scan_info *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			cinfo->script_space_size * SIZEOF(jpeg_scan_info));
  }
  scanptr = cinfo->script_space;
  cinfo->scan_info = scanptr;
  cinfo->num_scans = nscans;

  if (ncomps == 3 && cinfo->jpeg_color_space == JCS_YCbCr) {
    
    
    scanptr = fill_dc_scans(scanptr, ncomps, 0, 1);
    
    scanptr = fill_a_scan(scanptr, 0, 1, 5, 0, 2);
    
    scanptr = fill_a_scan(scanptr, 2, 1, 63, 0, 1);
    scanptr = fill_a_scan(scanptr, 1, 1, 63, 0, 1);
    
    scanptr = fill_a_scan(scanptr, 0, 6, 63, 0, 2);
    
    scanptr = fill_a_scan(scanptr, 0, 1, 63, 2, 1);
    
    scanptr = fill_dc_scans(scanptr, ncomps, 1, 0);
    
    scanptr = fill_a_scan(scanptr, 2, 1, 63, 1, 0);
    scanptr = fill_a_scan(scanptr, 1, 1, 63, 1, 0);
    
    scanptr = fill_a_scan(scanptr, 0, 1, 63, 1, 0);
  } else {
    
    
    scanptr = fill_dc_scans(scanptr, ncomps, 0, 1);
    scanptr = fill_scans(scanptr, ncomps, 1, 5, 0, 2);
    scanptr = fill_scans(scanptr, ncomps, 6, 63, 0, 2);
    
    scanptr = fill_scans(scanptr, ncomps, 1, 63, 2, 1);
    
    scanptr = fill_dc_scans(scanptr, ncomps, 1, 0);
    scanptr = fill_scans(scanptr, ncomps, 1, 63, 1, 0);
  }
}

#endif 
