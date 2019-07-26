




"use strict";

















do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

function load_ca(ca_name) {
  let ca_filename = ca_name + ".der";
  addCertFromFile(certdb, "test_cert_signatures/" + ca_filename, 'CTu,CTu,CTu');
}

function check_ca(ca_name) {
  do_print("ca_name=" + ca_name);
  let cert = certdb.findCertByNickname(null, ca_name);

  let verified = {};
  let usages = {};
  cert.getUsagesString(true, verified, usages);
  do_check_eq('SSL CA', usages.value);
}

function run_test() {
  
  load_ca("ca-rsa");
  load_ca("ca-p384");
  load_ca("ca-dsa");

  clearOCSPCache();
  clearSessionCache();

  check_ca("ca-rsa");
  check_ca("ca-p384");
  check_ca("ca-dsa");

  
  
  const int_usage = 'SSL CA';

  
  const ee_usage = 'Client,Server,Sign,Encrypt,Object Signer';

  let cert2usage = {
    
    'int-rsa-valid': int_usage,
    'rsa-valid': ee_usage,
    'int-p384-valid': int_usage,
    'p384-valid': ee_usage,
    'int-dsa-valid': int_usage,
    'dsa-valid': ee_usage,

    'rsa-valid-int-tampered-ee': "",
    'p384-valid-int-tampered-ee': "",
    'dsa-valid-int-tampered-ee': "",

    'int-rsa-tampered': "",
    'rsa-tampered-int-valid-ee': "",
    'int-p384-tampered': "",
    'p384-tampered-int-valid-ee': "",
    'int-dsa-tampered': "",
    'dsa-tampered-int-valid-ee': "",

  };

  
  for (let cert_name in cert2usage) {
    let cert_filename = cert_name + ".der";
    addCertFromFile(certdb, "test_cert_signatures/" + cert_filename, ',,');
  }

  for (let cert_name in cert2usage) {
    do_print("cert_name=" + cert_name);

    let cert = certdb.findCertByNickname(null, cert_name);

    let verified = {};
    let usages = {};
    cert.getUsagesString(true, verified, usages);
    do_check_eq(cert2usage[cert_name], usages.value);
  }
}
