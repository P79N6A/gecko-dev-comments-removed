





#ifndef NETWERK_SCTP_DATACHANNEL_DATACHANNELPROTOCOL_H_
#define NETWERK_SCTP_DATACHANNEL_DATACHANNELPROTOCOL_H_

#if defined(__GNUC__)
#define SCTP_PACKED __attribute__((packed))
#elif defined(_MSC_VER)
#pragma pack (push, 1)
#define SCTP_PACKED
#else
#error "Unsupported compiler"
#endif

#define DATA_CHANNEL_PPID_CONTROL   50
#define DATA_CHANNEL_PPID_DOMSTRING 51
#define DATA_CHANNEL_PPID_BINARY    52
#define DATA_CHANNEL_PPID_BINARY_LAST 53

#define DATA_CHANNEL_MAX_BINARY_FRAGMENT 0x4000

#define DATA_CHANNEL_FLAGS_SEND_REQ             0x00000001
#define DATA_CHANNEL_FLAGS_SEND_RSP             0x00000002
#define DATA_CHANNEL_FLAGS_SEND_ACK             0x00000004
#define DATA_CHANNEL_FLAGS_OUT_OF_ORDER_ALLOWED 0x00000008
#define DATA_CHANNEL_FLAGS_SEND_DATA            0x00000010
#define DATA_CHANNEL_FLAGS_FINISH_OPEN          0x00000020
#define DATA_CHANNEL_FLAGS_FINISH_RSP           0x00000040
#define DATA_CHANNEL_FLAGS_EXTERNAL_NEGOTIATED  0x00000080

#define INVALID_STREAM (0xFFFF)

#define MAX_NUM_STREAMS (2048)

struct rtcweb_datachannel_open_request {
  uint8_t  msg_type; 
  uint8_t  channel_type;  
  uint16_t flags;
  uint16_t reliability_params;
  int16_t  priority;
  uint16_t label_length;
  uint16_t protocol_length;
  char     label[1]; 
} SCTP_PACKED;



#define DATA_CHANNEL_OPEN_REQUEST             3


#define DATA_CHANNEL_RELIABLE                 0
#define DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT  1
#define DATA_CHANNEL_PARTIAL_RELIABLE_TIMED   2


#define DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED 0x0001



#define ERR_DATA_CHANNEL_ALREADY_OPEN   1
#define ERR_DATA_CHANNEL_NONE_AVAILABLE 2

#if defined(_MSC_VER)
#pragma pack (pop)
#undef SCTP_PACKED
#endif

#endif
