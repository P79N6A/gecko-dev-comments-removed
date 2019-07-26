














var EXPORTED_SYMBOLS = ["PdfJs"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cm = Components.manager;
const Cu = Components.utils;

const PREF_PREFIX = 'pdfjs';
const PREF_DISABLED = PREF_PREFIX + '.disabled';
const PREF_FIRST_RUN = PREF_PREFIX + '.firstRun';
const PREF_PREVIOUS_ACTION = PREF_PREFIX + '.previousHandler.preferredAction';
const PREF_PREVIOUS_ASK = PREF_PREFIX + '.previousHandler.alwaysAskBeforeHandling';
const TOPIC_PDFJS_HANDLER_CHANGED = 'pdfjs:handlerChanged';

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://pdf.js.components/PdfStreamConverter.js');

let Svc = {};
XPCOMUtils.defineLazyServiceGetter(Svc, 'mime',
                                   '@mozilla.org/mime;1',
                                   'nsIMIMEService');

function getBoolPref(aPref, aDefaultValue) {
  try {
    return Services.prefs.getBoolPref(aPref);
  } catch (ex) {
    return aDefaultValue;
  }
}


let Factory = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory]),
  _targetConstructor: null,

  register: function register(targetConstructor) {
    this._targetConstructor = targetConstructor;
    var proto = targetConstructor.prototype;
    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.registerFactory(proto.classID, proto.classDescription,
                              proto.contractID, this);
  },

  unregister: function unregister() {
    var proto = this._targetConstructor.prototype;
    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(proto.classID, this);
    this._targetConstructor = null;
  },

  
  createInstance: function createInstance(aOuter, iid) {
    if (aOuter !== null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new (this._targetConstructor)).QueryInterface(iid);
  },

  
  lockFactory: function lockFactory(lock) { 
    
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
};

let PdfJs = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  _registered: false,

  init: function init() {
    
    if (!getBoolPref(PREF_DISABLED, true) && getBoolPref(PREF_FIRST_RUN, false)) {
      Services.prefs.setBoolPref(PREF_FIRST_RUN, false);

      let handlerInfo = Svc.mime.getFromTypeAndExtension('application/pdf', 'pdf');
      
      
      
      Services.prefs.setIntPref(PREF_PREVIOUS_ACTION, handlerInfo.preferredAction);
      Services.prefs.setBoolPref(PREF_PREVIOUS_ASK, handlerInfo.alwaysAskBeforeHandling);

      let handlerService = Cc['@mozilla.org/uriloader/handler-service;1'].
                           getService(Ci.nsIHandlerService);

      
      handlerInfo.alwaysAskBeforeHandling = false;
      handlerInfo.preferredAction = Ci.nsIHandlerInfo.handleInternally;
      handlerService.store(handlerInfo);
    }

    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();

    
    
    Services.prefs.addObserver(PREF_DISABLED, this, false);
    Services.obs.addObserver(this, TOPIC_PDFJS_HANDLER_CHANGED, false);
  },

  
  observe: function observe(aSubject, aTopic, aData) {
    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();
  },
  
  




  get enabled() {
    var disabled = getBoolPref(PREF_DISABLED, true);
    if (disabled)
      return false;

    var handlerInfo = Svc.mime.
                        getFromTypeAndExtension('application/pdf', 'pdf');
    return handlerInfo.alwaysAskBeforeHandling == false &&
           handlerInfo.preferredAction == Ci.nsIHandlerInfo.handleInternally;
  },

  _ensureRegistered: function _ensureRegistered() {
    if (this._registered)
      return;

    Factory.register(PdfStreamConverter);
    this._registered = true;
  },

  _ensureUnregistered: function _ensureUnregistered() {
    if (!this._registered)
      return;

    Factory.unregister();
    this._registered = false;
  }
};
