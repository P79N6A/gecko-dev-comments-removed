






var PREF = "privacy.trackingprotection.enabled";
var BENIGN_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html";
var TRACKING_PAGE = "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html";

function testBenignPage(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  is(notification, null, "Tracking Content Doorhanger did NOT appear when protection was ON and tracking was NOT present");
}

function* testTrackingPage(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Tracking Content Doorhanger did appear when protection was ON and tracking was present");
  notification.reshow();
  var notificationElement = PopupNotifications.panel.firstChild;

  
  yield promiseWaitForCondition(() => {
    return notificationElement.disableTrackingContentProtection;
  });

  
  ok(notificationElement.isTrackingContentBlocked,
     "Tracking Content is being blocked");

  
  ok(!notificationElement.hasAttribute("trackingblockdisabled"),
    "Doorhanger must have no trackingblockdisabled attribute");
}

function* testTrackingPageWhitelisted(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Tracking Content Doorhanger did appear when protection was ON and tracking was present but white-listed");
  notification.reshow();
  var notificationElement = PopupNotifications.panel.firstChild;

  
  yield promiseWaitForCondition(() => {
    return notificationElement.disableTrackingContentProtection;
  });

  var notificationElement = PopupNotifications.panel.firstChild;

  
  ok(!notificationElement.isTrackingContentBlocked,
    "Tracking Content is NOT being blocked");

  
  is(notificationElement.getAttribute("trackingblockdisabled"), "true",
    "Doorhanger must have [trackingblockdisabled='true'] attribute");
}

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

  
  Services.prefs.setBoolPref(PREF, true);

  
  yield promiseTabLoadEvent(tab, BENIGN_PAGE);
  testBenignPage(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab, TRACKING_PAGE);

  
  yield testTrackingPage(gBrowser.getBrowserForTab(tab));

  
  PopupNotifications.panel.firstChild.disableTrackingContentProtection();

  
  yield promiseTabLoadEvent(tab);

  
  yield testTrackingPageWhitelisted(gBrowser.getBrowserForTab(tab));

  
  PopupNotifications.panel.firstChild.enableTrackingContentProtection();

  
  yield promiseTabLoadEvent(tab);

  
  yield testTrackingPage(gBrowser.getBrowserForTab(tab));
});
