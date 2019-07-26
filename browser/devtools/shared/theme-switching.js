



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

  function switchTheme(theme, old_theme) {
    let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);
    if (old_theme && theme != old_theme) {
      let old_theme_url = Services.io.newURI(DEVTOOLS_SKIN_URL + old_theme +
                                             "-theme.css", null, null);
      try {
        winUtils.removeSheet(old_theme_url, window.AUTHOR_SHEET);
      } catch(ex) {}
    }
    let theme_url = Services.io.newURI(DEVTOOLS_SKIN_URL + theme + "-theme.css",
                                       null, null);
    winUtils.loadSheet(theme_url, window.AUTHOR_SHEET);
    let scrollbar_url =
      Services.io.newURI(DEVTOOLS_SKIN_URL + "floating-scrollbars-light.css",
                         null, null);
    if (theme == "dark") {
      winUtils.loadSheet(scrollbar_url, window.AGENT_SHEET);
      forceStyle();
    }
    else if (old_theme == "dark") {
      try {
        winUtils.removeSheet(scrollbar_url, window.AGENT_SHEET);
      } catch(ex) {}
      forceStyle();
    }
    document.documentElement.classList.remove("theme-" + old_theme);
    document.documentElement.classList.add("theme-" + theme);
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
})()
