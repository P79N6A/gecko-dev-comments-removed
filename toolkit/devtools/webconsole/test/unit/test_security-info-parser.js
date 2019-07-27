


"use strict";



const { devtools } = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {});
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  writeable: false,
  enumerable: true
});

const Ci = Components.interfaces;
const wpl = Ci.nsIWebProgressListener;
const MockCertificate = {
  commonName: "cn",
  organization: "o",
  organizationalUnit: "ou",
  issuerCommonName: "issuerCN",
  issuerOrganization: "issuerO",
  issuerOrganizationUnit: "issuerOU",
  sha256Fingerprint: "qwertyuiopoiuytrewq",
  sha1Fingerprint: "qwertyuiop",
  validity: {
    notBeforeLocalDay: "yesterday",
    notAfterLocalDay: "tomorrow",
  }
};

const MockSecurityInfo = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITransportSecurityInfo,
                                         Ci.nsISSLStatusProvider]),
  securityState: wpl.STATE_IS_SECURE,
  errorCode: 0,
  SSLStatus: {
    cipherSuite: "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256",
    protocolVersion: 3, 
    serverCert: MockCertificate,
  }
};

function run_test() {
  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, {});

  equal(result.state, "secure", "State is correct.");

  equal(result.cipherSuite, MockSecurityInfo.cipherSuite,
    "Cipher suite is correct.");

  equal(result.protocolVersion, "TLSv1.2", "Protocol version is correct.");

  deepEqual(result.cert, NetworkHelper.parseCertificateInfo(MockCertificate),
    "Certificate information is correct.");

  equal(result.hpkp, false, "HPKP is false when URI is not available.");
  equal(result.hsts, false, "HSTS is false when URI is not available.");
}
