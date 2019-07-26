

































#ifndef _stun_proc_h
#define _stun_proc_h

#include "stun.h"

int nr_stun_receive_message(nr_stun_message *req, nr_stun_message *msg);
int nr_stun_process_request(nr_stun_message *req, nr_stun_message *res);
int nr_stun_process_indication(nr_stun_message *ind);
int nr_stun_process_success_response(nr_stun_message *res);
int nr_stun_process_error_response(nr_stun_message *res);

int nr_stun_receive_request_or_indication_short_term_auth(nr_stun_message *msg, nr_stun_message *res);
int nr_stun_receive_response_short_term_auth(nr_stun_message *res);

int nr_stun_receive_request_long_term_auth(nr_stun_message *req, nr_stun_server_ctx *ctx, nr_stun_message *res);
int nr_stun_receive_response_long_term_auth(nr_stun_message *res, nr_stun_client_ctx *ctx);

#endif

