
































#ifndef _stun_hint_h
#define _stun_hint_h

int nr_is_stun_message(UCHAR *buf, int len);
int nr_is_stun_request_message(UCHAR *buf, int len);
int nr_is_stun_response_message(UCHAR *buf, int len);
int nr_is_stun_indication_message(UCHAR *buf, int len);
int nr_has_stun_cookie(UCHAR *buf, int len);

#endif
