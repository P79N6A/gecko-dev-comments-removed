



var EXPORTED_SYMBOLS = ["PrivateBrowsingUtils"];

const Cc = Components.classes;
const Ci = Components.interfaces;

var PrivateBrowsingUtils = {
  isWindowPrivate: function pbu_isWindowPrivate(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsILoadContext)
                  .usePrivateBrowsing;
  },

  get permanentPrivateBrowsing() {
#ifdef MOZ_PER_WINDOW_PRIVATE_BROWSING
    return false; 
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
