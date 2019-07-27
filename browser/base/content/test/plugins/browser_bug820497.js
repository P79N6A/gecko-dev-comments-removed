



var gTestBrowser = null;
var gNumPluginBindingsAttached = 0;

Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("plugins.click_to_play");
    gTestBrowser.removeEventListener("PluginBindingAttached", pluginBindingAttached, true, true);
    gBrowser.removeCurrentTab();
    window.focus();
  });

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Second Test Plug-in");

  gBrowser.selectedTab = gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("PluginBindingAttached", pluginBindingAttached, true, true);
  var gHttpTestRoot = getRootDirectory(gTestPath).replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
  gTestBrowser.contentWindow.location = gHttpTestRoot + "plugin_bug820497.html";
}

function pluginBindingAttached() {
  gNumPluginBindingsAttached++;

  if (gNumPluginBindingsAttached == 1) {
    var doc = gTestBrowser.contentDocument;
    var testplugin = doc.getElementById("test");
    ok(testplugin, "should have test plugin");
    var secondtestplugin = doc.getElementById("secondtest");
    ok(!secondtestplugin, "should not yet have second test plugin");
    var notification;
    waitForNotificationPopup("click-to-play-plugins", gTestBrowser, (notification => {
      ok(notification, "should have popup notification");
      
      notification.reshow();
      is(notification.options.pluginData.size, 1, "should be 1 type of plugin in the popup notification");
      XPCNativeWrapper.unwrap(gTestBrowser.contentWindow).addSecondPlugin();
    }));
  } else if (gNumPluginBindingsAttached == 2) {
    var doc = gTestBrowser.contentDocument;
    var testplugin = doc.getElementById("test");
    ok(testplugin, "should have test plugin");
    var secondtestplugin = doc.getElementById("secondtest");
    ok(secondtestplugin, "should have second test plugin");
    var notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
    ok(notification, "should have popup notification");
    notification.reshow();
    let condition = () => (notification.options.pluginData.size == 2);
    waitForCondition(condition, finish, "Waited too long for 2 types of plugins in popup notification");
  } else {
    ok(false, "if we've gotten here, something is quite wrong");
  }
}
