const gTestRoot = "http://mochi.test:8888/browser/browser/base/content/test/general/";

var gTestBrowser = null;
var gNextTest = null;

Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function() {
    clearAllPluginPermissions();
    Services.prefs.clearUserPref("plugins.click_to_play");
  });
  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("load", pageLoad, true);
  prepareTest(test1a, gTestRoot + "plugin_add_dynamically.html");
}

function finishTest() {
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


function test1a() {
  var popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(!popupNotification, "Test 1a, Should not have a click-to-play notification");
  var plugin = new XPCNativeWrapper(XPCNativeWrapper.unwrap(gTestBrowser.contentWindow).addPlugin());

  var condition = function() PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  waitForCondition(condition, test1b, "Test 1a, Waited too long for plugin notification");
}

function test1b() {
  var popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1b, Should have a click-to-play notification");
  var plugin = gTestBrowser.contentDocument.getElementsByTagName("embed")[0];
  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 1b, Plugin should not be activated");

  
  popupNotification.reshow();
  PopupNotifications.panel.firstChild._primaryButton.click();

  ok(objLoadingContent.activated, "Test 1b, Doorhanger should activate plugin");

  test1c();
}

function test1c() {
  var popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1c, Should still have a click-to-play notification");
  var plugin = new XPCNativeWrapper(XPCNativeWrapper.unwrap(gTestBrowser.contentWindow).addPlugin());

  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  var condition = function() objLoadingContent.activated;
  waitForCondition(condition, test1d, "Test 1c, Waited too long for plugin activation");
}

function test1d() {
  var plugin = gTestBrowser.contentDocument.getElementsByTagName("embed")[1];
  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(objLoadingContent.activated, "Test 1d, Plugin should be activated");

  gNextTest = test1e;
  gTestBrowser.contentWindow.addEventListener("hashchange", test1e, false);
  gTestBrowser.contentWindow.location += "#anchorNavigation";
}

function test1e() {
  gTestBrowser.contentWindow.removeEventListener("hashchange", test1e, false);

  var plugin = new XPCNativeWrapper(XPCNativeWrapper.unwrap(gTestBrowser.contentWindow).addPlugin());

  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  var condition = function() objLoadingContent.activated;
  waitForCondition(condition, test1f, "Test 1e, Waited too long for plugin activation");
}

function test1f() {
  var plugin = gTestBrowser.contentDocument.getElementsByTagName("embed")[2];
  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(objLoadingContent.activated, "Test 1f, Plugin should be activated");

  gTestBrowser.contentWindow.history.replaceState({}, "", "replacedState");
  var plugin = new XPCNativeWrapper(XPCNativeWrapper.unwrap(gTestBrowser.contentWindow).addPlugin());
  var objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  var condition = function() objLoadingContent.activated;
  waitForCondition(condition, test1g, "Test 1f, Waited too long for plugin activation");
}

function test1g() {
  var plugin = gTestBrowser.contentDocument.getElementsByTagName("embed")[3];
  var objLoadingContent2 = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(objLoadingContent2.activated, "Test 1g, Plugin should be activated");
  finishTest();
}
