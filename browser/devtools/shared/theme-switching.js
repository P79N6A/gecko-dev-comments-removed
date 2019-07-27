



(function() {
  const DEVTOOLS_SKIN_URL = "chrome://browser/skin/devtools/";
  let documentElement = document.documentElement;

  function forceStyle() {
    let computedStyle = window.getComputedStyle(documentElement);
    if (!computedStyle) {
      
      
      return;
    }
    let display = computedStyle.display; 
    documentElement.style.display = "none";
    window.getComputedStyle(documentElement).display; 
    documentElement.style.display = display; 
  }

  function switchTheme(newTheme, oldTheme) {
    if (newTheme === oldTheme) {
      return;
    }

    let oldThemeDef = gDevTools.getThemeDefinition(oldTheme);

    
    if (oldThemeDef) {
      for (let url of oldThemeDef.stylesheets) {
        StylesheetUtils.removeSheet(window, url, "author");
      }
    }

    
    let newThemeDef = gDevTools.getThemeDefinition(newTheme);

    
    
    if (!newThemeDef) {
      newThemeDef = gDevTools.getThemeDefinition("light");
    }

    for (let url of newThemeDef.stylesheets) {
      StylesheetUtils.loadSheet(window, url, "author");
    }

    
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

    if (oldThemeDef) {
      for (let name of oldThemeDef.classList) {
        documentElement.classList.remove(name);
      }

      if (oldThemeDef.onUnapply) {
        oldThemeDef.onUnapply(window, newTheme);
      }
    }

    for (let name of newThemeDef.classList) {
      documentElement.classList.add(name);
    }

    if (newThemeDef.onApply) {
      newThemeDef.onApply(window, oldTheme);
    }

    
    gDevTools.emit("theme-switched", window, newTheme, oldTheme);
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

  if (documentElement.hasAttribute("force-theme")) {
    switchTheme(documentElement.getAttribute("force-theme"));
  } else {
    switchTheme(Services.prefs.getCharPref("devtools.theme"));

    gDevTools.on("pref-changed", handlePrefChange);
    window.addEventListener("unload", function() {
      gDevTools.off("pref-changed", handlePrefChange);
    });
  }
})();
