



function test() {
  runTests();
}

gTests.push({
  desc: "Plugin keyboard input",
  run: function() {
    Services.prefs.setBoolPref("plugin.disable", false);
    Services.prefs.setBoolPref("plugins.click_to_play", false);
    registerCleanupFunction(Services.prefs.clearUserPref.bind(null, "plugin.disable"));
    registerCleanupFunction(Services.prefs.clearUserPref.bind(null, "plugins.click_to_play"));

    let tab = yield addTab(chromeRoot + "browser_plugin_input.html");

    let doc = tab.browser.contentDocument;
    let plugin = doc.getElementById("plugin1");
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    ok(objLoadingContent.activated, "Plugin activated");
    plugin.focus();

    try {
      is(plugin.getLastKeyText(), "", "Plugin should not have received "
                                    + "any character events yet.");
    } catch(e) {
      ok(false, "plugin.getLastKeyText should not throw: " + e);
    }

    let keys = [{ kbLayout: arSpanish,
                  keyCode: 65,
                  modifiers: 0,
                  expectedChar: 'a' }];

    






    while (keys.length > 0) {
      let key = keys.shift();
      info("Sending keypress: " + key.expectedChar);
      synthesizeNativeKey(key.kbLayout, key.keyCode, key.modifiers);
      let success = yield waitForCondition(function() plugin.getLastKeyText() == key.expectedChar);
      ok(success && !(success instanceof Error),
         "Plugin received char: " + key.expectedChar);
    }
  }
});
