



this.EXPORTED_SYMBOLS = ["PrivateBrowsingUtils"];

Components.utils.import("resource://gre/modules/Services.jsm");

const kAutoStartPref = "browser.components.autostart";

const Cc = Components.classes;
const Ci = Components.interfaces;

this.PrivateBrowsingUtils = {
  isWindowPrivate: function pbu_isWindowPrivate(aWindow) {
    return this.privacyContextFromWindow(aWindow).usePrivateBrowsing;
  },

  privacyContextFromWindow: function pbu_privacyContextFromWindow(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsILoadContext);
  },

  get permanentPrivateBrowsing() {
#ifdef MOZ_PER_WINDOW_PRIVATE_BROWSING
    return Services.prefs.getBoolPref(kAutoStart, false);
#else
    try {
      return Cc["@mozilla.org/privatebrowsing;1"].
             getService(Ci.nsIPrivateBrowsingService).
             autoStarted;
    } catch (e) {
      return false; 
    }
#endif
  }
};

#ifdef MOZ_PER_WINDOW_PRIVATE_BROWSING
function autoStartObserver(aSubject, aTopic, aData) {
  var newValue = Services.prefs.getBoolPref(kAutoStart);
  var windowsEnum = Services.wm.getEnumerator(null);
  while (windowsEnum.hasMoreElements()) {
    var window = windowsEnum.getNext();
    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIWebNavigation)
          .QueryInterface(Ci.nsILoadContext)
          .usePrivateBrowsing = newValue;
  }
}

Services.prefs.addObserver(kAutoStartPref, autoStartObserver, false);
#endif

