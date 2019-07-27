let gTestRoot = getRootDirectory(gTestPath).replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
let gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
let gTestBrowser = null;

function updateAllTestPlugins(aState) {
  setTestPluginEnabledState(aState, "Test Plug-in");
  setTestPluginEnabledState(aState, "Second Test Plug-in");
}

add_task(function* () {
  registerCleanupFunction(Task.async(function*() {
    clearAllPluginPermissions();
    Services.prefs.clearUserPref("plugins.click_to_play");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Test Plug-in");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Second Test Plug-in");
    yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);
    resetBlocklist();
    gTestBrowser = null;
    gBrowser.removeCurrentTab();
    window.focus();
  }));
});

add_task(function* () {
  gBrowser.selectedTab = gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;

  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);
  Services.prefs.setBoolPref("plugins.click_to_play", true);

  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  
  yield promiseTabLoadEvent(gBrowser.selectedTab, "data:text/html,<html></html>");
  let exmsg = yield promiseInitContentBlocklistSvc(gBrowser.selectedBrowser);
  ok(!exmsg, "exception: " + exmsg);

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_unknown.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("unknown");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_UNSUPPORTED,
     "Test 1a, plugin fallback type should be PLUGIN_UNSUPPORTED");
});


add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  yield promiseTabLoadEvent(gBrowser.selectedTab, "data:text/html,<html>hi</html>");

  
  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(!notification, "Test 11b, Should not have a click-to-play notification");

  gTestBrowser.contentWindow.history.back();

  yield promisePopupNotification("click-to-play-plugins");
});


add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 12a, Plugin should not be activated");

  
  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);

  yield promiseForNotificationShown(notification);

  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 12a, Plugin should be activated");
});



add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_two_types.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  yield promisePopupNotification("click-to-play-plugins");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let test = content.document.getElementById("test");
    let secondtestA = content.document.getElementById("secondtestA");
    let secondtestB = content.document.getElementById("secondtestB");
    return test.activated && !secondtestA.activated && !secondtestB.activated;
  });
  ok(result, "Content plugins are set up");

  clearAllPluginPermissions();
});



add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_ENABLED);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test2.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("test1");
  ok(pluginInfo.activated, "Test 14, Plugin should be activated");
});



add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_alternate_content.html");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let mainBox = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return !!mainBox;
  });
  ok(result, "Test 15, Plugin overlay should exist");
});



add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_bug749455.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 17, Should have a click-to-play notification");
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  yield asyncSetAndUpdateBlocklist(gTestRoot + "blockNoPlugins.xml", gTestBrowser);

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 18g, Plugin should not be activated");

  ok(PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed,
     "Test 19a, Doorhanger should start out dismissed");

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let icon = doc.getAnonymousElementByAttribute(plugin, "class", "icon");
    let bounds = icon.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });

  let condition = function() !PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed;
  yield promiseForCondition(condition);
});


add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 18g, Plugin should not be activated");

  ok(PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed,
     "Test 19c, Doorhanger should start out dismissed");

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let text = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgClickToPlay");
    let bounds = text.getBoundingClientRect();
    let left = (bounds.left + bounds.right) / 2;
    let top = (bounds.top + bounds.bottom) / 2;
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });

  let condition = function() !PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed;
  yield promiseForCondition(condition);
});



add_task(function* () {
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 18g, Plugin should not be activated");

  ok(PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed,
     "Test 19e, Doorhanger should start out dismissed");

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", 50, 50, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", 50, 50, 0, 1, 0, false, 0, 0);
  });

  let condition = function() !PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser).dismissed &&
    PopupNotifications.panel.firstChild;
  yield promiseForCondition(condition);
  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 19e, Plugin should not be activated");

  clearAllPluginPermissions();
});



