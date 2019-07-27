




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSettingsCellBroadcastDisabled = "ril.cellbroadcast.disabled";
const kSettingsCellBroadcastSearchList = "ril.cellbroadcast.searchlist";

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "gRadioInterfaceLayer",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

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

function CellBroadcastService() {
  this._listeners = [];

  this._updateDebugFlag();

  let lock = gSettingsService.createLock();

  







  lock.get(kSettingsCellBroadcastDisabled, this);

  













  lock.get(kSettingsCellBroadcastSearchList, this);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
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
                                         Ci.nsISettingsServiceCallback,
                                         Ci.nsIObserver]),

  
  _listeners: null,

  
  _cellBroadcastSearchList: null,

  _updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _convertCbGsmGeographicalScope: function(aGeographicalScope) {
    return (aGeographicalScope >= Ci.nsICellBroadcastService.GSM_GEOGRAPHICAL_SCOPE_INVALID)
      ? null
      : RIL.CB_GSM_GEOGRAPHICAL_SCOPE_NAMES[aGeographicalScope];
  },

  _convertCbMessageClass: function(aMessageClass) {
    return (aMessageClass >= Ci.nsICellBroadcastService.GSM_MESSAGE_CLASS)
      ? null
      : RIL.GECKO_SMS_MESSAGE_CLASSES[aMessageClass];
  },

  _convertCbEtwsWarningType: function(aWarningType) {
    return (aWarningType >= Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID)
      ? null
      : RIL.CB_ETWS_WARNING_TYPE_NAMES[aWarningType];
  },

  _retrieveSettingValueByClient: function(aClientId, aSettings) {
    return Array.isArray(aSettings) ? aSettings[aClientId] : aSettings;
  },

  


  setCellBroadcastDisabled: function(aSettings) {
    let numOfRilClients = gRadioInterfaceLayer.numRadioInterfaces;
    let responses = [];
    for (let clientId = 0; clientId < numOfRilClients; clientId++) {
      gRadioInterfaceLayer
        .getRadioInterface(clientId)
        .sendWorkerMessage("setCellBroadcastDisabled",
                           { disabled: this._retrieveSettingValueByClient(clientId, aSettings) });
    }
  },

  


  setCellBroadcastSearchList: function(aSettings) {
    let numOfRilClients = gRadioInterfaceLayer.numRadioInterfaces;
    let responses = [];
    for (let clientId = 0; clientId < numOfRilClients; clientId++) {
      let newSearchList = this._retrieveSettingValueByClient(clientId, aSettings);
      let oldSearchList = this._retrieveSettingValueByClient(clientId,
                                                          this._cellBroadcastSearchList);

      if ((newSearchList == oldSearchList) ||
          (newSearchList && oldSearchList &&
           newSearchList.gsm == oldSearchList.gsm &&
           newSearchList.cdma == oldSearchList.cdma)) {
        return;
      }

      gRadioInterfaceLayer
        .getRadioInterface(clientId).sendWorkerMessage("setCellBroadcastSearchList",
                                                       { searchList: newSearchList },
                                                       (function callback(aResponse) {
        if (DEBUG && !aResponse.success) {
          debug("Failed to set new search list: " + newSearchList +
                " to client id: " + clientId);
        }

        responses.push(aResponse);
        if (responses.length == numOfRilClients) {
          let successCount = 0;
          for (let i = 0; i < responses.length; i++) {
            if (responses[i].success) {
              successCount++;
            }
          }
          if (successCount == numOfRilClients) {
            this._cellBroadcastSearchList = aSettings;
          } else {
            
            let lock = gSettingsService.createLock();
            lock.set(kSettingsCellBroadcastSearchList,
                     this._cellBroadcastSearchList, null);
          }
        }

        return false;
      }).bind(this));
    }
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
    
    
    let systemMessage = {
      serviceId: aServiceId,
      gsmGeographicalScope: this._convertCbGsmGeographicalScope(aGsmGeographicalScope),
      messageCode: aMessageCode,
      messageId: aMessageId,
      language: aLanguage,
      body: aBody,
      messageClass: this._convertCbMessageClass(aMessageClass),
      timestamp: aTimestamp,
      cdmaServiceCategory: null,
      etws: null
    };

    if (aHasEtwsInfo) {
      systemMessage.etws = {
        warningType: this._convertCbEtwsWarningType(aEtwsWarningType),
        emergencyUserAlert: aEtwsEmergencyUserAlert,
        popup: aEtwsPopup
      };
    }

    if (aCdmaServiceCategory !=
        Ci.nsICellBroadcastService.CDMA_SERVICE_CATEGORY_INVALID) {
      systemMessage.cdmaServiceCategory = aCdmaServiceCategory;
    }

    if (DEBUG) {
      debug("CBS system message to be broadcasted: " + JSON.stringify(systemMessage));
    }

    gSystemMessenger.broadcastMessage("cellbroadcast-received", systemMessage);

    
    for (let listener of this._listeners) {
      try {
        listener.notifyMessageReceived(aServiceId,
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
      } catch (e) {
        debug("listener threw an exception: " + e);
      }
    }
  },

  


  handle: function(aName, aResult) {
    switch (aName) {
      case kSettingsCellBroadcastSearchList:
        if (DEBUG) {
          debug("'" + kSettingsCellBroadcastSearchList +
                "' is now " + JSON.stringify(aResult));
        }

        this.setCellBroadcastSearchList(aResult);
        break;
      case kSettingsCellBroadcastDisabled:
        if (DEBUG) {
          debug("'" + kSettingsCellBroadcastDisabled +
                "' is now " + JSON.stringify(aResult));
        }

        this.setCellBroadcastDisabled(aResult);
        break;
    }
  },

  


  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case kMozSettingsChangedObserverTopic:
        if ("wrappedJSObject" in aSubject) {
          aSubject = aSubject.wrappedJSObject;
        }
        this.handle(aSubject.key, aSubject.value);
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);

        
        this._listeners = [];
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CellBroadcastService]);
