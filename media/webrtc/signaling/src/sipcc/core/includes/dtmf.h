



#ifndef _DTMF_H_
#define _DTMF_H_

typedef enum _DTMFState {
    DTMF_IDLE,
    DTMF_START,
    DTMF_CONT,
    DTMF_END
} DTMFState;

typedef enum {
    DTMF_OUTOFBAND_NONE = 0,
    DTMF_OUTOFBAND_AVT,
    DTMF_OUTOFBAND_AVT_ALWAYS
} DtmfOutOfBandTransport_t;

boolean RTPDtmfInbandGet(void);
DtmfOutOfBandTransport_t RTPDtmfOutofbandGet(void);
void RTPDtmfInbandSet(boolean val);
void RTPDtmfOutofbandSet(uint16_t channel, DtmfOutOfBandTransport_t transport,
                         int payload_type);

#endif