add_task(function* () {
  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_hidden_to_visible.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 20a, Should have a click-to-play notification");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    return !!overlay;
  });
  ok(result, "Test 20a, Plugin overlay should exist");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let mainBox = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    let overlayRect = mainBox.getBoundingClientRect();
    return overlayRect.width == 0 && overlayRect.height == 0;
  });
  ok(result, "Test 20a, plugin should have an overlay with 0px width and height");

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 20b, plugin should not be activated");

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let div = doc.getElementById("container");
    return div.style.display == "none";
  });
  ok(result, "Test 20b, container div should be display: none");

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let div = doc.getElementById("container");
    div.style.display = "block";
  });

  result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let mainBox = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    let overlayRect = mainBox.getBoundingClientRect();
    return overlayRect.width == 200 && overlayRect.height == 200;
  });
  ok(result, "Test 20b, Waited too long for plugin to become visible");

  pluginInfo = yield promiseForPluginInfo("test");
  ok(!pluginInfo.activated, "Test 20b, plugin should not be activated");

  ok(notification.dismissed, "Test 20c, Doorhanger should start out dismissed");

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

  let condition = function() !notification.dismissed && !!PopupNotifications.panel.firstChild;
  yield promiseForCondition(condition);
  PopupNotifications.panel.firstChild._primaryButton.click();

  pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 20c, plugin should be activated");

  result = ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlayRect = doc.getAnonymousElementByAttribute(plugin, "anonid", "main").getBoundingClientRect();
    return overlayRect.width == 0 && overlayRect.height == 0;
  });
  ok(result, "Test 20c, plugin should have overlay dims of 200px");

  clearAllPluginPermissions();
});


add_task(function* () {
  
  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_two_types.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 21a, Should have a click-to-play notification");

  
  let ids = ["test", "secondtestA", "secondtestB"];
  for (let id of ids) {
    let result = yield ContentTask.spawn(gTestBrowser, {aId: id}, function* () {
      let doc = content.document;
      let plugin = doc.getElementById(arguments[0].aId);
      let overlayRect = doc.getAnonymousElementByAttribute(plugin, "anonid", "main").getBoundingClientRect();
      return overlayRect.width == 200 && overlayRect.height == 200;
    });
    ok(result, "Test 21a, plugin " + id + " should have click-to-play overlay with dims");

    let pluginInfo = yield promiseForPluginInfo(id);
    ok(!pluginInfo.activated, "Test 21a, Plugin with id=" + id + " should not be activated");
  }

  notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 21a, Should have a click-to-play notification");

  
  yield promiseForNotificationShown(notification);

  is(notification.options.pluginData.size, 2, "Test 21a, Should have two types of plugin in the notification");

  let centerAction = null;
  for (let action of notification.options.pluginData.values()) {
    if (action.pluginName == "Test") {
      centerAction = action;
      break;
    }
  }
  ok(centerAction, "Test 21b, found center action for the Test plugin");

  let centerItem = null;
  for (let item of PopupNotifications.panel.firstChild.childNodes) {
    is(item.value, "block", "Test 21b, all plugins should start out blocked");
    if (item.action == centerAction) {
      centerItem = item;
      break;
    }
  }
  ok(centerItem, "Test 21b, found center item for the Test plugin");

  
  centerItem.value = "allownow";

  
  PopupNotifications.panel.firstChild._primaryButton.click();

  let pluginInfo = yield promiseForPluginInfo("test");
  ok(pluginInfo.activated, "Test 21b, plugin should be activated");

  notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 21b, Should have a click-to-play notification");

  yield promiseForNotificationShown(notification);

  ok(notification.options.pluginData.size == 2, "Test 21c, Should have one type of plugin in the notification");

  let result = ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    let overlayRect = doc.getAnonymousElementByAttribute(plugin, "anonid", "main").getBoundingClientRect();
    return overlayRect.width == 0 && overlayRect.height == 0;
  });
  ok(result, "Test 21c, plugin should have overlay dims of 0px");

  ids = ["secondtestA", "secondtestB"];
  for (let id of ids) {
    let result = yield ContentTask.spawn(gTestBrowser, {aId: id}, function* () {
      let doc = content.document;
      let plugin = doc.getElementById(arguments[0].aId);
      let overlayRect = doc.getAnonymousElementByAttribute(plugin, "anonid", "main").getBoundingClientRect();
      return overlayRect.width == 200 && overlayRect.height == 200;
    });
    ok(result, "Test 21c, plugin " + id + " should have click-to-play overlay with zero dims");

    let pluginInfo = yield promiseForPluginInfo(id);
    ok(!pluginInfo.activated, "Test 21c, Plugin with id=" + id + " should not be activated");
  }

  centerAction = null;
  for (let action of notification.options.pluginData.values()) {
    if (action.pluginName == "Second Test") {
      centerAction = action;
      break;
    }
  }
  ok(centerAction, "Test 21d, found center action for the Second Test plugin");

  centerItem = null;
  for (let item of PopupNotifications.panel.firstChild.childNodes) {
    if (item.action == centerAction) {
      is(item.value, "block", "Test 21d, test plugin 2 should start blocked");
      centerItem = item;
      break;
    }
    else {
      is(item.value, "allownow", "Test 21d, test plugin should be enabled");
    }
  }
  ok(centerItem, "Test 21d, found center item for the Second Test plugin");

  
  centerItem.value = "allownow";

  
  PopupNotifications.panel.firstChild._primaryButton.click();

  notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 21d, Should have a click-to-play notification");

  ids = ["test", "secondtestA", "secondtestB"];
  for (let id of ids) {
    let result = yield ContentTask.spawn(gTestBrowser, {aId: id}, function* () {
      let doc = content.document;
      let plugin = doc.getElementById(arguments[0].aId);
      let overlayRect = doc.getAnonymousElementByAttribute(plugin, "anonid", "main").getBoundingClientRect();
      return overlayRect.width == 0 && overlayRect.height == 0;
    });
    ok(result, "Test 21d, plugin " + id + " should have click-to-play overlay with zero dims");

    let pluginInfo = yield promiseForPluginInfo(id);
    ok(pluginInfo.activated, "Test 21d, Plugin with id=" + id + " should not be activated");
  }
});


