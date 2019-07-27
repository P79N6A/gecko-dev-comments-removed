



this.EXPORTED_SYMBOLS = ["PrivateBrowsingUtils"];

Components.utils.import("resource://gre/modules/Services.jsm");

const kAutoStartPref = "browser.privatebrowsing.autostart";



let gTemporaryAutoStartMode = false;

const Cc = Components.classes;
const Ci = Components.interfaces;

this.PrivateBrowsingUtils = {
  
  
  isWindowPrivate: function pbu_isWindowPrivate(aWindow) {
    if (!(aWindow instanceof Components.interfaces.nsIDOMChromeWindow)) {
      dump("WARNING: content window passed to PrivateBrowsingUtils.isWindowPrivate. " +
           "Use isContentWindowPrivate instead (but only for frame scripts).\n"
           + new Error().stack);
    }

    return this.privacyContextFromWindow(aWindow).usePrivateBrowsing;
  },

  
  isContentWindowPrivate: function pbu_isWindowPrivate(aWindow) {
    return this.privacyContextFromWindow(aWindow).usePrivateBrowsing;
  },

  isBrowserPrivate: function(aBrowser) {
    return this.isWindowPrivate(aBrowser.ownerDocument.defaultView);
  },

  privacyContextFromWindow: function pbu_privacyContextFromWindow(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsILoadContext);
  },

  get permanentPrivateBrowsing() {
    try {
      return gTemporaryAutoStartMode ||
             Services.prefs.getBoolPref(kAutoStartPref);
    } catch (e) {
      
      return false;
    }
  },

  
  enterTemporaryAutoStartMode: function pbu_enterTemporaryAutoStartMode() {
    gTemporaryAutoStartMode = true;
  },
  get isInTemporaryAutoStartMode() {
    return gTemporaryAutoStartMode;
  },

  whenHiddenPrivateWindowReady: function pbu_whenHiddenPrivateWindowReady(cb) {
    Components.utils.import("resource://gre/modules/Timer.jsm");

    let win = Services.appShell.hiddenPrivateDOMWindow;
    function isNotLoaded() {
      return ["complete", "interactive"].indexOf(win.document.readyState) == -1;
    }
    if (isNotLoaded()) {
      setTimeout(function poll() {
        if (isNotLoaded()) {
          setTimeout(poll, 100);
          return;
        }
        cb(Services.appShell.hiddenPrivateDOMWindow);
      }, 4);
    } else {
      cb(Services.appShell.hiddenPrivateDOMWindow);
    }
  }
};

