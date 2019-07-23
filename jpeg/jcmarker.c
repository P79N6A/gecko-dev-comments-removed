









#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


typedef enum {			
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01
} JPEG_MARKER;




typedef struct {
  struct jpeg_marker_writer pub; 

  unsigned int last_restart_interval; 
} my_marker_writer;

typedef my_marker_writer * my_marker_ptr;














LOCAL(void)
emit_byte (j_compress_ptr cinfo, int16 val)

{
  struct jpeg_destination_mgr * dest = cinfo->dest;

  *(dest->next_output_byte)++ = (JOCTET) val;
  if (--dest->free_in_buffer == 0) {
    if (! (*dest->empty_output_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
}


LOCAL(void)
emit_marker (j_compress_ptr cinfo, JPEG_MARKER mark)

{
  emit_byte(cinfo, 0xFF);
  emit_byte(cinfo, (int16) mark);
}


LOCAL(void)
emit_2bytes (j_compress_ptr cinfo, int16 value)

{
  emit_byte(cinfo, (value >> 8) & 0xFF);
  emit_byte(cinfo, value & 0xFF);
}






LOCAL(int16)
emit_dqt (j_compress_ptr cinfo, int16 index)


{
  JQUANT_TBL * qtbl = cinfo->quant_tbl_ptrs[index];
  int16 prec;
  int16 i;

  if (qtbl == NULL)
    ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, index);

  prec = 0;
  for (i = 0; i < DCTSIZE2; i++) {
    if (qtbl->quantval[i] > 255)
      prec = 1;
  }

  if (! qtbl->sent_table) {
    emit_marker(cinfo, M_DQT);

    emit_2bytes(cinfo, prec ? DCTSIZE2*2 + 1 + 2 : DCTSIZE2 + 1 + 2);

    emit_byte(cinfo, index + (prec<<4));

    for (i = 0; i < DCTSIZE2; i++) {
      
      unsigned int qval = qtbl->quantval[jpeg_natural_order[i]];
      if (prec)
	emit_byte(cinfo, (int16) (qval >> 8));
      emit_byte(cinfo, (int16) (qval & 0xFF));
    }

    qtbl->sent_table = TRUE;
  }

  return prec;
}


LOCAL(void)
emit_dht (j_compress_ptr cinfo, int16 index, boolean is_ac)

{
  JHUFF_TBL * htbl;
  int16 length, i;
  
  if (is_ac) {
    htbl = cinfo->ac_huff_tbl_ptrs[index];
    index += 0x10;		
  } else {
    htbl = cinfo->dc_huff_tbl_ptrs[index];
  }

  if (htbl == NULL)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, index);
  
  if (! htbl->sent_table) {
    emit_marker(cinfo, M_DHT);
    
    length = 0;
    for (i = 1; i <= 16; i++)
      length += htbl->bits[i];
    
    emit_2bytes(cinfo, length + 2 + 1 + 16);
    emit_byte(cinfo, index);
    
    for (i = 1; i <= 16; i++)
      emit_byte(cinfo, htbl->bits[i]);
    
    for (i = 0; i < length; i++)
      emit_byte(cinfo, htbl->huffval[i]);
    
    htbl->sent_table = TRUE;
  }
}


LOCAL(void)
emit_dac (j_compress_ptr cinfo)



{
#ifdef C_ARITH_CODING_SUPPORTED
  char dc_in_use[NUM_ARITH_TBLS];
  char ac_in_use[NUM_ARITH_TBLS];
  int16 length, i;
  jpeg_component_info *compptr;
  
  for (i = 0; i < NUM_ARITH_TBLS; i++)
    dc_in_use[i] = ac_in_use[i] = 0;
  
  for (i = 0; i < cinfo->comps_in_scan; i++) {
    compptr = cinfo->cur_comp_info[i];
    dc_in_use[compptr->dc_tbl_no] = 1;
    ac_in_use[compptr->ac_tbl_no] = 1;
  }
  
  length = 0;
  for (i = 0; i < NUM_ARITH_TBLS; i++)
    length += dc_in_use[i] + ac_in_use[i];
  
  emit_marker(cinfo, M_DAC);
  
  emit_2bytes(cinfo, length*2 + 2);
  
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    if (dc_in_use[i]) {
      emit_byte(cinfo, i);
      emit_byte(cinfo, cinfo->arith_dc_L[i] + (cinfo->arith_dc_U[i]<<4));
    }
    if (ac_in_use[i]) {
      emit_byte(cinfo, i + 0x10);
      emit_byte(cinfo, cinfo->arith_ac_K[i]);
    }
  }
#endif 
}


