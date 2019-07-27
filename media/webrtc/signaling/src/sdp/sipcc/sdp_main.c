



#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"

#include "CSFLog.h"

static const char* logTag = "sdp_main";


const sdp_tokenarray_t sdp_token[SDP_MAX_TOKENS] =
{
    {"v=", sdp_parse_version,      sdp_build_version },
    {"o=", sdp_parse_owner,        sdp_build_owner },
    {"s=", sdp_parse_sessname,     sdp_build_sessname },
    {"i=", sdp_parse_sessinfo,     sdp_build_sessinfo },
    {"u=", sdp_parse_uri,          sdp_build_uri },
    {"e=", sdp_parse_email,        sdp_build_email },
    {"p=", sdp_parse_phonenum,     sdp_build_phonenum },
    {"c=", sdp_parse_connection,   sdp_build_connection },
    {"b=", sdp_parse_bandwidth,    sdp_build_bandwidth },
    {"t=", sdp_parse_timespec,     sdp_build_timespec },
    {"r=", sdp_parse_repeat_time,  sdp_build_repeat_time },
    {"z=", sdp_parse_timezone_adj, sdp_build_timezone_adj },
    {"k=", sdp_parse_encryption,   sdp_build_encryption },
    {"a=", sdp_parse_attribute,    sdp_build_attribute },
    {"m=", sdp_parse_media,        sdp_build_media }
};



