






















#include "iccjpeg.h"
#include <stdlib.h>  















#define ICC_MARKER  (JPEG_APP0 + 2)      /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14             /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533       /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)





void
setup_read_icc_profile (j_decompress_ptr cinfo)
{
  
  jpeg_save_markers(cinfo, ICC_MARKER, 0xFFFF);
}






static boolean
marker_is_icc (jpeg_saved_marker_ptr marker)
{
  return
    marker->marker == ICC_MARKER &&
    marker->data_length >= ICC_OVERHEAD_LEN &&
    
    GETJOCTET(marker->data[0]) == 0x49 &&
    GETJOCTET(marker->data[1]) == 0x43 &&
    GETJOCTET(marker->data[2]) == 0x43 &&
    GETJOCTET(marker->data[3]) == 0x5F &&
    GETJOCTET(marker->data[4]) == 0x50 &&
    GETJOCTET(marker->data[5]) == 0x52 &&
    GETJOCTET(marker->data[6]) == 0x4F &&
    GETJOCTET(marker->data[7]) == 0x46 &&
    GETJOCTET(marker->data[8]) == 0x49 &&
    GETJOCTET(marker->data[9]) == 0x4C &&
    GETJOCTET(marker->data[10]) == 0x45 &&
    GETJOCTET(marker->data[11]) == 0x0;
}





















boolean
read_icc_profile (j_decompress_ptr cinfo,
  JOCTET** icc_data_ptr,
  unsigned int* icc_data_len)
{
  jpeg_saved_marker_ptr marker;
  int num_markers = 0;
  int seq_no;
  JOCTET* icc_data;
  unsigned int total_length;
#define MAX_SEQ_NO  255        /* sufficient since marker numbers are bytes */
  char marker_present[MAX_SEQ_NO+1];      
  unsigned int data_length[MAX_SEQ_NO+1]; 
  unsigned int data_offset[MAX_SEQ_NO+1]; 

  *icc_data_ptr = NULL;                   
  *icc_data_len = 0;

  



  for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++) {
    marker_present[seq_no] = 0;
  }

  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      if (num_markers == 0) {
        num_markers = GETJOCTET(marker->data[13]);
      } else if (num_markers != GETJOCTET(marker->data[13])) {
        return FALSE;  
      }
      seq_no = GETJOCTET(marker->data[12]);
      if (seq_no <= 0 || seq_no > num_markers) {
        return FALSE;   
      }
      if (marker_present[seq_no]) {
        return FALSE;   
      }
      marker_present[seq_no] = 1;
      data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
    }
  }

  if (num_markers == 0) {
    return FALSE;
  }

  



  total_length = 0;
  for (seq_no = 1; seq_no <= num_markers; seq_no++) {
    if (marker_present[seq_no] == 0) {
      return FALSE;  
    }
    data_offset[seq_no] = total_length;
    total_length += data_length[seq_no];
  }

  if (total_length <= 0) {
    return FALSE;  
  }

  
  icc_data = (JOCTET*) malloc(total_length * sizeof(JOCTET));
  if (icc_data == NULL) {
    return FALSE;   
  }

  
  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      JOCTET FAR* src_ptr;
      JOCTET* dst_ptr;
      unsigned int length;
      seq_no = GETJOCTET(marker->data[12]);
      dst_ptr = icc_data + data_offset[seq_no];
      src_ptr = marker->data + ICC_OVERHEAD_LEN;
      length = data_length[seq_no];
      while (length--) {
        *dst_ptr++ = *src_ptr++;
      }
    }
  }

  *icc_data_ptr = icc_data;
  *icc_data_len = total_length;

  return TRUE;
}
