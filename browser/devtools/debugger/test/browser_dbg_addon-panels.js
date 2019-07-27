





const ADDON_URL = EXAMPLE_URL + "addon3.xpi";

let gAddon, gClient, gThreadClient, gDebugger, gSources;
let PREFS = [
  "devtools.canvasdebugger.enabled",
  "devtools.shadereditor.enabled",
  "devtools.performance.enabled",
  "devtools.netmonitor.enabled"
];
function test() {
  Task.spawn(function () {
    let addon = yield addAddon(ADDON_URL);
    let addonDebugger = yield initAddonDebugger(ADDON_URL);

    
    let originalPrefs = PREFS.map(pref => {
      let original = Services.prefs.getBoolPref(pref);
      Services.prefs.setBoolPref(pref, true)
      return original;
    });

    
    let tabs = addonDebugger.frame.contentDocument.getElementById("toolbox-tabs").children;
    let expectedTabs = ["webconsole", "jsdebugger", "scratchpad"];

    is(tabs.length, expectedTabs.length, "displaying only " + expectedTabs.length + " tabs in addon debugger");
    Array.forEach(tabs, (tab, i) => {
      let toolName = expectedTabs[i];
      is(tab.getAttribute("toolid"), toolName, "displaying " + toolName);
    });

    
    let buttons = addonDebugger.frame.contentDocument.getElementById("toolbox-buttons").children;
    Array.forEach(buttons, (btn, i) => {
      is(btn.hidden, true, "no toolbox buttons for the addon debugger -- " + btn.className);
    });

    yield addonDebugger.destroy();
    yield removeAddon(addon);

    PREFS.forEach((pref, i) => Services.prefs.setBoolPref(pref, originalPrefs[i]));

    finish();
  });
}