const sdp_attrarray_t sdp_attr[SDP_MAX_ATTR_TYPES] =
{
    {"bearer", sizeof("bearer"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"called", sizeof("called"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"connection_type", sizeof("connection_type"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"dialed", sizeof("dialed"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"dialing", sizeof("dialing"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"direction", sizeof("direction"),
     sdp_parse_attr_comediadir, sdp_build_attr_comediadir },
    {"eecid", sizeof("eecid"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"fmtp", sizeof("fmtp"),
     sdp_parse_attr_fmtp, sdp_build_attr_fmtp },
    {"sctpmap", sizeof("sctpmap"),
     sdp_parse_attr_sctpmap, sdp_build_attr_sctpmap },
    {"framing", sizeof("framing"),
     sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"inactive", sizeof("inactive"),
     sdp_parse_attr_direction, sdp_build_attr_direction },
    {"ptime", sizeof("ptime"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"qos", sizeof("qos"),
     sdp_parse_attr_qos, sdp_build_attr_qos },
    {"curr", sizeof("curr"),
     sdp_parse_attr_curr, sdp_build_attr_curr },
    {"des", sizeof("des"),
     sdp_parse_attr_des, sdp_build_attr_des},
    {"conf", sizeof("conf"),
     sdp_parse_attr_conf, sdp_build_attr_conf},
    {"recvonly", sizeof("recvonly"),
     sdp_parse_attr_direction, sdp_build_attr_direction },
    {"rtpmap", sizeof("rtpmap"),
     sdp_parse_attr_transport_map, sdp_build_attr_transport_map },
    {"secure", sizeof("secure"),
     sdp_parse_attr_qos, sdp_build_attr_qos },
    {"sendonly", sizeof("sendonly"),
     sdp_parse_attr_direction, sdp_build_attr_direction },
    {"sendrecv", sizeof("sendrecv"),
     sdp_parse_attr_direction, sdp_build_attr_direction },
    {"subnet", sizeof("subnet"),
     sdp_parse_attr_subnet, sdp_build_attr_subnet },
    {"T38FaxVersion", sizeof("T38FaxVersion"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"T38MaxBitRate", sizeof("T38MaxBitRate"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"T38FaxFillBitRemoval", sizeof("T38FaxFillBitRemoval"),
     sdp_parse_attr_simple_bool, sdp_build_attr_simple_bool },
    {"T38FaxTranscodingMMR", sizeof("T38FaxTranscodingMMR"),
     sdp_parse_attr_simple_bool, sdp_build_attr_simple_bool },
    {"T38FaxTranscodingJBIG", sizeof("T38FaxTranscodingJBIG"),
     sdp_parse_attr_simple_bool, sdp_build_attr_simple_bool },
    {"T38FaxRateManagement", sizeof("T38FaxRateManagement"),
     sdp_parse_attr_t38_ratemgmt, sdp_build_attr_t38_ratemgmt },
    {"T38FaxMaxBuffer", sizeof("T38FaxMaxBuffer"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"T38FaxMaxDatagram", sizeof("T38FaxMaxDatagram"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"T38FaxUdpEC", sizeof("T38FaxUdpEC"),
     sdp_parse_attr_t38_udpec, sdp_build_attr_t38_udpec },
    {"X-cap", sizeof("X-cap"),
     sdp_parse_attr_cap, sdp_build_attr_cap },
    {"X-cpar", sizeof("X-cpar"),
     sdp_parse_attr_cpar, sdp_build_attr_cpar },
    {"X-pc-codec", sizeof("X-pc-codec"),
     sdp_parse_attr_pc_codec, sdp_build_attr_pc_codec },
    {"X-pc-qos", sizeof("X-pc-qos"),
     sdp_parse_attr_qos, sdp_build_attr_qos },
    {"X-qos", sizeof("X-qos"),
     sdp_parse_attr_qos, sdp_build_attr_qos },
    {"X-sqn", sizeof("X-sqn"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"TMRGwXid", sizeof("TMRGwXid"),
     sdp_parse_attr_simple_bool, sdp_build_attr_simple_bool },
    {"TC1PayloadBytes", sizeof("TC1PayloadBytes"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"TC1WindowSize", sizeof("TC1WindowSize"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"TC2PayloadBytes", sizeof("TC2PayloadBytes"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"TC2WindowSize", sizeof("TC2WindowSize"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"rtcp", sizeof("rtcp"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"rtr", sizeof("rtr"),
     sdp_parse_attr_rtr, sdp_build_attr_rtr},
    {"silenceSupp", sizeof("silenceSupp"),
     sdp_parse_attr_silencesupp, sdp_build_attr_silencesupp },
    {"X-crypto", sizeof("X-crypto"),
     sdp_parse_attr_srtpcontext, sdp_build_attr_srtpcontext },
    {"mptime", sizeof("mptime"),
      sdp_parse_attr_mptime, sdp_build_attr_mptime },
    {"X-sidin", sizeof("X-sidin"),
      sdp_parse_attr_x_sidin, sdp_build_attr_x_sidin },
    {"X-sidout", sizeof("X-sidout"),
      sdp_parse_attr_x_sidout, sdp_build_attr_x_sidout },
    {"X-confid", sizeof("X-confid"),
      sdp_parse_attr_x_confid, sdp_build_attr_x_confid },
    {"group", sizeof("group"),
      sdp_parse_attr_group, sdp_build_attr_group },
    {"mid", sizeof("mid"),
      sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"source-filter", sizeof("source-filter"),
      sdp_parse_attr_source_filter, sdp_build_source_filter},
    {"rtcp-unicast", sizeof("rtcp-unicast"),
      sdp_parse_attr_rtcp_unicast, sdp_build_attr_rtcp_unicast},
    {"maxprate", sizeof("maxprate"),
      sdp_parse_attr_maxprate, sdp_build_attr_simple_string},
    {"sqn", sizeof("sqn"),
     sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"cdsc", sizeof("cdsc"),
     sdp_parse_attr_cap, sdp_build_attr_cap },
    {"cpar", sizeof("cpar"),
     sdp_parse_attr_cpar, sdp_build_attr_cpar },
    {"sprtmap", sizeof("sprtmap"),
     sdp_parse_attr_transport_map, sdp_build_attr_transport_map },
    {"crypto", sizeof("crypto"),
     sdp_parse_attr_sdescriptions, sdp_build_attr_sdescriptions },
    {"label", sizeof("label"),
      sdp_parse_attr_simple_string, sdp_build_attr_simple_string },
    {"framerate", sizeof("framerate"),
      sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32 },
    {"candidate", sizeof("candidate"),
      sdp_parse_attr_ice_attr, sdp_build_attr_ice_attr },
    {"ice-ufrag", sizeof("ice-ufrag"),
      sdp_parse_attr_ice_attr, sdp_build_attr_ice_attr },
    {"ice-pwd", sizeof("ice-pwd"),
      sdp_parse_attr_ice_attr, sdp_build_attr_ice_attr},
    {"ice-lite", sizeof("ice-lite"),
      sdp_parse_attr_simple_flag, sdp_build_attr_simple_flag},
    {"rtcp-mux", sizeof("rtcp-mux"),
      sdp_parse_attr_simple_flag, sdp_build_attr_simple_flag},
    {"fingerprint", sizeof("fingerprint"),
      sdp_parse_attr_complete_line, sdp_build_attr_simple_string},
    {"maxptime", sizeof("maxptime"),
      sdp_parse_attr_simple_u32, sdp_build_attr_simple_u32},
    {"rtcp-fb", sizeof("rtcp-fb"),
      sdp_parse_attr_rtcp_fb, sdp_build_attr_rtcp_fb},
    {"setup", sizeof("setup"),
      sdp_parse_attr_setup, sdp_build_attr_setup},
    {"connection", sizeof("connection"),
      sdp_parse_attr_connection, sdp_build_attr_connection},
    {"extmap", sizeof("extmap"),
      sdp_parse_attr_extmap, sdp_build_attr_extmap},
    {"identity", sizeof("identity"),
      sdp_parse_attr_simple_string, sdp_build_attr_simple_string},
    {"msid", sizeof("msid"),
      sdp_parse_attr_msid, sdp_build_attr_msid},
    {"msid-semantic", sizeof("msid-semantic"),
      sdp_parse_attr_msid_semantic, sdp_build_attr_msid_semantic},
    {"bundle-only", sizeof("bundle-only"),
      sdp_parse_attr_simple_flag, sdp_build_attr_simple_flag},
    {"end-of-candidates", sizeof("end-of-candidates"),
      sdp_parse_attr_simple_flag, sdp_build_attr_simple_flag},
    {"ice-options", sizeof("ice-options"),
      sdp_parse_attr_complete_line, sdp_build_attr_simple_string},
    {"ssrc", sizeof("ssrc"),
      sdp_parse_attr_ssrc, sdp_build_attr_ssrc},
};


const sdp_namearray_t sdp_media[SDP_MAX_MEDIA_TYPES] =
{
    {"audio",        sizeof("audio")},
    {"video",        sizeof("video")},
    {"application",  sizeof("application")},
    {"data",         sizeof("data")},
    {"control",      sizeof("control")},
    {"nas/radius",   sizeof("nas/radius")},
    {"nas/tacacs",   sizeof("nas/tacacs")},
    {"nas/diameter", sizeof("nas/diameter")},
    {"nas/l2tp",     sizeof("nas/l2tp")},
    {"nas/login",    sizeof("nas/login")},
    {"nas/none",     sizeof("nas/none")},
    {"image",        sizeof("image")},
    {"text",         sizeof("text")}
};



const sdp_namearray_t sdp_nettype[SDP_MAX_NETWORK_TYPES] =
{
    {"IN",           sizeof("IN")},
    {"ATM",          sizeof("ATM")},
    {"FR",           sizeof("FR")},
    {"LOCAL",        sizeof("LOCAL")}
};



const sdp_namearray_t sdp_addrtype[SDP_MAX_ADDR_TYPES] =
{
    {"IP4",          sizeof("IP4")},
    {"IP6",          sizeof("IP6")},
    {"NSAP",         sizeof("NSAP")},
    {"EPN",          sizeof("EPN")},
    {"E164",         sizeof("E164")},
    {"GWID",         sizeof("GWID")}
};



const sdp_namearray_t sdp_transport[SDP_MAX_TRANSPORT_TYPES] =
{
    {"RTP/AVP",      sizeof("RTP/AVP")},
    {"udp",          sizeof("udp")},
    {"udptl",        sizeof("udptl")},
    {"ces10",        sizeof("ces10")},
    {"LOCAL",        sizeof("LOCAL")},
    {"AAL2/ITU",     sizeof("AAL2/ITU")},
    {"AAL2/ATMF",    sizeof("AAL2/ATMF")},
    {"AAL2/custom",  sizeof("AAL2/custom")},
    {"AAL1/AVP",     sizeof("AAL1/AVP")},
    {"udpsprt",      sizeof("udpsprt")},
    {"RTP/SAVP",     sizeof("RTP/SAVP")},
    {"tcp",          sizeof("tcp")},
    {"RTP/SAVPF",    sizeof("RTP/SAVPF")},
    {"DTLS/SCTP",    sizeof("DTLS/SCTP")},
    {"RTP/AVPF",     sizeof("RTP/AVPF")},
    {"UDP/TLS/RTP/SAVP", sizeof("UDP/TLS/RTP/SAVP")},
    {"UDP/TLS/RTP/SAVPF", sizeof("UDP/TLS/RTP/SAVPF")},
    {"TCP/TLS/RTP/SAVP", sizeof("TCP/TLS/RTP/SAVP")},
    {"TCP/TLS/RTP/SAVPF", sizeof("TCP/TLS/RTP/SAVPF")},
};


const sdp_namearray_t sdp_encrypt[SDP_MAX_ENCRYPT_TYPES] =
{
    {"clear",        sizeof("clear")},
    {"base64",       sizeof("base64")},
    {"uri",          sizeof("uri")},
    {"prompt",       sizeof("prompt")}
};


const sdp_namearray_t sdp_payload[SDP_MAX_STRING_PAYLOAD_TYPES] =
{
    {"t38",          sizeof("t38")},
    {"X-tmr",        sizeof("X-tmr")},
    {"T120",         sizeof("T120")}
};


const sdp_namearray_t sdp_t38_rate[SDP_T38_MAX_RATES] =
{
    {"localTCF",        sizeof("localTCF")},
    {"transferredTCF",  sizeof("transferredTCF")},
    {"unknown",         sizeof("unknown")}
};


const sdp_namearray_t sdp_t38_udpec[SDP_T38_MAX_UDPEC] =
{
    {"t38UDPRedundancy",  sizeof("t38UDPRedundancy")},
    {"t38UDPFEC",         sizeof("t38UDPFEC")},
    {"unknown",           sizeof("unknown")}
};


const sdp_namearray_t sdp_qos_strength[SDP_MAX_QOS_STRENGTH] =
{
    {"optional",          sizeof("optional")},
    {"mandatory",         sizeof("mandatory")},
    {"success",           sizeof("success")},
    {"failure",           sizeof("failure")},
    {"none",              sizeof("none")}
};


const sdp_namearray_t sdp_qos_status_type[SDP_MAX_QOS_STATUS_TYPES] =
{
    {"local",          sizeof("local")},
    {"remote",         sizeof("remote")},
    {"e2e",            sizeof("e2e")}
};


const sdp_namearray_t sdp_curr_type[SDP_MAX_CURR_TYPES] =
{
    {"qos",            sizeof("qos")},
    {"unknown",         sizeof("unknown")}
};


const sdp_namearray_t sdp_des_type[SDP_MAX_DES_TYPES] =
{
    {"qos",            sizeof("qos")},
    {"unknown",         sizeof("unknown")}
};


const sdp_namearray_t sdp_conf_type[SDP_MAX_CONF_TYPES] =
{
    {"qos",            sizeof("qos")},
    {"unknown",         sizeof("unknown")}
};

const sdp_namearray_t sdp_qos_direction[SDP_MAX_QOS_DIR] =
{
    {"send",              sizeof("send")},
    {"recv",              sizeof("recv")},
    {"sendrecv",          sizeof("sendrecv")},
    {"none",              sizeof("none")}
};


const sdp_namearray_t sdp_silencesupp_pref[SDP_MAX_SILENCESUPP_PREF] = {
    {"standard",          sizeof("standard")},
    {"custom",            sizeof("custom")},
    {"-",                 sizeof("-")}
};


const sdp_namearray_t sdp_silencesupp_siduse[SDP_MAX_SILENCESUPP_SIDUSE] = {
    {"No SID",            sizeof("No SID")},
    {"Fixed Noise",       sizeof("Fixed Noise")},
    {"Sampled Noise",     sizeof("Sampled Noise")},
    {"-",                 sizeof("-")}
};


const sdp_namearray_t sdp_mediadir_role[SDP_MAX_MEDIADIR_ROLES] =
{
    {"passive",       sizeof("passive")},
    {"active",        sizeof("active")},
    {"both",          sizeof("both")},
    {"reuse",         sizeof("reuse")},
    {"unknown",       sizeof("unknown")}
};


const sdp_namearray_t sdp_fmtp_codec_param[SDP_MAX_FMTP_PARAM] =
{
    {"annexa",              sizeof("annexa")}, 
    {"annexb",              sizeof("annexb")}, 
    {"bitrate",             sizeof("bitrate")}, 
    {"QCIF",                sizeof("QCIF")}, 
    {"CIF",                 sizeof("CIF")},  
    {"MAXBR",               sizeof("MAXBR")}, 
    {"SQCIF",               sizeof("SQCIF")}, 
    {"CIF4",                sizeof("CIF4")}, 
    {"CIF16",               sizeof("CIF16")}, 
    {"CUSTOM",              sizeof("CUSTOM")}, 
    {"PAR",                 sizeof("PAR")}, 
    {"CPCF",                sizeof("CPCF")}, 
    {"BPP",                 sizeof("BPP")}, 
    {"HRD",                 sizeof("HRD")}, 
    {"PROFILE",             sizeof("PROFILE")}, 
    {"LEVEL",               sizeof("LEVEL")}, 
    {"INTERLACE",           sizeof("INTERLACE")}, 

    
    {"profile-level-id",      sizeof("profile-level-id")}, 
    {"sprop-parameter-sets",  sizeof("sprop-parameter-sets")}, 
    {"packetization-mode",    sizeof("packetization-mode")}, 
    {"sprop-interleaving-depth",    sizeof("sprop-interleaving-depth")}, 
    {"sprop-deint-buf-req",   sizeof("sprop-deint-buf-req")}, 
    {"sprop-max-don-diff",    sizeof("sprop-max-don-diff")}, 
    {"sprop-init-buf-time",   sizeof("sprop-init-buf-time")}, 

    {"max-mbps",              sizeof("max-mbps")}, 
    {"max-fs",                sizeof("max-fs")}, 
    {"max-cpb",               sizeof("max-cpb")}, 
    {"max-dpb",               sizeof("max-dpb")}, 
    {"max-br",                sizeof("max-br")}, 
    {"redundant-pic-cap",     sizeof("redundant-pic-cap")}, 
    {"deint-buf-cap",         sizeof("deint-buf-cap")}, 
    {"max-rcmd-nalu-size",    sizeof("max-rcmd_nali-size")}, 
    {"parameter-add",         sizeof("parameter-add")}, 

    
     {"D", sizeof("D")}, 
     {"F", sizeof("F")}, 
     {"I", sizeof("I")}, 
     {"J", sizeof("J")}, 
     {"T", sizeof("T")}, 
     {"K", sizeof("K")}, 
     {"N", sizeof("N")}, 
     {"P", sizeof("P")}, 

     {"mode",                sizeof("mode")},  
    {"level-asymmetry-allowed",         sizeof("level-asymmetry-allowed")}, 
    {"maxaveragebitrate",               sizeof("maxaveragebitrate")}, 
    {"usedtx",                          sizeof("usedtx")}, 
    {"stereo",                          sizeof("stereo")}, 
    {"useinbandfec",                    sizeof("useinbandfec")}, 
    {"maxcodedaudiobandwidth",          sizeof("maxcodedaudiobandwidth")}, 
    {"cbr",                             sizeof("cbr")}, 
    {"max-fr",                          sizeof("max-fr")} 
} ;


const sdp_namearray_t sdp_fmtp_codec_param_val[SDP_MAX_FMTP_PARAM_VAL] =
{
    {"yes",                 sizeof("yes")},
    {"no",                  sizeof("no")}
};

const sdp_namearray_t sdp_bw_modifier_val[SDP_MAX_BW_MODIFIER_VAL] =
{
    {"AS",                  sizeof("AS")},
    {"CT",                  sizeof("CT")},
    {"TIAS",                sizeof("TIAS")}
};

const sdp_namearray_t sdp_group_attr_val[SDP_MAX_GROUP_ATTR_VAL] =
{
    {"FID",                 sizeof("FID")},
    {"LS",                  sizeof("LS")},
    {"ANAT",                sizeof("ANAT")},
    {"BUNDLE",              sizeof("BUNDLE")}
};

const sdp_namearray_t sdp_srtp_context_crypto_suite[SDP_SRTP_MAX_NUM_CRYPTO_SUITES] =
{
  {"UNKNOWN_CRYPTO_SUITE",    sizeof("UNKNOWN_CRYPTO_SUITE")},
  {"AES_CM_128_HMAC_SHA1_32", sizeof("AES_CM_128_HMAC_SHA1_32")},
  {"AES_CM_128_HMAC_SHA1_80", sizeof("AES_CM_128_HMAC_SHA1_80")},
  {"F8_128_HMAC_SHA1_80", sizeof("F8_128_HMAC_SHA1_80")}
};


const sdp_namearray_t sdp_src_filter_mode_val[SDP_MAX_FILTER_MODE] =
{
    {"incl", sizeof("incl")},
    {"excl", sizeof("excl")}
};


const sdp_namearray_t sdp_rtcp_unicast_mode_val[SDP_RTCP_MAX_UNICAST_MODE] =
{
    {"reflection", sizeof("reflection")},
    {"rsi",        sizeof("rsi")}
};

#define SDP_NAME(x) {x, sizeof(x)}

const sdp_namearray_t sdp_rtcp_fb_type_val[SDP_MAX_RTCP_FB] =
{
    SDP_NAME("ack"),
    SDP_NAME("ccm"),
    SDP_NAME("nack"),
    SDP_NAME("trr-int")
};


const sdp_namearray_t sdp_rtcp_fb_nack_type_val[SDP_MAX_RTCP_FB_NACK] =
{
    SDP_NAME(""),
    SDP_NAME("sli"),
    SDP_NAME("pli"),
    SDP_NAME("rpsi"),
    SDP_NAME("app"),
    SDP_NAME("rai"),
    SDP_NAME("tllei"),
    SDP_NAME("pslei"),
    SDP_NAME("ecn")
};


const sdp_namearray_t sdp_rtcp_fb_ack_type_val[SDP_MAX_RTCP_FB_ACK] =
{
    SDP_NAME("rpsi"),
    SDP_NAME("app")
};


const sdp_namearray_t sdp_rtcp_fb_ccm_type_val[SDP_MAX_RTCP_FB_CCM] =
{
    SDP_NAME("fir"),
    SDP_NAME("tmmbr"),
    SDP_NAME("tstr"),
    SDP_NAME("vbcm")
};


const sdp_namearray_t sdp_setup_type_val[SDP_MAX_SETUP] =
{
    SDP_NAME("active"),
    SDP_NAME("passive"),
    SDP_NAME("actpass"),
    SDP_NAME("holdconn")
};


const sdp_namearray_t sdp_connection_type_val[SDP_MAX_CONNECTION] =
{
    SDP_NAME("new"),
    SDP_NAME("existing")
};


const sdp_srtp_crypto_suite_list sdp_srtp_crypto_suite_array[SDP_SRTP_MAX_NUM_CRYPTO_SUITES] =
{
  {SDP_SRTP_UNKNOWN_CRYPTO_SUITE, UNKNOWN_CRYPTO_SUITE, 0, 0},
  {SDP_SRTP_AES_CM_128_HMAC_SHA1_32, AES_CM_128_HMAC_SHA1_32,
      SDP_SRTP_AES_CM_128_HMAC_SHA1_32_KEY_BYTES,
      SDP_SRTP_AES_CM_128_HMAC_SHA1_32_SALT_BYTES},
  {SDP_SRTP_AES_CM_128_HMAC_SHA1_80, AES_CM_128_HMAC_SHA1_80,
      SDP_SRTP_AES_CM_128_HMAC_SHA1_80_KEY_BYTES,
      SDP_SRTP_AES_CM_128_HMAC_SHA1_80_SALT_BYTES},
  {SDP_SRTP_F8_128_HMAC_SHA1_80, F8_128_HMAC_SHA1_80,
      SDP_SRTP_F8_128_HMAC_SHA1_80_KEY_BYTES,
      SDP_SRTP_F8_128_HMAC_SHA1_80_SALT_BYTES}
};

const char* sdp_result_name[SDP_MAX_RC] =
    {"SDP_SUCCESS",
     "SDP_FAILURE",
     "SDP_INVALID_SDP_PTR",
     "SDP_NOT_SDP_DESCRIPTION",
     "SDP_INVALID_TOKEN_ORDERING",
     "SDP_INVALID_PARAMETER",
     "SDP_INVALID_MEDIA_LEVEL",
     "SDP_INVALID_CAPABILITY",
     "SDP_NO_RESOURCE",
     "SDP_UNRECOGNIZED_TOKEN",
     "SDP_NULL_BUF_PTR",
     "SDP_POTENTIAL_SDP_OVERFLOW",
     "SDP_EMPTY_TOKEN"};

const char *sdp_get_result_name ( sdp_result_e rc )
{
    if (rc >= SDP_MAX_RC) {
        return ("Invalid SDP result code");
    } else {
        return (sdp_result_name[rc]);
    }
}

const char *sdp_get_attr_name ( sdp_attr_e attr_type )
{
    if (attr_type >= SDP_MAX_ATTR_TYPES) {
        return ("Invalid attribute type");
    } else {
        return (sdp_attr[attr_type].name);
    }
}

const char *sdp_get_media_name ( sdp_media_e media_type )
{
    if (media_type == SDP_MEDIA_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (media_type >= SDP_MAX_MEDIA_TYPES) {
        return ("Invalid media type");
    } else {
        return (sdp_media[media_type].name);
    }
}

const char *sdp_get_network_name ( sdp_nettype_e network_type )
{
    if (network_type == SDP_NT_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (network_type >= SDP_MAX_NETWORK_TYPES) {
        return ("Invalid network type");
    } else {
        return (sdp_nettype[network_type].name);
    }
}

const char *sdp_get_address_name ( sdp_addrtype_e addr_type )
{
    if (addr_type == SDP_AT_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (addr_type >= SDP_MAX_ADDR_TYPES) {
        if (addr_type == SDP_AT_FQDN) {
            return ("*");
        } else {
            return ("Invalid address type");
        }
    } else {
        return (sdp_addrtype[addr_type].name);
    }
}

const char *sdp_get_transport_name ( sdp_transport_e transport_type )
{
    if (transport_type == SDP_TRANSPORT_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (transport_type >= SDP_MAX_TRANSPORT_TYPES) {
        return ("Invalid transport type");
    } else {
        return (sdp_transport[transport_type].name);
    }
}

const char *sdp_get_encrypt_name ( sdp_encrypt_type_e encrypt_type )
{
    if (encrypt_type == SDP_ENCRYPT_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (encrypt_type >= SDP_MAX_ENCRYPT_TYPES) {
        return ("Invalid encryption type");
    } else {
        return (sdp_encrypt[encrypt_type].name);
    }
}

const char *sdp_get_payload_name ( sdp_payload_e payload )
{
    if (payload == SDP_PAYLOAD_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (payload >= SDP_MAX_STRING_PAYLOAD_TYPES) {
        return ("Invalid payload type");
    } else {
        return (sdp_payload[payload].name);
    }
}

const char *sdp_get_t38_ratemgmt_name ( sdp_t38_ratemgmt_e rate )
{
    if (rate >= SDP_T38_MAX_RATES) {
        return ("Invalid rate");
    } else {
        return (sdp_t38_rate[rate].name);
    }
}

const char *sdp_get_t38_udpec_name ( sdp_t38_udpec_e udpec )
{
    if (udpec >= SDP_T38_MAX_UDPEC) {
        return ("Invalid udpec");
    } else {
        return (sdp_t38_udpec[udpec].name);
    }
}

const char *sdp_get_qos_strength_name ( sdp_qos_strength_e strength )
{
    if (strength >= SDP_MAX_QOS_STRENGTH) {
        return ("Invalid qos strength");
    } else {
        return (sdp_qos_strength[strength].name);
    }
}

const char *sdp_get_qos_direction_name ( sdp_qos_dir_e direction )
{
    if (direction >= SDP_MAX_QOS_DIR) {
        return ("Invalid qos direction");
    } else {
        return (sdp_qos_direction[direction].name);
    }
}

const char *sdp_get_qos_status_type_name ( sdp_qos_status_types_e status_type )
{
    if (status_type >= SDP_MAX_QOS_STATUS_TYPES) {
        return ("Invalid qos status type");
    } else {
        return (sdp_qos_status_type[status_type].name);
    }
}

const char *sdp_get_curr_type_name (sdp_curr_type_e curr_type )
{
    if (curr_type >= SDP_MAX_CURR_TYPES) {
        return ("Invalid curr type");
    } else {
        return (sdp_curr_type[curr_type].name);
    }
}

const char *sdp_get_des_type_name (sdp_des_type_e des_type )
{
    if (des_type >= SDP_MAX_DES_TYPES) {
        return ("Invalid des type");
    } else {
        return (sdp_des_type[des_type].name);
    }
}

const char *sdp_get_conf_type_name (sdp_conf_type_e conf_type )
{
    if (conf_type >= SDP_MAX_CONF_TYPES) {
        return ("Invalid conf type");
    } else {
        return (sdp_conf_type[conf_type].name);
    }
}

const char *sdp_get_silencesupp_pref_name (sdp_silencesupp_pref_e pref)
{
    if (pref >= SDP_MAX_SILENCESUPP_PREF) {
        return ("Invalid silencesupp pref");
    } else {
        return (sdp_silencesupp_pref[pref].name);
    }
}

const char *sdp_get_silencesupp_siduse_name (sdp_silencesupp_siduse_e siduse)
{
    if (siduse >= SDP_MAX_SILENCESUPP_SIDUSE) {
        return ("Invalid silencesupp siduse");
    } else {
        return (sdp_silencesupp_siduse[siduse].name);
    }
}

const char *sdp_get_mediadir_role_name (sdp_mediadir_role_e role)
{
    if (role >= SDP_MEDIADIR_ROLE_UNKNOWN) {
        return ("Invalid media direction role");
    } else {
        return (sdp_mediadir_role[role].name);
    }
}


const char *sdp_get_bw_modifier_name (sdp_bw_modifier_e bw_modifier_type)
{
    if (bw_modifier_type == SDP_BW_MODIFIER_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (bw_modifier_type < SDP_BW_MODIFIER_AS ||
            bw_modifier_type >= SDP_MAX_BW_MODIFIER_VAL) {
        return ("Invalid bw modifier type");
    } else {
        return (sdp_bw_modifier_val[bw_modifier_type].name);
    }
}

const char *sdp_get_group_attr_name (sdp_group_attr_e group_attr_type)
{
    if (group_attr_type == SDP_GROUP_ATTR_UNSUPPORTED) {
        return (SDP_UNSUPPORTED);
    } else if (group_attr_type >= SDP_MAX_GROUP_ATTR_VAL) {
        return ("Invalid a=group: attribute type");
    } else {
        return (sdp_group_attr_val[group_attr_type].name);
    }
}

const char *sdp_get_src_filter_mode_name (sdp_src_filter_mode_e type)
{
    if (type >= SDP_MAX_FILTER_MODE) {
        return ("Invalid source filter mode");
    } else {
        return (sdp_src_filter_mode_val[type].name);
    }
}

const char *sdp_get_rtcp_unicast_mode_name (sdp_rtcp_unicast_mode_e type)
{
    if (type >= SDP_RTCP_MAX_UNICAST_MODE) {
        return ("Invalid rtcp unicast mode");
    } else {
        return (sdp_rtcp_unicast_mode_val[type].name);
    }
}










tinybool sdp_verify_sdp_ptr (sdp_t *sdp_p)
{
    if ((sdp_p != NULL) && (sdp_p->magic_num == SDP_MAGIC_NUM)) {
        return (TRUE);
    } else {
        CSFLogError(logTag, "SDP: Invalid SDP pointer.");
        return (FALSE);
    }
}











sdp_t *sdp_init_description (sdp_conf_options_t *conf_p)
{
    int i;
    sdp_t *sdp_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return (NULL);
    }

    sdp_p = (sdp_t *)SDP_MALLOC(sizeof(sdp_t));
    if (sdp_p == NULL) {
        return (NULL);
    }

    
    sdp_p->magic_num = SDP_MAGIC_NUM;

    sdp_p->conf_p             = conf_p;
    sdp_p->version            = SDP_CURRENT_VERSION;
    sdp_p->owner_name[0]      = '\0';
    sdp_p->owner_sessid[0]    = '\0';
    sdp_p->owner_version[0]   = '\0';
    sdp_p->owner_network_type = SDP_NT_INVALID;
    sdp_p->owner_addr_type    = SDP_AT_INVALID;
    sdp_p->owner_addr[0]      = '\0';
    sdp_p->sessname[0]        = '\0';
    sdp_p->sessinfo_found     = FALSE;
    sdp_p->uri_found          = FALSE;

    sdp_p->default_conn.nettype      = SDP_NT_INVALID;
    sdp_p->default_conn.addrtype     = SDP_AT_INVALID;
    sdp_p->default_conn.conn_addr[0] = '\0';
    sdp_p->default_conn.is_multicast = FALSE;
    sdp_p->default_conn.ttl          = 0;
    sdp_p->default_conn.num_of_addresses = 0;

    sdp_p->bw.bw_data_count   = 0;
    sdp_p->bw.bw_data_list    = NULL;

    sdp_p->timespec_p         = NULL;
    sdp_p->sess_attrs_p       = NULL;
    sdp_p->mca_p              = NULL;
    sdp_p->mca_count          = 0;

    
    for (i=0; i < SDP_MAX_DEBUG_TYPES; i++) {
        sdp_p->debug_flag[i] = conf_p->debug_flag[i];
    }

    return (sdp_p);
}















void sdp_debug (sdp_t *sdp_p, sdp_debug_e debug_type, tinybool debug_flag)
{
    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return;
    }

    if (debug_type < SDP_MAX_DEBUG_TYPES)  {
        sdp_p->debug_flag[debug_type] = debug_flag;
    }
}












void sdp_set_string_debug (sdp_t *sdp_p, const char *debug_str)
{
    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return;
    }

    sstrncpy(sdp_p->debug_str, debug_str, sizeof(sdp_p->debug_str));
}








sdp_result_e sdp_validate_sdp (sdp_t *sdp_p)
{
    int i;
    uint16_t num_media_levels;

    


    if (sdp_connection_valid((void *)sdp_p, SDP_SESSION_LEVEL) == FALSE) {
        num_media_levels = sdp_get_num_media_lines((void *)sdp_p);
        for (i=1; i <= num_media_levels; i++) {
            if (sdp_connection_valid((void *)sdp_p, (unsigned short)i) == FALSE) {
                sdp_parse_error(sdp_p,
                    "%s c= connection line not specified for "
                    "every media level, validation failed.",
                    sdp_p->debug_str);
                return (SDP_FAILURE);
            }
        }
    }

    
    if ((sdp_owner_valid((void *)sdp_p) == FALSE) &&
        (sdp_p->conf_p->owner_reqd == TRUE)) {
        sdp_parse_error(sdp_p,
            "%s o= owner line not specified, validation failed.",
            sdp_p->debug_str);
        return (SDP_FAILURE);
    }

    if ((sdp_session_name_valid((void *)sdp_p) == FALSE) &&
        (sdp_p->conf_p->session_name_reqd == TRUE)) {
        sdp_parse_error(sdp_p,
            "%s s= session name line not specified, validation failed.",
            sdp_p->debug_str);
        return (SDP_FAILURE);
    }

    if ((sdp_timespec_valid((void *)sdp_p) == FALSE) &&
        (sdp_p->conf_p->timespec_reqd == TRUE)) {
        sdp_parse_error(sdp_p,
            "%s t= timespec line not specified, validation failed.",
            sdp_p->debug_str);
        return (SDP_FAILURE);
    }

    return (SDP_SUCCESS);
}











sdp_result_e sdp_parse (sdp_t *sdp_p, const char *buf, size_t len)
{
    uint8_t           i;
    uint16_t          cur_level = SDP_SESSION_LEVEL;
    const char  *ptr;
    const char  *next_ptr = NULL;
    char        *line_end;
    sdp_token_e  last_token = SDP_TOKEN_V;
    sdp_result_e result = SDP_SUCCESS;
    tinybool     parse_done = FALSE;
    tinybool     end_found = FALSE;
    tinybool     first_line = TRUE;
    tinybool     unrec_token = FALSE;
    const char   **bufp = &buf;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if ((bufp == NULL) || (*bufp == NULL)) {
        return (SDP_NULL_BUF_PTR);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Trace SDP Parse:", sdp_p->debug_str);
    }

    next_ptr = *bufp;
    sdp_p->conf_p->num_parses++;

    

    sdp_p->cap_valid = FALSE;
    sdp_p->last_cap_inst = 0;

    sdp_p->parse_line = 0;

    


    while (!end_found) {
        


        ptr = next_ptr;
        sdp_p->parse_line++;
        line_end = sdp_findchar(ptr, "\n");
        if ((line_end >= (*bufp + len)) ||
           (*line_end == '\0')) {
            



            sdp_parse_error(sdp_p,
                "%s End of line beyond end of buffer.",
                sdp_p->debug_str);
            CSFLogError(logTag, "SDP: Invalid SDP, no \\n (len %u): %*s",
                        (unsigned)len, (int)len, *bufp);
            end_found = TRUE;
            break;
        }

        
        if ((parse_done == FALSE) &&
          (sdp_p->debug_flag[SDP_DEBUG_TRACE])) {
            SDP_PRINT("%s ", sdp_p->debug_str);

            SDP_PRINT("%*s", (int)(line_end - ptr), ptr);

        }

        
        for (i=0; i < SDP_MAX_TOKENS; i++) {
            if (strncmp(ptr, sdp_token[i].name, SDP_TOKEN_LEN) == 0) {
                break;
            }
        }
        if (i == SDP_MAX_TOKENS) {
            

            if (ptr[1] == '=') {
                unrec_token = TRUE;
            }
            if (first_line == TRUE) {
                sdp_parse_error(sdp_p,
                    "%s Attempt to parse text not recognized as "
                    "SDP text, parse fails.", sdp_p->debug_str);
                    


                if (!sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
                    SDP_PRINT("%s ", sdp_p->debug_str);
                    SDP_PRINT("%*s", (int)(line_end - ptr), ptr);
                }
                sdp_p->conf_p->num_not_sdp_desc++;
                return (SDP_NOT_SDP_DESCRIPTION);
            } else {
                end_found = TRUE;
                break;
            }
        }

        
        if ((first_line != TRUE) && (i == SDP_TOKEN_V)) {
            end_found = TRUE;
            break;
        }

        
        next_ptr = line_end + 1;
        if (next_ptr >= (*bufp + len)) {
            end_found = TRUE;
        }

        


        if (parse_done == TRUE) {
            continue;
        }

        
        if (cur_level != SDP_SESSION_LEVEL) {
            if ((i != SDP_TOKEN_I) && (i != SDP_TOKEN_C) &&
                (i != SDP_TOKEN_B) && (i != SDP_TOKEN_K) &&
                (i != SDP_TOKEN_A) && (i != SDP_TOKEN_M)) {
                sdp_p->conf_p->num_invalid_token_order++;
                sdp_parse_error(sdp_p,
                    "%s Warning: Invalid token %s found at media level",
                    sdp_p->debug_str, sdp_token[i].name);
                continue;
            }
        }

        
        if (first_line == TRUE) {
            if (i != SDP_TOKEN_V) {
                if (sdp_p->conf_p->version_reqd == TRUE) {
                    sdp_parse_error(sdp_p,
                        "%s First line not v=, parse fails",
                        sdp_p->debug_str);
                    sdp_p->conf_p->num_invalid_token_order++;
                    result = SDP_INVALID_TOKEN_ORDERING;
                    parse_done = TRUE;
                } else {
                    last_token = (sdp_token_e)i;
                }
            } else {
                last_token = (sdp_token_e)i;
            }
            first_line = FALSE;
        } else {
            if (i < last_token) {
                sdp_p->conf_p->num_invalid_token_order++;
                sdp_parse_error(sdp_p,
                    "%s Warning: Invalid token ordering detected, "
                    "token %s found after token %s", sdp_p->debug_str,
                    sdp_token[i].name, sdp_token[last_token].name);
            }
        }

        
        ptr += SDP_TOKEN_LEN;
        result = sdp_token[i].parse_func(sdp_p, cur_level, (const char *)ptr);
        last_token = (sdp_token_e)i;
        if (last_token == SDP_TOKEN_M) {
            if (cur_level == SDP_SESSION_LEVEL) {
                cur_level = 1;
            } else {
                cur_level++;
            }
            
            last_token = (sdp_token_e)(SDP_TOKEN_I - 1);
        }
        if (result != SDP_SUCCESS) {
            parse_done = TRUE;
        }

        


        if ((line_end + 1) == (*bufp + len)) {
            end_found = TRUE;
        }
    }

    
    if (first_line == TRUE) {
        sdp_p->conf_p->num_not_sdp_desc++;
        return (SDP_NOT_SDP_DESCRIPTION);
    }

    
    if (result == SDP_SUCCESS) {
        result = sdp_validate_sdp(sdp_p);
    }
    
    *bufp = next_ptr;
    

    if ((result == SDP_SUCCESS) && (unrec_token == TRUE)) {
        return (SDP_UNRECOGNIZED_TOKEN);
    } else {
        return (result);
    }
}











sdp_result_e sdp_build (sdp_t *sdp_p, flex_string *fs)
{
    int i, j;
    sdp_result_e        result = SDP_SUCCESS;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (!fs) {
        return (SDP_NULL_BUF_PTR);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Trace SDP Build:", sdp_p->debug_str);
    }

    sdp_p->conf_p->num_builds++;

    for (i=0; ((i < SDP_TOKEN_M) &&
               (result == SDP_SUCCESS)); i++) {
        result = sdp_token[i].build_func(sdp_p, SDP_SESSION_LEVEL, fs);
        
    }
    
    if (result == SDP_SUCCESS) {
        for (i=1; ((i <= sdp_p->mca_count) &&
                   (result == SDP_SUCCESS)); i++) {
            result = sdp_token[SDP_TOKEN_M].build_func(sdp_p, (uint16_t)i, fs);

            
            for (j=SDP_TOKEN_I;
                 ((j < SDP_TOKEN_M) && (result == SDP_SUCCESS));
                 j++) {
                if ((j == SDP_TOKEN_U) || (j == SDP_TOKEN_E) ||
                    (j == SDP_TOKEN_P) || (j == SDP_TOKEN_T) ||
                    (j == SDP_TOKEN_R) || (j == SDP_TOKEN_Z)) {
                    
                    continue;
                }
                result = sdp_token[j].build_func(sdp_p, (uint16_t)i, fs);
                
            }
        }
    }

    return (result);
}








sdp_result_e sdp_free_description (sdp_t *sdp_p)
{
    sdp_timespec_t  *time_p, *next_time_p;
    sdp_attr_t      *attr_p, *next_attr_p;
    sdp_mca_t       *mca_p, *next_mca_p;
    sdp_bw_t        *bw_p;
    sdp_bw_data_t   *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    
    sdp_free_config(sdp_p->conf_p);

    


    time_p = sdp_p->timespec_p;
    while (time_p != NULL) {
        next_time_p = time_p->next_p;
        SDP_FREE(time_p);
        time_p = next_time_p;
    }

    bw_p = &(sdp_p->bw);
    bw_data_p = bw_p->bw_data_list;
    while (bw_data_p != NULL) {
        bw_p->bw_data_list = bw_data_p->next_p;
        SDP_FREE(bw_data_p);
        bw_data_p = bw_p->bw_data_list;
    }

    
    attr_p = sdp_p->sess_attrs_p;
    while (attr_p != NULL) {
        next_attr_p = attr_p->next_p;
        sdp_free_attr(attr_p);
        attr_p = next_attr_p;
    }

    
    mca_p = sdp_p->mca_p;
    while (mca_p != NULL) {
        next_mca_p = mca_p->next_p;

        
        attr_p = mca_p->media_attrs_p;
        while (attr_p != NULL) {
            next_attr_p = attr_p->next_p;
            sdp_free_attr(attr_p);
            attr_p = next_attr_p;
        }

        
        if (mca_p->media_profiles_p != NULL) {
            SDP_FREE(mca_p->media_profiles_p);
        }

        bw_p = &(mca_p->bw);
        bw_data_p = bw_p->bw_data_list;
        while (bw_data_p != NULL) {
            bw_p->bw_data_list = bw_data_p->next_p;
            SDP_FREE(bw_data_p);
            bw_data_p = bw_p->bw_data_list;
        }

        SDP_FREE(mca_p);
        mca_p = next_mca_p;
    }

    SDP_FREE(sdp_p);

    return (SDP_SUCCESS);
}





void sdp_parse_error(sdp_t* sdp, const char *format, ...) {
    flex_string fs;
    va_list ap;

    flex_string_init(&fs);

    va_start(ap, format);
    flex_string_vsprintf(&fs, format, ap);
    va_end(ap);

    CSFLogError("SDP Parse", "SDP Parse Error %s, line %u", fs.buffer,
                sdp->parse_line);

    if (sdp->conf_p->error_handler) {
        sdp->conf_p->error_handler(sdp->conf_p->error_handler_context,
                                   sdp->parse_line,
                                   fs.buffer);
    }

    flex_string_free(&fs);
}
