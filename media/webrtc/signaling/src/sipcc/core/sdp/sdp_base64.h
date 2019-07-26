






































#ifndef _SDP_BASE64_H_
#define _SDP_BASE64_H_





typedef enum base64_result_t_ {
    BASE64_INVALID=-1,
    BASE64_SUCCESS=0,
    BASE64_BUFFER_OVERRUN,
    BASE64_BAD_DATA,
    BASE64_BAD_PADDING,
    BASE64_BAD_BLOCK_SIZE,
    BASE64_RESULT_MAX
} base64_result_t;

#define MAX_BASE64_STRING_LEN 60


extern char *base64_result_table[];





#define BASE64_RESULT_TO_STRING(_result) (((_result)>=0 && (_result)<BASE64_RESULT_MAX)?(base64_result_table[_result]):("UNKNOWN Result Code"))



int base64_est_encode_size_bytes(int raw_size_bytes);
int base64_est_decode_size_bytes(int base64_size_bytes);

base64_result_t base64_encode(unsigned char *src, int src_bytes, unsigned char *dest, int *dest_bytes);

base64_result_t base64_decode(unsigned char *src, int src_bytes, unsigned char *dest, int *dest_bytes);

#endif 
