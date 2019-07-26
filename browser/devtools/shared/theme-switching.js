



(function() {
  const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
  Cu.import("resource://gre/modules/Services.jsm");
  let theme = Services.prefs.getCharPref("devtools.theme");
  let theme_url = Services.io.newURI("chrome://browser/skin/devtools/" + theme + "-theme.css", null, null);
  let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  winUtils.loadSheet(theme_url, window.AUTHOR_SHEET);
  if (theme == "dark") {
    let scrollbar_url = Services.io.newURI("chrome://browser/skin/devtools/floating-scrollbars-light.css", null, null);
    winUtils.loadSheet(scrollbar_url, window.AGENT_SHEET);
  }
  document.documentElement.classList.add("theme-" + theme);
})()
