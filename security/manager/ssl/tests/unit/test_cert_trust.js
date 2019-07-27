




"use strict";

do_get_profile(); 
const certdb  = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);

let certList = [
  'ee',
  'int',
  'ca',
];

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

function test_ca_distrust(ee_cert, cert_to_modify_trust, isRootCA) {
  
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);


  
  setCertTrust(cert_to_modify_trust, 'p,p,p');
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert,
                        !isRootCA ? SEC_ERROR_UNTRUSTED_ISSUER
                                  : SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);

  
  
  setCertTrust(cert_to_modify_trust, 'T,T,T');
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageSSLServer);

  
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageSSLClient);

  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);

  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);


  
  setCertTrust(cert_to_modify_trust, 'p,C,C');
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageSSLServer);

  
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert,
                        isRootCA ? SEC_ERROR_INADEQUATE_CERT_TYPE
                                 : SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageStatusResponder);

  
  setCertTrust(cert_to_modify_trust, ',C,C');
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageSSLServer);
  
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);

  
  setCertTrust(cert_to_modify_trust, 'C,p,C');
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_UNTRUSTED_ISSUER,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);


  
  setCertTrust(cert_to_modify_trust, 'C,,C');
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, ee_cert, isRootCA ? SEC_ERROR_UNKNOWN_ISSUER
                                                  : PRErrorCodeSuccess,
                        certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, ee_cert, PRErrorCodeSuccess,
                        certificateUsageObjectSigner);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_CA_CERT_INVALID,
                        certificateUsageVerifyCA);
  checkCertErrorGeneric(certdb, ee_cert, SEC_ERROR_INADEQUATE_CERT_TYPE,
                        certificateUsageStatusResponder);
}


function run_test() {
  for (let i = 0 ; i < certList.length; i++) {
    load_cert(certList[i], ',,');
  }

  let ca_cert = certdb.findCertByNickname(null, 'ca');
  do_check_false(!ca_cert)
  let int_cert = certdb.findCertByNickname(null, 'int');
  do_check_false(!int_cert)
  let ee_cert = certdb.findCertByNickname(null, 'ee');
  do_check_false(!ee_cert);

  setup_basic_trusts(ca_cert, int_cert);
  test_ca_distrust(ee_cert, ca_cert, true);

  setup_basic_trusts(ca_cert, int_cert);
  test_ca_distrust(ee_cert, int_cert, false);
}
