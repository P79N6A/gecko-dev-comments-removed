



this.EXPORTED_SYMBOLS = ["PrivateBrowsingUtils"];

Components.utils.import("resource://gre/modules/Services.jsm");

const kAutoStartPref = "browser.privatebrowsing.autostart";



let gTemporaryAutoStartMode = false;

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
  }
};

