










let PREF = "privacy.trackingprotection.enabled";
let BENIGN_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html";
let TRACKING_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html";
let TrackingProtection = null;

registerCleanupFunction(function() {
  TrackingProtection = null;
  Services.prefs.clearUserPref(PREF);
  gBrowser.removeCurrentTab();
});

function hidden(sel) {
  let win = gBrowser.ownerGlobal;
  let el = win.document.querySelector(sel);
  let display = win.getComputedStyle(el).getPropertyValue("display", null);
  return display === "none";
}

function clickButton(sel) {
  let win = gBrowser.ownerGlobal;
  let el = win.document.querySelector(sel);
  el.doCommand();
}

function testBenignPage() {
  info("Non-tracking content must not be blocked");
  ok (!TrackingProtection.content.hasAttribute("block-disabled"), "blocking not disabled");
  ok (!TrackingProtection.content.hasAttribute("block-active"), "blocking is not active");

  ok (hidden("#tracking-action-block"), "blockButton is hidden");
  ok (hidden("#tracking-action-unblock"), "unblockButton is hidden");

  
  ok (!hidden("#tracking-not-detected"), "labelNoTracking is visible");
  ok (hidden("#tracking-loaded"), "labelTrackingLoaded is hidden");
  ok (hidden("#tracking-blocked"), "labelTrackingBlocked is hidden");
}

function testTrackingPage() {
  info("Tracking content must be blocked");
  ok (!TrackingProtection.content.hasAttribute("block-disabled"), "blocking not disabled");
  ok (TrackingProtection.content.hasAttribute("block-active"), "blocking is active");

  ok (hidden("#tracking-action-block"), "blockButton is hidden");
  ok (!hidden("#tracking-action-unblock"), "unblockButton is visible");

  
  ok (hidden("#tracking-not-detected"), "labelNoTracking is hidden");
  ok (hidden("#tracking-loaded"), "labelTrackingLoaded is hidden");
  ok (!hidden("#tracking-blocked"), "labelTrackingBlocked is visible");
}

function testTrackingPageWhitelisted() {
  info("Tracking content must be white-listed and not blocked");
  ok (TrackingProtection.content.hasAttribute("block-disabled"), "blocking is disabled");
  ok (!TrackingProtection.content.hasAttribute("block-active"), "blocking is not active");

  ok (!hidden("#tracking-action-block"), "blockButton is visible");
  ok (hidden("#tracking-action-unblock"), "unblockButton is hidden");

  
  ok (hidden("#tracking-not-detected"), "labelNoTracking is hidden");
  ok (!hidden("#tracking-loaded"), "labelTrackingLoaded is visible");
  ok (hidden("#tracking-blocked"), "labelTrackingBlocked is hidden");
}

add_task(function* () {
  yield updateTrackingProtectionDatabase();

  let tab = gBrowser.selectedTab = gBrowser.addTab();

  TrackingProtection = gBrowser.ownerGlobal.TrackingProtection;
  ok (TrackingProtection, "Functionality is attached to the browser window");
  is (TrackingProtection.enabled, Services.prefs.getBoolPref(PREF),
    "TP.enabled is based on the original pref value");

  Services.prefs.setBoolPref(PREF, true);
  ok (TrackingProtection.enabled, "TP is enabled after setting the pref");

  info("Load a test page not containing tracking elements");
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  testBenignPage();

  info("Load a test page containing tracking elements");
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);
  testTrackingPage();

  info("Disable TP for the page (which reloads the page)");
  clickButton("#tracking-action-unblock");

  info("Wait for tab to reload following TP white-listing");
  yield promiseTabLoadEvent(tab);
  testTrackingPageWhitelisted();

  info("Re-enable TP for the page (which reloads the page)");
  clickButton("#tracking-action-block");

  info("Wait for tab to reload following TP black-listing");
  yield promiseTabLoadEvent(tab);
  testTrackingPage();
});
