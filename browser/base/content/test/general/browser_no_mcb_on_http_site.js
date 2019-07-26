
























const PREF_ACTIVE = "security.mixed_content.block_active_content";
const PREF_DISPLAY = "security.mixed_content.block_display_content";

const gHttpTestRoot = "http://example.com/browser/browser/base/content/test/general/";

var origBlockActive, origBlockDisplay;
var gTestBrowser = null;

registerCleanupFunction(function() {
  
  Services.prefs.setBoolPref(PREF_ACTIVE, origBlockActive);
  Services.prefs.setBoolPref(PREF_DISPLAY, origBlockDisplay);
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



function test1A() {
  gTestBrowser.removeEventListener("load", test1A, true);

  var expected = "Verifying MCB does not trigger warning/error for an http page ";
  expected += "with https css that includes http image";
  waitForCondition(
    function() content.document.getElementById('testDiv').innerHTML == expected,
    test1B, "Error: Waited too long for status in Test 1!",
    "OK: Expected result in innerHTML!");
}

function test1B() {
  
  gTestBrowser.addEventListener("load", test2A, true);
  var url = gHttpTestRoot + "test_no_mcb_on_http_site_font.html";
  gTestBrowser.contentWindow.location = url;
}



function test2A() {
  gTestBrowser.removeEventListener("load", test2A, true);

  var expected = "Verifying MCB does not trigger warning/error for an http page ";
  expected += "with https css that includes http font";
  waitForCondition(
    function() content.document.getElementById('testDiv').innerHTML == expected,
    test2B, "Error: Waited too long for status in Test 2!",
    "OK: Expected result in innerHTML!");
}

function test2B() {
  
  gTestBrowser.addEventListener("load", test3, true);
  var url = gHttpTestRoot + "test_no_mcb_on_http_site_font2.html";
  gTestBrowser.contentWindow.location = url;
}



function test3() {
  gTestBrowser.removeEventListener("load", test3, true);

  var expected = "Verifying MCB does not trigger warning/error for an http page "
  expected += "with https css that imports another http css which includes http font";
  waitForCondition(
    function() content.document.getElementById('testDiv').innerHTML == expected,
    cleanUpAfterTests, "Error: Waited too long for status in Test 3!",
    "OK: Expected result in innerHTML!");
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

  gTestBrowser.addEventListener("load", test1A, true);
  var url = gHttpTestRoot + "test_no_mcb_on_http_site_img.html";
  gTestBrowser.contentWindow.location = url;
}
