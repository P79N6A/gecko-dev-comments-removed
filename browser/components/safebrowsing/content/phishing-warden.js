# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:





























const kPhishWardenEnabledPref = "browser.safebrowsing.enabled";









function PROT_PhishingWarden() {
  PROT_ListWarden.call(this);

  this.debugZone = "phishwarden";

  
  this.prefs_ = new G_Preferences();

  
  

  
  this.phishWardenEnabled_ = this.prefs_.getPref(kPhishWardenEnabledPref, null);

  
  var phishWardenPrefObserver = 
    BindToObject(this.onPhishWardenEnabledPrefChanged, this);
  this.prefs_.addObserver(kPhishWardenEnabledPref, phishWardenPrefObserver);

  G_Debug(this, "phishWarden initialized");
}

PROT_PhishingWarden.inherits(PROT_ListWarden);

PROT_PhishingWarden.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) || 
      iid.equals(Ci.nsISupportsWeakReference))
    return this;
  throw Components.results.NS_ERROR_NO_INTERFACE;
}




PROT_PhishingWarden.prototype.shutdown = function() {
  this.prefs_.removeAllObservers();
  this.listManager_ = null;
}











 
PROT_PhishingWarden.prototype.maybeToggleUpdateChecking = function() {
  var phishWardenEnabled = this.prefs_.getPref(kPhishWardenEnabledPref, null);

  G_Debug(this, "Maybe toggling update checking. " +
          "Warden enabled? " + phishWardenEnabled);

  
  
  if (phishWardenEnabled === null)
    return;

  
  if (phishWardenEnabled === true) {
    this.enableBlacklistTableUpdates();
    this.enableWhitelistTableUpdates();
  } else {
    
    this.disableBlacklistTableUpdates();
    this.disableWhitelistTableUpdates();
  }
}








PROT_PhishingWarden.prototype.onPhishWardenEnabledPrefChanged = function(
                                                                    prefName) {
  
  if (prefName != "browser.safebrowsing.enabled")
    return;

  this.phishWardenEnabled_ = 
    this.prefs_.getPref(prefName, this.phishWardenEnabled_);
  this.maybeToggleUpdateChecking();
}
