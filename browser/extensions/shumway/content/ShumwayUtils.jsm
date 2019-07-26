














var EXPORTED_SYMBOLS = ["ShumwayUtils"];

const RESOURCE_NAME = 'shumway';
const EXT_PREFIX = 'shumway@research.mozilla.org';
const SWF_CONTENT_TYPE = 'application/x-shockwave-flash';
const PREF_PREFIX = 'shumway.';
const PREF_DISABLED = PREF_PREFIX + 'disabled';
const PREF_IGNORE_CTP = PREF_PREFIX + 'ignoreCTP';

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cm = Components.manager;
let Cu = Components.utils;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://shumway/ShumwayStreamConverter.jsm');

let Svc = {};
XPCOMUtils.defineLazyServiceGetter(Svc, 'mime',
                                   '@mozilla.org/mime;1',
                                   'nsIMIMEService');
XPCOMUtils.defineLazyServiceGetter(Svc, 'pluginHost',
                                   '@mozilla.org/plugin/host;1',
                                   'nsIPluginHost');

function getBoolPref(pref, def) {
  try {
    return Services.prefs.getBoolPref(pref);
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
    this._factory = null;
  }
};

let converterFactory = new Factory();
let overlayConverterFactory = new Factory();

let ShumwayUtils = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  _registered: false,

  init: function init() {
    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();

    
    Services.prefs.addObserver(PREF_DISABLED, this, false);
  },

  
  observe: function observe(aSubject, aTopic, aData) {
    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();
  },
  
  



  get enabled() {
    return !getBoolPref(PREF_DISABLED, true);
  },

  _ensureRegistered: function _ensureRegistered() {
    if (this._registered)
      return;

    
    converterFactory.register(ShumwayStreamConverter);
    overlayConverterFactory.register(ShumwayStreamOverlayConverter);

    var ignoreCTP = getBoolPref(PREF_IGNORE_CTP, true);

    Svc.pluginHost.registerPlayPreviewMimeType(SWF_CONTENT_TYPE, ignoreCTP);

    this._registered = true;

    log('Shumway is registered');
  },

  _ensureUnregistered: function _ensureUnregistered() {
    if (!this._registered)
      return;

    
    converterFactory.unregister();
    overlayConverterFactory.unregister();

    Svc.pluginHost.unregisterPlayPreviewMimeType(SWF_CONTENT_TYPE);

    this._registered = false;

    log('Shumway is unregistered');
  }
};
