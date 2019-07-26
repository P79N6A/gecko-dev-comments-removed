





























#if !defined(_G722_ENC_DEC_H_)
#define _G722_ENC_DEC_H_














#define WEBRTC_INT16_MAX 32767
#define WEBRTC_INT16_MIN -32768

enum
{
    G722_SAMPLE_RATE_8000 = 0x0001,
    G722_PACKED = 0x0002
};

typedef struct
{
    

    int itu_test_mode;
    
    int packed;
    
    int eight_k;
    
    int bits_per_sample;

    
    int x[24];

    struct
    {
        int s;
        int sp;
        int sz;
        int r[3];
        int a[3];
        int ap[3];
        int p[3];
        int d[7];
        int b[7];
        int bp[7];
        int sg[7];
        int nb;
        int det;
    } band[2];

    unsigned int in_buffer;
    int in_bits;
    unsigned int out_buffer;
    int out_bits;
} g722_encode_state_t;

typedef struct
{
    

    int itu_test_mode;
    
    int packed;
    
    int eight_k;
    
    int bits_per_sample;

    
    int x[24];

    struct
    {
        int s;
        int sp;
        int sz;
        int r[3];
        int a[3];
        int ap[3];
        int p[3];
        int d[7];
        int b[7];
        int bp[7];
        int sg[7];
        int nb;
        int det;
    } band[2];
    
    unsigned int in_buffer;
    int in_bits;
    unsigned int out_buffer;
    int out_bits;
} g722_decode_state_t;

#ifdef __cplusplus
extern "C" {
#endif

g722_encode_state_t *WebRtc_g722_encode_init(g722_encode_state_t *s,
                                             int rate,
                                             int options);
int WebRtc_g722_encode_release(g722_encode_state_t *s);
int WebRtc_g722_encode(g722_encode_state_t *s,
                       WebRtc_UWord8 g722_data[],
                       const WebRtc_Word16 amp[],
                       int len);

g722_decode_state_t *WebRtc_g722_decode_init(g722_decode_state_t *s,
                                             int rate,
                                             int options);
int WebRtc_g722_decode_release(g722_decode_state_t *s);
int WebRtc_g722_decode(g722_decode_state_t *s,
                       WebRtc_Word16 amp[],
                       const WebRtc_UWord8 g722_data[],
                       int len);

#ifdef __cplusplus
}
#endif

#endif
