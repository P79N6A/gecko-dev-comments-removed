



const gTestRoot = getRootDirectory(gTestPath);
const gHttpTestRoot = gTestRoot.replace("chrome://mochitests/content/",
                                        "http://127.0.0.1:8888/");

add_task(function* () {
  Services.prefs.setBoolPref("plugins.click_to_play", true);
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("plugins.click_to_play");
  });
})






add_task(function* () {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = gBrowser.selectedBrowser;

  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  
  let notificationPromise =
    waitForNotificationBar("plugin-hidden", gBrowser.selectedBrowser);
  yield loadPage(browser, gHttpTestRoot + "plugin_small.html")
  yield forcePluginBindingAttached(browser);
  yield notificationPromise;

  
  
  let plugin = browser.contentDocument.getElementById("test");
  plugin.remove();
  yield loadPage(browser, "about:mozilla");

  
  let notificationBox = gBrowser.getNotificationBox(browser);
  is(notificationBox.getNotificationWithValue("plugin-hidden"), null,
     "Expected no notification box");
  gBrowser.removeTab(newTab);
});






add_task(function* () {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = gBrowser.selectedBrowser;

  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY,
                            "Second Test Plug-in");

  
  let notificationPromise =
    waitForNotificationBar("plugin-hidden", browser);
  yield loadPage(browser, gHttpTestRoot + "plugin_small.html")
  yield forcePluginBindingAttached(browser);
  yield notificationPromise;

  
  
  let plugin = browser.contentDocument.getElementById("test");
  plugin.remove();
  yield loadPage(browser, gTestRoot + "plugin_small_2.html");
  let notification = yield waitForNotificationBar("plugin-hidden", browser);
  ok(notification, "There should be a notification shown for the new page.");

  
  
  ok(notification.label.includes("Second Test"), "Should mention the second plugin");
  ok(!notification.label.includes("127.0.0.1"), "Should not refer to old principal");
  ok(notification.label.includes("null"), "Should refer to the new principal");
  gBrowser.removeTab(newTab);
});
