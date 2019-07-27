


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
const MockSecurityInfo = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITransportSecurityInfo,
                                         Ci.nsISSLStatusProvider]),
  securityState: wpl.STATE_IS_BROKEN,
  errorCode: 0,
  SSLStatus: {
    protocolVersion: 3, 
    cipherSuite: "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256",
  }
};

function run_test() {
  test_nullSecurityInfo();
  test_insecureSecurityInfoWithNSSError();
  test_insecureSecurityInfoWithoutNSSError();
  test_brokenSecurityInfo();
  test_secureSecurityInfo();
}




function test_nullSecurityInfo() {
  let result = NetworkHelper.parseSecurityInfo(null, {});
  equal(result.state, "insecure",
    "state == 'insecure' when securityInfo was undefined");
}




function test_insecureSecurityInfoWithNSSError() {
  MockSecurityInfo.securityState = wpl.STATE_IS_INSECURE;

  
  MockSecurityInfo.errorCode = -8180;

  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, {});
  equal(result.state, "broken",
    "state == 'broken' if securityState contains STATE_IS_INSECURE flag AND " +
    "errorCode is NSS error.");

  MockSecurityInfo.errorCode = 0;
}




function test_insecureSecurityInfoWithoutNSSError() {
  MockSecurityInfo.securityState = wpl.STATE_IS_INSECURE;

  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, {});
  equal(result.state, "insecure",
    "state == 'insecure' if securityState contains STATE_IS_INSECURE flag BUT " +
    "errorCode is not NSS error.");
}




function test_secureSecurityInfo() {
  MockSecurityInfo.securityState = wpl.STATE_IS_SECURE;

  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, {});
  equal(result.state, "secure",
    "state == 'secure' if securityState contains STATE_IS_SECURE flag");
}




function test_brokenSecurityInfo() {
  MockSecurityInfo.securityState = wpl.STATE_IS_BROKEN;

  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, {});
  equal(result.state, "insecure",
    "state == 'insecure' if securityState contains STATE_IS_BROKEN flag");
}
