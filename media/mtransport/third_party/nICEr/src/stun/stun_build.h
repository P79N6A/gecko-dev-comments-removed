
































#ifndef _stun_build_h
#define _stun_build_h

#include "stun.h"

#define NR_STUN_MODE_STUN               1
#ifdef USE_STUND_0_96
#define NR_STUN_MODE_STUND_0_96         2    /* backwards compatibility mode */
#endif 
int nr_stun_form_request_or_indication(int mode, int msg_type, nr_stun_message **msg);

typedef struct nr_stun_client_stun_binding_request_params_ {
    char *username;
    Data *password;
    char *nonce;
    char *realm;
} nr_stun_client_stun_binding_request_params;

int nr_stun_build_req_lt_auth(nr_stun_client_stun_binding_request_params *params, nr_stun_message **msg);
int nr_stun_build_req_st_auth(nr_stun_client_stun_binding_request_params *params, nr_stun_message **msg);
int nr_stun_build_req_no_auth(nr_stun_client_stun_binding_request_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_stun_keepalive_params_ {
#ifdef WIN32  
    int dummy;
#endif
} nr_stun_client_stun_keepalive_params;

int nr_stun_build_keepalive(nr_stun_client_stun_keepalive_params *params, nr_stun_message **msg);


#ifdef USE_STUND_0_96
typedef struct nr_stun_client_stun_binding_request_stund_0_96_params_ {
#ifdef WIN32  
    int dummy;
#endif
} nr_stun_client_stun_binding_request_stund_0_96_params;

int nr_stun_build_req_stund_0_96(nr_stun_client_stun_binding_request_stund_0_96_params *params, nr_stun_message **msg);
#endif 


#ifdef USE_ICE
typedef struct nr_stun_client_ice_use_candidate_params_ {
    char *username;
    Data password;
    UINT4 priority;
} nr_stun_client_ice_use_candidate_params;

int nr_stun_build_use_candidate(nr_stun_client_ice_use_candidate_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_ice_binding_request_params_ {
    char *username;
    Data password;
    UINT4 priority;
    int control;
#define NR_ICE_CONTROLLING  1
#define NR_ICE_CONTROLLED   2
    UINT8 tiebreaker;
} nr_stun_client_ice_binding_request_params;

int nr_stun_build_req_ice(nr_stun_client_ice_binding_request_params *params, nr_stun_message **msg);
#endif 


#ifdef USE_TURN
typedef struct nr_stun_client_allocate_request1_params_ {
    char *username;
} nr_stun_client_allocate_request1_params;

int nr_stun_build_allocate_request1(nr_stun_client_allocate_request1_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_allocate_request2_params_ {
    char *username;
    char *realm;
    char *nonce;
    Data *password;
    UINT4 bandwidth_kbps;
    UINT4 lifetime_secs;
} nr_stun_client_allocate_request2_params;

int nr_stun_build_allocate_request2(nr_stun_client_allocate_request2_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_send_indication_params_ {
    nr_transport_addr remote_addr;
    Data data;
} nr_stun_client_send_indication_params;

int nr_stun_build_send_indication(nr_stun_client_send_indication_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_set_active_dest_request_params_ {
    nr_transport_addr remote_addr;
    Data *password;
} nr_stun_client_set_active_dest_request_params;

int nr_stun_build_set_active_dest_request(nr_stun_client_set_active_dest_request_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_data_indication_params_ {
    nr_transport_addr remote_addr;
    Data data;
} nr_stun_client_data_indication_params;

int nr_stun_build_data_indication(nr_stun_client_data_indication_params *params, nr_stun_message **msg);
#endif 

int nr_stun_form_success_response(nr_stun_message *req, nr_transport_addr *from, Data *password, nr_stun_message *res);
void nr_stun_form_error_response(nr_stun_message *request, nr_stun_message* response, int number, char* msg);

#endif
