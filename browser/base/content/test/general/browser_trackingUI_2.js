







const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const PREF = "privacy.trackingprotection.enabled";
const PB_PREF = "privacy.trackingprotection.pbmode.enabled";
const BENIGN_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html";
const TRACKING_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html";
let TrackingProtection = null;
let browser = null;

let {UrlClassifierTestUtils} = Cu.import("resource://testing-common/UrlClassifierTestUtils.jsm", {});

registerCleanupFunction(function() {
  TrackingProtection = browser = null;
  UrlClassifierTestUtils.cleanupTestTrackers();
  Services.prefs.clearUserPref(PREF);
  Services.prefs.clearUserPref(PB_PREF);
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});

add_task(function* testNormalBrowsing() {
  yield UrlClassifierTestUtils.addTestTrackers();

  browser = gBrowser;
  let tab = browser.selectedTab = browser.addTab();

  TrackingProtection = browser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "TP is attached to the browser window");
  is (TrackingProtection.enabled, Services.prefs.getBoolPref(PREF),
    "TP.enabled is based on the original pref value");

  Services.prefs.setBoolPref(PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled after setting the pref");

  Services.prefs.setBoolPref(PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled after setting the pref");

  info("Load a test page containing tracking elements");
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);
  ok (TrackingProtection.container.hidden, "The container is hidden");

  info("Load a test page not containing tracking elements");
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  ok (TrackingProtection.container.hidden, "The container is hidden");
});

add_task(function* testPrivateBrowsing() {
  let privateWin = yield promiseOpenAndLoadWindow({private: true}, true);
  browser = privateWin.gBrowser;
  let tab = browser.selectedTab = browser.addTab();

  TrackingProtection = browser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "TP is attached to the private window");
  is (TrackingProtection.enabled, Services.prefs.getBoolPref(PB_PREF),
    "TP.enabled is based on the pb pref value");

  Services.prefs.setBoolPref(PB_PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled after setting the pref");

  Services.prefs.setBoolPref(PB_PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled after setting the pref");

  info("Load a test page containing tracking elements");
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);
  ok (TrackingProtection.container.hidden, "The container is hidden");

  info("Load a test page not containing tracking elements");
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  ok (TrackingProtection.container.hidden, "The container is hidden");

  privateWin.close();
});
