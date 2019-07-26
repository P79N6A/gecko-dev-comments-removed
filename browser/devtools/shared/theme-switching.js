



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
    if (newTheme === oldTheme) {
      return;
    }

    if (oldTheme && newTheme != oldTheme) {
      StylesheetUtils.removeSheet(
        window,
        DEVTOOLS_SKIN_URL + oldTheme + "-theme.css",
        "author"
      );
    }

    StylesheetUtils.loadSheet(
      window,
      DEVTOOLS_SKIN_URL + newTheme + "-theme.css",
      "author"
    );

    
    let hiddenDOMWindow = Cc["@mozilla.org/appshell/appShellService;1"]
                 .getService(Ci.nsIAppShellService)
                 .hiddenDOMWindow;
    if (!hiddenDOMWindow.matchMedia("(-moz-overlay-scrollbars)").matches) {
      let scrollbarsUrl = Services.io.newURI(
        DEVTOOLS_SKIN_URL + "floating-scrollbars-light.css", null, null);

      if (newTheme == "dark") {
        StylesheetUtils.loadSheet(
          window,
          scrollbarsUrl,
          "agent"
        );
      } else if (oldTheme == "dark") {
        StylesheetUtils.removeSheet(
          window,
          scrollbarsUrl,
          "agent"
        );
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
  const {devtools} = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {});
  const StylesheetUtils = devtools.require("sdk/stylesheet/utils");

  let theme = Services.prefs.getCharPref("devtools.theme");
  switchTheme(theme);

  gDevTools.on("pref-changed", handlePrefChange);
  window.addEventListener("unload", function() {
    gDevTools.off("pref-changed", handlePrefChange);
  });
})();
