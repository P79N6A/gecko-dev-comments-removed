



function test() {
  runTests();
}

gTests.push({
  desc: "Plugin mouse input",
  run: function() {
    
    let origValue = MetroUtils.swapMouseButton(false);
    registerCleanupFunction(function() MetroUtils.swapMouseButton(origValue));

    Services.prefs.setBoolPref("plugin.disable", false);
    Services.prefs.setBoolPref("plugins.click_to_play", false);
    registerCleanupFunction(Services.prefs.clearUserPref.bind(null, "plugin.disable"));
    registerCleanupFunction(Services.prefs.clearUserPref.bind(null, "plugins.click_to_play"));

    let tab = yield addTab(chromeRoot + "browser_plugin_input.html");

    let doc = tab.browser.contentDocument;
    let plugin = doc.getElementById("plugin1");
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    ok(objLoadingContent.activated, "Plugin activated");

    
    
    let wait = yield waitForMs(0);
    ok(wait, "Initial wait");

    try {
      is(plugin.getMouseUpEventCount(), 0, "Plugin should not have received "
                                         + "any mouse up events yet.");
    } catch(e) {
      ok(false, "plugin.getMouseUpEventCount should not throw: " + e);
    }

    let bottom = plugin.getBoundingClientRect().height - 1;
    let right = plugin.getBoundingClientRect().width - 1;
    let middleX = right / 2;
    let middleY = bottom / 2;
    let left = 1;
    let top = 1;

    let clicks = [{ x: left, y: top},          
                  { x: left, y: middleY},      
                  { x: left, y: bottom},       
                  { x: middleX, y: bottom},    
                  { x: right, y: bottom},      
                  { x: right, y: middleY},     
                  { x: right, y: top},         
                  { x: middleX, y: top},       
                  { x: middleX, y: middleY}];  

    let curClicks = 0;
    while (clicks.length > 0) {
      let click = clicks.shift();
      curClicks++;
      info("Sending click " + curClicks + " { x: " + click.x + ", y: " + click.y + "}");
      synthesizeNativeMouseLDown(plugin, click.x, click.y);
      synthesizeNativeMouseLUp(plugin, click.x, click.y);
      let success = yield waitForCondition(function() plugin.getMouseUpEventCount() == curClicks);
      ok(success && !(success instanceof Error),
         "Plugin received click " + curClicks);
    }
  }
});
