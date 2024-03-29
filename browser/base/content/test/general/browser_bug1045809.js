

const PREF_ACTIVE = "security.mixed_content.block_active_content";

var origBlockActive;

add_task(function* () {
  registerCleanupFunction(function() {
    Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
    gBrowser.removeCurrentTab();
  });

  
  origBlockActive = Services.prefs.getBoolPref(PREF_ACTIVE);

  
  Services.prefs.setBoolPref(PREF_ACTIVE, true);

  var url =
    "https://test1.example.com/browser/browser/base/content/test/general/" +
    "file_bug1045809_1.html";
  let tab = gBrowser.selectedTab = gBrowser.addTab();

  
  yield promiseTabLoadEvent(tab, url);
  yield* test1(gBrowser.getBrowserForTab(tab));

  yield promiseTabLoadEvent(tab);
  
  yield* test2(gBrowser.getBrowserForTab(tab));

  
  yield promiseTabLoadEvent(tab);
  yield* test3(gBrowser.getBrowserForTab(tab));
});

function* test1(gTestBrowser) {
  var notification =
    PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Mixed Content Doorhanger did appear in Test1");
  yield promiseNotificationShown(notification);
  isnot(PopupNotifications.panel.firstChild.isMixedContentBlocked, 0,
    "Mixed Content is being blocked in Test1");

  var x = content.document.getElementsByTagName('iframe')[0].contentDocument.getElementById('mixedContentContainer');
  is(x, null, "Mixed Content is NOT to be found in Test1");

  
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}

function* test2(gTestBrowser) {
  var notification =
    PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Mixed Content Doorhanger did appear in Test2");
  yield promiseNotificationShown(notification);
  is(PopupNotifications.panel.firstChild.isMixedContentBlocked, 0,
    "Mixed Content is NOT being blocked in Test2");

  var x = content.document.getElementsByTagName('iframe')[0].contentDocument.getElementById('mixedContentContainer');
  isnot(x, null, "Mixed Content is to be found in Test2");

  
  PopupNotifications.panel.firstChild.enableMixedContentProtection();
}

function* test3(gTestBrowser) {
  var notification =
    PopupNotifications.getNotification("bad-content", gTestBrowser);
  isnot(notification, null, "Mixed Content Doorhanger did appear in Test3");
  yield promiseNotificationShown(notification);
  isnot(PopupNotifications.panel.firstChild.isMixedContentBlocked, 0,
    "Mixed Content is being blocked in Test3");

  var x = content.document.getElementsByTagName('iframe')[0].contentDocument.getElementById('mixedContentContainer');
  is(x, null, "Mixed Content is NOT to be found in Test3");
}