LOCAL(void)
emit_dri (j_compress_ptr cinfo)

{
  emit_marker(cinfo, M_DRI);
  
  emit_2bytes(cinfo, 4);	

  emit_2bytes(cinfo, (int16) cinfo->restart_interval);
}


LOCAL(void)
emit_sof (j_compress_ptr cinfo, JPEG_MARKER code)

{
  int16 ci;
  jpeg_component_info *compptr;
  
  emit_marker(cinfo, code);
  
  emit_2bytes(cinfo, 3 * cinfo->num_components + 2 + 5 + 1); 

  
  if ((long) cinfo->image_height > 65535L ||
      (long) cinfo->image_width > 65535L)
    ERREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (unsigned int) 65535);

  emit_byte(cinfo, cinfo->data_precision);
  emit_2bytes(cinfo, (int16) cinfo->image_height);
  emit_2bytes(cinfo, (int16) cinfo->image_width);

  emit_byte(cinfo, cinfo->num_components);

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    emit_byte(cinfo, compptr->component_id);
    emit_byte(cinfo, (compptr->h_samp_factor << 4) + compptr->v_samp_factor);
    emit_byte(cinfo, compptr->quant_tbl_no);
  }
}


LOCAL(void)
emit_sos (j_compress_ptr cinfo)

{
  int16 i, td, ta;
  jpeg_component_info *compptr;
  
  emit_marker(cinfo, M_SOS);
  
  emit_2bytes(cinfo, 2 * cinfo->comps_in_scan + 2 + 1 + 3); 
  
  emit_byte(cinfo, cinfo->comps_in_scan);
  
  for (i = 0; i < cinfo->comps_in_scan; i++) {
    compptr = cinfo->cur_comp_info[i];
    emit_byte(cinfo, compptr->component_id);
    td = compptr->dc_tbl_no;
    ta = compptr->ac_tbl_no;
    if (cinfo->progressive_mode) {
      




      if (cinfo->Ss == 0) {
	ta = 0;			
	if (cinfo->Ah != 0 && !cinfo->arith_code)
	  td = 0;		
      } else {
	td = 0;			
      }
    }
    emit_byte(cinfo, (td << 4) + ta);
  }

  emit_byte(cinfo, cinfo->Ss);
  emit_byte(cinfo, cinfo->Se);
  emit_byte(cinfo, (cinfo->Ah << 4) + cinfo->Al);
}


LOCAL(void)
emit_jfif_app0 (j_compress_ptr cinfo)

{
  










  
  emit_marker(cinfo, M_APP0);
  
  emit_2bytes(cinfo, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); 

  emit_byte(cinfo, 0x4A);	
  emit_byte(cinfo, 0x46);
  emit_byte(cinfo, 0x49);
  emit_byte(cinfo, 0x46);
  emit_byte(cinfo, 0);
  emit_byte(cinfo, cinfo->JFIF_major_version); 
  emit_byte(cinfo, cinfo->JFIF_minor_version);
  emit_byte(cinfo, cinfo->density_unit); 
  emit_2bytes(cinfo, (int16) cinfo->X_density);
  emit_2bytes(cinfo, (int16) cinfo->Y_density);
  emit_byte(cinfo, 0);		
  emit_byte(cinfo, 0);
}


LOCAL(void)
emit_adobe_app14 (j_compress_ptr cinfo)

{
  














  
  emit_marker(cinfo, M_APP14);
  
  emit_2bytes(cinfo, 2 + 5 + 2 + 2 + 2 + 1); 

  emit_byte(cinfo, 0x41);	
  emit_byte(cinfo, 0x64);
  emit_byte(cinfo, 0x6F);
  emit_byte(cinfo, 0x62);
  emit_byte(cinfo, 0x65);
  emit_2bytes(cinfo, 100);	
  emit_2bytes(cinfo, 0);	
  emit_2bytes(cinfo, 0);	
  switch (cinfo->jpeg_color_space) {
  case JCS_YCbCr:
    emit_byte(cinfo, 1);	
    break;
  case JCS_YCCK:
    emit_byte(cinfo, 2);	
    break;
  default:
    emit_byte(cinfo, 0);	
    break;
  }
}










METHODDEF(void)
write_marker_header (j_compress_ptr cinfo, int marker, unsigned int datalen)

{
  if (datalen > (unsigned int) 65533)		
    ERREXIT(cinfo, JERR_BAD_LENGTH);

  emit_marker(cinfo, (JPEG_MARKER) marker);

  emit_2bytes(cinfo, (int16) (datalen + 2));	
}

