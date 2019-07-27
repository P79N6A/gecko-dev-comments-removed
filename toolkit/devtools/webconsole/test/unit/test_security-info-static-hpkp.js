


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
  securityState: wpl.STATE_IS_SECURE,
  errorCode: 0,
  SSLStatus: {
    cipherSuite: "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256",
    protocolVersion: 3, 
    serverCert: {
      validity: {}
    },
  }
};

const MockRequest = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrivateBrowsingChannel]),
  URI: {
    host: "include-subdomains.pinning.example.com"
  }
};

function run_test() {
  let result = NetworkHelper.parseSecurityInfo(MockSecurityInfo, MockRequest);
  equal(result.hpkp, true, "Static HPKP detected.");
}
