










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

function hidden(sel) {
  let win = browser.ownerGlobal;
  let el = win.document.querySelector(sel);
  let display = win.getComputedStyle(el).getPropertyValue("display", null);
  return display === "none";
}

function clickButton(sel) {
  let win = browser.ownerGlobal;
  let el = win.document.querySelector(sel);
  el.doCommand();
}

function testBenignPage() {
  info("Non-tracking content must not be blocked");
  ok (!TrackingProtection.container.hidden, "The container is visible");
  ok (!TrackingProtection.content.hasAttribute("state"), "content: no state");
  ok (!TrackingProtection.icon.hasAttribute("state"), "icon: no state");

  ok (hidden("#tracking-protection-icon"), "icon is hidden");
  ok (hidden("#tracking-action-block"), "blockButton is hidden");
  ok (hidden("#tracking-action-unblock"), "unblockButton is hidden");

  
  ok (!hidden("#tracking-not-detected"), "labelNoTracking is visible");
  ok (hidden("#tracking-loaded"), "labelTrackingLoaded is hidden");
  ok (hidden("#tracking-blocked"), "labelTrackingBlocked is hidden");
}

function testTrackingPage(window) {
  info("Tracking content must be blocked");
  ok (!TrackingProtection.container.hidden, "The container is visible");
  is (TrackingProtection.content.getAttribute("state"), "blocked-tracking-content",
      'content: state="blocked-tracking-content"');
  is (TrackingProtection.icon.getAttribute("state"), "blocked-tracking-content",
      'icon: state="blocked-tracking-content"');

  ok (!hidden("#tracking-protection-icon"), "icon is visible");
  ok (hidden("#tracking-action-block"), "blockButton is hidden");


  if (PrivateBrowsingUtils.isWindowPrivate(window)) {
    ok(hidden("#tracking-action-unblock"), "unblockButton is hidden");
    ok(!hidden("#tracking-action-unblock-private"), "unblockButtonPrivate is visible");
  } else {
    ok(!hidden("#tracking-action-unblock"), "unblockButton is visible");
    ok(hidden("#tracking-action-unblock-private"), "unblockButtonPrivate is hidden");
  }

  
  ok (hidden("#tracking-not-detected"), "labelNoTracking is hidden");
  ok (hidden("#tracking-loaded"), "labelTrackingLoaded is hidden");
  ok (!hidden("#tracking-blocked"), "labelTrackingBlocked is visible");
}

function testTrackingPageUnblocked() {
  info("Tracking content must be white-listed and not blocked");
  ok (!TrackingProtection.container.hidden, "The container is visible");
  is (TrackingProtection.content.getAttribute("state"), "loaded-tracking-content",
      'content: state="loaded-tracking-content"');
  is (TrackingProtection.icon.getAttribute("state"), "loaded-tracking-content",
      'icon: state="loaded-tracking-content"');

  ok (!hidden("#tracking-protection-icon"), "icon is visible");
  ok (!hidden("#tracking-action-block"), "blockButton is visible");
  ok (hidden("#tracking-action-unblock"), "unblockButton is hidden");

  
  ok (hidden("#tracking-not-detected"), "labelNoTracking is hidden");
  ok (!hidden("#tracking-loaded"), "labelTrackingLoaded is visible");
  ok (hidden("#tracking-blocked"), "labelTrackingBlocked is hidden");
}

function* testTrackingProtectionForTab(tab) {
  info("Load a test page not containing tracking elements");
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  testBenignPage();

  info("Load a test page containing tracking elements");
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);
  testTrackingPage(tab.ownerDocument.defaultView);

  info("Disable TP for the page (which reloads the page)");
  let tabReloadPromise = promiseTabLoadEvent(tab);
  clickButton("#tracking-action-unblock");
  yield tabReloadPromise;
  testTrackingPageUnblocked();

  info("Re-enable TP for the page (which reloads the page)");
  tabReloadPromise = promiseTabLoadEvent(tab);
  clickButton("#tracking-action-block");
  yield tabReloadPromise;
  testTrackingPage(tab.ownerDocument.defaultView);
}

add_task(function* testNormalBrowsing() {
  yield UrlClassifierTestUtils.addTestTrackers();

  browser = gBrowser;
  let tab = browser.selectedTab = browser.addTab();

  TrackingProtection = gBrowser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "TP is attached to the browser window");
  is (TrackingProtection.enabled, Services.prefs.getBoolPref(PREF),
    "TP.enabled is based on the original pref value");

  Services.prefs.setBoolPref(PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled after setting the pref");

  yield testTrackingProtectionForTab(tab);

  Services.prefs.setBoolPref(PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled after setting the pref");
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

  yield testTrackingProtectionForTab(tab);

  Services.prefs.setBoolPref(PB_PREF, false);
  ok (!TrackingProtection.enabled, "TP is disabled after setting the pref");

  privateWin.close();
});
