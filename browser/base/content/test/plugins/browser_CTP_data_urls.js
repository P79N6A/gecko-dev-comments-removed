let rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
let gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
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

  gBrowser.selectedTab =  gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Test Plug-in");
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Second Test Plug-in");
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_data_url.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1a, Should have a click-to-play notification");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 1a, plugin should not be activated");

  let loadPromise = promiseTabLoadEvent(gBrowser.selectedTab);
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    
    content.document.getElementById("data-link-1").click();
  });
  yield loadPromise;

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1b, Should have a click-to-play notification");

  pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 1b, plugin should not be activated");

  let promise = promisePopupNotification("click-to-play-plugins");
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    let bounds = plugin.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });
  yield promise;

  
  let condition = function() !PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed &&
    PopupNotifications.panel.firstChild;
  yield promiseForCondition(condition);
  PopupNotifications.panel.firstChild._primaryButton.click();

  
  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 1b, plugin should be activated");
});



add_task(function* () {
  
  clearAllPluginPermissions();

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_data_url.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 2a, Should have a click-to-play notification");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 2a, plugin should not be activated");

  let loadPromise = promiseTabLoadEvent(gBrowser.selectedTab);
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    
    content.document.getElementById("data-link-2").click();
  });
  yield loadPromise;

  
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    content.document.getElementById("test1").clientTop;
    content.document.getElementById("test2").clientTop;
  });

  pluginInfo = yield promiseForPluginInfo("test1");
  ok(!pluginInfo.activated, "Test 2a, test1 should not be activated");
  pluginInfo = yield promiseForPluginInfo("test2");
  ok(!pluginInfo.activated, "Test 2a, test2 should not be activated");

  notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 2b, Should have a click-to-play notification");

  yield promiseForNotificationShown(notification);

  
  is(notification.options.pluginData.size, 2, "Test 2b, Should have two types of plugin in the notification");

  let centerAction = null;
  for (let action of notification.options.pluginData.values()) {
    if (action.pluginName == "Test") {
      centerAction = action;
      break;
    }
  }
  ok(centerAction, "Test 2b, found center action for the Test plugin");

  let centerItem = null;
  for (let item of PopupNotifications.panel.firstChild.childNodes) {
    is(item.value, "block", "Test 2b, all plugins should start out blocked");
    if (item.action == centerAction) {
      centerItem = item;
      break;
    }
  }
  ok(centerItem, "Test 2b, found center item for the Test plugin");

  
  centerItem.value = "allownow";
  PopupNotifications.panel.firstChild._primaryButton.click();

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  
  pluginInfo = yield promiseForPluginInfo("test1");
  ok(pluginInfo.activated, "Test 2b, plugin should be activated");
});

add_task(function* () {
  
  clearAllPluginPermissions();

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_data_url.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);
});


add_task(function* () {
  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 3a, Should have a click-to-play notification");

  
  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 3a, plugin should not be activated");

  let promise = promisePopupNotification("click-to-play-plugins");
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    let bounds = plugin.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });
  yield promise;

  
  let condition = function() !PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed &&
    PopupNotifications.panel.firstChild;
  yield promiseForCondition(condition);
  PopupNotifications.panel.firstChild._primaryButton.click();

  
  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 3a, plugin should be activated");

  let loadPromise = promiseTabLoadEvent(gBrowser.selectedTab);
  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    
    content.document.getElementById("data-link-1").click();
  });
  yield loadPromise;

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  
  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 3b, plugin should be activated");

  clearAllPluginPermissions();
});










































