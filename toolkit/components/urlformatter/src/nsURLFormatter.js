



















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CID =         Components.ID("{e6156350-2be8-11db-a98b-0800200c9a66}");
const CONTRACT_ID = "@mozilla.org/toolkit/URLFormatterService;1";
const CLASS_NAME =  "Application URL Formatter Service";






function nsURLFormatterService() {
}

nsURLFormatterService.prototype = {

  


  _defaults: {

    get appInfo() {
      if (!this._appInfo)
        this._appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
      return this._appInfo;
    }, 

    LOCALE: function() {
      var chromereg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                      getService(Ci.nsIXULChromeRegistry);
      return chromereg.getSelectedLocale('global');
    },
    VENDOR:           function() { return this.appInfo.vendor; },
    NAME:             function() { return this.appInfo.name; },
    ID:               function() { return this.appInfo.ID; },
    VERSION:          function() { return this.appInfo.version; },
    APPBUILDID:       function() { return this.appInfo.appBuildID; },
    PLATFORMVERSION:  function() { return this.appInfo.platformVersion; },
    PLATFORMBUILDID:  function() { return this.appInfo.platformBuildID; },
    APP:              function() { return this.appInfo.name.toLowerCase().replace(/ /, ""); }
  },

  


  formatURL: function uf_formatURL(aFormat) {
    var _this = this;
    var replacementCallback = function(aMatch, aKey) {
      if (aKey in _this._defaults) 
        return _this._defaults[aKey]();
      Components.utils.reportError("formatURL: Couldn't find value for key: " + aKey);
      return '';
    }
    return aFormat.replace(/%([A-Z]+)%/gi, replacementCallback);
  },

  


  formatURLPref: function uf_formatURLPref(aPref) {
    var format = null;
    var PS = Cc['@mozilla.org/preferences-service;1'].
             getService(Ci.nsIPrefBranch);

    try {
      format = PS.getComplexValue(aPref, Ci.nsIPrefLocalizedString).data;
    } catch(ex) {}

    if (!format) {
      try {
        format = PS.getComplexValue(aPref, Ci.nsISupportsString).data;
      } catch(ex) {
        Components.utils.reportError("formatURLPref: Couldn't get pref: " + aPref);
        return "about:blank";
      }
    }

    return this.formatURL(format);
  },

  QueryInterface: function uf_QueryInterface(aIID) {
    if (!aIID.equals(Ci.nsIURLFormatter) &&    
        !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




var nsURLFormatterFactory = {
  createInstance: function (aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new nsURLFormatterService()).QueryInterface(aIID);
  }
};

var nsURLFormatterModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CID, CLASS_NAME, 
        CONTRACT_ID, aFileSpec, aLocation, aType);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr = aCompMgr.QueryInterface(Cinterfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CID, aLocation);        
  },
  
  getClassObject: function(aCompMgr, aCID, aIID) {
    if (!aIID.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CID))
      return nsURLFormatterFactory;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

function NSGetModule(aCompMgr, aFileSpec) { return nsURLFormatterModule; }
