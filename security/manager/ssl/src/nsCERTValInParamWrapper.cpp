




































#include "nsCERTValInParamWrapper.h"

NS_IMPL_THREADSAFE_ADDREF(nsCERTValInParamWrapper)
NS_IMPL_THREADSAFE_RELEASE(nsCERTValInParamWrapper)

nsCERTValInParamWrapper::nsCERTValInParamWrapper()
:mAlreadyConstructed(PR_FALSE)
,mCVIN(nsnull)
,mRev(nsnull)
{
  MOZ_COUNT_CTOR(nsCERTValInParamWrapper);
}

nsCERTValInParamWrapper::~nsCERTValInParamWrapper()
{
  MOZ_COUNT_DTOR(nsCERTValInParamWrapper);
  if (mRev) {
    CERT_DestroyCERTRevocationFlags(mRev);
  }
  if (mCVIN)
    PORT_Free(mCVIN);
}

nsresult nsCERTValInParamWrapper::Construct(missing_cert_download_config mcdc,
                                            crl_download_config cdc,
                                            ocsp_download_config odc,
                                            ocsp_strict_config osc,
                                            any_revo_fresh_config arfc,
                                            const char *firstNetworkRevocationMethod)
{
  if (mAlreadyConstructed)
    return NS_ERROR_FAILURE;

  CERTValInParam *p = (CERTValInParam*)PORT_Alloc(3 * sizeof(CERTValInParam));
  if (!p)
    return NS_ERROR_OUT_OF_MEMORY;

  CERTRevocationFlags *rev = CERT_AllocCERTRevocationFlags(
      cert_revocation_method_ocsp +1, 1,
      cert_revocation_method_ocsp +1, 1);
  
  if (!rev) {
    PORT_Free(p);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  p[0].type = cert_pi_useAIACertFetch;
  p[0].value.scalar.b = (mcdc == missing_cert_download_on);
  p[1].type = cert_pi_revocationFlags;
  p[1].value.pointer.revocation = rev;
  p[2].type = cert_pi_end;
  
  rev->leafTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
  rev->chainTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
    
    CERT_REV_M_IGNORE_IMPLICIT_DEFAULT_SOURCE

    
    | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO

    
    | CERT_REV_M_IGNORE_MISSING_FRESH_INFO

    
    | CERT_REV_M_TEST_USING_THIS_METHOD

    
    | CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE

    
    | ((cdc == crl_download_allowed) ?
        CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)
    ;

  rev->leafTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
  rev->chainTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
    
    ((odc == ocsp_on) ?
      CERT_REV_M_TEST_USING_THIS_METHOD : CERT_REV_M_DO_NOT_TEST_USING_THIS_METHOD)

    
    | ((odc == ocsp_on) ?
        CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)

    
    | ((osc == ocsp_strict) ?
        CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO : CERT_REV_M_IGNORE_MISSING_FRESH_INFO)

    
    | CERT_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE

    
    | CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE

    
    | CERT_REV_M_STOP_TESTING_ON_FRESH_INFO
    ;

  PRBool wantsCrlFirst = (firstNetworkRevocationMethod != nsnull)
                          && (strcmp("crl", firstNetworkRevocationMethod) == 0);
    
  rev->leafTests.preferred_methods[0] =
  rev->chainTests.preferred_methods[0] =
    wantsCrlFirst ? cert_revocation_method_crl : cert_revocation_method_ocsp;

  rev->leafTests.cert_rev_method_independent_flags =
  rev->chainTests.cert_rev_method_independent_flags =
    
    CERT_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST

    
    | ((arfc == any_revo_strict) ?
        CERT_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE : CERT_REV_MI_NO_OVERALL_INFO_REQUIREMENT)
    ;

  mAlreadyConstructed = PR_TRUE;
  mCVIN = p;
  mRev = rev;
  return NS_OK;
}
