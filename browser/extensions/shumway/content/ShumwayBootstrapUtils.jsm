















'use strict';

var EXPORTED_SYMBOLS = ['ShumwayBootstrapUtils'];

const PREF_PREFIX = 'shumway.';
const PREF_IGNORE_CTP = PREF_PREFIX + 'ignoreCTP';
const PREF_WHITELIST = PREF_PREFIX + 'swf.whitelist';
const SWF_CONTENT_TYPE = 'application/x-shockwave-flash';

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cm = Components.manager;
let Cu = Components.utils;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

Cu.import('resource://shumway/ShumwayStreamConverter.jsm');

let Ph = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
let registerOverlayPreview = 'registerPlayPreviewMimeType' in Ph;

function getBoolPref(pref, def) {
  try {
    return Services.prefs.getBoolPref(pref);
  } catch (ex) {
    return def;
  }
}

function getStringPref(pref, def) {
  try {
    return Services.prefs.getComplexValue(pref, Ci.nsISupportsString).data;
  } catch (ex) {
    return def;
  }
}

function log(str) {
  dump(str + '\n');
}


function Factory() {}
Factory.prototype = {
  register: function register(targetConstructor) {
    var proto = targetConstructor.prototype;
    this._classID = proto.classID;

    var factory = XPCOMUtils._getFactory(targetConstructor);
    this._factory = factory;

    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.registerFactory(proto.classID, proto.classDescription,
      proto.contractID, factory);
  },

  unregister: function unregister() {
    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(this._classID, this._factory);
  }
};

let converterFactory = new Factory();
let overlayConverterFactory = new Factory();

var ShumwayBootstrapUtils = {
  register: function () {
    
    converterFactory.register(ShumwayStreamConverter);
    overlayConverterFactory.register(ShumwayStreamOverlayConverter);

    if (registerOverlayPreview) {
      var ignoreCTP = getBoolPref(PREF_IGNORE_CTP, true);
      var whitelist = getStringPref(PREF_WHITELIST); 
      Ph.registerPlayPreviewMimeType(SWF_CONTENT_TYPE, ignoreCTP,
                                     undefined, whitelist);
    }
  },

  unregister: function () {
    
    converterFactory.unregister();
    overlayConverterFactory.unregister();

    if (registerOverlayPreview) {
      Ph.unregisterPlayPreviewMimeType(SWF_CONTENT_TYPE);
    }
  }
};
