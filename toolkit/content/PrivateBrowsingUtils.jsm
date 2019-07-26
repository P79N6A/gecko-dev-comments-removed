



var EXPORTED_SYMBOLS = ["PrivateBrowsingUtils"];

const Ci = Components.interfaces;

var PrivateBrowsingUtils = {
  isWindowPrivate: function pbu_isWindowPrivate(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsILoadContext)
                  .usePrivateBrowsing;
  }
};
