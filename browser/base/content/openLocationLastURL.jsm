



































const LAST_URL_PREF = "general.open_location.last_url";
const nsISupportsString = Components.interfaces.nsISupportsString;

var EXPORTED_SYMBOLS = [ "gOpenLocationLastURL" ];

let pbSvc = Components.classes["@mozilla.org/privatebrowsing;1"]
                      .getService(Components.interfaces.nsIPrivateBrowsingService);
let prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);

let observer = {
  QueryInterface: function (aIID) {
    if (aIID.equals(Components.interfaces.nsIObserver) ||
        aIID.equals(Components.interfaces.nsISupports) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
  observe: function (aSubject, aTopic, aData) {
    gOpenLocationLastURLData = "";
  }
};

Components.classes["@mozilla.org/observer-service;1"]
          .getService(Components.interfaces.nsIObserverService)
          .addObserver(observer, "private-browsing", true);

let gOpenLocationLastURLData = "";
let gOpenLocationLastURL = {
  get value() {
    if (pbSvc.privateBrowsingEnabled)
      return gOpenLocationLastURLData;
    else {
      try {
        return prefSvc.getComplexValue(LAST_URL_PREF, nsISupportsString).data;
      }
      catch (e) {
        return "";
      }
    }
  },
  set value(val) {
    if (typeof val != "string")
      val = "";
    if (pbSvc.privateBrowsingEnabled)
      gOpenLocationLastURLData = val;
    else {
      let str = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      str.data = val;
      prefSvc.setComplexValue(LAST_URL_PREF, nsISupportsString, str);
    }
  },
  reset: function() {
    if (prefSvc.prefHasUserValue(LAST_URL_PREF))
        prefSvc.clearUserPref(LAST_URL_PREF);
    gOpenLocationLastURLData = "";
  }
};
