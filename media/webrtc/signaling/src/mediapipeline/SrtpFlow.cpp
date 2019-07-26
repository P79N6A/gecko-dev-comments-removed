





#include "logging.h"
#include "SrtpFlow.h"

#include "srtp.h"
#include "ssl.h"
#include "sslproto.h"

#include "mozilla/RefPtr.h"


using namespace mozilla;
MOZ_MTLOG_MODULE("mediapipeline")

namespace mozilla {

bool SrtpFlow::initialized;  

SrtpFlow::~SrtpFlow() {
  if (session_) {
    srtp_dealloc(session_);
  }
}

RefPtr<SrtpFlow> SrtpFlow::Create(int cipher_suite,
                                           bool inbound,
                                           const void *key,
                                           size_t key_len) {
  nsresult res = Init();
  if (!NS_SUCCEEDED(res))
    return nullptr;

  RefPtr<SrtpFlow> flow = new SrtpFlow();

  if (!key) {
    MOZ_MTLOG(ML_ERROR, "Null SRTP key specified");
    return nullptr;
  }

  if (key_len != SRTP_TOTAL_KEY_LENGTH) {
    MOZ_MTLOG(ML_ERROR, "Invalid SRTP key length");
    return nullptr;
  }

  srtp_policy_t policy;
  memset(&policy, 0, sizeof(srtp_policy_t));

  
  
  switch (cipher_suite) {
    case SRTP_AES128_CM_HMAC_SHA1_80:
      MOZ_MTLOG(ML_DEBUG,
                "Setting SRTP cipher suite SRTP_AES128_CM_HMAC_SHA1_80");
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
      break;
    case SRTP_AES128_CM_HMAC_SHA1_32:
      MOZ_MTLOG(ML_DEBUG,
                "Setting SRTP cipher suite SRTP_AES128_CM_HMAC_SHA1_32");
      crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp); 
      break;                                                   
    default:
      MOZ_MTLOG(ML_ERROR, "Request to set unknown SRTP cipher suite");
      return nullptr;
  }
  
  
  policy.key = const_cast<unsigned char *>(
      static_cast<const unsigned char *>(key));
  policy.ssrc.type = inbound ? ssrc_any_inbound : ssrc_any_outbound;
  policy.ssrc.value = 0;
  policy.ekt = nullptr;
  policy.window_size = 1024;   
  policy.allow_repeat_tx = 1;  
  policy.next = nullptr;

  
  err_status_t r = srtp_create(&flow->session_, &policy);
  if (r != err_status_ok) {
    MOZ_MTLOG(ML_ERROR, "Error creating srtp session");
    return nullptr;
  }

  return flow;
}


nsresult SrtpFlow::CheckInputs(bool protect, void *in, int in_len,
                               int max_len, int *out_len) {
  MOZ_ASSERT(in);
  if (!in) {
    MOZ_MTLOG(ML_ERROR, "NULL input value");
    return NS_ERROR_NULL_POINTER;
  }

  if (in_len < 0) {
    MOZ_MTLOG(ML_ERROR, "Input length is negative");
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (max_len < 0) {
    MOZ_MTLOG(ML_ERROR, "Max output length is negative");
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (protect) {
    if ((max_len < SRTP_MAX_EXPANSION) ||
        ((max_len - SRTP_MAX_EXPANSION) < in_len)) {
      MOZ_MTLOG(ML_ERROR, "Output too short");
      return NS_ERROR_ILLEGAL_VALUE;
    }
  }
  else {
    if (in_len > max_len) {
      MOZ_MTLOG(ML_ERROR, "Output too short");
      return NS_ERROR_ILLEGAL_VALUE;
    }
  }

  return NS_OK;
}

nsresult SrtpFlow::ProtectRtp(void *in, int in_len,
                              int max_len, int *out_len) {
  nsresult res = CheckInputs(true, in, in_len, max_len, out_len);
  if (NS_FAILED(res))
    return res;

  int len = in_len;
  err_status_t r = srtp_protect(session_, in, &len);

  if (r != err_status_ok) {
    MOZ_MTLOG(ML_ERROR, "Error protecting SRTP packet");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(len <= max_len);
  *out_len = len;


  MOZ_MTLOG(ML_DEBUG, "Successfully protected an SRTP packet of len "
            << *out_len);

  return NS_OK;
}

nsresult SrtpFlow::UnprotectRtp(void *in, int in_len,
                                int max_len, int *out_len) {
  nsresult res = CheckInputs(false, in, in_len, max_len, out_len);
  if (NS_FAILED(res))
    return res;

  int len = in_len;
  err_status_t r = srtp_unprotect(session_, in, &len);

  if (r != err_status_ok) {
    MOZ_MTLOG(ML_ERROR, "Error unprotecting SRTP packet error=" << (int)r);
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(len <= max_len);
  *out_len = len;

  MOZ_MTLOG(ML_DEBUG, "Successfully unprotected an SRTP packet of len "
            << *out_len);

  return NS_OK;
}

nsresult SrtpFlow::ProtectRtcp(void *in, int in_len,
                               int max_len, int *out_len) {
  nsresult res = CheckInputs(true, in, in_len, max_len, out_len);
  if (NS_FAILED(res))
    return res;

  int len = in_len;
  err_status_t r = srtp_protect_rtcp(session_, in, &len);

  if (r != err_status_ok) {
    MOZ_MTLOG(ML_ERROR, "Error protecting SRTCP packet");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(len <= max_len);
  *out_len = len;

  MOZ_MTLOG(ML_DEBUG, "Successfully protected an SRTCP packet of len "
            << *out_len);

  return NS_OK;
}

nsresult SrtpFlow::UnprotectRtcp(void *in, int in_len,
                                 int max_len, int *out_len) {
  nsresult res = CheckInputs(false, in, in_len, max_len, out_len);
  if (NS_FAILED(res))
    return res;

  int len = in_len;
  err_status_t r = srtp_unprotect_rtcp(session_, in, &len);

  if (r != err_status_ok) {
    MOZ_MTLOG(ML_ERROR, "Error unprotecting SRTCP packet error=" << (int)r);
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(len <= max_len);
  *out_len = len;

  MOZ_MTLOG(ML_DEBUG, "Successfully unprotected an SRTCP packet of len "
            << *out_len);

  return NS_OK;
}


void SrtpFlow::srtp_event_handler(srtp_event_data_t *data) {
  
  MOZ_CRASH();
}

nsresult SrtpFlow::Init() {
  if (!initialized) {
    err_status_t r = srtp_init();
    if (r != err_status_ok) {
      MOZ_MTLOG(ML_ERROR, "Could not initialize SRTP");
      MOZ_ASSERT(PR_FALSE);
      return NS_ERROR_FAILURE;
    }

    r = srtp_install_event_handler(&SrtpFlow::srtp_event_handler);
    if (r != err_status_ok) {
      MOZ_MTLOG(ML_ERROR, "Could not install SRTP event handler");
      MOZ_ASSERT(PR_FALSE);
      return NS_ERROR_FAILURE;
    }

    initialized = true;
  }

  return NS_OK;
}

}  

