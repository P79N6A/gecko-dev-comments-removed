# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****





























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
