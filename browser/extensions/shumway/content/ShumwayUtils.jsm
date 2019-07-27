














var EXPORTED_SYMBOLS = ["ShumwayUtils"];

const PREF_PREFIX = 'shumway.';
const PREF_DISABLED = PREF_PREFIX + 'disabled';
const PREF_WHITELIST = PREF_PREFIX + 'swf.whitelist';

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cm = Components.manager;
let Cu = Components.utils;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

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

let ShumwayUtils = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  _registered: false,

  init: function init() {
    this.migratePreferences();
    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();

    Cc["@mozilla.org/parentprocessmessagemanager;1"]
      .getService(Ci.nsIMessageBroadcaster)
      .addMessageListener('Shumway:Chrome:isEnabled', this);

    
    Services.prefs.addObserver(PREF_DISABLED, this, false);
  },

  migratePreferences: function migratePreferences() {
    
    
    
    
    if (Services.prefs.prefHasUserValue(PREF_DISABLED) &&
        !Services.prefs.prefHasUserValue(PREF_WHITELIST) &&
        !getBoolPref(PREF_DISABLED, false)) {
      
      Services.prefs.setCharPref(PREF_WHITELIST, '*');
    }
  },

  
  observe: function observe(aSubject, aTopic, aData) {
    if (this.enabled)
      this._ensureRegistered();
    else
      this._ensureUnregistered();
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case 'Shumway:Chrome:isEnabled':
        return this.enabled;
    }
  },
  
  



  get enabled() {
    return !getBoolPref(PREF_DISABLED, true);
  },

  _ensureRegistered: function _ensureRegistered() {
    if (this._registered)
      return;

    
    Cu.import('resource://shumway/ShumwayBootstrapUtils.jsm');
    ShumwayBootstrapUtils.register();

    this._registered = true;

    log('Shumway is registered');

    let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
      .getService(Ci.nsIFrameScriptLoader);
    globalMM.broadcastAsyncMessage('Shumway:Child:refreshSettings');
  },

  _ensureUnregistered: function _ensureUnregistered() {
    if (!this._registered)
      return;

    
    ShumwayBootstrapUtils.unregister();
    Cu.unload('resource://shumway/ShumwayBootstrapUtils.jsm');

    this._registered = false;

    log('Shumway is unregistered');

    let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
      .getService(Ci.nsIFrameScriptLoader);
    globalMM.broadcastAsyncMessage('Shumway:Child:refreshSettings');
  }
};
