




































#ifndef _nsCERTValInParamWrapper_H
#define _nsCERTValInParamWrapper_H

#include "nsISupports.h"
#include "cert.h"



























class nsCERTValInParamWrapper
{
 public:
    NS_IMETHOD_(nsrefcnt) AddRef();
    NS_IMETHOD_(nsrefcnt) Release();

public:
  nsCERTValInParamWrapper();
  virtual ~nsCERTValInParamWrapper();

  enum missing_cert_download_config { missing_cert_download_off = 0, missing_cert_download_on };
  enum crl_download_config { crl_local_only = 0, crl_download_allowed };
  enum ocsp_download_config { ocsp_off = 0, ocsp_on };
  enum ocsp_strict_config { ocsp_relaxed = 0, ocsp_strict };
  enum any_revo_fresh_config { any_revo_relaxed = 0, any_revo_strict };

  nsresult Construct(missing_cert_download_config ac, crl_download_config cdc,
                     ocsp_download_config odc, ocsp_strict_config osc,
                     any_revo_fresh_config arfc,
                     const char *firstNetworkRevocationMethod);

private:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
  PRBool mAlreadyConstructed;
  CERTValInParam *mCVIN;
  CERTRevocationFlags *mRev;
  
public:
  CERTValInParam *GetRawPointerForNSS() { return mCVIN; }
};

#endif
