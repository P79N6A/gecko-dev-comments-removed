let gTestRoot = getRootDirectory(gTestPath).replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
let gTestBrowser = null;
let gNumPluginBindingsAttached = 0;

function pluginBindingAttached() {
  gNumPluginBindingsAttached++;
  if (gNumPluginBindingsAttached != 1) {
    ok(false, "if we've gotten here, something is quite wrong");
  }
}

add_task(function* () {
  registerCleanupFunction(function () {
    gTestBrowser.removeEventListener("PluginBindingAttached", pluginBindingAttached, true, true);
    clearAllPluginPermissions();
    Services.prefs.clearUserPref("plugins.click_to_play");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Test Plug-in");
    setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED, "Second Test Plug-in");
    gBrowser.removeCurrentTab();
    window.focus();
    gTestBrowser = null;
  });
});

add_task(function* () {
  gBrowser.selectedTab = gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;

  Services.prefs.setBoolPref("plugins.click_to_play", true);

  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Test Plug-in");

  gTestBrowser.addEventListener("PluginBindingAttached", pluginBindingAttached, true, true);

  let testRoot = getRootDirectory(gTestPath).replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");
  yield promiseTabLoadEvent(gBrowser.selectedTab, testRoot + "plugin_bug744745.html");

  yield promiseForCondition(function () { return gNumPluginBindingsAttached == 1; });

  let result = yield ContentTask.spawn(gTestBrowser, {}, function* () {
    let plugin = content.document.getElementById("test");
    if (!plugin) {
      return false;
    }
    
    let style = content.getComputedStyle(plugin);
    return 'opacity' in style && style.opacity == 1;
  });

  ok(result, true, "plugin style properly configured.");
});
