




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const GONK_VOICEMAIL_SERVICE_CONTRACTID =
  "@mozilla.org/voicemail/gonkvoicemailservice;1";
const GONK_VOICEMAIL_SERVICE_CID =
  Components.ID("{c332f318-1cce-4f02-b676-bb5031d10736}");

const NS_MOBILE_CONNECTION_SERVICE_CONTRACTID =
  "@mozilla.org/mobileconnection/mobileconnectionservice;1";

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefDefaultServiceId = "dom.voicemail.defaultServiceId";

let DEBUG;
function debug(s) {
  dump("VoicemailService: " + s);
}

function VoicemailProvider(aServiceId) {
  this.serviceId = aServiceId;
}
VoicemailProvider.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIVoicemailProvider]),

  

  serviceId: 0,

  number: null,
  displayName: null,

  hasMessages: false,
  messageCount: 0,
  returnNumber: null,
  returnMessage: null,
};

function VoicemailService() {
  
  let mcService = Cc[NS_MOBILE_CONNECTION_SERVICE_CONTRACTID]
                  .getService(Ci.nsIMobileConnectionService);
  let numItems = mcService.numItems;
  this._providers = [];
  for (let i = 0; i < numItems; i++) {
    this._providers.push(new VoicemailProvider(i));
  }

  this._listeners = [];

  
  this._defaultServiceId = this._getDefaultServiceId();
  this._updateDebugFlag();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}

VoicemailService.prototype = {
  classID: GONK_VOICEMAIL_SERVICE_CID,

  classInfo: XPCOMUtils.generateCI({
    classID: GONK_VOICEMAIL_SERVICE_CID,
    contractID: GONK_VOICEMAIL_SERVICE_CONTRACTID,
    classDescription: "VoicemailService",
    interfaces: [
      Ci.nsIVoicemailService,
      Ci.nsIGonkVoicemailService
    ],
    flags: Ci.nsIClassInfo.SINGLETON
  }),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIVoicemailService,
    Ci.nsIGonkVoicemailService,
    Ci.nsIObserver
  ]),

  _defaultServiceId: null,
  _providers: null,

  _updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _getDefaultServiceId: function() {
    let id = Services.prefs.getIntPref(kPrefDefaultServiceId);
    if (id >= this.numItems || id < 0) {
      id = 0;
    }

    return id;
  },

  _listeners: null,

  _notifyListeners: function(aMethodName, aItem) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      try {
        listener[aMethodName].call(listener, aItem);
      } catch (e) {
        if (DEBUG) {
          debug("listener for " + aMethodName + " threw an exception: " + e);
        }
      }
    }
  },

  



  get numItems() {
    return this._providers.length;
  },

  getItemByServiceId: function(aServiceId) {
    let provider = this._providers[aServiceId];
    if (!provider) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }
    return provider;
  },

  getDefaultItem: function() {
    return this.getItemByServiceId(this._defaultServiceId);
  },

  registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.push(aListener);
  },

  unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index < 0) {
      return Cr.NS_ERROR_FAILURE;
    }

    this._listeners.splice(index, 1);
  },

  



  notifyStatusChanged: function(aServiceId, aHasMessages, aMessageCount,
                                aReturnNumber, aReturnMessage) {
    if (DEBUG) {
      debug("notifyStatusChanged: " +
            JSON.stringify(Array.prototype.slice.call(arguments)));
    }

    let provider = this.getItemByServiceId(aServiceId);

    let changed = false;
    if (provider.hasMessages != aHasMessages) {
      provider.hasMessages = aHasMessages;
      changed = true;
    }
    if (provider.messageCount != aMessageCount) {
      provider.messageCount = aMessageCount;
      changed = true;
    } else if (aMessageCount == -1) {
      
      changed = true;
    }
    if (provider.returnNumber != aReturnNumber) {
      provider.returnNumber = aReturnNumber;
      changed = true;
    }
    if (provider.returnMessage != aReturnMessage) {
      provider.returnMessage = aReturnMessage;
      changed = true;
    }

    if (changed) {
      this._notifyListeners("notifyStatusChanged", provider);
    }
  },

  notifyInfoChanged: function(aServiceId, aNumber, aDisplayName) {
    if (DEBUG) {
      debug("notifyInfoChanged: " +
            JSON.stringify(Array.prototype.slice.call(arguments)));
    }

    let provider = this.getItemByServiceId(aServiceId);

    let changed = false;
    if (provider.number != aNumber) {
      provider.number = aNumber;
      changed = true;
    }
    if (provider.displayName != aDisplayName) {
      provider.displayName = aDisplayName;
      changed = true;
    }

    if (changed) {
      this._notifyListeners("notifyInfoChanged", provider);
    }
  },

  



  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        } else if (aData === kPrefDefaultServiceId) {
          this._defaultServiceId = this._getDefaultServiceId();
        }
        break;

      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
        Services.prefs.removeObserver(kPrefDefaultServiceId, this);

        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

        
        this._listeners = [];
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([VoicemailService]);
