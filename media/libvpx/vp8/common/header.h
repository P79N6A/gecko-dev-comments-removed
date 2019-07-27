










#ifndef VP8_COMMON_HEADER_H_
#define VP8_COMMON_HEADER_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    unsigned int type: 1;
    unsigned int version: 3;
    unsigned int show_frame: 1;

    

    unsigned int first_partition_length_in_bytes: 19;

#ifdef PACKET_TESTING
    unsigned int frame_number;
    unsigned int update_gold: 1;
    unsigned int uses_gold: 1;
    unsigned int update_last: 1;
    unsigned int uses_last: 1;
#endif

} VP8_HEADER;

#ifdef PACKET_TESTING
#define VP8_HEADER_SIZE 8
#else
#define VP8_HEADER_SIZE 3
#endif


#ifdef __cplusplus
}  
#endif

#endif
