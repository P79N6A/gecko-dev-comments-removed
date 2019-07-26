



(function() {
  const DEVTOOLS_SKIN_URL = "chrome://browser/skin/devtools/";

  function forceStyle() {
    let computedStyle = window.getComputedStyle(document.documentElement);
    if (!computedStyle) {
      
      
      return;
    }
    let display = computedStyle.display; 
    document.documentElement.style.display = "none";
    window.getComputedStyle(document.documentElement).display; 
    document.documentElement.style.display = display; 
  }

  function switchTheme(newTheme, oldTheme) {
    let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);

    if (oldTheme && newTheme != oldTheme) {
      let oldThemeUrl = Services.io.newURI(
        DEVTOOLS_SKIN_URL + oldTheme + "-theme.css", null, null);
      try {
        winUtils.removeSheet(oldThemeUrl, winUtils.AUTHOR_SHEET);
      } catch(ex) {}
    }

    let newThemeUrl = Services.io.newURI(
      DEVTOOLS_SKIN_URL + newTheme + "-theme.css", null, null);
    winUtils.loadSheet(newThemeUrl, winUtils.AUTHOR_SHEET);

    
    if (!window.matchMedia("(-moz-overlay-scrollbars)").matches) {
      let scrollbarsUrl = Services.io.newURI(
        DEVTOOLS_SKIN_URL + "floating-scrollbars-light.css", null, null);

      if (newTheme == "dark") {
        winUtils.loadSheet(scrollbarsUrl, winUtils.AGENT_SHEET);
      } else if (oldTheme == "dark") {
        try {
          winUtils.removeSheet(scrollbarsUrl, winUtils.AGENT_SHEET);
        } catch(ex) {}
      }
      forceStyle();
    }

    document.documentElement.classList.remove("theme-" + oldTheme);
    document.documentElement.classList.add("theme-" + newTheme);
  }

  function handlePrefChange(event, data) {
    if (data.pref == "devtools.theme") {
      switchTheme(data.newValue, data.oldValue);
    }
  }

  const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

  Cu.import("resource://gre/modules/Services.jsm");
  Cu.import("resource:///modules/devtools/gDevTools.jsm");

  let theme = Services.prefs.getCharPref("devtools.theme");
  switchTheme(theme);

  gDevTools.on("pref-changed", handlePrefChange);
  window.addEventListener("unload", function() {
    gDevTools.off("pref-changed", handlePrefChange);
  });
})();
