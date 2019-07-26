

































#ifndef _stun_reg_h
#define _stun_reg_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

#define NR_STUN_REG_PREF_CLNT_RETRANSMIT_TIMEOUT    "stun.client.retransmission_timeout"
#define NR_STUN_REG_PREF_CLNT_RETRANSMIT_BACKOFF    "stun.client.retransmission_backoff_factor"
#define NR_STUN_REG_PREF_CLNT_MAXIMUM_TRANSMITS     "stun.client.maximum_transmits"
#define NR_STUN_REG_PREF_CLNT_FINAL_RETRANSMIT_BACKOFF   "stun.client.final_retransmit_backoff"

#define NR_STUN_REG_PREF_ADDRESS_PRFX               "stun.address"
#define NR_STUN_REG_PREF_SERVER_NAME                "stun.server.name"
#define NR_STUN_REG_PREF_SERVER_NONCE_SIZE          "stun.server.nonce_size"
#define NR_STUN_REG_PREF_SERVER_REALM               "stun.server.realm"

#ifdef __cplusplus
}
#endif 
#endif

