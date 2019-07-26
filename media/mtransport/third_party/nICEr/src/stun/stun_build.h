
































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


typedef struct nr_stun_client_auth_params_ {
    char authenticate;
    char *username;
    char *realm;
    char *nonce;
    Data password;
} nr_stun_client_auth_params;

#ifdef USE_TURN
typedef struct nr_stun_client_allocate_request_params_ {
    UINT4 lifetime_secs;
} nr_stun_client_allocate_request_params;

int nr_stun_build_allocate_request(nr_stun_client_auth_params *auth, nr_stun_client_allocate_request_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_refresh_request_params_ {
    UINT4 lifetime_secs;
} nr_stun_client_refresh_request_params;

int nr_stun_build_refresh_request(nr_stun_client_auth_params *auth, nr_stun_client_refresh_request_params *params, nr_stun_message **msg);



typedef struct nr_stun_client_permission_request_params_ {
    nr_transport_addr remote_addr;
} nr_stun_client_permission_request_params;

int nr_stun_build_permission_request(nr_stun_client_auth_params *auth, nr_stun_client_permission_request_params *params, nr_stun_message **msg);


typedef struct nr_stun_client_send_indication_params_ {
    nr_transport_addr remote_addr;
    Data data;
} nr_stun_client_send_indication_params;

int nr_stun_build_send_indication(nr_stun_client_send_indication_params *params, nr_stun_message **msg);

typedef struct nr_stun_client_data_indication_params_ {
    nr_transport_addr remote_addr;
    Data data;
} nr_stun_client_data_indication_params;

int nr_stun_build_data_indication(nr_stun_client_data_indication_params *params, nr_stun_message **msg);
#endif 

int nr_stun_form_success_response(nr_stun_message *req, nr_transport_addr *from, Data *password, nr_stun_message *res);
void nr_stun_form_error_response(nr_stun_message *request, nr_stun_message* response, int number, char* msg);
int nr_stun_compute_lt_message_integrity_password(const char *username, const char *realm,
                                                  Data *password, Data *hmac_key);

#endif
