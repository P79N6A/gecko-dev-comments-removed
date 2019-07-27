var rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir;
const gHttpTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");

var gTestBrowser = null;
var gNextTest = null;
var gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);

Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function() {
    clearAllPluginPermissions();
    Services.prefs.clearUserPref("extensions.blocklist.suppressUI");
  });
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("load", pageLoad, true);

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  prepareTest(runAfterPluginBindingAttached(test1), gHttpTestRoot + "plugin_small.html");
}

function finishTest() {
  clearAllPluginPermissions();
  gTestBrowser.removeEventListener("load", pageLoad, true);
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}

function pageLoad() {
  
  
  executeSoon(gNextTest);
}

function prepareTest(nextTest, url) {
  gNextTest = nextTest;
  gTestBrowser.contentWindow.location = url;
}





function runAfterPluginBindingAttached(func) {
  return function() {
    let doc = gTestBrowser.contentDocument;
    let elems = doc.getElementsByTagName('embed');
    if (elems.length < 1) {
      elems = doc.getElementsByTagName('object');
    }
    elems[0].clientTop;
    executeSoon(func);
  };
}




function test1() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1, Should have a click-to-play notification");

  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let doc = gTestBrowser.contentDocument;
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 1, Should have an overlay.");
  ok(!overlay.classList.contains("visible"), "Test 1, Overlay should be hidden");

  plugin.style.width = '300px';
  executeSoon(test2);
}

function test2() {
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let doc = gTestBrowser.contentDocument;
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 2, Should have an overlay.");
  ok(!overlay.classList.contains("visible"), "Test 2, Overlay should be hidden");

  plugin.style.height = '300px';
  let condition = () => overlay.classList.contains("visible");
  waitForCondition(condition, test3, "Test 2, Waited too long for overlay to become visible");
}

function test3() {
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let doc = gTestBrowser.contentDocument;
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 3, Should have an overlay.");
  ok(overlay.classList.contains("visible"), "Test 3, Overlay should be visible");

  plugin.style.width = '10px';
  plugin.style.height = '10px';
  let condition = () => !overlay.classList.contains("visible");
  waitForCondition(condition, test4, "Test 3, Waited too long for overlay to become hidden");
}

function test4() {
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let doc = gTestBrowser.contentDocument;
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 4, Should have an overlay.");
  ok(!overlay.classList.contains("visible"), "Test 4, Overlay should be hidden");

  clearAllPluginPermissions();
  finishTest();
}