METHODDEF(void)
write_marker_byte (j_compress_ptr cinfo, int val)

{
  emit_byte(cinfo, (int16) val);
}













METHODDEF(void)
write_file_header (j_compress_ptr cinfo)
{
  my_marker_ptr marker = (my_marker_ptr) cinfo->marker;

  emit_marker(cinfo, M_SOI);	

  
  marker->last_restart_interval = 0;

  if (cinfo->write_JFIF_header)	
    emit_jfif_app0(cinfo);
  if (cinfo->write_Adobe_marker) 
    emit_adobe_app14(cinfo);
}










METHODDEF(void)
write_frame_header (j_compress_ptr cinfo)
{
  int16 ci, prec;
  boolean is_baseline;
  jpeg_component_info *compptr;
  
  


  prec = 0;
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    prec += emit_dqt(cinfo, compptr->quant_tbl_no);
  }
  

  


  if (cinfo->arith_code || cinfo->progressive_mode ||
      cinfo->data_precision != 8) {
    is_baseline = FALSE;
  } else {
    is_baseline = TRUE;
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      if (compptr->dc_tbl_no > 1 || compptr->ac_tbl_no > 1)
	is_baseline = FALSE;
    }
    if (prec && is_baseline) {
      is_baseline = FALSE;
      
      TRACEMS(cinfo, 0, JTRC_16BIT_TABLES);
    }
  }

  
  if (cinfo->arith_code) {
    emit_sof(cinfo, M_SOF9);	
  } else {
    if (cinfo->progressive_mode)
      emit_sof(cinfo, M_SOF2);	
    else if (is_baseline)
      emit_sof(cinfo, M_SOF0);	
    else
      emit_sof(cinfo, M_SOF1);	
  }
}








METHODDEF(void)
write_scan_header (j_compress_ptr cinfo)
{
  my_marker_ptr marker = (my_marker_ptr) cinfo->marker;
  int16 i;
  jpeg_component_info *compptr;

  if (cinfo->arith_code) {
    



    emit_dac(cinfo);
  } else {
    


    for (i = 0; i < cinfo->comps_in_scan; i++) {
      compptr = cinfo->cur_comp_info[i];
      if (cinfo->progressive_mode) {
	
	if (cinfo->Ss == 0) {
	  if (cinfo->Ah == 0)	
	    emit_dht(cinfo, compptr->dc_tbl_no, FALSE);
	} else {
	  emit_dht(cinfo, compptr->ac_tbl_no, TRUE);
	}
      } else {
	
	emit_dht(cinfo, compptr->dc_tbl_no, FALSE);
	emit_dht(cinfo, compptr->ac_tbl_no, TRUE);
      }
    }
  }

  


  if (cinfo->restart_interval != marker->last_restart_interval) {
    emit_dri(cinfo);
    marker->last_restart_interval = cinfo->restart_interval;
  }

  emit_sos(cinfo);
}






METHODDEF(void)
write_file_trailer (j_compress_ptr cinfo)
{
  emit_marker(cinfo, M_EOI);
}









METHODDEF(void)
write_tables_only (j_compress_ptr cinfo)
{
  int16 i;

  emit_marker(cinfo, M_SOI);

  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    if (cinfo->quant_tbl_ptrs[i] != NULL)
      (void) emit_dqt(cinfo, i);
  }

  if (! cinfo->arith_code) {
    for (i = 0; i < NUM_HUFF_TBLS; i++) {
      if (cinfo->dc_huff_tbl_ptrs[i] != NULL)
	emit_dht(cinfo, i, FALSE);
      if (cinfo->ac_huff_tbl_ptrs[i] != NULL)
	emit_dht(cinfo, i, TRUE);
    }
  }

  emit_marker(cinfo, M_EOI);
}






GLOBAL(void)
jinit_marker_writer (j_compress_ptr cinfo)
{
  my_marker_ptr marker;

  
  marker = (my_marker_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_marker_writer));
  cinfo->marker = (struct jpeg_marker_writer *) marker;
  
  marker->pub.write_file_header = write_file_header;
  marker->pub.write_frame_header = write_frame_header;
  marker->pub.write_scan_header = write_scan_header;
  marker->pub.write_file_trailer = write_file_trailer;
  marker->pub.write_tables_only = write_tables_only;
  marker->pub.write_marker_header = write_marker_header;
  marker->pub.write_marker_byte = write_marker_byte;
  
  marker->last_restart_interval = 0;
}
