




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const GONK_CELLBROADCAST_SERVICE_CONTRACTID =
  "@mozilla.org/cellbroadcast/gonkservice;1";
const GONK_CELLBROADCAST_SERVICE_CID =
  Components.ID("{7ba407ce-21fd-11e4-a836-1bfdee377e5c}");
const CELLBROADCASTMESSAGE_CID =
  Components.ID("{29474c96-3099-486f-bb4a-3c9a1da834e4}");
const CELLBROADCASTETWSINFO_CID =
  Components.ID("{59f176ee-9dcd-4005-9d47-f6be0cd08e17}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

let DEBUG;
function debug(s) {
  dump("CellBroadcastService: " + s);
}

function CellBroadcastMessage(aServiceId,
                              aGsmGeographicalScope,
                              aMessageCode,
                              aMessageId,
                              aLanguage,
                              aBody,
                              aMessageClass,
                              aTimestamp,
                              aCdmaServiceCategory,
                              aHasEtwsInfo,
                              aEtwsWarningType,
                              aEtwsEmergencyUserAlert,
                              aEtwsPopup) {
  this.serviceId = aServiceId;
  this.gsmGeographicalScope = aGsmGeographicalScope;
  this.messageCode = aMessageCode;
  this.messageId = aMessageId;
  this.language = aLanguage;
  this.body = aBody;
  this.messageClass = aMessageClass;
  this.timestamp = aTimestamp;

  this.cdmaServiceCategory = aCdmaServiceCategory;

  if (aHasEtwsInfo) {
    this.etws = new CellBroadcastEtwsInfo(aEtwsWarningType,
                                          aEtwsEmergencyUserAlert,
                                          aEtwsPopup);
  }
}
CellBroadcastMessage.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozCellBroadcastMessage]),
  classID:        CELLBROADCASTMESSAGE_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          CELLBROADCASTMESSAGE_CID,
    classDescription: "CellBroadcastMessage",
    flags:            Ci.nsIClassInfo.DOM_OBJECT,
    interfaces:       [Ci.nsIDOMMozCellBroadcastMessage]
  }),

  
  serviceId: -1,

  gsmGeographicalScope: null,
  messageCode: null,
  messageId: null,
  language: null,
  body: null,
  messageClass: null,
  timestamp: null,

  etws: null,
  cdmaServiceCategory: null
};

function CellBroadcastEtwsInfo(aEtwsWarningType,
                               aEtwsEmergencyUserAlert,
                               aEtwsPopup) {
  this.warningType = aEtwsWarningType;
  this.emergencyUserAlert = aEtwsEmergencyUserAlert;
  this.popup = aEtwsPopup;
}
CellBroadcastEtwsInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozCellBroadcastEtwsInfo]),
  classID:        CELLBROADCASTETWSINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          CELLBROADCASTETWSINFO_CID,
    classDescription: "CellBroadcastEtwsInfo",
    flags:            Ci.nsIClassInfo.DOM_OBJECT,
    interfaces:       [Ci.nsIDOMMozCellBroadcastEtwsInfo]
  }),

  

  warningType: null,
  emergencyUserAlert: null,
  popup: null
};

function CellBroadcastService() {
  this._listeners = [];

  this._updateDebugFlag();

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}
CellBroadcastService.prototype = {
  classID: GONK_CELLBROADCAST_SERVICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_CELLBROADCAST_SERVICE_CID,
                                    contractID: GONK_CELLBROADCAST_SERVICE_CONTRACTID,
                                    classDescription: "CellBroadcastService",
                                    interfaces: [Ci.nsICellBroadcastService,
                                                 Ci.nsIGonkCellBroadcastService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsICellBroadcastService,
                                         Ci.nsIGonkCellBroadcastService,
                                         Ci.nsIObserver]),

  
  _listeners: null,

  _updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
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
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.splice(index, 1);
  },

  


  notifyMessageReceived: function(aServiceId,
                                  aGsmGeographicalScope,
                                  aMessageCode,
                                  aMessageId,
                                  aLanguage,
                                  aBody,
                                  aMessageClass,
                                  aTimestamp,
                                  aCdmaServiceCategory,
                                  aHasEtwsInfo,
                                  aEtwsWarningType,
                                  aEtwsEmergencyUserAlert,
                                  aEtwsPopup) {
    let message = new CellBroadcastMessage(aServiceId,
                                           aGsmGeographicalScope,
                                           aMessageCode,
                                           aMessageId,
                                           aLanguage,
                                           aBody,
                                           aMessageClass,
                                           aTimestamp,
                                           aCdmaServiceCategory,
                                           aHasEtwsInfo,
                                           aEtwsWarningType,
                                           aEtwsEmergencyUserAlert,
                                           aEtwsPopup);

    for (let listener of this._listeners) {
      try {
        
        
        
        listener.notifyMessageReceived(message);
      } catch (e) {
        debug("listener threw an exception: " + e);
      }
    }
  },

  


  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

        
        this._listeners = [];
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CellBroadcastService]);
