






var PREF = "privacy.trackingprotection.enabled";
var TABLE = "urlclassifier.trackingTable";


function doUpdate() {
  
  var testData = "tracking.example.com/";
  var testUpdate =
    "n:1000\ni:test-track-simple\nad:1\n" +
    "a:524:32:" + testData.length + "\n" +
    testData;

  var dbService = Cc["@mozilla.org/url-classifier/dbservice;1"]
                  .getService(Ci.nsIUrlClassifierDBService);

  let deferred = Promise.defer();

  var listener = {
    QueryInterface: function(iid)
    {
      if (iid.equals(Ci.nsISupports) ||
          iid.equals(Ci.nsIUrlClassifierUpdateObserver))
        return this;

      throw Cr.NS_ERROR_NO_INTERFACE;
    },
    updateUrlRequested: function(url) { },
    streamFinished: function(status) { },
    updateError: function(errorCode) {
      ok(false, "Couldn't update classifier.");
      deferred.resolve();
    },
    updateSuccess: function(requestedTimeout) {
      deferred.resolve();
    }
  };

  dbService.beginUpdate(listener, "test-track-simple", "");
  dbService.beginStream("", "");
  dbService.updateStream(testUpdate);
  dbService.finishStream();
  dbService.finishUpdate();

  return deferred.promise;
}

function testBenignPage(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  is(notification, null, "Tracking Content Doorhanger did NOT appear when protection was ON and tracking was NOT present");
}

function testTrackingPage(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Tracking Content Doorhanger did appear when protection was ON and tracking was present");
  notification.reshow();
  
  isnot(PopupNotifications.panel.firstChild.isTrackingContentBlocked, 0,
    "Tracking Content is being blocked");

  
  ok(!PopupNotifications.panel.firstChild.hasAttribute("trackingblockdisabled"),
    "Doorhanger must have no trackingblockdisabled attribute");

  
  PopupNotifications.panel.firstChild.disableTrackingContentProtection();
}

function testTrackingPageWhitelisted(gTestBrowser)
{
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Tracking Content Doorhanger did appear when protection was ON and tracking was present but white-listed");
  notification.reshow();
  
  is(PopupNotifications.panel.firstChild.isTrackingContentBlocked, 0,
    "Tracking Content is NOT being blocked");

  
  is(PopupNotifications.panel.firstChild.getAttribute("trackingblockdisabled"), "true",
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
    Services.prefs.clearUserPref(TABLE);
    gBrowser.removeCurrentTab();
  });

  
  Services.prefs.setCharPref(TABLE, "test-track-simple");
  yield doUpdate();

  let tab = gBrowser.selectedTab = gBrowser.addTab();

  
  Services.prefs.setBoolPref(PREF, true);

  
  yield promiseTabLoadEvent(tab, "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html");
  testBenignPage(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab, "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html");
  testTrackingPage(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab);
  
  testTrackingPageWhitelisted(gBrowser.getBrowserForTab(tab));

  
  Services.prefs.setBoolPref(PREF, false);

  
  yield promiseTabLoadEvent(tab, "http://tracking.example.org/browser/browser/base/content/test/general/trackingPage.html");
  testTrackingPageOFF(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab, "http://tracking.example.org/browser/browser/base/content/test/general/benignPage.html");
  testBenignPageOFF(gBrowser.getBrowserForTab(tab));
});
