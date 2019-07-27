

































#ifndef _ice_reg_h
#define _ice_reg_h
#ifdef __cplusplus
using namespace std;
extern "C" {
#endif

#define NR_ICE_REG_PREF_TYPE_HOST           "ice.pref.type.host"
#define NR_ICE_REG_PREF_TYPE_RELAYED        "ice.pref.type.relayed"
#define NR_ICE_REG_PREF_TYPE_SRV_RFLX       "ice.pref.type.srv_rflx"
#define NR_ICE_REG_PREF_TYPE_PEER_RFLX      "ice.pref.type.peer_rflx"
#define NR_ICE_REG_PREF_TYPE_HOST_TCP       "ice.pref.type.host_tcp"
#define NR_ICE_REG_PREF_TYPE_RELAYED_TCP    "ice.pref.type.relayed_tcp"
#define NR_ICE_REG_PREF_TYPE_SRV_RFLX_TCP   "ice.pref.type.srv_rflx_tcp"
#define NR_ICE_REG_PREF_TYPE_PEER_RFLX_TCP  "ice.pref.type.peer_rflx_tcp"

#define NR_ICE_REG_PREF_INTERFACE_PRFX      "ice.pref.interface"
#define NR_ICE_REG_SUPPRESS_INTERFACE_PRFX  "ice.suppress.interface"

#define NR_ICE_REG_STUN_SRV_PRFX            "ice.stun.server"
#define NR_ICE_REG_STUN_SRV_ADDR            "addr"
#define NR_ICE_REG_STUN_SRV_PORT            "port"

#define NR_ICE_REG_TURN_SRV_PRFX            "ice.turn.server"
#define NR_ICE_REG_TURN_SRV_ADDR            "addr"
#define NR_ICE_REG_TURN_SRV_PORT            "port"
#define NR_ICE_REG_TURN_SRV_BANDWIDTH       "bandwidth"
#define NR_ICE_REG_TURN_SRV_LIFETIME        "lifetime"
#define NR_ICE_REG_TURN_SRV_USERNAME        "username"
#define NR_ICE_REG_TURN_SRV_PASSWORD        "password"

#define NR_ICE_REG_ICE_TCP_SO_SOCK_COUNT    "ice.tcp.so_sock_count"

#define NR_ICE_REG_KEEPALIVE_TIMER          "ice.keepalive_timer"

#define NR_ICE_REG_TRICKLE_GRACE_PERIOD     "ice.trickle_grace_period"
#ifdef __cplusplus
}
#endif 
#endif

