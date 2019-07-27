






















































const PREF_ACTIVE = "security.mixed_content.block_active_content";
const PREF_DISPLAY = "security.mixed_content.block_display_content";
const gHttpsTestRoot = "https://example.com/browser/browser/base/content/test/general/";
const gHttpTestRoot = "http://example.com/browser/browser/base/content/test/general/";

let origBlockActive;
let origBlockDisplay;
var gTestBrowser = null;



registerCleanupFunction(function() {
  
  Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
  Services.prefs.setBoolPref(PREF_DISPLAY, origBlockDisplay);

  
  Services.io.offline = false;
});

function cleanUpAfterTests() {
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}

function waitForCondition(condition, nextTest, errorMsg, okMsg) {
  var tries = 0;
  var interval = setInterval(function() {
    if (tries >= 30) {
      ok(false, errorMsg);
      moveOn();
    }
    if (condition()) {
      ok(true, okMsg)
      moveOn();
    }
    tries++;
  }, 100);
  var moveOn = function() {
    clearInterval(interval); nextTest();
  };
}



function test1() {
  gTestBrowser.addEventListener("load", checkPopUpNotificationsForTest1, true);
  var url = gHttpsTestRoot + "test_mcb_redirect.html"
  gTestBrowser.contentWindow.location = url;
}

function checkPopUpNotificationsForTest1() {
  gTestBrowser.removeEventListener("load", checkPopUpNotificationsForTest1, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser.selectedBrowser);
  ok(notification, "OK: Mixed Content Doorhanger appeared in Test1!");

  var expected = "script blocked";
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test2, "Error: Waited too long for status in Test 1!",
    "OK: Expected result in innerHTML for Test1!");
}



function test2() {
  gTestBrowser.addEventListener("load", checkPopUpNotificationsForTest2, true);
  var url = gHttpTestRoot + "test_mcb_redirect.html"
  gTestBrowser.contentWindow.location = url;
}

function checkPopUpNotificationsForTest2() {
  gTestBrowser.removeEventListener("load", checkPopUpNotificationsForTest2, true);

  var notification = PopupNotifications.getNotification("bad-content", gTestBrowser.selectedBrowser);
  ok(!notification, "OK: Mixed Content Doorhanger did not appear in 2!");

  var expected = "script executed";
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test3, "Error: Waited too long for status in Test 2!",
    "OK: Expected result in innerHTML for Test2!");
}



function test3() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest3, true);
  var url = gHttpsTestRoot + "test_mcb_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest3() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest3, true);

  var expected = "image blocked"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test4, "Error: Waited too long for status in Test 3!",
    "OK: Expected result in innerHTML for Test3!");
}



function test4() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest4, true);
  var url = gHttpTestRoot + "test_mcb_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest4() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest4, true);

  var expected = "image loaded"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test5, "Error: Waited too long for status in Test 4!",
    "OK: Expected result in innerHTML for Test4!");
}






function test5() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest5, true);
  
  Services.io.offline = true;
  var url = gHttpTestRoot + "test_mcb_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest5() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest5, true);

  var expected = "image loaded"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test6, "Error: Waited too long for status in Test 5!",
    "OK: Expected result in innerHTML for Test5!");
  
  Services.io.offline = false;
}






function test6() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest6, true);
  
  Services.io.offline = true;
  var url = gHttpsTestRoot + "test_mcb_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest6() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest6, true);

  var expected = "image blocked"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test7, "Error: Waited too long for status in Test 6!",
    "OK: Expected result in innerHTML for Test6!");
  
  Services.io.offline = false;
}



function test7() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest7, true);
  var url = gHttpTestRoot + "test_mcb_double_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest7() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest7, true);

  var expected = "image loaded"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test8, "Error: Waited too long for status in Test 7!",
    "OK: Expected result in innerHTML for Test7!");
}






function test8() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest8, true);
  
  Services.io.offline = true;
  var url = gHttpTestRoot + "test_mcb_double_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest8() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest8, true);

  var expected = "image loaded"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    test9, "Error: Waited too long for status in Test 8!",
    "OK: Expected result in innerHTML for Test8!");
  
  Services.io.offline = false;
}






function test9() {
  gTestBrowser.addEventListener("load", checkLoadEventForTest9, true);
  
  Services.io.offline = true;
  var url = gHttpsTestRoot + "test_mcb_double_redirect_image.html"
  gTestBrowser.contentWindow.location = url;
}

function checkLoadEventForTest9() {
  gTestBrowser.removeEventListener("load", checkLoadEventForTest9, true);

  var expected = "image blocked"
  waitForCondition(
    function() content.document.getElementById('mctestdiv').innerHTML == expected,
    cleanUpAfterTests, "Error: Waited too long for status in Test 9!",
    "OK: Expected result in innerHTML for Test9!");
  
  Services.io.offline = false;
}



function test() {
  
  waitForExplicitFinish();

  
  origBlockActive = Services.prefs.getBoolPref(PREF_ACTIVE);
  origBlockDisplay = Services.prefs.getBoolPref(PREF_DISPLAY);
  Services.prefs.setBoolPref(PREF_ACTIVE, true);
  Services.prefs.setBoolPref(PREF_DISPLAY, true);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  newTab.linkedBrowser.stop();

  executeSoon(test1);
}
