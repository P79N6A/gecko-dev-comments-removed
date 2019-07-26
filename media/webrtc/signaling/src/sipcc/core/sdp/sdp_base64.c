



#include "sdp_base64.h"




#define INVALID_CHAR 0xFF /* Character not in supported Base64 set */
#define WHITE_SPACE  0xFE /* Space, tab, newline, etc character */
#define PADDING      0xFD /* The character '=' */

#define PAD_CHAR     '=' /* The character '=' */


#define MAX_BASE64_LINE_LENGTH 76






char *base64_result_table[BASE64_RESULT_MAX] =
{
    "Base64 successful",
    "Base64 Buffer Overrun",
    "Base64 Bad Data",
    "Base64 Bad Padding",
    "Base64 Bad Block Size"
};









unsigned char base64_to_raw_table[128] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 
    0xFE, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF,   62, 0xFF, 0xFF, 0xFF,   63,   52,   53, 
      54,   55,   56,   57,   58,   59,   60,   61, 0xFF, 0xFF, 
    0xFF, 0xFD, 0xFF, 0xFF, 0xFF,    0,    1,    2,    3,    4, 
       5,    6,    7,    8,    9,   10,   11,   12,   13,   14, 
      15,   16,   17,   18,   19,   20,   21,   22,   23,   24, 
      25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   26,   27,   28, 
      29,   30,   31,   32,   33,   34,   35,   36,   37,   38, 
      39,   40,   41,   42,   43,   44,   45,   46,   47,   48, 
      49,   50,   51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF              
};

unsigned char raw_to_base64_table[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', 
    '8', '9', '+', '/'				      
};














int base64_est_encode_size_bytes (int raw_size_bytes)
{
    int length;

    






    length = ((((raw_size_bytes * 4 + 2)/ 3) + 3) & ~(0x3)) +
	raw_size_bytes / MAX_BASE64_LINE_LENGTH;

    return length;
}














int base64_est_decode_size_bytes (int base64_size_bytes)
{
    int length;

    length = (base64_size_bytes * 3 + 3) / 4;
    return length;
}




























base64_result_t base64_encode(unsigned char *src, int src_bytes, unsigned char *dest, int *dest_bytes)
{
    int i, j=0;
    int line_count = 0;
    unsigned char index; 
    int smax = src_bytes-2; 
    int dmax = *dest_bytes; 

    *dest_bytes = 0;

    
    for (i=0; i<smax; i+=3) {
	
	if (line_count>=MAX_BASE64_LINE_LENGTH) {
	    if (j<dmax){
		dest[j++] = '\n';
	    } else {
		return BASE64_BUFFER_OVERRUN;
	    }
	    line_count = 0;
	}

	line_count += 4;

	if ((j+3) < dmax) {

	    
	    index = (src[i] >> 2) & 0x3F;
	    dest[j++] = raw_to_base64_table[index];

	    
	    index = ((src[i] << 4) & 0x30) | ((src[i+1] >> 4) & 0x0F);
	    dest[j++] = raw_to_base64_table[index];

	    
	    index = ((src[i+1] << 2) & 0x3C) | ((src[i+2] >> 6) & 0x03);
	    dest[j++] = raw_to_base64_table[index];

	    
	    index = src[i+2] & 0x3F;
	    dest[j++] = raw_to_base64_table[index];
	} else {
	    return BASE64_BUFFER_OVERRUN;
	}
    }

    
    if (i<src_bytes) {

	
	if (line_count>=MAX_BASE64_LINE_LENGTH) {
	    if (j<dmax){
		dest[j++] = '\n';
	    } else {
		return BASE64_BUFFER_OVERRUN;
	    }
	    line_count = 0;
	}

	line_count += 4;

	
	if (j+4>dmax) {
	    
	    return BASE64_BUFFER_OVERRUN;
	}

	
	index = (src[i] >> 2) & 0x3F;
	dest[j++] = raw_to_base64_table[index];

	
	if ((i+1)<src_bytes) {
	    
	    index = ((src[i] << 4) & 0x30) | ((src[i+1] >> 4) & 0x0F);
	    dest[j++] = raw_to_base64_table[index];

	    
	    index = (src[i+1] << 2) & 0x3C;
	    dest[j++] = raw_to_base64_table[index];
	    dest[j++] = PAD_CHAR;
	} else {
	    
	    index = (src[i] << 4) & 0x30;
	    dest[j++] = raw_to_base64_table[index];
	    dest[j++] = PAD_CHAR;
	    dest[j++] = PAD_CHAR;
	}
    }

    *dest_bytes = j;

    return BASE64_SUCCESS;
}
























base64_result_t base64_decode(unsigned char *src, int src_bytes, unsigned char *dest, int *dest_bytes)
{
    int i, j = 0;
    int sindex = 0;			

    int pad_count=0;			

    int dest_size_bytes = *dest_bytes;	
    unsigned char cindex;		

    unsigned char val;			


    *dest_bytes = 0;

    for (i=0; i<src_bytes; i++) {
	cindex = src[i];

	if ((cindex & 0x80) || 
	    ((val = base64_to_raw_table[cindex]) == INVALID_CHAR)) {
	    
	    return BASE64_BAD_DATA;
	}

	if (val == WHITE_SPACE) {
	    
	    continue;
	}

	if (val == PADDING) {
	    
	    pad_count++;
	    if (++i<src_bytes) {
		
		if (base64_to_raw_table[src[i]] != PADDING) {
		    return BASE64_BAD_PADDING;
		}

		if (++i<src_bytes) {
		    
		    return BASE64_BAD_PADDING;
		}

		pad_count++;
	    }

	    
	    break;
	}

	
	switch (sindex & 0x3) {
	case 0:
	    
	    if (j<dest_size_bytes) {
		dest[j] = val << 2;
	    } else {
		return BASE64_BUFFER_OVERRUN;
	    }
	    break;
	case 1:
	    
	    dest[j++] |= val >> 4;

	    if (j<dest_size_bytes) {
		
		dest[j] = (val << 4) & 0xF0;
	    } else {
		







		if ((val & 0x0F) ||
		    (i+1>=src_bytes) ||
		    (base64_to_raw_table[src[i+1]] != PADDING)) {
		    return BASE64_BUFFER_OVERRUN;
		}
	    }
	    break;
	case 2:
	    
	    dest[j++] |= val >> 2;

	    if (j<dest_size_bytes) {
		
		dest[j] = (val << 6) & 0xC0;
	    } else {
		







		if ((val & 0x03) ||
		    (i+1>=src_bytes) ||
		    (base64_to_raw_table[src[i+1]] != PADDING)) {
		    return BASE64_BUFFER_OVERRUN;
		}
	    }
	    break;
	case 3:
	    





	    
	    dest[j++] |= val;
	    break;
	}
	sindex++;
    }

    
    if (((j + pad_count)% 3) != 0) {
	return BASE64_BAD_BLOCK_SIZE;
    }

    
    *dest_bytes = j;

    return BASE64_SUCCESS;
}
