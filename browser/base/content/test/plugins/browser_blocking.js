let gTestRoot = getRootDirectory(gTestPath).replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
let gTestBrowser = null;
let gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);

function updateAllTestPlugins(aState) {
  setTestPluginEnabledState(aState, "Test Plug-in");
  setTestPluginEnabledState(aState, "Second Test Plug-in");
}

add_task(function* () {
  registerCleanupFunction(Task.async(function*() {
    clearAllPluginPermissions();
    updateAllTestPlugins(Ci.nsIPluginTag.STATE_ENABLED);
    Services.prefs.clearUserPref("plugins.click_to_play");
    Services.prefs.clearUserPref("extensions.blocklist.suppressUI");
    yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);
    resetBlocklist();
    gBrowser.removeCurrentTab();
    window.focus();
    gTestBrowser = null;
  }));
});

add_task(function* () {
  gBrowser.selectedTab = gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;

  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);
  Services.prefs.setBoolPref("plugins.click_to_play", true);

  
  yield promiseTabLoadEvent(gBrowser.selectedTab, "data:text/html,<html>hi</html>");

  
  Components.classes["@mozilla.org/extensions/blocklist;1"]
            .getService(Components.interfaces.nsIBlocklistService);
  let exmsg = yield promiseInitContentBlocklistSvc(gBrowser.selectedBrowser);
  ok(!exmsg, "exception: " + exmsg);
});

add_task(function* () {
  
  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockPluginHard.xml", gTestBrowser);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  let notification = PopupNotifications.getNotification("click-to-play-plugins");
  ok(notification.dismissed, "Test 5: The plugin notification should be dismissed by default");

  yield promiseForNotificationShown(notification);

  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_BLOCKLISTED, "Test 5, plugin fallback type should be PLUGIN_BLOCKLISTED");

  is(notification.options.pluginData.size, 1, "Test 5: Only the blocked plugin should be present in the notification");
  ok(PopupNotifications.panel.firstChild._buttonContainer.hidden, "Part 5: The blocked plugins notification should not have any buttons visible.");

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);
});



add_task(function* () {
  
  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockPluginVulnerableUpdatable.xml", gTestBrowser);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE,
     "Test 18a, plugin fallback type should be PLUGIN_VULNERABLE_UPDATABLE");
  ok(!pluginInfo.activated, "Test 18a, Plugin should not be activated");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    let doc = content.document;
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return overlay && overlay.classList.contains("visible");
  });
  ok(result, "Test 18a, Plugin overlay should exist, not be hidden");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let updateLink = doc.getAnonymousElementByAttribute(plugin, "anonid", "checkForUpdatesLink");
    return updateLink.style.visibility != "hidden";
  });
  ok(result, "Test 18a, Plugin should have an update link");

  let promise = waitForEvent(gBrowser.tabContainer, "TabOpen", null, true);
  let pluginUpdateURL = Services.urlFormatter.formatURLPref("plugins.update.url");
  info(pluginUpdateURL);

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let updateLink = doc.getAnonymousElementByAttribute(plugin, "anonid", "checkForUpdatesLink");
    let bounds = updateLink.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });
  yield promise;

  promise = waitForEvent(gBrowser.tabContainer, "TabClose", null, true);
  gBrowser.removeCurrentTab();
  yield promise;
});

add_task(function* () {
  
  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE,
     "Test 18a, plugin fallback type should be PLUGIN_VULNERABLE_UPDATABLE");
  ok(!pluginInfo.activated, "Test 18b, Plugin should not be activated");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return overlay && overlay.classList.contains("visible");
  });
  ok(result, "Test 18b, Plugin overlay should exist, not be hidden");
});


add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockPluginVulnerableNoUpdate.xml", gTestBrowser);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 18c, Should have a click-to-play notification");

  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE,
     "Test 18c, plugin fallback type should be PLUGIN_VULNERABLE_NO_UPDATE");
  ok(!pluginInfo.activated, "Test 18c, Plugin should not be activated");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return overlay && overlay.classList.contains("visible");
  });
  ok(result, "Test 18c, Plugin overlay should exist, not be hidden");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let updateLink = doc.getAnonymousElementByAttribute(plugin, "anonid", "checkForUpdatesLink");
    return updateLink && updateLink.style.display != "block";
  });
  ok(result, "Test 18c, Plugin should not have an update link");

  
  yield promiseForNotificationShown(notification);

  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE,
     "Test 18c, plugin fallback type should be PLUGIN_VULNERABLE_NO_UPDATE");
  ok(pluginInfo.activated, "Test 18c, Plugin should be activated");
  let enabledState = getTestPluginEnabledState();
  ok(enabledState, "Test 18c, Plugin enabled state should be STATE_CLICKTOPLAY");
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 18d, Waited too long for plugin to activate");

  clearAllPluginPermissions();
});



add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 18f, Should have a click-to-play notification");
  ok(notification.dismissed, "Test 18f, notification should start dismissed");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 18f, Waited too long for plugin to activate");

  var oldEventCallback = notification.options.eventCallback;
  let promise = promiseForCondition(() => oldEventCallback == null);
  notification.options.eventCallback = function() {
    if (oldEventCallback) {
      oldEventCallback();
    }
    oldEventCallback = null;
  };

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let bounds = plugin.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });
  yield promise;

  ok(notification, "Test 18g, Should have a click-to-play notification");
  ok(!notification.dismissed, "Test 18g, notification should be open");

  pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 18g, Plugin should not be activated");
});



add_task(function* () {
  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);

  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 24a, Should have a click-to-play notification");

  
  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 24a, plugin fallback type should be PLUGIN_CLICK_TO_PLAY");
  ok(!pluginInfo.activated, "Test 24a, Plugin should not be active.");

  
  yield promiseForNotificationShown(notification);

  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 24a, Plugin should be active.");

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockPluginVulnerableUpdatable.xml", gTestBrowser);
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 24b, Should have a click-to-play notification");

  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE,
     "Test 24b, plugin fallback type should be PLUGIN_VULNERABLE_UPDATABLE");
  ok(!pluginInfo.activated, "Test 24b, Plugin should not be active.");

  
  yield promiseForNotificationShown(notification);

  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 24b, Plugin should be active.");

  clearAllPluginPermissions();

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);
});



add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_syncRemoved.html");

  
  
  yield waitForMs(500);

  let notification = PopupNotifications.getNotification("click-to-play-plugins");
  ok(notification, "Test 25: There should be a plugin notification even if the plugin was immediately removed");
  ok(notification.dismissed, "Test 25: The notification should be dismissed by default");

  yield promiseTabLoadEvent(gBrowser.selectedTab, "data:text/html,<html>hi</html>");
});



add_task(function* () {
  clearAllPluginPermissions();

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockPluginInfoURL.xml", gTestBrowser);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins");

  
  yield promiseForNotificationShown(notification);

  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_BLOCKLISTED,
     "Test 26, plugin fallback type should be PLUGIN_BLOCKLISTED");

  let result = ContentTask.spawn(gTestBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    return objLoadingContent.activated;
  });
  ok(result, "Plugin should be activated.");

  const testUrl = "http://test.url.com/";

  let firstPanelChild = PopupNotifications.panel.firstChild;
  let infoLink = document.getAnonymousElementByAttribute(firstPanelChild, "anonid",
    "click-to-play-plugins-notification-link");
  is(infoLink.href, testUrl,
    "Test 26, the notification URL needs to match the infoURL from the blocklist file.");
});

