




"use strict";

do_get_profile(); 
const certdb  = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);


let certList = [
  'ee',
  'ca-1',
  'ca-2',
];

function load_cert(cert_name, trust_string) {
  var cert_filename = cert_name + ".der";
  addCertFromFile(certdb, "test_getchain/" + cert_filename, trust_string);
}




function get_ca_array() {
  let ret_array = new Array();
  let allCerts = certdb.getCerts();
  let enumerator = allCerts.getEnumerator();
  while (enumerator.hasMoreElements()) {
    let cert = enumerator.getNext().QueryInterface(Ci.nsIX509Cert);
    if (cert.commonName == 'ca') {
      ret_array[parseInt(cert.serialNumber)] = cert;
    }
  }
  return ret_array;
}


function check_matching_issuer_and_getchain(expected_issuer_serial, cert) {
  const nsIX509Cert = Components.interfaces.nsIX509Cert;

  equal(expected_issuer_serial, cert.issuer.serialNumber,
        "Expected and actual issuer serial numbers should match");
  let chain = cert.getChain();
  let issuer_via_getchain = chain.queryElementAt(1, nsIX509Cert);
  
  equal(cert.issuer.serialNumber, issuer_via_getchain.serialNumber,
        "Serial numbers via cert.issuer and via getChain() should match");
}

function check_getchain(ee_cert, ssl_ca, email_ca){
  
  
  

  const nsIX509Cert = Components.interfaces.nsIX509Cert;
  certdb.setCertTrust(ssl_ca, nsIX509Cert.CA_CERT,
                      Ci.nsIX509CertDB.TRUSTED_SSL);
  certdb.setCertTrust(email_ca, nsIX509Cert.CA_CERT,
                      Ci.nsIX509CertDB.TRUSTED_EMAIL);
  check_matching_issuer_and_getchain(ssl_ca.serialNumber, ee_cert);
  certdb.setCertTrust(ssl_ca, nsIX509Cert.CA_CERT, 0);
  check_matching_issuer_and_getchain(email_ca.serialNumber, ee_cert);
  certdb.setCertTrust(email_ca, nsIX509Cert.CA_CERT, 0);
  
  
  check_matching_issuer_and_getchain(ee_cert.issuer.serialNumber, ee_cert);
}

function run_test() {
  clearOCSPCache();
  clearSessionCache();

  for (let i = 0 ; i < certList.length; i++) {
    load_cert(certList[i], ',,');
  }

  let ee_cert = certdb.findCertByNickname(null, 'ee');
  notEqual(ee_cert, null, "EE cert should be in the cert DB");

  let ca = get_ca_array();

  check_getchain(ee_cert, ca[1], ca[2]);
  
  check_getchain(ee_cert, ca[2], ca[1]);
}
