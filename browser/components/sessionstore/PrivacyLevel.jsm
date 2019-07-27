



"use strict";

this.EXPORTED_SYMBOLS = ["PrivacyLevel"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gSessionStartup",
  "@mozilla.org/browser/sessionstartup;1", "nsISessionStartup");

const PREF_NORMAL = "browser.sessionstore.privacy_level";
const PREF_DEFERRED = "browser.sessionstore.privacy_level_deferred";






const PRIVACY_NONE = 0;

const PRIVACY_ENCRYPTED = 1;

const PRIVACY_FULL = 2;








function getCurrentLevel(isPinned) {
  let pref = PREF_NORMAL;

  
  
  if (!isPinned && Services.startup.shuttingDown && !gSessionStartup.isAutomaticRestoreEnabled()) {
    pref = PREF_DEFERRED;
  }

  return Services.prefs.getIntPref(pref);
}




let PrivacyLevel = Object.freeze({
  








  canSave: function ({isHttps, isPinned}) {
    let level = getCurrentLevel(isPinned);

    
    if (level == PRIVACY_FULL) {
      return false;
    }

    
    if (isHttps && level == PRIVACY_ENCRYPTED) {
      return false;
    }

    return true;
  }
});
