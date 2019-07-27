




"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function certFromFile(filename) {
  let der = readFile(do_get_file("test_name_constraints/" + filename, false));
  return certdb.constructX509(der, der.length);
}

function load_cert(cert_name, trust_string) {
  var cert_filename = cert_name + ".der";
  addCertFromFile(certdb, "test_name_constraints/" + cert_filename, trust_string);
  return certFromFile(cert_filename);
}

function check_cert_err_generic(cert, expected_error, usage) {
  do_print("cert cn=" + cert.commonName);
  do_print("cert issuer cn=" + cert.issuerCommonName);
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certdb.verifyCertNow(cert, usage,
                                   NO_FLAGS, verifiedChain, hasEVPolicy);
  do_check_eq(error,  expected_error);
}

function check_cert_err(cert, expected_error) {
  check_cert_err_generic(cert, expected_error, certificateUsageSSLServer)
}

function check_ok(x) {
  return check_cert_err(x, PRErrorCodeSuccess);
}

function check_ok_ca (x) {
  return check_cert_err_generic(x, PRErrorCodeSuccess, certificateUsageSSLCA);
}

function check_fail(x) {
  return check_cert_err(x, SEC_ERROR_CERT_NOT_IN_NAME_SPACE);
}

function check_fail_ca(x) {
  return check_cert_err_generic(x, SEC_ERROR_CERT_NOT_IN_NAME_SPACE, certificateUsageSSLCA);
}

function run_test() {
  load_cert("ca-nc-perm-foo.com", "CTu,CTu,CTu");
  load_cert("ca-nc", "CTu,CTu,CTu");

  

  
  
  
  check_ok_ca(load_cert('int-nc-perm-foo.com-ca-nc', ',,'));
  
  check_ok(certFromFile('cn-www.foo.com-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-perm-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-perm-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-perm-foo.com-ca-nc.der'));
  
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-perm-foo.com-ca-nc.der'));
  
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-perm-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-perm-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-perm-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-perm-foo.com-ca-nc.der'));
  
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-perm-foo.com-ca-nc.der'));

  
  
  
  check_ok_ca(load_cert('int-nc-excl-foo.com-ca-nc', ',,'));
  
  check_fail(certFromFile('cn-www.foo.com-int-nc-excl-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org-int-nc-excl-foo.com-ca-nc.der'));
  
  
  
  check_ok(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-excl-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-excl-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-excl-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-excl-foo.com-ca-nc.der'));
  
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-excl-foo.com-ca-nc.der'));
  
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-excl-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-excl-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-excl-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-excl-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-excl-foo.com-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-excl-foo.com-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-excl-foo.com-ca-nc.der'));

  
  
  
  check_ok_ca(load_cert('int-nc-c-us-ca-nc', ',,'));
  check_fail(certFromFile('cn-www.foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-c-us-ca-nc.der'));

  
  
  
  
  check_ok_ca(load_cert('int-nc-foo.com-int-nc-c-us-ca-nc', ',,'));
  check_fail(certFromFile('cn-www.foo.com-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-foo.com-int-nc-c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com-int-nc-c-us-ca-nc.der'));

  
  
  
  check_ok_ca(load_cert('int-nc-perm-foo.com_c-us-ca-nc' , ',,'));
  check_fail(certFromFile('cn-www.foo.com-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-perm-foo.com_c-us-ca-nc.der'));
  
  
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-perm-foo.com_c-us-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-perm-foo.com_c-us-ca-nc.der'));

  
  
  
  check_ok_ca(load_cert('int-nc-perm-c-uk-ca-nc', ',,'));
  check_fail(certFromFile('cn-www.foo.com-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-perm-c-uk-ca-nc.der'));

  
  
  
  check_fail_ca(load_cert('int-c-us-int-nc-perm-c-uk-ca-nc', ',,'));
  check_fail(certFromFile('cn-www.foo.com-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.com-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-c-us-int-nc-perm-c-uk-ca-nc.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-c-us-int-nc-perm-c-uk-ca-nc.der'));

  
  
  check_ok_ca(load_cert('int-nc-foo.com_a.us', ',,'));
  check_ok(certFromFile('cn-www.foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com_a.us.der'));

  
  
  
  
  
  
  check_ok_ca(load_cert('int-nc-foo.com-int-nc-foo.com_a.us', ',,'));
  check_ok(certFromFile('cn-www.foo.com-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.org-alt-foo.com-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com-alt-foo.com-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-nc-foo.com-int-nc-foo.com_a.us.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-nc-foo.com-int-nc-foo.com_a.us.der'));

  
  
  
  check_ok_ca(load_cert('int-ca-nc-perm-foo.com', ',,'));
  check_ok(certFromFile('cn-www.foo.com-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.org-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.org-int-ca-nc-perm-foo.com.der'));
  check_ok(certFromFile('cn-www.foo.org-alt-foo.com-int-ca-nc-perm-foo.com.der'));
  check_ok(certFromFile('cn-www.foo.com-alt-foo.com-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.org-alt-foo.org-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.com-alt-foo.com-a.a.us-b.a.us-int-ca-nc-perm-foo.com.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.org-int-ca-nc-perm-foo.com.der'));
  check_ok(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.com-int-ca-nc-perm-foo.com.der'));
  check_ok(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.org_o-bar_c-us-alt-foo.org-int-ca-nc-perm-foo.com.der'));
  check_fail(certFromFile('cn-www.foo.com_o-bar_c-us-alt-foo.com-a.a.us-b.a.us-int-ca-nc-perm-foo.com.der'));

  
  
  
  {
    let cert = certFromFile('cn-www.foo.org-int-nc-perm-foo.com-ca-nc.der');
    check_cert_err_generic(cert, SEC_ERROR_CERT_NOT_IN_NAME_SPACE, certificateUsageSSLServer);
    check_cert_err_generic(cert, PRErrorCodeSuccess, certificateUsageSSLClient);
  }

  
  
  
  load_cert("dcisscopy", "C,C,C");
  check_ok(certFromFile('NameConstraints.dcissallowed.cert'));
  check_fail(certFromFile('NameConstraints.dcissblocked.cert'));
}
