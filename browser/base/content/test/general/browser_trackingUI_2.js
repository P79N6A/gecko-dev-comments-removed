






var PREF = "privacy.trackingprotection.enabled";
var BENIGN_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html";
var TRACKING_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html";

function testTrackingPageOFF(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  is(notification, null, "Tracking Content Doorhanger did NOT appear when protection was OFF and tracking was present");
}

function testBenignPageOFF(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  is(notification, null, "Tracking Content Doorhanger did NOT appear when protection was OFF and tracking was NOT present");
}

add_task(function* () {
  registerCleanupFunction(function() {
    Services.prefs.clearUserPref(PREF);
    gBrowser.removeCurrentTab();
  });

  yield updateTrackingProtectionDatabase();

  let tab = gBrowser.selectedTab = gBrowser.addTab();

  
  Services.prefs.setBoolPref(PREF, false);

  
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);
  testTrackingPageOFF(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  testBenignPageOFF(gBrowser.getBrowserForTab(tab));
});
