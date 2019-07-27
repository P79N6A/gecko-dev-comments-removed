























const PREF_ACTIVE = "security.mixed_content.block_active_content";


const gHttpTestRoot1 = "https://test1.example.com/browser/browser/base/content/test/general/";
const gHttpTestRoot2 = "https://test2.example.com/browser/browser/base/content/test/general/";

var origBlockActive;
var gTestBrowser = null;

registerCleanupFunction(function() {
  
  Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
});

function cleanUpAfterTests() {
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}



function test1A() {
  
  
  gTestBrowser.removeEventListener("load", test1A, true);
  gTestBrowser.addEventListener("load", test1B, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test1A!");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test1A!");

  
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
  notification.remove();
}

function test1B() {
  var expected = "Mixed Content Blocker disabled";
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test1C, "Error: Waited too long for mixed script to run in Test 1B");
}

function test1C() {
  gTestBrowser.removeEventListener("load", test1B, true);
  var actual = content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 1C");

  
  
  gTestBrowser.addEventListener("load", test1D, true);

  var url = gHttpTestRoot1 + "file_bug902156_2.html";
  gTestBrowser.contentWindow.location = url;
}

function test1D() {
  gTestBrowser.removeEventListener("load", test1D, true);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test1D!");
  notification.reshow();
  ok(!PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked in Test1D!");
  notification.remove();

  var actual = content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 1D");

  
  test2();
}



function test2() {
  gTestBrowser.addEventListener("load", test2A, true);
  var url = gHttpTestRoot2 + "file_bug902156_2.html";
  gTestBrowser.contentWindow.location = url;
}

function test2A() {
  
  
  gTestBrowser.removeEventListener("load", test2A, true);
  gTestBrowser.addEventListener("load", test2B, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 2A!");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 2A!");

  
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
  notification.remove();
}

function test2B() {
  var expected = "Mixed Content Blocker disabled";
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test2C, "Error: Waited too long for mixed script to run in Test 2B");
}

function test2C() {
  gTestBrowser.removeEventListener("load", test2B, true);
  var actual = content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 2C");

  
  
  gTestBrowser.addEventListener("load", test2D, true);

  
  var mctestlink = content.document.getElementById("mctestlink");
  mctestlink.click();
}

function test2D() {
  gTestBrowser.removeEventListener("load", test2D, true);

  
  
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test2D!");
  notification.reshow();
  ok(!PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is NOT being blocked");
  notification.remove();

  var actual = content.document.getElementById('mctestdiv').innerHTML;
  is(actual, "Mixed Content Blocker disabled", "OK: Executed mixed script in Test 2D");

  
  test3();
}



function test3() {
  gTestBrowser.addEventListener("load", test3A, true);
  var url = gHttpTestRoot1 + "file_bug902156_3.html";
  gTestBrowser.contentWindow.location = url;
}

function test3A() {
  
  
  gTestBrowser.removeEventListener("load", test3A, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "OK: Mixed Content Doorhanger did appear in Test 3A!");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 3A");
  notification.remove();

  
  cleanUpAfterTests();
}



function test() {
  
  waitForExplicitFinish();

  
  origBlockActive = Services.prefs.getBoolPref(PREF_ACTIVE);

  Services.prefs.setBoolPref(PREF_ACTIVE, true);

  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  newTab.linkedBrowser.stop()

  
  gTestBrowser.addEventListener("load", test1A, true);
  var url = gHttpTestRoot1 + "file_bug902156_1.html";
  gTestBrowser.contentWindow.location = url;
}
