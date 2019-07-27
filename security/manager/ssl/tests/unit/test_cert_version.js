




"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function cert_from_file(filename) {
  return constructCertFromFile("test_cert_version/" + filename);
}

function load_cert(cert_name, trust_string) {
  var cert_filename = cert_name + ".der";
  addCertFromFile(certdb, "test_cert_version/" + cert_filename, trust_string);
}

function check_cert_err_generic(cert, expected_error, usage) {
  do_print("cert cn=" + cert.commonName);
  do_print("cert issuer cn=" + cert.issuerCommonName);
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certdb.verifyCertNow(cert, usage,
                                   NO_FLAGS, verifiedChain, hasEVPolicy);
  do_check_eq(error, expected_error);
}

function check_cert_err(cert, expected_error) {
  check_cert_err_generic(cert, expected_error, certificateUsageSSLServer)
}

function check_ca_err(cert, expected_error) {
  check_cert_err_generic(cert, expected_error, certificateUsageSSLCA)
}

function check_ok(x) {
  return check_cert_err(x, PRErrorCodeSuccess);
}

function check_ok_ca(x) {
  return check_cert_err_generic(x, PRErrorCodeSuccess, certificateUsageSSLCA);
}

function run_test() {
  load_cert("v1_ca", "CTu,CTu,CTu");
  load_cert("v1_ca_bc", "CTu,CTu,CTu");
  load_cert("v2_ca", "CTu,CTu,CTu");
  load_cert("v2_ca_bc", "CTu,CTu,CTu");
  load_cert("v3_ca", "CTu,CTu,CTu");
  load_cert("v3_ca_missing_bc", "CTu,CTu,CTu");

  check_ok_ca(cert_from_file('v1_ca.der'));
  check_ok_ca(cert_from_file('v1_ca_bc.der'));
  check_ca_err(cert_from_file('v2_ca.der'), SEC_ERROR_CA_CERT_INVALID);
  check_ok_ca(cert_from_file('v2_ca_bc.der'));
  check_ok_ca(cert_from_file('v3_ca.der'));
  check_ca_err(cert_from_file('v3_ca_missing_bc.der'), SEC_ERROR_CA_CERT_INVALID);

  
  

  
  
  

  
  let error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v1_ca.der'), error);

  
  check_ok_ca(cert_from_file('v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v1_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v2_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v1_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v1_int_bc-v1_ca.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v1_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v1_ca.der'), error);

  
  check_ok_ca(cert_from_file('v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v1_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v2_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v2_int_bc-v1_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v2_int_bc-v1_ca.der'));

  
  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v1_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v1_ca.der'), error);

  
  check_ok_ca(cert_from_file('v3_int-v1_ca.der'));
  check_ok(cert_from_file('v1_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v2_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v3_int-v1_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v3_int-v1_ca.der'));

  
  
  

  
  error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v1_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v1_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v1_int_bc-v1_ca_bc.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v1_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v2_int_bc-v1_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v2_int_bc-v1_ca_bc.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v1_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v1_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v3_int-v1_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v3_int-v1_ca_bc.der'));


  
  
  

  
  error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v2_ca.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int_bc-v2_ca.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v2_ca.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int_bc-v2_ca.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v2_ca.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int-v2_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int-v2_ca.der'), error);

  
  error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v2_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v1_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v1_int_bc-v2_ca_bc.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v2_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v2_int_bc-v2_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v2_int_bc-v2_ca_bc.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v2_ca_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v2_ca_bc.der'), error);

  
  check_ok_ca(cert_from_file('v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v1_bc_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v2_bc_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v3_bc_ee-v3_int-v2_ca_bc.der'));
  check_ok(cert_from_file('v4_bc_ee-v3_int-v2_ca_bc.der'));

  
  
  

  
  error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v3_ca.der'), error);

  
  check_ok_ca(cert_from_file('v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v1_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v2_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v1_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v1_int_bc-v3_ca.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v3_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v3_ca.der'), error);

  
  check_ok_ca(cert_from_file('v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v1_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v2_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v2_int_bc-v3_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v2_int_bc-v3_ca.der'));

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v3_ca.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v3_ca.der'), error);

  
  check_ok_ca(cert_from_file('v3_int-v3_ca.der'));
  check_ok(cert_from_file('v1_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v2_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v3_missing_bc_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v3_bc_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v1_bc_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v2_bc_ee-v3_int-v3_ca.der'));
  check_ok(cert_from_file('v4_bc_ee-v3_int-v3_ca.der'));

  
  error = MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA;
  check_ca_err(cert_from_file('v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int-v3_ca_missing_bc.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v1_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v1_int_bc-v3_ca_missing_bc.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int-v3_ca_missing_bc.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v2_int_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v2_int_bc-v3_ca_missing_bc.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int_missing_bc-v3_ca_missing_bc.der'), error);

  
  error = SEC_ERROR_CA_CERT_INVALID;
  check_ca_err(cert_from_file('v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_missing_bc_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v3_bc_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v1_bc_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v2_bc_ee-v3_int-v3_ca_missing_bc.der'), error);
  check_cert_err(cert_from_file('v4_bc_ee-v3_int-v3_ca_missing_bc.der'), error);

  
  check_cert_err(cert_from_file('v1_self_signed.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v1_self_signed_bc.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v2_self_signed.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v2_self_signed_bc.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v3_self_signed.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v3_self_signed_bc.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v4_self_signed.der'), SEC_ERROR_UNKNOWN_ISSUER);
  check_cert_err(cert_from_file('v4_self_signed_bc.der'), SEC_ERROR_UNKNOWN_ISSUER);
}
