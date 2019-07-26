




"use strict";

do_get_profile(); 
const certdb  = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);

let certList = [
  'ee',
  'int',
  'ca',
]

function load_cert(cert_name, trust_string) {
  let cert_filename = cert_name + ".der";
  addCertFromFile(certdb, "test_cert_trust/" + cert_filename, trust_string);
}

function setup_basic_trusts(ca_cert, int_cert) {
  certdb.setCertTrust(ca_cert, Ci.nsIX509Cert.CA_CERT,
                      Ci.nsIX509CertDB.TRUSTED_SSL |
                      Ci.nsIX509CertDB.TRUSTED_EMAIL |
                      Ci.nsIX509CertDB.TRUSTED_OBJSIGN);

  certdb.setCertTrust(int_cert, Ci.nsIX509Cert.CA_CERT, 0);
}

function check_cert_err_generic(cert, expected_error, usage) {
  do_print("cert cn=" + cert.commonName);
  do_print("cert issuer cn=" + cert.issuerCommonName);
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certdb.verifyCertNow(cert, usage,
                                   NO_FLAGS, verifiedChain, hasEVPolicy);
  do_check_eq(error,  expected_error);
};

function test_ca_distrust(ee_cert, cert_to_modify_trust, isRootCA, useInsanity) {
  
  check_cert_err_generic(ee_cert, 0, certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, 0, certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);  
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? 0
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner); 
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA); 
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder); 


  
  setCertTrust(cert_to_modify_trust, 'p,p,p');
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_UNTRUSTED_ISSUER
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);


  
  
  setCertTrust(cert_to_modify_trust, 'T,T,T');
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageSSLServer);

  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER 
                                                         : 0
                                           : 0,
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);

  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_INADEQUATE_CERT_TYPE
                                           : useInsanity ? 0
                                                         : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);


  
  setCertTrust(cert_to_modify_trust, 'p,C,C');
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, useInsanity ? 0  
                                              : SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? 0
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);

  
  setCertTrust(cert_to_modify_trust, ',C,C');
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? 0  
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, 0, certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? 0
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);

  
  setCertTrust(cert_to_modify_trust, 'C,p,C');
  check_cert_err_generic(ee_cert, 0, certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, isRootCA ? SEC_ERROR_UNTRUSTED_ISSUER
                                           : useInsanity ? SEC_ERROR_UNTRUSTED_ISSUER
                                                         : 0, 
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                         certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? 0
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);


  
  setCertTrust(cert_to_modify_trust, 'C,,C');
  check_cert_err_generic(ee_cert, 0, certificateUsageSSLServer);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageSSLClient);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageSSLCA);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageEmailSigner);
  check_cert_err_generic(ee_cert, isRootCA ? useInsanity ? SEC_ERROR_UNKNOWN_ISSUER
                                                         : SEC_ERROR_UNTRUSTED_ISSUER
                                           : 0,
                         certificateUsageEmailRecipient);
  check_cert_err_generic(ee_cert, useInsanity ? 0
                                              : SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageObjectSigner);
  check_cert_err_generic(ee_cert, useInsanity ? SEC_ERROR_CA_CERT_INVALID
                                              : SEC_ERROR_INVALID_ARGS,
                         certificateUsageVerifyCA);
  check_cert_err_generic(ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                         certificateUsageStatusResponder);
}


function run_test_in_mode(useInsanity) {
  Services.prefs.setBoolPref("security.use_insanity_verification", useInsanity);

  let ca_cert = certdb.findCertByNickname(null, 'ca');
  do_check_false(!ca_cert)
  let int_cert = certdb.findCertByNickname(null, 'int');
  do_check_false(!int_cert)
  let ee_cert = certdb.findCertByNickname(null, 'ee');
  do_check_false(!ee_cert);

  setup_basic_trusts(ca_cert, int_cert);
  test_ca_distrust(ee_cert, ca_cert, true, useInsanity);

  setup_basic_trusts(ca_cert, int_cert);
  test_ca_distrust(ee_cert, int_cert, false, useInsanity);
}

function run_test() {
  for (let i = 0 ; i < certList.length; i++) {
    load_cert(certList[i], ',,');
  }

  run_test_in_mode(true);
  run_test_in_mode(false);
}
