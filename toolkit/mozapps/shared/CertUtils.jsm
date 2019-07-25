#if 0







































#endif
EXPORTED_SYMBOLS = [ "BadCertHandler", "checkCert" ];

const Ce = Components.Exception;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;





















function checkCert(aChannel, aCerts) {
  if (!aChannel.originalURI.schemeIs("https")) {
    
    if (aCerts) {
      throw new Ce("SSL is required and URI scheme is not https.",
                   Cr.NS_ERROR_UNEXPECTED);
    }
    return;
  }

  var cert =
      aChannel.securityInfo.QueryInterface(Ci.nsISSLStatusProvider).
      SSLStatus.QueryInterface(Ci.nsISSLStatus).serverCert;

  if (aCerts) {
    for (var i = 0; i < aCerts.length; ++i) {
      var error = false;
      var certAttrs = aCerts[i];
      for (var name in certAttrs) {
        if (!(name in cert)) {
          error = true;
          Cu.reportError("Expected attribute '" + name + "' not present in " +
                         "certificate.");
          break;
        }
        if (cert[name] != certAttrs[name]) {
          error = true;
          Cu.reportError("Expected certificate attribute '" + name + "' " +
                         "value incorrect, expected: '" + certAttrs[name] +
                         "', got: '" + cert[name] + "'.");
          break;
        }
      }

      if (!error)
        break;
    }

    if (error) {
      const certCheckErr = "Certificate checks failed. See previous errors " +
                           "for details.";
      Cu.reportError(certCheckErr);
      throw new Ce(certCheckErr, Cr.NS_ERROR_ILLEGAL_VALUE);
    }
  }


  var issuerCert = cert;
  while (issuerCert.issuer && !issuerCert.issuer.equals(issuerCert))
    issuerCert = issuerCert.issuer;

  const certNotBuiltInErr = "Certificate issuer is not built-in.";
  if (!issuerCert)
    throw new Ce(certNotBuiltInErr, Cr.NS_ERROR_ABORT);

  issuerCert = issuerCert.QueryInterface(Ci.nsIX509Cert3);
  var tokenNames = issuerCert.getAllTokenNames({});

  if (!tokenNames || !tokenNames.some(isBuiltinToken))
    throw new Ce(certNotBuiltInErr, Cr.NS_ERROR_ABORT);
}

function isBuiltinToken(tokenName) {
  return tokenName == "Builtin Object Token";
}






function BadCertHandler(aAllowNonBuiltInCerts) {
  this.allowNonBuiltInCerts = aAllowNonBuiltInCerts;
}
BadCertHandler.prototype = {

  
  onChannelRedirect: function(oldChannel, newChannel, flags) {
    if (this.allowNonBuiltInCerts)
      return;

    
    
    
    if (!(flags & Ci.nsIChannelEventSink.REDIRECT_INTERNAL))
      checkCert(oldChannel);
  },

  
  notifyCertProblem: function(socketInfo, status, targetSite) {
    return true;
  },

  
  notifySSLError: function(socketInfo, error, targetSite) {
    return true;
  },

  
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },

  
  QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsIChannelEventSink) &&
        !iid.equals(Ci.nsIBadCertListener2) &&
        !iid.equals(Ci.nsISSLErrorListener) &&
        !iid.equals(Ci.nsIInterfaceRequestor) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
