






























#ifndef SILK_ERRORS_H
#define SILK_ERRORS_H

#ifdef __cplusplus
extern "C"
{
#endif




#define SILK_NO_ERROR                               0






#define SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES        -101


#define SILK_ENC_FS_NOT_SUPPORTED                   -102


#define SILK_ENC_PACKET_SIZE_NOT_SUPPORTED          -103


#define SILK_ENC_PAYLOAD_BUF_TOO_SHORT              -104


#define SILK_ENC_INVALID_LOSS_RATE                  -105


#define SILK_ENC_INVALID_COMPLEXITY_SETTING         -106


#define SILK_ENC_INVALID_INBAND_FEC_SETTING         -107


#define SILK_ENC_INVALID_DTX_SETTING                -108


#define SILK_ENC_INVALID_CBR_SETTING                -109


#define SILK_ENC_INTERNAL_ERROR                     -110


#define SILK_ENC_INVALID_NUMBER_OF_CHANNELS_ERROR   -111






#define SILK_DEC_INVALID_SAMPLING_FREQUENCY         -200


#define SILK_DEC_PAYLOAD_TOO_LARGE                  -201


#define SILK_DEC_PAYLOAD_ERROR                      -202


#define SILK_DEC_INVALID_FRAME_SIZE                 -203

#ifdef __cplusplus
}
#endif

#endif