add_task(function* () {
  clearAllPluginPermissions();

  updateAllTestPlugins(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  yield promiseTabLoadEvent(gBrowser.selectedTab, gTestRoot + "plugin_test.html");

  
  yield promiseUpdatePluginBindings(gTestBrowser);

  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 22, Should have a click-to-play notification");

  
  let pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 23, plugin fallback type should be PLUGIN_CLICK_TO_PLAY");

  yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    plugin.type = null;
    
    
    plugin.parentNode.appendChild(plugin);
  });

  pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.displayedType, Ci.nsIObjectLoadingContent.TYPE_NULL, "Test 23, plugin should be TYPE_NULL");

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let doc = content.document;
    let plugin = doc.getElementById("test");
    plugin.type = "application/x-test";
    plugin.parentNode.appendChild(plugin);
  });

  pluginInfo = yield promiseForPluginInfo("test");
  is(pluginInfo.displayedType, Ci.nsIObjectLoadingContent.TYPE_NULL, "Test 23, plugin should be TYPE_NULL");
  is(pluginInfo.pluginFallbackType, Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY,
     "Test 23, plugin fallback type should be PLUGIN_CLICK_TO_PLAY");
  ok(!pluginInfo.activated, "Test 23, plugin node should not be activated");
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
  ok(!pluginInfo.activated, "Plugin should be activated.");

  const testUrl = "http://test.url.com/";

  let firstPanelChild = PopupNotifications.panel.firstChild;
  let infoLink = document.getAnonymousElementByAttribute(firstPanelChild, "anonid",
    "click-to-play-plugins-notification-link");
  is(infoLink.href, testUrl,
    "Test 26, the notification URL needs to match the infoURL from the blocklist file.");
});
