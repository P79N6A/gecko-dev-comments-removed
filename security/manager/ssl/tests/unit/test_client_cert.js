



"use strict";




function run_test() {
  do_get_profile();

  
  const tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"]
                    .getService(Ci.nsIPK11TokenDB);
  let keyToken = tokenDB.getInternalKeyToken();
  if (keyToken.needsUserInit) {
    keyToken.initPassword("");
  }

  
  
  do_load_manifest("test_client_cert/cert_dialog.manifest");

  
  const certDB = Cc["@mozilla.org/security/x509certdb;1"]
                   .getService(Ci.nsIX509CertDB);
  let clientCertFile = do_get_file("test_client_cert/client-cert.p12", false);
  certDB.importPKCS12File(null, clientCertFile);

  
  let clientCert;
  let certs = certDB.getCerts().getEnumerator();
  while (certs.hasMoreElements()) {
    let cert = certs.getNext().QueryInterface(Ci.nsIX509Cert);
    if (cert.certType === Ci.nsIX509Cert.USER_CERT &&
        cert.commonName === "client-cert") {
      clientCert = cert;
      break;
    }
  }
  ok(clientCert, "Client cert found");

  add_tls_server_setup("ClientAuthServer");

  add_connection_test("noclientauth.example.com", Cr.NS_OK);

  add_connection_test("requestclientauth.example.com", Cr.NS_OK);
  add_connection_test("requestclientauth.example.com", Cr.NS_OK,
                      null, null, transport => {
    do_print("Setting client cert on transport");
    let sslSocketControl = transport.securityInfo
                           .QueryInterface(Ci.nsISSLSocketControl);
    sslSocketControl.clientCert = clientCert;
  });

  add_connection_test("requireclientauth.example.com",
                      getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_ALERT));
  add_connection_test("requireclientauth.example.com", Cr.NS_OK,
                      null, null, transport => {
    do_print("Setting client cert on transport");
    let sslSocketControl =
      transport.securityInfo.QueryInterface(Ci.nsISSLSocketControl);
    sslSocketControl.clientCert = clientCert;
  });

  run_next_test();
}
