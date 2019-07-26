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
    getTestPlugin().enabledState = Ci.nsIPluginTag.STATE_ENABLED;
  });
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("load", pageLoad, true);

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  getTestPlugin().enabledState = Ci.nsIPluginTag.STATE_CLICKTOPLAY;

  prepareTest(runAfterPluginBindingAttached(test1), gHttpTestRoot + "plugin_test.html");
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
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 1, Plugin should not be activated");

  window.document.addEventListener("popupshown", test2, false);
  EventUtils.synthesizeMouseAtCenter(plugin,
                                     { type: "contextmenu", button: 2 },
                                     gTestBrowser.contentWindow);
}

function test2() {
  window.document.removeEventListener("popupshown", test2, false);
  let activate = window.document.getElementById("context-ctp-play");
  ok(activate, "Test 2, Should have a context menu entry for activating the plugin");

  
  activate.doCommand();

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 2, Should have a click-to-play notification");
  ok(!notification.dismissed, "Test 2, The click-to-play notification should not be dismissed");

  
  PopupNotifications.panel.firstChild._primaryButton.click();

  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  waitForCondition(() => objLoadingContent.activated, test3, "Waited too long for plugin to activate");
}

function test3() {
  finishTest();
}
