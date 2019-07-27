const gTestRoot = getRootDirectory(gTestPath);
const gHttpTestRoot = gTestRoot.replace("chrome://mochitests/content/",
                                        "http://127.0.0.1:8888/");

add_task(function* () {
  registerCleanupFunction(function () {
    clearAllPluginPermissions();
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Test Plug-in");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Second Test Plug-in");
    Services.prefs.clearUserPref("plugins.click_to_play");
    Services.prefs.clearUserPref("extensions.blocklist.suppressUI");
    gBrowser.removeCurrentTab();
    window.focus();
  });

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Test Plug-in");
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Second Test Plug-in");
});






add_task(function* () {
  gBrowser.selectedTab = gBrowser.addTab();

  
  let notificationPromise = waitForNotificationBar("plugin-hidden", gBrowser.selectedBrowser);
  yield promiseTabLoadEvent(gBrowser.selectedTab, gHttpTestRoot + "plugin_small.html");
  yield promiseUpdatePluginBindings(gBrowser.selectedBrowser);
  yield notificationPromise;

  
  
  yield ContentTask.spawn(gBrowser.selectedBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    plugin.remove();
  });

  yield promiseTabLoadEvent(gBrowser.selectedTab, "about:mozilla");

  
  let notificationBox = gBrowser.getNotificationBox(gBrowser.selectedBrowser);
  is(notificationBox.getNotificationWithValue("plugin-hidden"), null,
     "Expected no notification box");
});






add_task(function* () {
  
  let notificationPromise = waitForNotificationBar("plugin-hidden", gBrowser.selectedBrowser);
  yield promiseTabLoadEvent(gBrowser.selectedTab, gHttpTestRoot + "plugin_small.html");
  yield promiseUpdatePluginBindings(gBrowser.selectedBrowser);
  yield notificationPromise;

  
  
  yield ContentTask.spawn(gBrowser.selectedBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    plugin.remove();
  });
});

add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gHttpTestRoot + "plugin_small_2.html");
  let notification = yield waitForNotificationBar("plugin-hidden", gBrowser.selectedBrowser);
  ok(notification, "There should be a notification shown for the new page.");
  
  
  let label = notification.label;
  ok(label.contains("Second Test"), "Should mention the second plugin");
});
