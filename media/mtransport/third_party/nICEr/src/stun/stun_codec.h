

































#ifndef _stun_codec_h
#define _stun_codec_h

#include "stun_msg.h"

typedef struct nr_stun_attr_info_  nr_stun_attr_info;

typedef struct nr_stun_attr_codec_ {
    char     *name;
    int     (*print)(nr_stun_attr_info *attr_info, char *msg, void *data);
    int     (*encode)(nr_stun_attr_info *attr_info, void *data, int offset, int buflen, UCHAR *buf, int *attrlen);
    int     (*decode)(nr_stun_attr_info *attr_info, int attrlen, UCHAR *buf, int offset, int buflen, void *data);
} nr_stun_attr_codec;

struct nr_stun_attr_info_ {
     UINT2                 type;
     char                 *name;
     nr_stun_attr_codec   *codec;
     int                 (*illegal)(nr_stun_attr_info *attr_info, int attrlen, void *data);
};

extern nr_stun_attr_codec nr_stun_attr_codec_UINT4;
extern nr_stun_attr_codec nr_stun_attr_codec_UINT8;
extern nr_stun_attr_codec nr_stun_attr_codec_addr;
extern nr_stun_attr_codec nr_stun_attr_codec_bytes;
extern nr_stun_attr_codec nr_stun_attr_codec_data;
extern nr_stun_attr_codec nr_stun_attr_codec_error_code;
extern nr_stun_attr_codec nr_stun_attr_codec_fingerprint;
extern nr_stun_attr_codec nr_stun_attr_codec_flag;
extern nr_stun_attr_codec nr_stun_attr_codec_message_integrity;
extern nr_stun_attr_codec nr_stun_attr_codec_noop;
extern nr_stun_attr_codec nr_stun_attr_codec_quoted_string;
extern nr_stun_attr_codec nr_stun_attr_codec_string;
extern nr_stun_attr_codec nr_stun_attr_codec_unknown_attributes;
extern nr_stun_attr_codec nr_stun_attr_codec_xor_mapped_address;
extern nr_stun_attr_codec nr_stun_attr_codec_old_xor_mapped_address;


int nr_stun_encode_message(nr_stun_message *msg);
int nr_stun_decode_message(nr_stun_message *msg, int (*get_password)(void *arg, nr_stun_message *msg, Data **password), void *arg);

#endif

