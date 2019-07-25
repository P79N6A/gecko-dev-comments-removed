






































let Cc = Components.classes;
let Ci = Components.interfaces;





function SSLExceptions() {
  this._overrideService = Cc["@mozilla.org/security/certoverride;1"]
                          .getService(Ci.nsICertOverrideService);
}


SSLExceptions.prototype = {
  _overrideService: null,
  _sslStatus: null,

  getInterface: function SSLE_getInterface(aIID) {
    return this.QueryInterface(aIID);
  },
  QueryInterface: function SSLE_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIBadCertListener2) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  



  notifyCertProblem: function SSLE_notifyCertProblem(socketInfo, sslStatus, targetHost) {
    this._sslStatus = sslStatus.QueryInterface(Ci.nsISSLStatus);
    return true; 
  },

  


  _inPrivateBrowsingMode: function SSLE_inPrivateBrowsingMode() {
    try {
      var pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
      return pb.privateBrowsingEnabled;
    } catch (ex) {
      Components.utils.reportError("Could not get the Private Browsing service");
    }
    return false;
  },

  



  _checkCert: function SSLE_checkCert(aURI) {
    this._sslStatus = null;
  
    var req = new XMLHttpRequest();
    try {
      if (aURI) {
        req.open("GET", aURI.prePath, false);
        req.channel.notificationCallbacks = this;
        req.send(null);
      }
    } catch (e) {
      
      
      
      Components.utils.reportError("Attempted to connect to a site with a bad certificate in the add exception dialog. " +
                                   "This results in a (mostly harmless) exception being thrown. " +
                                   "Logged for information purposes only: " + e);
    }

    return this._sslStatus;
  },

  


  _addOverride: function SSLE_addOverride(aURI, temporary) {
    var SSLStatus = this._checkCert(aURI);
    var certificate = SSLStatus.serverCert;

    var flags = 0;

    
    if (this._inPrivateBrowsingMode()) {
      temporary = true;
    }

    if(SSLStatus.isUntrusted)
      flags |= this._overrideService.ERROR_UNTRUSTED;
    if(SSLStatus.isDomainMismatch)
      flags |= this._overrideService.ERROR_MISMATCH;
    if(SSLStatus.isNotValidAtThisTime)
      flags |= this._overrideService.ERROR_TIME;

    this._overrideService.rememberValidityOverride(
      aURI.asciiHost,
      aURI.port,
      certificate,
      flags,
      temporary);
  },

  



  addPermanentException: function SSLE_addPermanentException(aURI) {
    this._addOverride(aURI, false);
  },

  



  addTemporaryException: function SSLE_addTemporaryException(aURI) {
    this._addOverride(aURI, true);
  }
};
