


function test() {
  let prefService = Services.prefs;
  let prefs = [
    "network",
    "networkinfo",
    "csserror",
    "cssparser",
    "exception",
    "jswarn",
    "error",
    "warn",
    "info",
    "log"
  ];

  
  prefs.forEach(function(pref) {
    prefService.setBoolPref("devtools.webconsole.filter." + pref, true);
  });

  addTab("about:blank");
  openConsole();
  
  let hud = HUDService.getHudByWindow(content);
  let hudId = HUDService.getHudIdByWindow(content);

  
  prefs.forEach(function(pref) {
    let checked = hud.HUDBox.querySelector("menuitem[prefKey=" + pref + "]").
      getAttribute("checked");
    is(checked, "true", "menuitem for " + pref + " exists and is checked");
  });
  
  
  prefs.forEach(function(pref) {
    HUDService.setFilterState(hudId, pref, false);
  });

  
  closeConsole();
  openConsole();

  hud = HUDService.getHudByWindow(content);
  
  
  prefs.forEach(function(pref) {
    let checked = hud.HUDBox.querySelector("menuitem[prefKey=" + pref + "]").
      getAttribute("checked");
    is(checked, "false", "menuitem for " + pref + " exists and is not checked");
    prefService.clearUserPref("devtools.webconsole.filter." + pref);
  });
  
  gBrowser.removeCurrentTab();
  
  finish();
}
