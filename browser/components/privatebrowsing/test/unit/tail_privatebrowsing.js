




Cc["@mozilla.org/preferences-service;1"].
  getService(Ci.nsIPrefBranch).
  clearUserPref("browser.privatebrowsing.keep_current_session");
