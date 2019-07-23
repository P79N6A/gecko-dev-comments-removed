

















#include <stdio.h>		
#include "jpeglib.h"

















extern void setup_read_icc_profile JPP((j_decompress_ptr cinfo));


















extern boolean read_icc_profile JPP((j_decompress_ptr cinfo,
				     JOCTET **icc_data_ptr,
				     unsigned int *icc_data_len));
