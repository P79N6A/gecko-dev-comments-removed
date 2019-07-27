let rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
let gTestBrowser = null;

add_task(function* () {
  registerCleanupFunction(function () {
    clearAllPluginPermissions();
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Test Plug-in");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Second Test Plug-in");
    Services.prefs.clearUserPref("plugins.click_to_play");
    Services.prefs.clearUserPref("extensions.blocklist.suppressUI");
    gBrowser.removeCurrentTab();
    window.focus();
    gTestBrowser = null;
  });

  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
});

add_task(function* () {
  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Test Plug-in");

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_small.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  
  yield promiseForNotificationBar("plugin-hidden", gTestBrowser);
});

add_task(function* () {
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Test Plug-in");

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_small.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notificationBox = gBrowser.getNotificationBox(gTestBrowser);
  yield promiseForCondition(() => notificationBox.getNotificationWithValue("plugin-hidden") === null);
});

add_task(function* () {
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Test Plug-in");

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_overlayed.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  
  yield promiseForNotificationBar("plugin-hidden", gTestBrowser);
});

add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_overlayed.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    return plugin.pluginFallbackType;
  });
  is(result, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 3b, plugin fallback type should be PLUGIN_CLICK_TO_PLAY");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 1a, plugin should not be activated");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return overlay && overlay.classList.contains("visible");
  });
  ok(!result, "Test 3b, overlay should be hidden.");
});

add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_positioned.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  
  yield promisePopupNotification("click-to-play-plugins");
  yield promiseForNotificationBar("plugin-hidden", gTestBrowser);

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    return plugin.pluginFallbackType;
  });
  is(result, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 4b, plugin fallback type should be PLUGIN_CLICK_TO_PLAY");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return overlay && overlay.classList.contains("visible");
  });
  ok(!result, "Test 4b, overlay should be hidden.");
});




add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_small.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  
  yield promisePopupNotification("click-to-play-plugins");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    return plugin.pluginFallbackType;
  });
  is(result, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 6, Plugin should be click-to-play");

  yield promisePopupNotification("click-to-play-plugins");

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 6, Should have a click-to-play notification");

  
  yield promiseForNotificationShown(notification);

  PopupNotifications.panel.firstChild._primaryButton.click();

  let notificationBox = gBrowser.getNotificationBox(gTestBrowser);
  yield promiseForCondition(() => notificationBox.getNotificationWithValue("plugin-hidden") === null);

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 7, plugin should be activated");
});
