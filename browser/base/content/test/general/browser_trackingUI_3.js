









const PREF = "privacy.trackingprotection.enabled";
const PB_PREF = "privacy.trackingprotection.pbmode.enabled";
const BENIGN_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html";
const TRACKING_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html";

registerCleanupFunction(function() {
  Services.prefs.clearUserPref(PREF);
  Services.prefs.clearUserPref(PB_PREF);
});

add_task(function* testNormalBrowsing() {
  let browser = gBrowser;
  let TrackingProtection = browser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "TP is attached to the browser window");

  Services.prefs.setBoolPref(PREF, true);
  Services.prefs.setBoolPref(PB_PREF, false);
  ok (TrackingProtection.enabled, "TP is enabled (ENABLED=true,PB=false)");
  Services.prefs.setBoolPref(PB_PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled (ENABLED=true,PB=true)");

  Services.prefs.setBoolPref(PREF, false);
  Services.prefs.setBoolPref(PB_PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled (ENABLED=false,PB=false)");
  Services.prefs.setBoolPref(PB_PREF, true);
  ok (!TrackingProtection.enabled, "TP is disabled (ENABLED=false,PB=true)");
});

add_task(function* testPrivateBrowsing() {
  let privateWin = yield promiseOpenAndLoadWindow({private: true}, true);
  let browser = privateWin.gBrowser;

  let TrackingProtection = browser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "TP is attached to the browser window");

  Services.prefs.setBoolPref(PREF, true);
  Services.prefs.setBoolPref(PB_PREF, false);
  ok (TrackingProtection.enabled, "TP is enabled (ENABLED=true,PB=false)");
  Services.prefs.setBoolPref(PB_PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled (ENABLED=true,PB=true)");

  Services.prefs.setBoolPref(PREF, false);
  Services.prefs.setBoolPref(PB_PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled (ENABLED=false,PB=false)");
  Services.prefs.setBoolPref(PB_PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled (ENABLED=false,PB=true)");

  privateWin.close();
});
