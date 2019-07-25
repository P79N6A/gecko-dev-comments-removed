# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


var gDataProvider = null;















function PROT_Application() {
  this.debugZone= "application";

#ifdef DEBUG
  
  function runUnittests() {
    if (false) {

      G_DebugL("UNITTESTS", "STARTING UNITTESTS");
      TEST_G_Protocol4Parser();
      TEST_G_CryptoHasher();
      TEST_PROT_EnchashDecrypter();
      TEST_PROT_TRTable();
      TEST_PROT_ListManager();
      TEST_PROT_PhishingWarden();
      TEST_PROT_TRFetcher();
      TEST_PROT_URLCanonicalizer();
      TEST_G_Preferences();
      TEST_G_Observer();
      TEST_PROT_WireFormat();
      
      TEST_PROT_UrlCrypto();
      TEST_PROT_UrlCryptoKeyManager();
      G_DebugL("UNITTESTS", "END UNITTESTS");
    }
  };

  runUnittests();
#endif
  
  
  this.PROT_PhishingWarden = PROT_PhishingWarden;
  this.PROT_MalwareWarden = PROT_MalwareWarden;

  
  gDataProvider = new PROT_DataProvider();

  
  this.wrappedJSObject = this;
}

var gInitialized = false;
PROT_Application.prototype.initialize = function() {
  if (gInitialized)
    return;
  gInitialized = true;

  var obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.addObserver(this, "xpcom-shutdown", true);

  
  
  this.malwareWarden = new PROT_MalwareWarden();
  this.malwareWarden.registerBlackTable("goog-malware-shavar");
  this.malwareWarden.maybeToggleUpdateChecking();

  this.phishWarden = new PROT_PhishingWarden();
  this.phishWarden.registerBlackTable("goog-phish-shavar");
  this.phishWarden.maybeToggleUpdateChecking();
}

PROT_Application.prototype.observe = function(subject, topic, data) {
  switch (topic) {
    case "xpcom-shutdown":
      this.malwareWarden.shutdown();
      this.phishWarden.shutdown();
      break;
  }
}





PROT_Application.prototype.getReportURL = function(name) {
  return gDataProvider["getReport" + name + "URL"]();
}

PROT_Application.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) ||
      iid.equals(Ci.nsISupportsWeakReference) ||
      iid.equals(Ci.nsIObserver))
    return this;

  throw Components.results.NS_ERROR_NO_INTERFACE;
}
