




const PREF_DISPLAY = "security.mixed_content.block_display_content";
const PREF_ACTIVE = "security.mixed_content.block_active_content";


const gHttpTestRoot = "https://example.com/browser/browser/base/content/test/general/";
const gHttpTestRoot2 = "https://test1.example.com/browser/browser/base/content/test/general/";

var origBlockDisplay;
var origBlockActive;
var gTestBrowser = null;

registerCleanupFunction(function() {
  
  Services.prefs.setBoolPref(PREF_DISPLAY, origBlockDisplay);
  Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
});

function MixedTestsCompleted() {
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}

function test() {
  waitForExplicitFinish();

  origBlockDisplay = Services.prefs.getBoolPref(PREF_DISPLAY);
  origBlockActive = Services.prefs.getBoolPref(PREF_ACTIVE);

  Services.prefs.setBoolPref(PREF_DISPLAY, true);
  Services.prefs.setBoolPref(PREF_ACTIVE, true);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  newTab.linkedBrowser.stop()

  
  gTestBrowser.addEventListener("load", MixedTest1A, true);
  var url = gHttpTestRoot + "file_bug822367_1.html";
  gTestBrowser.contentWindow.location = url;
}


function MixedTest1A() {
  gTestBrowser.removeEventListener("load", MixedTest1A, true);
  gTestBrowser.addEventListener("load", MixedTest1B, true);
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked");
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}
function MixedTest1B() {
  waitForCondition(function() content.document.getElementById('p1').innerHTML == "hello", MixedTest1C, "Waited too long for mixed script to run in Test 1");
}
function MixedTest1C() {
  ok(content.document.getElementById('p1').innerHTML == "hello","Mixed script didn't load in Test 1");
  gTestBrowser.removeEventListener("load", MixedTest1B, true);
  MixedTest2();
}


function MixedTest2() {
  gTestBrowser.addEventListener("load", MixedTest2A, true);
  var url = gHttpTestRoot2 + "file_bug822367_2.html";
  gTestBrowser.contentWindow.location = url;
}

function MixedTest2A() {
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(!notification, "Mixed Content Doorhanger did not appear for mixed display content!");
  MixedTest3();
}


function MixedTest3() {
  gTestBrowser.removeEventListener("load", MixedTest2A, true);
  gTestBrowser.addEventListener("load", MixedTest3A, true);
  var url = gHttpTestRoot + "file_bug822367_3.html";
  gTestBrowser.contentWindow.location = url;
}
function MixedTest3A() {
  gTestBrowser.removeEventListener("load", MixedTest3A, true);
  gTestBrowser.addEventListener("load", MixedTest3B, true);
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear for test 3");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in test 3");
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}
function MixedTest3B() {
  waitForCondition(function() content.document.getElementById('p1').innerHTML == "hello", MixedTest3C, "Waited too long for mixed script to run in Test 3");
}
function MixedTest3C() {
  waitForCondition(function() content.document.getElementById('p2').innerHTML == "bye", MixedTest3D, "Waited too long for mixed image to load in Test 3");
}
function MixedTest3D() {
  ok(content.document.getElementById('p1').innerHTML == "hello","Mixed script didn't load in Test 3");
  ok(content.document.getElementById('p2').innerHTML == "bye","Mixed image didn't load in Test 3");
  MixedTest4();
}


function MixedTest4() {
  gTestBrowser.removeEventListener("load", MixedTest3B, true);
  gTestBrowser.addEventListener("load", MixedTest4A, true);
  var url = gHttpTestRoot2 + "file_bug822367_4.html";
  gTestBrowser.contentWindow.location = url;
}
function MixedTest4A() {
  gTestBrowser.removeEventListener("load", MixedTest4A, true);
  gTestBrowser.addEventListener("load", MixedTest4B, true);
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear for Test 4");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 4");
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}
function MixedTest4B() {
  waitForCondition(function() content.document.location == gHttpTestRoot + "file_bug822367_4B.html", MixedTest4C, "Waited too long for mixed script to run in Test 4");
}
function MixedTest4C() {
  ok(content.document.location == gHttpTestRoot + "file_bug822367_4B.html", "Location didn't change in test 4");
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear after location change in Test 4");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in test 4");
  notification.remove();
  waitForCondition(function() content.document.getElementById('p1').innerHTML == "", MixedTest4D, "Mixed script loaded in test 4 after location change!");
}
function MixedTest4D() {
  ok(content.document.getElementById('p1').innerHTML == "","p1.innerHTML changed; mixed script loaded after location change in Test 4");
  MixedTest5();
}


function MixedTest5() {
  gTestBrowser.removeEventListener("load", MixedTest4B, true);
  gTestBrowser.addEventListener("load", MixedTest5A, true);
  var url = gHttpTestRoot + "file_bug822367_5.html";
  gTestBrowser.contentWindow.location = url;
}
function MixedTest5A() {
  gTestBrowser.removeEventListener("load", MixedTest5A, true);
  gTestBrowser.addEventListener("load", MixedTest5B, true);
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear for Test 5");
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 5");
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}
function MixedTest5B() {
  waitForCondition(function() content.document.getElementById('p1').innerHTML == "hello", MixedTest5C, "Waited too long for mixed script to run in Test 5");
}
function MixedTest5C() {
  ok(content.document.getElementById('p1').innerHTML == "hello","Mixed script didn't load in Test 5");
  MixedTest6();
}


function MixedTest6() {
  gTestBrowser.removeEventListener("load", MixedTest5B, true);
  gTestBrowser.addEventListener("load", MixedTest6A, true);
  var url = gHttpTestRoot2 + "file_bug822367_6.html";
  gTestBrowser.contentWindow.location = url;
}
function MixedTest6A() {
  gTestBrowser.removeEventListener("load", MixedTest6A, true);
  waitForCondition(function() PopupNotifications.getNotification("bad-content", gTestBrowser), MixedTest6B, "waited too long for doorhanger");
}

function MixedTest6B() {
  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger did appear for Test 6");
  gTestBrowser.addEventListener("load", MixedTest6C, true);
  notification.reshow();
  ok(PopupNotifications.panel.firstChild.isMixedContentBlocked, "OK: Mixed Content is being blocked in Test 6");
  PopupNotifications.panel.firstChild.disableMixedContentProtection();
}

function MixedTest6C() {
  gTestBrowser.removeEventListener("load", MixedTest6C, true);
  waitForCondition(function() {
    try {
      return content.document.getElementById('f1').contentDocument.getElementById('p1').innerHTML == "hello";
    } catch (e) {
      return false;
    }
  }, MixedTest6D, "Waited too long for mixed script to run in Test 6");
}
function MixedTest6D() {
  ok(content.document.getElementById('f1').contentDocument.getElementById('p1').innerHTML == "hello","Mixed script didn't load in Test 6");
  MixedTestsCompleted();
}
