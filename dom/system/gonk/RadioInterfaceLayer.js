














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Sntp.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/Promise.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);


var DEBUG = RIL.DEBUG_RIL;


let debugPref = false;
try {
  debugPref = Services.prefs.getBoolPref("ril.debugging.enabled");
} catch(e) {
  debugPref = false;
}
DEBUG = RIL.DEBUG_RIL || debugPref;

function debug(s) {
  dump("-*- RadioInterfaceLayer: " + s + "\n");
}


let RILQUIRKS_DATA_REGISTRATION_ON_DEMAND =
  libcutils.property_get("ro.moz.ril.data_reg_on_demand", "false") == "true";



let RILQUIRKS_RADIO_OFF_WO_CARD =
  libcutils.property_get("ro.moz.ril.radio_off_wo_card", "false") == "true";

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");
const GSMICCINFO_CID =
  Components.ID("{d90c4261-a99d-47bc-8b05-b057bb7e8f8a}");
const CDMAICCINFO_CID =
  Components.ID("{39ba3c08-aacc-46d0-8c04-9b619c387061}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSilentSmsReceivedObserverTopic    = "silent-sms-received";
const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const kSysClockChangeObserverTopic       = "system-clock-change";
const kScreenStateChangedTopic           = "screen-state-changed";

const kSettingsCellBroadcastSearchList = "ril.cellbroadcast.searchlist";
const kSettingsClockAutoUpdateEnabled = "time.clock.automatic-update.enabled";
const kSettingsClockAutoUpdateAvailable = "time.clock.automatic-update.available";
const kSettingsTimezoneAutoUpdateEnabled = "time.timezone.automatic-update.enabled";
const kSettingsTimezoneAutoUpdateAvailable = "time.timezone.automatic-update.available";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefCellBroadcastDisabled = "ril.cellbroadcast.disabled";
const kPrefClirModePreference = "ril.clirMode";
const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

const RADIO_POWER_OFF_TIMEOUT = 30000;
const SMS_HANDLED_WAKELOCK_TIMEOUT = 5000;
const HW_DEFAULT_CLIENT_ID = 0;

const RIL_IPC_MOBILECONNECTION_MSG_NAMES = [
  "RIL:GetRilContext",
  "RIL:GetAvailableNetworks",
  "RIL:SelectNetwork",
  "RIL:SelectNetworkAuto",
  "RIL:SetPreferredNetworkType",
  "RIL:GetPreferredNetworkType",
  "RIL:SendMMI",
  "RIL:CancelMMI",
  "RIL:RegisterMobileConnectionMsg",
  "RIL:SetCallForwardingOptions",
  "RIL:GetCallForwardingOptions",
  "RIL:SetCallBarringOptions",
  "RIL:GetCallBarringOptions",
  "RIL:ChangeCallBarringPassword",
  "RIL:SetCallWaitingOptions",
  "RIL:GetCallWaitingOptions",
  "RIL:SetCallingLineIdRestriction",
  "RIL:GetCallingLineIdRestriction",
  "RIL:SetRoamingPreference",
  "RIL:GetRoamingPreference",
  "RIL:ExitEmergencyCbMode",
  "RIL:SetRadioEnabled",
  "RIL:SetVoicePrivacyMode",
  "RIL:GetVoicePrivacyMode",
  "RIL:GetSupportedNetworkTypes"
];

const RIL_IPC_MOBILENETWORK_MSG_NAMES = [
  "RIL:GetLastKnownNetwork",
  "RIL:GetLastKnownHomeNetwork"
];

const RIL_IPC_ICCMANAGER_MSG_NAMES = [
  "RIL:SendStkResponse",
  "RIL:SendStkMenuSelection",
  "RIL:SendStkTimerExpiration",
  "RIL:SendStkEventDownload",
  "RIL:GetCardLockState",
  "RIL:UnlockCardLock",
  "RIL:SetCardLock",
  "RIL:GetCardLockRetryCount",
  "RIL:IccOpenChannel",
  "RIL:IccExchangeAPDU",
  "RIL:IccCloseChannel",
  "RIL:ReadIccContacts",
  "RIL:UpdateIccContact",
  "RIL:RegisterIccMsg",
  "RIL:MatchMvno"
];

const RIL_IPC_VOICEMAIL_MSG_NAMES = [
  "RIL:RegisterVoicemailMsg",
  "RIL:GetVoicemailInfo"
];

const RIL_IPC_CELLBROADCAST_MSG_NAMES = [
  "RIL:RegisterCellBroadcastMsg"
];

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1",
                                   "nsIRilMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gTimeService",
                                   "@mozilla.org/time/timeservice;1",
                                   "nsITimeService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemWorkerManager",
                                   "@mozilla.org/telephony/system-worker-manager;1",
                                   "nsISystemWorkerManager");

XPCOMUtils.defineLazyServiceGetter(this, "gTelephonyProvider",
                                   "@mozilla.org/telephony/telephonyprovider;1",
                                   "nsIGonkTelephonyProvider");

XPCOMUtils.defineLazyGetter(this, "WAP", function() {
  let wap = {};
  Cu.import("resource://gre/modules/WapPushManager.js", wap);
  return wap;
});

XPCOMUtils.defineLazyGetter(this, "PhoneNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyGetter(this, "gMessageManager", function() {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    ril: null,

    
    
    targetsByTopic: {},
    topics: [],

    targetMessageQueue: [],
    ready: false,

    init: function(ril) {
      this.ril = ril;

      Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
      Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
      this._registerMessageListeners();
    },

    _shutdown: function() {
      this.ril = null;

      Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      this._unregisterMessageListeners();
    },

    _registerMessageListeners: function() {
      ppmm.addMessageListener("child-process-shutdown", this);
      for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
      for (let msgname of RIL_IPC_MOBILENETWORK_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
      for (let msgName of RIL_IPC_ICCMANAGER_MSG_NAMES) {
        ppmm.addMessageListener(msgName, this);
      }
      for (let msgname of RIL_IPC_VOICEMAIL_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
      for (let msgname of RIL_IPC_CELLBROADCAST_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
    },

    _unregisterMessageListeners: function() {
      ppmm.removeMessageListener("child-process-shutdown", this);
      for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      for (let msgname of RIL_IPC_MOBILENETWORK_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      for (let msgName of RIL_IPC_ICCMANAGER_MSG_NAMES) {
        ppmm.removeMessageListener(msgName, this);
      }
      for (let msgname of RIL_IPC_VOICEMAIL_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      for (let msgname of RIL_IPC_CELLBROADCAST_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      ppmm = null;
    },

    _registerMessageTarget: function(topic, target) {
      let targets = this.targetsByTopic[topic];
      if (!targets) {
        targets = this.targetsByTopic[topic] = [];
        let list = this.topics;
        if (list.indexOf(topic) == -1) {
          list.push(topic);
        }
      }

      if (targets.indexOf(target) != -1) {
        if (DEBUG) debug("Already registered this target!");
        return;
      }

      targets.push(target);
      if (DEBUG) debug("Registered " + topic + " target: " + target);
    },

    _unregisterMessageTarget: function(topic, target) {
      if (topic == null) {
        
        for (let type of this.topics) {
          this._unregisterMessageTarget(type, target);
        }
        return;
      }

      
      let targets = this.targetsByTopic[topic];
      if (!targets) {
        return;
      }

      let index = targets.indexOf(target);
      if (index != -1) {
        targets.splice(index, 1);
        if (DEBUG) debug("Unregistered " + topic + " target: " + target);
      }
    },

    _enqueueTargetMessage: function(topic, message, options) {
      let msg = { topic : topic,
                  message : message,
                  options : options };
      
      
      let messageQueue = this.targetMessageQueue;
      for(let i = 0; i < messageQueue.length; i++) {
        if (messageQueue[i].message === message &&
            messageQueue[i].options.clientId === options.clientId) {
          messageQueue.splice(i, 1);
          break;
        }
      }

      messageQueue.push(msg);
    },

    _sendTargetMessage: function(topic, message, options) {
      if (!this.ready) {
        this._enqueueTargetMessage(topic, message, options);
        return;
      }

      let targets = this.targetsByTopic[topic];
      if (!targets) {
        return;
      }

      for (let target of targets) {
        target.sendAsyncMessage(message, options);
      }
    },

    _resendQueuedTargetMessage: function() {
      this.ready = true;

      
      
      
      

      
      for each (let msg in this.targetMessageQueue) {
        this._sendTargetMessage(msg.topic, msg.message, msg.options);
      }
      this.targetMessageQueue = null;
    },

    



    receiveMessage: function(msg) {
      if (DEBUG) debug("Received '" + msg.name + "' message from content process");
      if (msg.name == "child-process-shutdown") {
        
        
        
        this._unregisterMessageTarget(null, msg.target);
        return null;
      }

      if (RIL_IPC_MOBILECONNECTION_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("mobileconnection")) {
          if (DEBUG) {
            debug("MobileConnection message " + msg.name +
                  " from a content process with no 'mobileconnection' privileges.");
          }
          return null;
        }
      } else if (RIL_IPC_MOBILENETWORK_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("mobilenetwork")) {
          if (DEBUG) {
            debug("MobileNetwork message " + msg.name +
                  " from a content process with no 'mobilenetwork' privileges.");
          }
          return null;
        }
      } else if (RIL_IPC_ICCMANAGER_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("mobileconnection")) {
          if (DEBUG) {
            debug("IccManager message " + msg.name +
                  " from a content process with no 'mobileconnection' privileges.");
          }
          return null;
        }
      } else if (RIL_IPC_VOICEMAIL_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("voicemail")) {
          if (DEBUG) {
            debug("Voicemail message " + msg.name +
                  " from a content process with no 'voicemail' privileges.");
          }
          return null;
        }
      } else if (RIL_IPC_CELLBROADCAST_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("cellbroadcast")) {
          if (DEBUG) {
            debug("Cell Broadcast message " + msg.name +
                  " from a content process with no 'cellbroadcast' privileges.");
          }
          return null;
        }
      } else {
        if (DEBUG) debug("Ignoring unknown message type: " + msg.name);
        return null;
      }

      switch (msg.name) {
        case "RIL:RegisterMobileConnectionMsg":
          this._registerMessageTarget("mobileconnection", msg.target);
          return null;
        case "RIL:RegisterIccMsg":
          this._registerMessageTarget("icc", msg.target);
          return null;
        case "RIL:RegisterVoicemailMsg":
          this._registerMessageTarget("voicemail", msg.target);
          return null;
        case "RIL:RegisterCellBroadcastMsg":
          this._registerMessageTarget("cellbroadcast", msg.target);
          return null;
      }

      let clientId = msg.json.clientId || 0;
      let radioInterface = this.ril.getRadioInterface(clientId);
      if (!radioInterface) {
        if (DEBUG) debug("No such radio interface: " + clientId);
        return null;
      }

      if (msg.name === "RIL:SetRadioEnabled") {
        
        return gRadioEnabledController.receiveMessage(msg);
      }

      return radioInterface.receiveMessage(msg);
    },

    



    observe: function(subject, topic, data) {
      switch (topic) {
        case kSysMsgListenerReadyObserverTopic:
          Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);
          this._resendQueuedTargetMessage();
          break;
        case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
          this._shutdown();
          break;
      }
    },

    sendMobileConnectionMessage: function(message, clientId, data) {
      this._sendTargetMessage("mobileconnection", message, {
        clientId: clientId,
        data: data
      });
    },

    sendVoicemailMessage: function(message, clientId, data) {
      this._sendTargetMessage("voicemail", message, {
        clientId: clientId,
        data: data
      });
    },

    sendCellBroadcastMessage: function(message, clientId, data) {
      this._sendTargetMessage("cellbroadcast", message, {
        clientId: clientId,
        data: data
      });
    },

    sendIccMessage: function(message, clientId, data) {
      this._sendTargetMessage("icc", message, {
        clientId: clientId,
        data: data
      });
    }
  };
});

XPCOMUtils.defineLazyGetter(this, "gRadioEnabledController", function() {
  return {
    ril: null,
    pendingMessages: [],  
    timer: null,
    request: null,
    deactivatingDeferred: {},

    init: function(ril) {
      this.ril = ril;
    },

    receiveMessage: function(msg) {
      if (DEBUG) debug("setRadioEnabled: receiveMessage: " + JSON.stringify(msg));
      this.pendingMessages.push(msg);
      if (this.pendingMessages.length === 1 && !this.isDeactivatingDataCalls()) {
        this._processNextMessage();
      }
    },

    isDeactivatingDataCalls: function() {
      return this.request !== null;
    },

    finishDeactivatingDataCalls: function(clientId) {
      if (DEBUG) debug("setRadioEnabled: finishDeactivatingDataCalls: " + clientId);
      let deferred = this.deactivatingDeferred[clientId];
      if (deferred) {
        deferred.resolve();
      }
    },

    _processNextMessage: function() {
      if (this.pendingMessages.length === 0) {
        return;
      }

      let msg = this.pendingMessages.shift();
      this._handleMessage(msg);
    },

    _getNumCards: function() {
      let numCards = 0;
      for (let i = 0, N = this.ril.numRadioInterfaces; i < N; ++i) {
        if (this._isCardPresentAtClient(i)) {
          numCards++;
        }
      }
      return numCards;
    },

    _isCardPresentAtClient: function(clientId) {
      let cardState = this.ril.getRadioInterface(clientId).rilContext.cardState;
      return cardState !== RIL.GECKO_CARDSTATE_UNDETECTED &&
        cardState !== RIL.GECKO_CARDSTATE_UNKNOWN;
    },

    _isRadioAbleToEnableAtClient: function(clientId, numCards) {
      if (!RILQUIRKS_RADIO_OFF_WO_CARD) {
        return true;
      }

      
      
      

      if (this._isCardPresentAtClient(clientId)) {
        return true;
      }

      numCards = numCards == null ? this._getNumCards() : numCards;
      if (clientId === HW_DEFAULT_CLIENT_ID && numCards === 0) {
        return true;
      }

      return false;
    },

    _handleMessage: function(msg) {
      if (DEBUG) debug("setRadioEnabled: handleMessage: " + JSON.stringify(msg));
      let clientId = msg.json.clientId || 0;
      let radioInterface = this.ril.getRadioInterface(clientId);

      if (!radioInterface.isValidStateForSetRadioEnabled()) {
        radioInterface.setRadioEnabledResponse(msg.target, msg.json.data,
                                               "InvalidStateError");
        this._processNextMessage();
        return;
      }

      if (radioInterface.isDummyForSetRadioEnabled(msg.json.data)) {
        radioInterface.setRadioEnabledResponse(msg.target, msg.json.data);
        this._processNextMessage();
        return;
      }

      if (msg.json.data.enabled) {
        if (this._isRadioAbleToEnableAtClient(clientId)) {
          radioInterface.receiveMessage(msg);
        } else {
          
          radioInterface.setRadioEnabledResponse(msg.target, msg.json.data);
        }

        this._processNextMessage();
      } else {
        this.request = (function() {
          radioInterface.receiveMessage(msg);
        }).bind(this);

        
        
        
        this._deactivateDataCalls().then(() => {
          if (DEBUG) debug("setRadioEnabled: deactivation done");
          this._executeRequest();
        });

        this._createTimer();
      }
    },

    _deactivateDataCalls: function() {
      if (DEBUG) debug("setRadioEnabled: deactivating data calls...");
      this.deactivatingDeferred = {};

      let promise = Promise.resolve();
      for (let i = 0, N = this.ril.numRadioInterfaces; i < N; ++i) {
        promise = promise.then(this._deactivateDataCallsForClient(i));
      }

      return promise;
    },

    _deactivateDataCallsForClient: function(clientId) {
      return (function() {
        let deferred = this.deactivatingDeferred[clientId] = Promise.defer();
        this.ril.getRadioInterface(clientId).deactivateDataCalls();
        return deferred.promise;
      }).bind(this);
    },

    _createTimer: function() {
      if (!this.timer) {
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      }
      this.timer.initWithCallback(this._executeRequest, RADIO_POWER_OFF_TIMEOUT,
                                  Ci.nsITimer.TYPE_ONE_SHOT);
    },

    _cancelTimer: function() {
      if (this.timer) {
        this.timer.cancel();
      }
    },

    _executeRequest: function() {
      if (typeof this.request === "function") {
        if (DEBUG) debug("setRadioEnabled: executeRequest");
        this._cancelTimer();
        this.request();
        this.request = null;
      }
      this._processNextMessage();
    },
  };
});

XPCOMUtils.defineLazyGetter(this, "gDataConnectionManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

    _connectionHandlers: null,

    debug: function(s) {
      dump("-*- DataConnectionManager: " + s + "\n");
    },

    init: function(ril) {
      if (!ril) {
        return;
      }

      this._connectionHandlers = [];
      for (let clientId = 0; clientId < ril.numRadioInterfaces; clientId++) {
        let radioInterface = ril.getRadioInterface(clientId);
        this._connectionHandlers.push(
          new DataConnectionHandler(clientId, radioInterface));
      }

      Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    },

    getConnectionHandler: function(clientId) {
      return this._connectionHandlers[clientId];
    },

    _shutdown: function() {
      for (let handler of this._connectionHandlers) {
        handler.shutdown();
      }
      this._connectionHandlers = null;
      Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    },

    


    observe: function(subject, topic, data) {
      switch (topic) {
        case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
          this._shutdown();
          break;
      }
    },
  };
});



try {
  Services.prefs.setIntPref(kPrefRilNumRadioInterfaces, (function() {
    
    
    
    try {
      let numString = libcutils.property_get("ro.moz.ril.numclients", "1");
      let num = parseInt(numString, 10);
      if (num >= 0) {
        return num;
      }
    } catch (e) {}

    return 1;
  })());
} catch (e) {}

function IccInfo() {}
IccInfo.prototype = {
  iccType: null,
  iccid: null,
  mcc: null,
  mnc: null,
  spn: null,
  isDisplayNetworkNameRequired: null,
  isDisplaySpnRequired: null
};

function GsmIccInfo() {}
GsmIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozGsmIccInfo]),
  classID: GSMICCINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          GSMICCINFO_CID,
    classDescription: "MozGsmIccInfo",
    flags:            Ci.nsIClassInfo.DOM_OBJECT,
    interfaces:       [Ci.nsIDOMMozGsmIccInfo]
  }),

  

  msisdn: null
};

function CdmaIccInfo() {}
CdmaIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozCdmaIccInfo]),
  classID: CDMAICCINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          CDMAICCINFO_CID,
    classDescription: "MozCdmaIccInfo",
    flags:            Ci.nsIClassInfo.DOM_OBJECT,
    interfaces:       [Ci.nsIDOMMozCdmaIccInfo]
  }),

  

  mdn: null
};

function DataConnectionHandler(clientId, radioInterface) {
  
  this.clientId = clientId;
  this.radioInterface = radioInterface;
}
DataConnectionHandler.prototype = {
  clientId: 0,
  radioInterface: null,

  debug: function(s) {
    dump("-*- DataConnectionHandler[" + this.clientId + "]: " + s + "\n");
  },

  shutdown: function() {
    this.clientId = null;
    this.radioInterface = null;
  },
};

function RadioInterfaceLayer() {
  let options = {
    debug: debugPref,
    cellBroadcastDisabled: false,
    clirMode: RIL.CLIR_DEFAULT,
    quirks: {
      callstateExtraUint32:
        libcutils.property_get("ro.moz.ril.callstate_extra_int", "false") === "true",
      v5Legacy:
        libcutils.property_get("ro.moz.ril.v5_legacy", "true") === "true",
      requestUseDialEmergencyCall:
        libcutils.property_get("ro.moz.ril.dial_emergency_call", "false") === "true",
      simAppStateExtraFields:
        libcutils.property_get("ro.moz.ril.simstate_extra_field", "false") === "true",
      extraUint2ndCall:
        libcutils.property_get("ro.moz.ril.extra_int_2nd_call", "false") == "true",
      haveQueryIccLockRetryCount:
        libcutils.property_get("ro.moz.ril.query_icc_count", "false") == "true",
      sendStkProfileDownload:
        libcutils.property_get("ro.moz.ril.send_stk_profile_dl", "false") == "true",
      dataRegistrationOnDemand:
        libcutils.property_get("ro.moz.ril.data_reg_on_demand", "false") == "true"
    },
    rilEmergencyNumbers: libcutils.property_get("ril.ecclist") ||
                         libcutils.property_get("ro.ril.ecclist")
  };

  try {
    options.cellBroadcastDisabled =
      Services.prefs.getBoolPref(kPrefCellBroadcastDisabled);
  } catch(e) {}

  try {
    options.clirMode = Services.prefs.getIntPref(kPrefClirModePreference);
  } catch(e) {}

  let numIfaces = this.numRadioInterfaces;
  if (DEBUG) debug(numIfaces + " interfaces");
  this.radioInterfaces = [];
  for (let clientId = 0; clientId < numIfaces; clientId++) {
    options.clientId = clientId;
    this.radioInterfaces.push(new RadioInterface(options));
  }

  
  
  let lock = gSettingsService.createLock();
  
  lock.get("ril.data.enabled", this);
  
  lock.get("ril.data.defaultServiceId", this);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic, false);

  gMessageManager.init(this);
  gRadioEnabledController.init(this);
  gDataConnectionManager.init(this);
}
RadioInterfaceLayer.prototype = {

  classID:   RADIOINTERFACELAYER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACELAYER_CID,
                                    classDescription: "RadioInterfaceLayer",
                                    interfaces: [Ci.nsIRadioInterfaceLayer]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioInterfaceLayer,
                                         Ci.nsIObserver]),

  
  
  _dataEnabled: null,

  
  
  _dataDefaultClientId: -1,

  
  
  
  _currentDataClientId: -1,

  
  
  _pendingDataCallRequest: null,

  



  observe: function(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        let setting = JSON.parse(data);
        this.handle(setting.key, setting.value);
        break;
      case kNetworkInterfaceStateChangedTopic:
        let network = subject.QueryInterface(Ci.nsINetworkInterface);
        
        
        
        
        if (network.state == Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN) {
          let oldRadioInterface =
            this.radioInterfaces[this._currentDataClientId];
          if (oldRadioInterface.allDataDisconnected() &&
              typeof this._pendingDataCallRequest === "function") {
            if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
              oldRadioInterface.setDataRegistration(false);
            }
            if (DEBUG) debug("All data calls disconnected, setup pending data call.");
            this._pendingDataCallRequest();
            this._pendingDataCallRequest = null;
          }
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.obs.removeObserver(this, TOPIC_XPCOM_SHUTDOWN);
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
        break;
     }
  },

  

  handle: function(name, result) {
    switch(name) {
      
      
      case "ril.data.enabled":
        if (DEBUG) debug("'ril.data.enabled' is now " + result);
        if (this._dataEnabled == result) {
          break;
        }
        this._dataEnabled = result;

        if (DEBUG) debug("Default id for data call: " + this._dataDefaultClientId);
        if (this._dataDefaultClientId == -1) {
          
          break;
        }

        let radioInterface = this.radioInterfaces[this._dataDefaultClientId];
        radioInterface.dataCallSettings.oldEnabled = radioInterface.dataCallSettings.enabled;
        radioInterface.dataCallSettings.enabled = result;
        radioInterface.updateRILNetworkInterface();
        break;
      case "ril.data.defaultServiceId":
        result = result ? result : 0;
        if (DEBUG) debug("'ril.data.defaultServiceId' is now " + result);
        if (this._dataDefaultClientId == result) {
          break;
        }
        this._dataDefaultClientId = result;
        this.handleDataClientIdChange();
        break;
    }
  },

  handleError: function(errorMessage) {
    if (DEBUG) {
      debug("There was an error while reading RIL settings: " + errorMessage);
    }
  },

  handleDataClientIdChange: function() {
    if (this._currentDataClientId == -1) {
      
      this._currentDataClientId = this._dataDefaultClientId;
      if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
        let radioInterface = this.radioInterfaces[this._currentDataClientId];
        radioInterface.setDataRegistration(true);
      }
      if (this._dataEnabled) {
        let radioInterface = this.radioInterfaces[this._currentDataClientId];
        radioInterface.dataCallSettings.oldEnabled =
          radioInterface.dataCallSettings.enabled;
        radioInterface.dataCallSettings.enabled = true;
        radioInterface.updateRILNetworkInterface();
      }
      return;
    }

    if (!this._dataEnabled) {
      if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
        let oldRadioInterface = this.radioInterfaces[this._currentDataClientId];
        let newRadioInterface = this.radioInterfaces[this._dataDefaultClientId];
        oldRadioInterface.setDataRegistration(false);
        newRadioInterface.setDataRegistration(true);
      }
      this._currentDataClientId = this._dataDefaultClientId;
      return;
    }

    let oldRadioInterface = this.radioInterfaces[this._currentDataClientId];
    oldRadioInterface.dataCallSettings.oldEnabled =
      oldRadioInterface.dataCallSettings.enabled;
    oldRadioInterface.dataCallSettings.enabled = false;

    if (oldRadioInterface.anyDataConnected()) {
      this._pendingDataCallRequest = function() {
        if (DEBUG) debug("Executing pending data call request.");
        let newRadioInterface = this.radioInterfaces[this._dataDefaultClientId];
        if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
          newRadioInterface.setDataRegistration(true);
        }
        newRadioInterface.dataCallSettings.oldEnabled =
          newRadioInterface.dataCallSettings.enabled;
        newRadioInterface.dataCallSettings.enabled = this._dataEnabled;

        this._currentDataClientId = this._dataDefaultClientId;
        newRadioInterface.updateRILNetworkInterface();
      };

      if (DEBUG) {
        debug("handleDataClientIdChange: existing data call(s) active,"
          + " wait for them to get disconnected.");
      }
      oldRadioInterface.deactivateDataCalls();
      return;
    }

    let newRadioInterface = this.radioInterfaces[this._dataDefaultClientId];
    newRadioInterface.dataCallSettings.oldEnabled =
      newRadioInterface.dataCallSettings.enabled;
    newRadioInterface.dataCallSettings.enabled = true;

    this._currentDataClientId = this._dataDefaultClientId;
    if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
      oldRadioInterface.setDataRegistration(false);
      newRadioInterface.setDataRegistration(true);
    }
    newRadioInterface.updateRILNetworkInterface();
  },

  



  getRadioInterface: function(clientId) {
    return this.radioInterfaces[clientId];
  },

  getClientIdByIccId: function(iccId) {
    if (!iccId) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    for (let clientId = 0; clientId < this.numRadioInterfaces; clientId++) {
      let radioInterface = this.radioInterfaces[clientId];
      if (radioInterface.rilContext.iccInfo.iccid == iccId) {
        return clientId;
      }
    }

    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  setMicrophoneMuted: function(muted) {
    for (let clientId = 0; clientId < this.numRadioInterfaces; clientId++) {
      let radioInterface = this.radioInterfaces[clientId];
      radioInterface.workerMessenger.send("setMute", { muted: muted });
    }
  }
};

XPCOMUtils.defineLazyGetter(RadioInterfaceLayer.prototype,
                            "numRadioInterfaces", function() {
  try {
    return Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);
  } catch(e) {}

  return 1;
});

function WorkerMessenger(radioInterface, options) {
  
  this.radioInterface = radioInterface;
  this.tokenCallbackMap = {};

  
  this.debug = radioInterface.debug.bind(radioInterface);

  if (DEBUG) this.debug("Starting RIL Worker[" + options.clientId + "]");
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  this.send("setInitialOptions", options);

  gSystemWorkerManager.registerRilWorker(options.clientId, this.worker);
}
WorkerMessenger.prototype = {
  radioInterface: null,
  worker: null,

  
  token: 1,

  
  tokenCallbackMap: null,

  onerror: function(event) {
    if (DEBUG) {
      this.debug("Got an error: " + event.filename + ":" +
                 event.lineno + ": " + event.message + "\n");
    }
    event.preventDefault();
  },

  


  onmessage: function(event) {
    let message = event.data;
    if (DEBUG) {
      this.debug("Received message from worker: " + JSON.stringify(message));
    }

    let token = message.rilMessageToken;
    if (token == null) {
      
      this.radioInterface.handleUnsolicitedWorkerMessage(message);
      return;
    }

    let callback = this.tokenCallbackMap[message.rilMessageToken];
    if (!callback) {
      if (DEBUG) this.debug("Ignore orphan token: " + message.rilMessageToken);
      return;
    }

    let keep = false;
    try {
      keep = callback(message);
    } catch(e) {
      if (DEBUG) this.debug("callback throws an exception: " + e);
    }

    if (!keep) {
      delete this.tokenCallbackMap[message.rilMessageToken];
    }
  },

  














  send: function(rilMessageType, message, callback) {
    message = message || {};

    message.rilMessageToken = this.token;
    this.token++;

    if (callback) {
      
      
      
      
      
      
      
      this.tokenCallbackMap[message.rilMessageToken] = callback;
    }

    message.rilMessageType = rilMessageType;
    this.worker.postMessage(message);
  },

  











  sendWithIPCMessage: function(msg, rilMessageType, ipcType) {
    this.send(rilMessageType, msg.json.data, (function(reply) {
      ipcType = ipcType || msg.name;
      msg.target.sendAsyncMessage(ipcType, {
        clientId: this.radioInterface.clientId,
        data: reply
      });
      return false;
    }).bind(this));
  }
};

function RadioInterface(options) {
  this.clientId = options.clientId;
  this.workerMessenger = new WorkerMessenger(this, options);

  this.dataCallSettings = {
    oldEnabled: false,
    enabled: false,
    roamingEnabled: false
  };

  
  
  
  
  
  this.apnSettings = {
    byType: {},
    byApn: {}
  };

  this.supportedNetworkTypes = this.getSupportedNetworkTypes();

  this.rilContext = {
    radioState:     RIL.GECKO_RADIOSTATE_UNAVAILABLE,
    detailedRadioState: null,
    cardState:      RIL.GECKO_CARDSTATE_UNKNOWN,
    networkSelectionMode: RIL.GECKO_NETWORK_SELECTION_UNKNOWN,
    iccInfo:        null,
    imsi:           null,

    
    
    
    
    voice:          {connected: false,
                     emergencyCallsOnly: false,
                     roaming: false,
                     network: null,
                     cell: null,
                     type: null,
                     signalStrength: null,
                     relSignalStrength: null},
    data:           {connected: false,
                     emergencyCallsOnly: false,
                     roaming: false,
                     network: null,
                     cell: null,
                     type: null,
                     signalStrength: null,
                     relSignalStrength: null},
  };

  this.voicemailInfo = {
    number: null,
    displayName: null
  };

  this.operatorInfo = {};

  let lock = gSettingsService.createLock();

  
  lock.get("ril.radio.preferredNetworkType", this);

  
  lock.get("ril.data.roaming_enabled", this);
  lock.get("ril.data.apnSettings", this);

  
  
  lock.get(kSettingsClockAutoUpdateEnabled, this);

  
  
  lock.get(kSettingsTimezoneAutoUpdateEnabled, this);

  
  this.setClockAutoUpdateAvailable(false);

  
  this.setTimezoneAutoUpdateAvailable(false);

  
  
  lock.get(kSettingsCellBroadcastSearchList, this);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic, false);
  Services.prefs.addObserver(kPrefCellBroadcastDisabled, this, false);

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] = this.handleSmsWdpPortPush.bind(this);

  this._sntp = new Sntp(this.setClockBySntp.bind(this),
                        Services.prefs.getIntPref("network.sntp.maxRetryCount"),
                        Services.prefs.getIntPref("network.sntp.refreshPeriod"),
                        Services.prefs.getIntPref("network.sntp.timeout"),
                        Services.prefs.getCharPref("network.sntp.pools").split(";"),
                        Services.prefs.getIntPref("network.sntp.port"));
}

RadioInterface.prototype = {

  classID:   RADIOINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACE_CID,
                                    classDescription: "RadioInterface",
                                    interfaces: [Ci.nsIRadioInterface]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioInterface,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  
  workerMessenger: null,

  debug: function(s) {
    dump("-*- RadioInterface[" + this.clientId + "]: " + s + "\n");
  },

  



  updateInfo: function(srcInfo, destInfo) {
    for (let key in srcInfo) {
      if (key === "rilMessageType") {
        continue;
      }
      destInfo[key] = srcInfo[key];
    }
  },

  



  isInfoChanged: function(srcInfo, destInfo) {
    if (!destInfo) {
      return true;
    }

    for (let key in srcInfo) {
      if (key === "rilMessageType") {
        continue;
      }
      if (srcInfo[key] !== destInfo[key]) {
        return true;
      }
    }

    return false;
  },

  


  getSupportedNetworkTypes: function() {
    let key = "ro.moz.ril." + this.clientId + ".network_types";
    let supportedNetworkTypes = libcutils.property_get(key, "").split(",");
    for (let type of supportedNetworkTypes) {
      
      
      if (RIL.GECKO_SUPPORTED_NETWORK_TYPES.indexOf(type) < 0) {
        if (DEBUG) this.debug("Unknown network type: " + type);
        supportedNetworkTypes =
          RIL.GECKO_SUPPORTED_NETWORK_TYPES_DEFAULT.split(",");
        break;
      }
    }
    if (DEBUG) this.debug("Supported Network Types: " + supportedNetworkTypes);
    return supportedNetworkTypes;
  },

  


  receiveMessage: function(msg) {
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
      case "RIL:GetLastKnownNetwork":
        
        return this._lastKnownNetwork;
      case "RIL:GetLastKnownHomeNetwork":
        
        return this._lastKnownHomeNetwork;
      case "RIL:GetAvailableNetworks":
        this.workerMessenger.sendWithIPCMessage(msg, "getAvailableNetworks");
        break;
      case "RIL:SelectNetwork":
        this.workerMessenger.sendWithIPCMessage(msg, "selectNetwork");
        break;
      case "RIL:SelectNetworkAuto":
        this.workerMessenger.sendWithIPCMessage(msg, "selectNetworkAuto");
        break;
      case "RIL:SetPreferredNetworkType":
        this.setPreferredNetworkType(msg.target, msg.json.data);
        break;
      case "RIL:GetPreferredNetworkType":
        this.getPreferredNetworkType(msg.target, msg.json.data);
        break;
      case "RIL:GetCardLockState":
        this.workerMessenger.sendWithIPCMessage(msg, "iccGetCardLockState",
                                                "RIL:CardLockResult");
        break;
      case "RIL:UnlockCardLock":
        this.workerMessenger.sendWithIPCMessage(msg, "iccUnlockCardLock",
                                                "RIL:CardLockResult");
        break;
      case "RIL:SetCardLock":
        this.workerMessenger.sendWithIPCMessage(msg, "iccSetCardLock",
                                                "RIL:CardLockResult");
        break;
      case "RIL:GetCardLockRetryCount":
        this.workerMessenger.sendWithIPCMessage(msg, "iccGetCardLockRetryCount",
                                                "RIL:CardLockRetryCount");
        break;
      case "RIL:SendMMI":
        this.sendMMI(msg.target, msg.json.data);
        break;
      case "RIL:CancelMMI":
        this.workerMessenger.sendWithIPCMessage(msg, "cancelUSSD");
        break;
      case "RIL:SendStkResponse":
        this.workerMessenger.send("sendStkTerminalResponse", msg.json.data);
        break;
      case "RIL:SendStkMenuSelection":
        this.workerMessenger.send("sendStkMenuSelection", msg.json.data);
        break;
      case "RIL:SendStkTimerExpiration":
        this.workerMessenger.send("sendStkTimerExpiration", msg.json.data);
        break;
      case "RIL:SendStkEventDownload":
        this.workerMessenger.send("sendStkEventDownload", msg.json.data);
        break;
      case "RIL:IccOpenChannel":
        this.workerMessenger.sendWithIPCMessage(msg, "iccOpenChannel");
        break;
      case "RIL:IccCloseChannel":
        this.workerMessenger.sendWithIPCMessage(msg, "iccCloseChannel");
        break;
      case "RIL:IccExchangeAPDU":
        this.workerMessenger.sendWithIPCMessage(msg, "iccExchangeAPDU");
        break;
      case "RIL:ReadIccContacts":
        this.workerMessenger.sendWithIPCMessage(msg, "readICCContacts");
        break;
      case "RIL:UpdateIccContact":
        this.workerMessenger.sendWithIPCMessage(msg, "updateICCContact");
        break;
      case "RIL:MatchMvno":
        this.matchMvno(msg.target, msg.json.data);
        break;
      case "RIL:SetCallForwardingOptions":
        this.setCallForwardingOptions(msg.target, msg.json.data);
        break;
      case "RIL:GetCallForwardingOptions":
        this.workerMessenger.sendWithIPCMessage(msg, "queryCallForwardStatus");
        break;
      case "RIL:SetCallBarringOptions":
        this.workerMessenger.sendWithIPCMessage(msg, "setCallBarring");
        break;
      case "RIL:GetCallBarringOptions":
        this.workerMessenger.sendWithIPCMessage(msg, "queryCallBarringStatus");
        break;
      case "RIL:ChangeCallBarringPassword":
        this.workerMessenger.sendWithIPCMessage(msg, "changeCallBarringPassword");
        break;
      case "RIL:SetCallWaitingOptions":
        this.workerMessenger.sendWithIPCMessage(msg, "setCallWaiting");
        break;
      case "RIL:GetCallWaitingOptions":
        this.workerMessenger.sendWithIPCMessage(msg, "queryCallWaiting");
        break;
      case "RIL:SetCallingLineIdRestriction":
        this.setCallingLineIdRestriction(msg.target, msg.json.data);
        break;
      case "RIL:GetCallingLineIdRestriction":
        this.workerMessenger.sendWithIPCMessage(msg, "getCLIR");
        break;
      case "RIL:ExitEmergencyCbMode":
        this.workerMessenger.sendWithIPCMessage(msg, "exitEmergencyCbMode");
        break;
      case "RIL:SetRadioEnabled":
        this.setRadioEnabled(msg.target, msg.json.data);
        break;
      case "RIL:GetVoicemailInfo":
        
        return this.voicemailInfo;
      case "RIL:SetRoamingPreference":
        this.workerMessenger.sendWithIPCMessage(msg, "setRoamingPreference");
        break;
      case "RIL:GetRoamingPreference":
        this.workerMessenger.sendWithIPCMessage(msg, "queryRoamingPreference");
        break;
      case "RIL:SetVoicePrivacyMode":
        this.workerMessenger.sendWithIPCMessage(msg, "setVoicePrivacyMode");
        break;
      case "RIL:GetVoicePrivacyMode":
        this.workerMessenger.sendWithIPCMessage(msg, "queryVoicePrivacyMode");
        break;
      case "RIL:GetSupportedNetworkTypes":
        
        return this.supportedNetworkTypes;
    }
    return null;
  },

  handleUnsolicitedWorkerMessage: function(message) {
    switch (message.rilMessageType) {
      case "callRing":
        gTelephonyProvider.notifyCallRing();
        break;
      case "callStateChange":
        gTelephonyProvider.notifyCallStateChanged(this.clientId, message.call);
        break;
      case "callDisconnected":
        gTelephonyProvider.notifyCallDisconnected(this.clientId, message.call);
        break;
      case "conferenceCallStateChanged":
        gTelephonyProvider.notifyConferenceCallStateChanged(message.state);
        break;
      case "cdmaCallWaiting":
        gTelephonyProvider.notifyCdmaCallWaiting(this.clientId, message.number);
        break;
      case "callError":
        gTelephonyProvider.notifyCallError(this.clientId, message.callIndex,
                                           message.errorMsg);
        break;
      case "suppSvcNotification":
        gTelephonyProvider.notifySupplementaryService(this.clientId,
                                                      message.callIndex,
                                                      message.notification);
        break;
      case "conferenceError":
        gTelephonyProvider.notifyConferenceError(message.errorName,
                                                 message.errorMsg);
        break;
      case "emergencyCbModeChange":
        this.handleEmergencyCbModeChange(message);
        break;
      case "networkinfochanged":
        this.updateNetworkInfo(message);
        break;
      case "networkselectionmodechange":
        this.updateNetworkSelectionMode(message);
        break;
      case "voiceregistrationstatechange":
        this.updateVoiceConnection(message);
        break;
      case "dataregistrationstatechange":
        this.updateDataConnection(message);
        break;
      case "datacallerror":
        this.handleDataCallError(message);
        break;
      case "signalstrengthchange":
        this.handleSignalStrengthChange(message);
        break;
      case "operatorchange":
        this.handleOperatorChange(message);
        break;
      case "otastatuschange":
        this.handleOtaStatus(message);
        break;
      case "radiostatechange":
        this.handleRadioStateChange(message);
        break;
      case "cardstatechange":
        this.rilContext.cardState = message.cardState;
        gMessageManager.sendIccMessage("RIL:CardStateChanged",
                                       this.clientId, message);
        break;
      case "sms-received":
        let ackOk = this.handleSmsReceived(message);
        
        if (ackOk && message.simStatus === undefined) {
          this.workerMessenger.send("ackSMS", { result: RIL.PDU_FCS_OK });
        }
        return;
      case "broadcastsms-received":
      case "cellbroadcast-received":
        message.timestamp = Date.now();
        gMessageManager.sendCellBroadcastMessage("RIL:CellBroadcastReceived",
                                                 this.clientId, message);
        break;
      case "datacallstatechange":
        message.ip = null;
        message.netmask = null;
        message.broadcast = null;
        if (message.ipaddr) {
          message.ip = message.ipaddr.split("/")[0];
          let ip_value = netHelpers.stringToIP(message.ip);
          let prefix_len = message.ipaddr.split("/")[1];
          let mask_value = netHelpers.makeMask(prefix_len);
          message.netmask = netHelpers.ipToString(mask_value);
          message.broadcast = netHelpers.ipToString((ip_value & mask_value) + ~mask_value);
        }
        this.handleDataCallState(message);
        break;
      case "datacalllist":
        this.handleDataCallList(message);
        break;
      case "nitzTime":
        this.handleNitzTime(message);
        break;
      case "iccinfochange":
        this.handleIccInfoChange(message);
        break;
      case "iccimsi":
        this.rilContext.imsi = message.imsi;
        break;
      case "iccmbdn":
        this.handleIccMbdn(message);
        break;
      case "iccmwis":
        gMessageManager.sendVoicemailMessage("RIL:VoicemailNotification",
                                             this.clientId, message.mwi);
        break;
      case "USSDReceived":
        if (DEBUG) this.debug("USSDReceived " + JSON.stringify(message));
        this.handleUSSDReceived(message);
        break;
      case "stkcommand":
        this.handleStkProactiveCommand(message);
        break;
      case "stksessionend":
        gMessageManager.sendIccMessage("RIL:StkSessionEnd", this.clientId, null);
        break;
      case "exitEmergencyCbMode":
        this.handleExitEmergencyCbMode(message);
        break;
      case "cdma-info-rec-received":
        if (DEBUG) this.debug("cdma-info-rec-received: " + JSON.stringify(message));
        gSystemMessenger.broadcastMessage("cdma-info-rec-received", message);
        break;
      default:
        throw new Error("Don't know about this message type: " +
                        message.rilMessageType);
    }
  },

  








  getPhoneNumber: function() {
    let iccInfo = this.rilContext.iccInfo;

    if (!iccInfo) {
      return null;
    }

    
    
    
    
    let number = (iccInfo instanceof GsmIccInfo) ? iccInfo.msisdn : iccInfo.mdn;

    
    
    if (number === undefined || number === "undefined") {
      return null;
    }

    return number;
  },

  


  getIccId: function() {
    let iccInfo = this.rilContext.iccInfo;

    if (!iccInfo || !(iccInfo instanceof GsmIccInfo)) {
      return null;
    }

    let iccId = iccInfo.iccid;

    
    
    if (iccId === undefined || iccId === "undefined") {
      return null;
    }

    return iccId;
  },

  
  
  
  
  isImsiMatches: function(mvnoData) {
    let imsi = this.rilContext.imsi;

    
    if (mvnoData.length > imsi.length) {
      return false;
    }

    for (let i = 0; i < mvnoData.length; i++) {
      let c = mvnoData[i];
      if ((c !== 'x') && (c !== 'X') && (c !== imsi[i])) {
        return false;
      }
    }
    return true;
  },

  matchMvno: function(target, message) {
    if (DEBUG) this.debug("matchMvno: " + JSON.stringify(message));

    if (!message || !message.mvnoType || !message.mvnoData) {
      message.errorMsg = RIL.GECKO_ERROR_INVALID_PARAMETER;
    }
    
    if (message.mvnoType != "imsi") {
      message.errorMsg = RIL.GECKO_ERROR_MODE_NOT_SUPPORTED;
    }
    
    if (!this.rilContext.imsi) {
      message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
    }

    if (!message.errorMsg) {
      message.result = this.isImsiMatches(message.mvnoData);
    }

    target.sendAsyncMessage("RIL:MatchMvno", {
      clientId: this.clientId,
      data: message
    });
  },

  updateNetworkInfo: function(message) {
    let voiceMessage = message[RIL.NETWORK_INFO_VOICE_REGISTRATION_STATE];
    let dataMessage = message[RIL.NETWORK_INFO_DATA_REGISTRATION_STATE];
    let operatorMessage = message[RIL.NETWORK_INFO_OPERATOR];
    let selectionMessage = message[RIL.NETWORK_INFO_NETWORK_SELECTION_MODE];
    let signalMessage = message[RIL.NETWORK_INFO_SIGNAL];

    
    if (voiceMessage) {
      this.updateVoiceConnection(voiceMessage, true);
    }

    if (dataMessage) {
      this.updateDataConnection(dataMessage, true);
    }

    if (operatorMessage) {
      this.handleOperatorChange(operatorMessage, true);
    }

    if (signalMessage) {
      this.handleSignalStrengthChange(signalMessage, true);
    }

    let voice = this.rilContext.voice;
    let data = this.rilContext.data;

    this.checkRoamingBetweenOperators(voice);
    this.checkRoamingBetweenOperators(data);

    if (voiceMessage || operatorMessage || signalMessage) {
      gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                  this.clientId, voice);
    }
    if (dataMessage || operatorMessage || signalMessage) {
      gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                  this.clientId, data);
    }

    if (selectionMessage) {
      this.updateNetworkSelectionMode(selectionMessage);
    }
  },

  






  checkRoamingBetweenOperators: function(registration) {
    let iccInfo = this.rilContext.iccInfo;
    let operator = registration.network;
    let state = registration.state;

    if (!iccInfo || !operator ||
        state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
      return;
    }

    let spn = iccInfo.spn && iccInfo.spn.toLowerCase();
    let longName = operator.longName && operator.longName.toLowerCase();
    let shortName = operator.shortName && operator.shortName.toLowerCase();

    let equalsLongName = longName && (spn == longName);
    let equalsShortName = shortName && (spn == shortName);
    let equalsMcc = iccInfo.mcc == operator.mcc;

    registration.roaming = registration.roaming &&
                           !(equalsMcc && (equalsLongName || equalsShortName));
  },

  






  updateVoiceConnection: function(newInfo, batch) {
    let voiceInfo = this.rilContext.voice;
    voiceInfo.state = newInfo.state;
    voiceInfo.connected = newInfo.connected;
    voiceInfo.roaming = newInfo.roaming;
    voiceInfo.emergencyCallsOnly = newInfo.emergencyCallsOnly;
    voiceInfo.type = newInfo.type;

    
    
    if (newInfo.state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
      voiceInfo.cell = null;
      voiceInfo.network = null;
      voiceInfo.signalStrength = null;
      voiceInfo.relSignalStrength = null;
    } else {
      voiceInfo.cell = newInfo.cell;
      voiceInfo.network = this.operatorInfo;
    }

    if (!batch) {
      gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                  this.clientId, voiceInfo);
    }
  },

  






  updateDataConnection: function(newInfo, batch) {
    let dataInfo = this.rilContext.data;
    dataInfo.state = newInfo.state;
    dataInfo.roaming = newInfo.roaming;
    dataInfo.emergencyCallsOnly = newInfo.emergencyCallsOnly;
    dataInfo.type = newInfo.type;
    
    
    let apnSetting = this.apnSettings.byType.default;
    dataInfo.connected = false;
    if (apnSetting) {
      dataInfo.connected = (this.getDataCallStateByType("default") ==
                            RIL.GECKO_NETWORK_STATE_CONNECTED);
    }

    
    
    if (newInfo.state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
      dataInfo.cell = null;
      dataInfo.network = null;
      dataInfo.signalStrength = null;
      dataInfo.relSignalStrength = null;
    } else {
      dataInfo.cell = newInfo.cell;
      dataInfo.network = this.operatorInfo;
    }

    if (!batch) {
      gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                  this.clientId, dataInfo);
    }
    this.updateRILNetworkInterface();
  },

  


  handleDataCallError: function(message) {
    
    if (this.apnSettings.byType.default) {
      let apnSetting = this.apnSettings.byType.default;
      if (message.apn == apnSetting.apn &&
          apnSetting.iface.inConnectedTypes("default")) {
        gMessageManager.sendMobileConnectionMessage("RIL:DataError",
                                                    this.clientId, message);
      }
    }

    this._deliverDataCallCallback("dataCallError", [message]);
  },

  _preferredNetworkType: null,
  getPreferredNetworkType: function(target, message) {
    this.workerMessenger.send("getPreferredNetworkType", message, (function(response) {
      if (response.success) {
        this._preferredNetworkType = response.networkType;
        response.type = RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType];
        if (DEBUG) {
          this.debug("_preferredNetworkType is now " +
                     RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType]);
        }
      }

      target.sendAsyncMessage("RIL:GetPreferredNetworkType", {
        clientId: this.clientId,
        data: response
      });
      return false;
    }).bind(this));
  },

  setPreferredNetworkType: function(target, message) {
    if (DEBUG) this.debug("setPreferredNetworkType: " + JSON.stringify(message));
    let networkType = RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO.indexOf(message.type);
    if (networkType < 0) {
      message.errorMsg = RIL.GECKO_ERROR_INVALID_PARAMETER;
      target.sendAsyncMessage("RIL:SetPreferredNetworkType", {
        clientId: this.clientId,
        data: message
      });
      return false;
    }
    message.networkType = networkType;

    this.workerMessenger.send("setPreferredNetworkType", message, (function(response) {
      if (response.success) {
        this._preferredNetworkType = response.networkType;
        if (DEBUG) {
          this.debug("_preferredNetworkType is now " +
                      RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType]);
        }
      }

      target.sendAsyncMessage("RIL:SetPreferredNetworkType", {
        clientId: this.clientId,
        data: response
      });
      return false;
    }).bind(this));
  },

  
  
  setPreferredNetworkTypeBySetting: function(value) {
    let networkType = RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO.indexOf(value);
    if (networkType < 0) {
      networkType = (this._preferredNetworkType != null)
                    ? RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType]
                    : RIL.GECKO_PREFERRED_NETWORK_TYPE_DEFAULT;
      gSettingsService.createLock().set("ril.radio.preferredNetworkType",
                                        networkType, null);
      return;
    }

    if (networkType == this._preferredNetworkType) {
      return;
    }

    this.workerMessenger.send("setPreferredNetworkType",
                              { networkType: networkType },
                              (function(response) {
      if ((this._preferredNetworkType != null) && !response.success) {
        gSettingsService.createLock().set("ril.radio.preferredNetworkType",
                                          this._preferredNetworkType,
                                          null);
        return false;
      }

      this._preferredNetworkType = response.networkType;
      if (DEBUG) {
        this.debug("_preferredNetworkType is now " +
                   RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType]);
      }

      return false;
    }).bind(this));
  },

  setCellBroadcastSearchList: function(newSearchListStr) {
    if (newSearchListStr == this._cellBroadcastSearchListStr) {
      return;
    }

    this.workerMessenger.send("setCellBroadcastSearchList",
                              { searchListStr: newSearchListStr },
                              (function callback(response) {
      if (!response.success) {
        let lock = gSettingsService.createLock();
        lock.set(kSettingsCellBroadcastSearchList,
                 this._cellBroadcastSearchListStr, null);
      } else {
        this._cellBroadcastSearchListStr = response.searchListStr;
      }

      return false;
    }).bind(this));
  },

  






  handleSignalStrengthChange: function(message, batch) {
    let voiceInfo = this.rilContext.voice;
    
    if (voiceInfo.state === RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED &&
        this.isInfoChanged(message.voice, voiceInfo)) {
      this.updateInfo(message.voice, voiceInfo);
      if (!batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                    this.clientId, voiceInfo);
      }
    }

    let dataInfo = this.rilContext.data;
    
    if (dataInfo.state === RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED &&
        this.isInfoChanged(message.data, dataInfo)) {
      this.updateInfo(message.data, dataInfo);
      if (!batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, dataInfo);
      }
    }
  },

  






  handleOperatorChange: function(message, batch) {
    let operatorInfo = this.operatorInfo;
    let voice = this.rilContext.voice;
    let data = this.rilContext.data;

    if (this.isInfoChanged(message, operatorInfo)) {
      this.updateInfo(message, operatorInfo);

      
      if (message.mcc && message.mnc) {
        this._lastKnownNetwork = message.mcc + "-" + message.mnc;
      }

      
      if (voice.network && !batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                    this.clientId, voice);
      }

      
      if (data.network && !batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, data);
      }
    }
  },

  handleOtaStatus: function(message) {
    if (message.status < 0 ||
        RIL.CDMA_OTA_PROVISION_STATUS_TO_GECKO.length <= message.status) {
      return;
    }

    let status = RIL.CDMA_OTA_PROVISION_STATUS_TO_GECKO[message.status];

    gMessageManager.sendMobileConnectionMessage("RIL:OtaStatusChanged",
                                                this.clientId, status);
  },

  _convertRadioState: function(state) {
    switch (state) {
      case RIL.GECKO_RADIOSTATE_OFF:
        return RIL.GECKO_DETAILED_RADIOSTATE_DISABLED;
      case RIL.GECKO_RADIOSTATE_READY:
        return RIL.GECKO_DETAILED_RADIOSTATE_ENABLED;
      default:
        return RIL.GECKO_DETAILED_RADIOSTATE_UNKNOWN;
    }
  },

  handleRadioStateChange: function(message) {
    let newState = message.radioState;
    if (this.rilContext.radioState == newState) {
      return;
    }
    this.rilContext.radioState = newState;
    this.handleDetailedRadioStateChanged(this._convertRadioState(newState));

    
  },

  handleDetailedRadioStateChanged: function(state) {
    if (this.rilContext.detailedRadioState == state) {
      return;
    }
    this.rilContext.detailedRadioState = state;
    gMessageManager.sendMobileConnectionMessage("RIL:RadioStateChanged",
                                                this.clientId, state);
  },

  deactivateDataCalls: function() {
    let dataDisconnecting = false;
    for each (let apnSetting in this.apnSettings.byApn) {
      for each (let type in apnSetting.types) {
        if (this.getDataCallStateByType(type) ==
            RIL.GECKO_NETWORK_STATE_CONNECTED) {
          this.deactivateDataCallByType(type);
          dataDisconnecting = true;
        }
      }
    }

    
    
    if (gRadioEnabledController.isDeactivatingDataCalls() && !dataDisconnecting) {
      gRadioEnabledController.finishDeactivatingDataCalls(this.clientId);
    }
  },

  updateApnSettings: function(allApnSettings) {
    let simApnSettings = allApnSettings[this.clientId];
    if (!simApnSettings) {
      return;
    }
    if (this._pendingApnSettings) {
      
      this._pengingApnSettings = simApnSettings;
      return;
    }

    let isDeactivatingDataCalls = false;
    
    for each (let apnSetting in this.apnSettings.byApn) {
      for each (let type in apnSetting.types) {
        if (this.getDataCallStateByType(type) ==
            RIL.GECKO_NETWORK_STATE_CONNECTED) {
          this.deactivateDataCallByType(type);
          isDeactivatingDataCalls = true;
        }
      }
    }
    if (isDeactivatingDataCalls) {
      
      this._pendingApnSettings = simApnSettings;
      return;
    }
    this.setupApnSettings(simApnSettings);
  },

  








  setupApnSettings: function(simApnSettings) {
    if (!simApnSettings) {
      return;
    }
    if (DEBUG) this.debug("setupApnSettings: " + JSON.stringify(simApnSettings));

    
    for each (let apnSetting in this.apnSettings.byApn) {
      if (apnSetting.iface.name in gNetworkManager.networkInterfaces) {
        gNetworkManager.unregisterNetworkInterface(apnSetting.iface);
      }
      this.unregisterDataCallCallback(apnSetting.iface);
      delete apnSetting.iface;
    }
    this.apnSettings.byApn = {};
    this.apnSettings.byType = {};

    
    for (let i = 0; simApnSettings[i]; i++) {
      let inputApnSetting = simApnSettings[i];
      if (!this.validateApnSetting(inputApnSetting)) {
        continue;
      }

      
      
      let apnKey = inputApnSetting.apn +
                   (inputApnSetting.user || "") +
                   (inputApnSetting.password || "");

      if (!this.apnSettings.byApn[apnKey]) {
        this.apnSettings.byApn[apnKey] = inputApnSetting;
      } else {
        this.apnSettings.byApn[apnKey].types =
          this.apnSettings.byApn[apnKey].types.concat(inputApnSetting.types);
      }

      
      
      for each (let type in inputApnSetting.types) {
        this.apnSettings.byType[type] = this.apnSettings.byApn[apnKey];
      }
    }

    
    for each (let apnSetting in this.apnSettings.byApn) {
      apnSetting.iface = new RILNetworkInterface(this, apnSetting);
    }
  },

  allDataDisconnected: function() {
    for each (let apnSetting in this.apnSettings.byApn) {
      let iface = apnSetting.iface;
      if (iface && iface.state != RIL.GECKO_NETWORK_STATE_UNKNOWN &&
          iface.state != RIL.GECKO_NETWORK_STATE_DISCONNECTED) {
        return false;
      }
    }
    return true;
  },

  anyDataConnected: function() {
    for each (let apnSetting in this.apnSettings.byApn) {
      let iface = apnSetting.iface;
      if (iface && iface.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
        return true;
      }
    }
    return false;
  },

  


  validateApnSetting: function(apnSetting) {
    return (apnSetting &&
            apnSetting.apn &&
            apnSetting.types &&
            apnSetting.types.length);
  },

  setDataRegistration: function(attach) {
    this.workerMessenger.send("setDataRegistration", {attach: attach});
  },

  updateRILNetworkInterface: function() {
    let apnSetting = this.apnSettings.byType.default;
    if (!this.validateApnSetting(apnSetting)) {
      if (DEBUG) {
        this.debug("We haven't gotten completely the APN data.");
      }
      return;
    }

    
    
    if (this.rilContext.radioState != RIL.GECKO_RADIOSTATE_READY) {
      if (DEBUG) {
        this.debug("RIL is not ready for data connection: radio's not ready");
      }
      return;
    }

    
    
    
    
    
    if (this.dataCallSettings.oldEnabled == this.dataCallSettings.enabled) {
      if (DEBUG) {
        this.debug("No changes for ril.data.enabled flag. Nothing to do.");
      }
      return;
    }

    let defaultDataCallState = this.getDataCallStateByType("default");
    if (defaultDataCallState == RIL.GECKO_NETWORK_STATE_CONNECTING ||
        defaultDataCallState == RIL.GECKO_NETWORK_STATE_DISCONNECTING) {
      if (DEBUG) {
        this.debug("Nothing to do during connecting/disconnecting in progress.");
      }
      return;
    }

    let dataInfo = this.rilContext.data;
    let isRegistered =
      dataInfo.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;
    let haveDataConnection =
      dataInfo.type != RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN;
    if (!isRegistered || !haveDataConnection) {
      if (DEBUG) {
        this.debug("RIL is not ready for data connection: Phone's not " +
                   "registered or doesn't have data connection.");
      }
      return;
    }
    let wifi_active = false;
    if (gNetworkManager.active &&
        gNetworkManager.active.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
      wifi_active = true;
    }

    let defaultDataCallConnected = defaultDataCallState ==
                                   RIL.GECKO_NETWORK_STATE_CONNECTED;
    if (defaultDataCallConnected &&
        (!this.dataCallSettings.enabled ||
         (dataInfo.roaming && !this.dataCallSettings.roamingEnabled))) {
      if (DEBUG) this.debug("Data call settings: disconnect data call.");
      this.deactivateDataCallByType("default");
      return;
    }

    if (defaultDataCallConnected && wifi_active) {
      if (DEBUG) this.debug("Disconnect data call when Wifi is connected.");
      this.deactivateDataCallByType("default");
      return;
    }

    if (!this.dataCallSettings.enabled || defaultDataCallConnected) {
      if (DEBUG) this.debug("Data call settings: nothing to do.");
      return;
    }
    if (dataInfo.roaming && !this.dataCallSettings.roamingEnabled) {
      if (DEBUG) this.debug("We're roaming, but data roaming is disabled.");
      return;
    }
    if (wifi_active) {
      if (DEBUG) this.debug("Don't connect data call when Wifi is connected.");
      return;
    }
    if (this._pendingApnSettings) {
      if (DEBUG) this.debug("We're changing apn settings, ignore any changes.");
      return;
    }

    let detailedRadioState = this.rilContext.detailedRadioState;
    if (gRadioEnabledController.isDeactivatingDataCalls() ||
        detailedRadioState == RIL.GECKO_DETAILED_RADIOSTATE_ENABLING ||
        detailedRadioState == RIL.GECKO_DETAILED_RADIOSTATE_DISABLING) {
      
      return;
    }

    if (DEBUG) this.debug("Data call settings: connect data call.");
    this.setupDataCallByType("default");
  },

  


  updateNetworkSelectionMode: function(message) {
    if (DEBUG) this.debug("updateNetworkSelectionMode: " + JSON.stringify(message));
    this.rilContext.networkSelectionMode = message.mode;
    gMessageManager.sendMobileConnectionMessage("RIL:NetworkSelectionModeChanged",
                                                this.clientId, message);
  },

  


  handleEmergencyCbModeChange: function(message) {
    if (DEBUG) this.debug("handleEmergencyCbModeChange: " + JSON.stringify(message));
    gMessageManager.sendMobileConnectionMessage("RIL:EmergencyCbModeChanged",
                                                this.clientId, message);
  },

  






  handleSmsWdpPortPush: function(message) {
    if (message.encoding != RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      if (DEBUG) {
        this.debug("Got port addressed SMS but not encoded in 8-bit alphabet." +
                   " Drop!");
      }
      return;
    }

    let options = {
      bearer: WAP.WDP_BEARER_GSM_SMS_GSM_MSISDN,
      sourceAddress: message.sender,
      sourcePort: message.header.originatorPort,
      destinationAddress: this.rilContext.iccInfo.msisdn,
      destinationPort: message.header.destinationPort,
      serviceId: this.clientId
    };
    WAP.WapPushManager.receiveWdpPDU(message.fullData, message.fullData.length,
                                     0, options);
  },

  








  broadcastSmsSystemMessage: function(aName, aDomMessage) {
    if (DEBUG) this.debug("Broadcasting the SMS system message: " + aName);

    
    
    
    gSystemMessenger.broadcastMessage(aName, {
      type:              aDomMessage.type,
      id:                aDomMessage.id,
      threadId:          aDomMessage.threadId,
      delivery:          aDomMessage.delivery,
      deliveryStatus:    aDomMessage.deliveryStatus,
      sender:            aDomMessage.sender,
      receiver:          aDomMessage.receiver,
      body:              aDomMessage.body,
      messageClass:      aDomMessage.messageClass,
      timestamp:         aDomMessage.timestamp,
      sentTimestamp:     aDomMessage.sentTimestamp,
      deliveryTimestamp: aDomMessage.deliveryTimestamp,
      read:              aDomMessage.read
    });
  },

  
  
  
  _smsHandledWakeLock: null,
  _smsHandledWakeLockTimer: null,

  _releaseSmsHandledWakeLock: function() {
    if (DEBUG) this.debug("Releasing the CPU wake lock for handling SMS.");
    if (this._smsHandledWakeLockTimer) {
      this._smsHandledWakeLockTimer.cancel();
    }
    if (this._smsHandledWakeLock) {
      this._smsHandledWakeLock.unlock();
      this._smsHandledWakeLock = null;
    }
  },

  portAddressedSmsApps: null,
  handleSmsReceived: function(message) {
    if (DEBUG) this.debug("handleSmsReceived: " + JSON.stringify(message));

    
    
    if (!this._smsHandledWakeLock) {
      if (DEBUG) this.debug("Acquiring a CPU wake lock for handling SMS.");
      this._smsHandledWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._smsHandledWakeLockTimer) {
      if (DEBUG) this.debug("Creating a timer for releasing the CPU wake lock.");
      this._smsHandledWakeLockTimer =
        Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    if (DEBUG) this.debug("Setting the timer for releasing the CPU wake lock.");
    this._smsHandledWakeLockTimer
        .initWithCallback(this._releaseSmsHandledWakeLock.bind(this),
                          SMS_HANDLED_WAKELOCK_TIMEOUT,
                          Ci.nsITimer.TYPE_ONE_SHOT);

    
    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      message.fullData = new Uint8Array(message.fullData);
    }

    
    
    
    if (message.header && message.header.destinationPort != null) {
      let handler = this.portAddressedSmsApps[message.header.destinationPort];
      if (handler) {
        handler(message);
      }
      return true;
    }

    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      return true;
    }

    message.type = "sms";
    message.sender = message.sender || null;
    message.receiver = this.getPhoneNumber();
    message.body = message.fullBody = message.fullBody || null;
    message.timestamp = Date.now();
    message.iccId = this.getIccId();

    if (gSmsService.isSilentNumber(message.sender)) {
      message.id = -1;
      message.threadId = 0;
      message.delivery = DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED;
      message.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      message.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(message.id,
                                               message.threadId,
                                               message.iccId,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.sentTimestamp,
                                               0,
                                               message.read);

      Services.obs.notifyObservers(domMessage,
                                   kSilentSmsReceivedObserverTopic,
                                   null);
      return true;
    }

    let mwi = message.mwi;
    if (mwi) {
      mwi.returnNumber = message.sender;
      mwi.returnMessage = message.fullBody;
      gMessageManager.sendVoicemailMessage("RIL:VoicemailNotification",
                                           this.clientId, mwi);

      
      
      if (mwi.discard) {
        return true;
      }
    }

    let notifyReceived = function notifyReceived(rv, domMessage) {
      let success = Components.isSuccessCode(rv);

      
      
      if (message.simStatus === undefined) {
        this.workerMessenger.send("ackSMS", {
          result: (success ? RIL.PDU_FCS_OK
                           : RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED)
        });
      }

      if (!success) {
        
        
        if (DEBUG) {
          this.debug("Could not store SMS, error code " + rv);
        }
        return;
      }

      this.broadcastSmsSystemMessage(kSmsReceivedObserverTopic, domMessage);
      Services.obs.notifyObservers(domMessage, kSmsReceivedObserverTopic, null);
    }.bind(this);

    if (message.messageClass != RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0]) {
      gMobileMessageDatabaseService.saveReceivedMessage(message,
                                                        notifyReceived);
    } else {
      message.id = -1;
      message.threadId = 0;
      message.delivery = DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED;
      message.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      message.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(message.id,
                                               message.threadId,
                                               message.iccId,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.sentTimestamp,
                                               0,
                                               message.read);

      notifyReceived(Cr.NS_OK, domMessage);
    }

    
    return false;
  },

  


  handleDataCallState: function(datacall) {
    let data = this.rilContext.data;
    let apnSetting = this.apnSettings.byType.default;
    let dataCallConnected =
        (datacall.state == RIL.GECKO_NETWORK_STATE_CONNECTED);
    if (apnSetting && datacall.ifname) {
      if (dataCallConnected && datacall.apn == apnSetting.apn &&
          apnSetting.iface.inConnectedTypes("default")) {
        data.connected = dataCallConnected;
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                     this.clientId, data);
        data.apn = datacall.apn;
      } else if (!dataCallConnected && datacall.apn == data.apn) {
        data.connected = dataCallConnected;
        delete data.apn;
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                     this.clientId, data);
      }
    }

    this._deliverDataCallCallback("dataCallStateChanged",
                                  [datacall]);

    
    
    if (datacall.state == RIL.GECKO_NETWORK_STATE_UNKNOWN &&
        this.allDataDisconnected()) {
      if (gRadioEnabledController.isDeactivatingDataCalls()) {
        if (DEBUG) this.debug("All data connections are disconnected.");
        gRadioEnabledController.finishDeactivatingDataCalls(this.clientId);
      }

      if (this._pendingApnSettings) {
        if (DEBUG) this.debug("Setup pending apn settings.");
        this.setupApnSettings(this._pendingApnSettings);
        this._pendingApnSettings = null;
        this.updateRILNetworkInterface();
      }
    }
  },

  


  handleDataCallList: function(message) {
    this._deliverDataCallCallback("receiveDataCallList",
                                  [message.datacalls, message.datacalls.length]);
  },

  


  setClockAutoUpdateAvailable: function(value) {
    gSettingsService.createLock().set(kSettingsClockAutoUpdateAvailable, value, null,
                                      "fromInternalSetting");
  },

  


  setTimezoneAutoUpdateAvailable: function(value) {
    gSettingsService.createLock().set(kSettingsTimezoneAutoUpdateAvailable, value, null,
                                      "fromInternalSetting");
  },

  


  setClockByNitz: function(message) {
    
    
    gTimeService.set(
      message.networkTimeInMS + (Date.now() - message.receiveTimeInMS));
  },

  


  setTimezoneByNitz: function(message) {
    
    
    
    
    
    
    if (message.networkTimeZoneInMinutes != (new Date()).getTimezoneOffset()) {
      let absTimeZoneInMinutes = Math.abs(message.networkTimeZoneInMinutes);
      let timeZoneStr = "UTC";
      timeZoneStr += (message.networkTimeZoneInMinutes > 0 ? "-" : "+");
      timeZoneStr += ("0" + Math.floor(absTimeZoneInMinutes / 60)).slice(-2);
      timeZoneStr += ":";
      timeZoneStr += ("0" + absTimeZoneInMinutes % 60).slice(-2);
      gSettingsService.createLock().set("time.timezone", timeZoneStr, null);
    }
  },

  


  handleNitzTime: function(message) {
    
    this.setClockAutoUpdateAvailable(true);
    this.setTimezoneAutoUpdateAvailable(true);

    
    this._lastNitzMessage = message;

    
    if (this._clockAutoUpdateEnabled) {
      this.setClockByNitz(message);
    }
    
    if (this._timezoneAutoUpdateEnabled) {
      this.setTimezoneByNitz(message);
    }
  },

  


  setClockBySntp: function(offset) {
    
    this.setClockAutoUpdateAvailable(true);
    if (!this._clockAutoUpdateEnabled) {
      return;
    }
    if (this._lastNitzMessage) {
      if (DEBUG) debug("SNTP: NITZ available, discard SNTP");
      return;
    }
    gTimeService.set(Date.now() + offset);
  },

  handleIccMbdn: function(message) {
    let voicemailInfo = this.voicemailInfo;

    voicemailInfo.number = message.number;
    voicemailInfo.displayName = message.alphaId;

    gMessageManager.sendVoicemailMessage("RIL:VoicemailInfoChanged",
                                         this.clientId, voicemailInfo);
  },

  handleIccInfoChange: function(message) {
    let oldSpn = this.rilContext.iccInfo ? this.rilContext.iccInfo.spn : null;

    if (!message || !message.iccType) {
      
      this.rilContext.iccInfo = null;
    } else {
      if (!this.rilContext.iccInfo) {
        if (message.iccType === "ruim" || message.iccType === "csim") {
          this.rilContext.iccInfo = new CdmaIccInfo();
        } else {
          this.rilContext.iccInfo = new GsmIccInfo();
        }
      }

      if (!this.isInfoChanged(message, this.rilContext.iccInfo)) {
        return;
      }

      this.updateInfo(message, this.rilContext.iccInfo);
    }

    
    
    gMessageManager.sendIccMessage("RIL:IccInfoChanged",
                                   this.clientId,
                                   message.iccType ? message : null);

    
    if (message.mcc) {
      try {
        Services.prefs.setCharPref("ril.lastKnownSimMcc",
                                   message.mcc.toString());
      } catch (e) {}
    }

    
    if (message.mcc && message.mnc) {
      this._lastKnownHomeNetwork = message.mcc + "-" + message.mnc;
    }

    
    if (!oldSpn && message.spn) {
      let voice = this.rilContext.voice;
      let data = this.rilContext.data;
      let voiceRoaming = voice.roaming;
      let dataRoaming = data.roaming;
      this.checkRoamingBetweenOperators(voice);
      this.checkRoamingBetweenOperators(data);
      if (voiceRoaming != voice.roaming) {
        gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                    this.clientId, voice);
      }
      if (dataRoaming != data.roaming) {
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, data);
      }
    }
  },

  handleUSSDReceived: function(ussd) {
    if (DEBUG) this.debug("handleUSSDReceived " + JSON.stringify(ussd));
    gSystemMessenger.broadcastMessage("ussd-received", ussd);
    gMessageManager.sendMobileConnectionMessage("RIL:USSDReceived",
                                                this.clientId, ussd);
  },

  handleStkProactiveCommand: function(message) {
    if (DEBUG) this.debug("handleStkProactiveCommand " + JSON.stringify(message));
    let iccId = this.rilContext.iccInfo && this.rilContext.iccInfo.iccid;
    if (iccId) {
      gSystemMessenger.broadcastMessage("icc-stkcommand",
                                        {iccId: iccId,
                                         command: message});
    }
    gMessageManager.sendIccMessage("RIL:StkCommand", this.clientId, message);
  },

  handleExitEmergencyCbMode: function(message) {
    if (DEBUG) this.debug("handleExitEmergencyCbMode: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:ExitEmergencyCbMode", message);
  },

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        let setting = JSON.parse(data);
        this.handleSettingsChange(setting.key, setting.value, setting.message);
        break;
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (data === kPrefCellBroadcastDisabled) {
          let value = false;
          try {
            value = Services.prefs.getBoolPref(kPrefCellBroadcastDisabled);
          } catch(e) {}
          this.workerMessenger.send("setCellBroadcastDisabled",
                                    { disabled: value });
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        
        this._releaseSmsHandledWakeLock();

        
        for each (let apnSetting in this.apnSettings.byApn) {
          if (apnSetting.iface) {
            apnSetting.iface.shutdown();
          }
        }
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        Services.obs.removeObserver(this, kSysClockChangeObserverTopic);
        Services.obs.removeObserver(this, kScreenStateChangedTopic);
        Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
        break;
      case kSysClockChangeObserverTopic:
        let offset = parseInt(data, 10);
        if (this._lastNitzMessage) {
          this._lastNitzMessage.receiveTimeInMS += offset;
        }
        this._sntp.updateOffset(offset);
        break;
      case kNetworkInterfaceStateChangedTopic:
        let network = subject.QueryInterface(Ci.nsINetworkInterface);
        if (network.state != Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
          return;
        }

        
        if (network.type != Ci.nsINetworkInterface.NETWORK_TYPE_WIFI &&
            network.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
          return;
        }

        
        if (subject instanceof Ci.nsIRilNetworkInterface) {
          network = subject.QueryInterface(Ci.nsIRilNetworkInterface);
          if (network.serviceId != this.clientId) {
            return;
          }
        }

        
        if (this._sntp.isExpired()) {
          this._sntp.request();
        }
        break;
      case kScreenStateChangedTopic:
        this.workerMessenger.send("setScreenState", { on: (data === "on") });
        break;
    }
  },

  supportedNetworkTypes: null,

  
  dataCallSettings: null,

  apnSettings: null,

  
  _pendingApnSettings: null,

  
  
  _clockAutoUpdateEnabled: null,

  
  
  _timezoneAutoUpdateEnabled: null,

  
  
  _lastNitzMessage: null,

  
  _sntp: null,

  
  _cellBroadcastSearchListStr: null,

  
  _lastKnownNetwork: null,

  
  _lastKnownHomeNetwork: null,

  handleSettingsChange: function(aName, aResult, aMessage) {
    
    
    if (aName === kSettingsClockAutoUpdateAvailable &&
        aMessage !== "fromInternalSetting") {
      let isClockAutoUpdateAvailable = this._lastNitzMessage !== null ||
                                       this._sntp.isAvailable();
      if (aResult !== isClockAutoUpdateAvailable) {
        if (DEBUG) {
          debug("Content processes cannot modify 'time.clock.automatic-update.available'. Restore!");
        }
        
        this.setClockAutoUpdateAvailable(isClockAutoUpdateAvailable);
      }
    }

    
    
    
    if (aName === kSettingsTimezoneAutoUpdateAvailable &&
        aMessage !== "fromInternalSetting") {
      let isTimezoneAutoUpdateAvailable = this._lastNitzMessage !== null;
      if (aResult !== isTimezoneAutoUpdateAvailable) {
        if (DEBUG) {
          this.debug("Content processes cannot modify 'time.timezone.automatic-update.available'. Restore!");
        }
        
        this.setTimezoneAutoUpdateAvailable(isTimezoneAutoUpdateAvailable);
      }
    }

    this.handle(aName, aResult);
  },

  
  handle: function(aName, aResult) {
    switch(aName) {
      
      
      case "ril.radio.preferredNetworkType":
        if (DEBUG) this.debug("'ril.radio.preferredNetworkType' is now " + aResult);
        this.setPreferredNetworkTypeBySetting(aResult);
        break;
      case "ril.data.roaming_enabled":
        if (DEBUG) this.debug("'ril.data.roaming_enabled' is now " + aResult);
        this.dataCallSettings.roamingEnabled = aResult;
        this.updateRILNetworkInterface();
        break;
      case "ril.data.apnSettings":
        if (DEBUG) this.debug("'ril.data.apnSettings' is now " + JSON.stringify(aResult));
        if (aResult) {
          this.updateApnSettings(aResult);
          this.updateRILNetworkInterface();
        }
        break;
      case kSettingsClockAutoUpdateEnabled:
        this._clockAutoUpdateEnabled = aResult;
        if (!this._clockAutoUpdateEnabled) {
          break;
        }

        
        if (this._lastNitzMessage) {
          this.setClockByNitz(this._lastNitzMessage);
        } else if (gNetworkManager.active && gNetworkManager.active.state ==
                 Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
          
          if (!this._sntp.isExpired()) {
            this.setClockBySntp(this._sntp.getOffset());
          } else {
            
            this._sntp.request();
          }
        }
        break;
      case kSettingsTimezoneAutoUpdateEnabled:
        this._timezoneAutoUpdateEnabled = aResult;

        if (this._timezoneAutoUpdateEnabled) {
          
          if (this._timezoneAutoUpdateEnabled && this._lastNitzMessage) {
            this.setTimezoneByNitz(this._lastNitzMessage);
          }
        }
        break;
      case kSettingsCellBroadcastSearchList:
        if (DEBUG) {
          this.debug("'" + kSettingsCellBroadcastSearchList + "' is now " + aResult);
        }
        this.setCellBroadcastSearchList(aResult);
        break;
    }
  },

  handleError: function(aErrorMessage) {
    if (DEBUG) this.debug("There was an error while reading RIL settings.");

    
    this.dataCallSettings.oldEnabled = false;
    this.dataCallSettings.enabled = false;
    this.dataCallSettings.roamingEnabled = false;
    this.apnSettings = {
      byType: {},
      byApn: {},
    };
  },

  

  rilContext: null,

  

  _sendCfStateChanged: function(message) {
    gMessageManager.sendMobileConnectionMessage("RIL:CfStateChanged",
                                                this.clientId, message);
  },

  _updateCallingLineIdRestrictionPref: function(mode) {
    try {
      Services.prefs.setIntPref(kPrefClirModePreference, mode);
      Services.prefs.savePrefFile(null);
      if (DEBUG) {
        this.debug(kPrefClirModePreference + " pref is now " + mode);
      }
    } catch (e) {}
  },

  sendMMI: function(target, message) {
    if (DEBUG) this.debug("SendMMI " + JSON.stringify(message));
    this.workerMessenger.send("sendMMI", message, (function(response) {
      if (response.isSetCallForward) {
        this._sendCfStateChanged(response);
      } else if (response.isSetCLIR && response.success) {
        this._updateCallingLineIdRestrictionPref(response.clirMode);
      }

      target.sendAsyncMessage("RIL:SendMMI", {
        clientId: this.clientId,
        data: response
      });
      return false;
    }).bind(this));
  },

  setCallForwardingOptions: function(target, message) {
    if (DEBUG) this.debug("setCallForwardingOptions: " + JSON.stringify(message));
    message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE;
    this.workerMessenger.send("setCallForward", message, (function(response) {
      this._sendCfStateChanged(response);
      target.sendAsyncMessage("RIL:SetCallForwardingOptions", {
        clientId: this.clientId,
        data: response
      });
      return false;
    }).bind(this));
  },

  setCallingLineIdRestriction: function(target, message) {
    if (DEBUG) {
      this.debug("setCallingLineIdRestriction: " + JSON.stringify(message));
    }
    this.workerMessenger.send("setCLIR", message, (function(response) {
      if (response.success) {
        this._updateCallingLineIdRestrictionPref(response.clirMode);
      }
      target.sendAsyncMessage("RIL:SetCallingLineIdRestriction", {
        clientId: this.clientId,
        data: response
      });
      return false;
    }).bind(this));
  },

  isValidStateForSetRadioEnabled: function() {
    let state = this.rilContext.detailedRadioState;
    return state == RIL.GECKO_DETAILED_RADIOSTATE_ENABLED ||
      state == RIL.GECKO_DETAILED_RADIOSTATE_DISABLED;
  },

  isDummyForSetRadioEnabled: function(message) {
    let state = this.rilContext.detailedRadioState;
    return (state == RIL.GECKO_DETAILED_RADIOSTATE_ENABLED && message.enabled) ||
      (state == RIL.GECKO_DETAILED_RADIOSTATE_DISABLED && !message.enabled);
  },

  setRadioEnabledResponse: function(target, message, errorMsg) {
    if (errorMsg) {
      message.errorMsg = errorMsg;
    }

    target.sendAsyncMessage("RIL:SetRadioEnabled", {
      clientId: this.clientId,
      data: message
    });
  },

  setRadioEnabled: function(target, message) {
    if (DEBUG) {
      this.debug("setRadioEnabled: " + JSON.stringify(message));
    }

    if (!this.isValidStateForSetRadioEnabled()) {
      this.setRadioEnabledResponse(target, message, "InvalidStateError");
      return;
    }

    if (this.isDummyForSetRadioEnabled(message)) {
      this.setRadioEnabledResponse(target, message);
      return;
    }

    let callback = (function(response) {
      if (response.errorMsg) {
        
        let state = message.enabled ? RIL.GECKO_DETAILED_RADIOSTATE_DISABLED
                                    : RIL.GECKO_DETAILED_RADIOSTATE_ENABLED;
        this.handleDetailedRadioStateChanged(state);
      }
      this.setRadioEnabledResponse(target, response);
      return false;
    }).bind(this);

    this.setRadioEnabledInternal(message, callback);
  },

  setRadioEnabledInternal: function(message, callback) {
    let state = message.enabled ? RIL.GECKO_DETAILED_RADIOSTATE_ENABLING
                                : RIL.GECKO_DETAILED_RADIOSTATE_DISABLING;
    this.handleDetailedRadioStateChanged(state);
    this.workerMessenger.send("setRadioEnabled", message, callback);
  },

  




  enabledGsmTableTuples: [
    [RIL.PDU_NL_IDENTIFIER_DEFAULT, RIL.PDU_NL_IDENTIFIER_DEFAULT],
  ],

  




  segmentRef16Bit: false,

  


  _segmentRef: 0,
  get nextSegmentRef() {
    let ref = this._segmentRef++;

    this._segmentRef %= (this.segmentRef16Bit ? 65535 : 255);

    
    return ref + 1;
  },

  

















  _countGsm7BitSeptets: function(message, langTable, langShiftTable, strict7BitEncoding) {
    let length = 0;
    for (let msgIndex = 0; msgIndex < message.length; msgIndex++) {
      let c = message.charAt(msgIndex);
      if (strict7BitEncoding) {
        c = RIL.GSM_SMS_STRICT_7BIT_CHARMAP[c] || c;
      }

      let septet = langTable.indexOf(c);

      
      
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        length++;
        continue;
      }

      septet = langShiftTable.indexOf(c);
      if (septet < 0) {
        if (!strict7BitEncoding) {
          return -1;
        }

        
        
        c = "*";
        if (langTable.indexOf(c) >= 0) {
          length++;
        } else if (langShiftTable.indexOf(c) >= 0) {
          length += 2;
        } else {
          
          return -1;
        }

        continue;
      }

      
      
      
      if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
        continue;
      }

      
      
      
      
      
      length += 2;
    }

    return length;
  },

  















  _calculateUserDataLength7Bit: function(message, strict7BitEncoding) {
    let options = null;
    let minUserDataSeptets = Number.MAX_VALUE;
    for (let i = 0; i < this.enabledGsmTableTuples.length; i++) {
      let [langIndex, langShiftIndex] = this.enabledGsmTableTuples[i];

      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];

      let bodySeptets = this._countGsm7BitSeptets(message,
                                                  langTable,
                                                  langShiftTable,
                                                  strict7BitEncoding);
      if (bodySeptets < 0) {
        continue;
      }

      let headerLen = 0;
      if (langIndex != RIL.PDU_NL_IDENTIFIER_DEFAULT) {
        headerLen += 3; 
      }
      if (langShiftIndex != RIL.PDU_NL_IDENTIFIER_DEFAULT) {
        headerLen += 3; 
      }

      
      let headerSeptets = Math.ceil((headerLen ? headerLen + 1 : 0) * 8 / 7);
      let segmentSeptets = RIL.PDU_MAX_USER_DATA_7BIT;
      if ((bodySeptets + headerSeptets) > segmentSeptets) {
        headerLen += this.segmentRef16Bit ? 6 : 5;
        headerSeptets = Math.ceil((headerLen + 1) * 8 / 7);
        segmentSeptets -= headerSeptets;
      }

      let segments = Math.ceil(bodySeptets / segmentSeptets);
      let userDataSeptets = bodySeptets + headerSeptets * segments;
      if (userDataSeptets >= minUserDataSeptets) {
        continue;
      }

      minUserDataSeptets = userDataSeptets;

      options = {
        dcs: RIL.PDU_DCS_MSG_CODING_7BITS_ALPHABET,
        encodedFullBodyLength: bodySeptets,
        userDataHeaderLength: headerLen,
        langIndex: langIndex,
        langShiftIndex: langShiftIndex,
        segmentMaxSeq: segments,
        segmentChars: segmentSeptets,
      };
    }

    return options;
  },

  










  _calculateUserDataLengthUCS2: function(message) {
    let bodyChars = message.length;
    let headerLen = 0;
    let headerChars = Math.ceil((headerLen ? headerLen + 1 : 0) / 2);
    let segmentChars = RIL.PDU_MAX_USER_DATA_UCS2;
    if ((bodyChars + headerChars) > segmentChars) {
      headerLen += this.segmentRef16Bit ? 6 : 5;
      headerChars = Math.ceil((headerLen + 1) / 2);
      segmentChars -= headerChars;
    }

    let segments = Math.ceil(bodyChars / segmentChars);

    return {
      dcs: RIL.PDU_DCS_MSG_CODING_16BITS_ALPHABET,
      encodedFullBodyLength: bodyChars * 2,
      userDataHeaderLength: headerLen,
      segmentMaxSeq: segments,
      segmentChars: segmentChars,
    };
  },

  




























  _calculateUserDataLength: function(message, strict7BitEncoding) {
    let options = this._calculateUserDataLength7Bit(message, strict7BitEncoding);
    if (!options) {
      options = this._calculateUserDataLengthUCS2(message);
    }

    if (DEBUG) this.debug("_calculateUserDataLength: " + JSON.stringify(options));
    return options;
  },

  
















  _fragmentText7Bit: function(text, langTable, langShiftTable, segmentSeptets, strict7BitEncoding) {
    let ret = [];
    let body = "", len = 0;
    
    if (text.length === 0) {
      ret.push({
        body: text,
        encodedBodyLength: text.length,
      });
      return ret;
    }

    for (let i = 0, inc = 0; i < text.length; i++) {
      let c = text.charAt(i);
      if (strict7BitEncoding) {
        c = RIL.GSM_SMS_STRICT_7BIT_CHARMAP[c] || c;
      }

      let septet = langTable.indexOf(c);
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        inc = 1;
      } else {
        septet = langShiftTable.indexOf(c);
        if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
          continue;
        }

        inc = 2;
        if (septet < 0) {
          if (!strict7BitEncoding) {
            throw new Error("Given text cannot be encoded with GSM 7-bit Alphabet!");
          }

          
          
          c = "*";
          if (langTable.indexOf(c) >= 0) {
            inc = 1;
          }
        }
      }

      if ((len + inc) > segmentSeptets) {
        ret.push({
          body: body,
          encodedBodyLength: len,
        });
        body = c;
        len = inc;
      } else {
        body += c;
        len += inc;
      }
    }

    if (len) {
      ret.push({
        body: body,
        encodedBodyLength: len,
      });
    }

    return ret;
  },

  









  _fragmentTextUCS2: function(text, segmentChars) {
    let ret = [];
    
    if (text.length === 0) {
      ret.push({
        body: text,
        encodedBodyLength: text.length,
      });
      return ret;
    }

    for (let offset = 0; offset < text.length; offset += segmentChars) {
      let str = text.substr(offset, segmentChars);
      ret.push({
        body: str,
        encodedBodyLength: str.length * 2,
      });
    }

    return ret;
  },

  

















  _fragmentText: function(text, options, strict7BitEncoding) {
    if (!options) {
      options = this._calculateUserDataLength(text, strict7BitEncoding);
    }

    if (options.dcs == RIL.PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[options.langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[options.langShiftIndex];
      options.segments = this._fragmentText7Bit(text,
                                                langTable, langShiftTable,
                                                options.segmentChars,
                                                strict7BitEncoding);
    } else {
      options.segments = this._fragmentTextUCS2(text,
                                                options.segmentChars);
    }

    
    options.segmentMaxSeq = options.segments.length;

    return options;
  },

  getSegmentInfoForText: function(text, request) {
    let strict7BitEncoding;
    try {
      strict7BitEncoding = Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      strict7BitEncoding = false;
    }

    let options = this._fragmentText(text, null, strict7BitEncoding);
    let charsInLastSegment;
    if (options.segmentMaxSeq) {
      let lastSegment = options.segments[options.segmentMaxSeq - 1];
      charsInLastSegment = lastSegment.encodedBodyLength;
      if (options.dcs == RIL.PDU_DCS_MSG_CODING_16BITS_ALPHABET) {
        
        charsInLastSegment /= 2;
      }
    } else {
      charsInLastSegment = 0;
    }

    let result = gMobileMessageService
                 .createSmsSegmentInfo(options.segmentMaxSeq,
                                       options.segmentChars,
                                       options.segmentChars - charsInLastSegment);
    request.notifySegmentInfoForTextGot(result);
  },

  getSmscAddress: function(request) {
    this.workerMessenger.send("getSmscAddress",
                              null,
                              (function(response) {
      if (!response.errorMsg) {
        request.notifyGetSmscAddress(response.smscAddress);
      } else {
        request.notifyGetSmscAddressFailed(response.errorMsg);
      }
    }).bind(this));
  },

  sendSMS: function(number, message, silent, request) {
    let strict7BitEncoding;
    try {
      strict7BitEncoding = Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      strict7BitEncoding = false;
    }

    let options = this._fragmentText(message, null, strict7BitEncoding);
    options.number = PhoneNumberUtils.normalize(number);
    let requestStatusReport;
    try {
      requestStatusReport =
        Services.prefs.getBoolPref("dom.sms.requestStatusReport");
    } catch (e) {
      requestStatusReport = true;
    }
    options.requestStatusReport = requestStatusReport && !silent;
    if (options.segmentMaxSeq > 1) {
      options.segmentRef16Bit = this.segmentRef16Bit;
      options.segmentRef = this.nextSegmentRef;
    }

    let notifyResult = (function notifyResult(rv, domMessage) {
      
      if (!silent) {
        Services.obs.notifyObservers(domMessage, kSmsSendingObserverTopic, null);
      }

      
      
      let errorCode;
      if (!PhoneNumberUtils.isPlainPhoneNumber(options.number)) {
        if (DEBUG) this.debug("Error! Address is invalid when sending SMS: " +
                              options.number);
        errorCode = Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
      } else if (this.rilContext.detailedRadioState ==
                 RIL.GECKO_DETAILED_RADIOSTATE_DISABLED) {
        if (DEBUG) this.debug("Error! Radio is disabled when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
      } else if (this.rilContext.cardState != "ready") {
        if (DEBUG) this.debug("Error! SIM card is not ready when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
      }
      if (errorCode) {
        if (silent) {
          request.notifySendMessageFailed(errorCode);
          return;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(domMessage.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                         RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                         null,
                                         function notifyResult(rv, domMessage) {
          
          request.notifySendMessageFailed(errorCode);
          Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
        });
        return;
      }

      
      let context = {
        request: request,
        sms: domMessage,
        requestStatusReport: options.requestStatusReport,
        silent: silent
      };

      
      this.workerMessenger.send("sendSMS", options,
                                (function(context, response) {
        if (response.errorMsg) {
          
          let error = Ci.nsIMobileMessageCallback.UNKNOWN_ERROR;
          switch (response.errorMsg) {
            case RIL.ERROR_RADIO_NOT_AVAILABLE:
              error = Ci.nsIMobileMessageCallback.NO_SIGNAL_ERROR;
              break;
            case RIL.ERROR_FDN_CHECK_FAILURE:
              error = Ci.nsIMobileMessageCallback.FDN_CHECK_ERROR;
              break;
          }

          if (context.silent) {
            context.request.notifySendMessageFailed(error);
            return false;
          }

          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(context.sms.id,
                                           null,
                                           DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                           RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                           null,
                                           function notifyResult(rv, domMessage) {
            
            context.request.notifySendMessageFailed(error);
            Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
          });
          return false;
        } 

        if (response.deliveryStatus) {
          
          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(context.sms.id,
                                           null,
                                           context.sms.delivery,
                                           response.deliveryStatus,
                                           null,
                                           (function notifyResult(rv, domMessage) {
            

            let topic = (response.deliveryStatus ==
                         RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
                        ? kSmsDeliverySuccessObserverTopic
                        : kSmsDeliveryErrorObserverTopic;

            
            if (topic == kSmsDeliverySuccessObserverTopic) {
              this.broadcastSmsSystemMessage(topic, domMessage);
            }

            
            Services.obs.notifyObservers(domMessage, topic, null);
          }).bind(this));

          
          return false;
        } 

        
        if (context.silent) {
          
          
          
          let sms = context.sms;
          context.request.notifyMessageSent(
            gMobileMessageService.createSmsMessage(sms.id,
                                                   sms.threadId,
                                                   sms.iccId,
                                                   DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                                   sms.deliveryStatus,
                                                   sms.sender,
                                                   sms.receiver,
                                                   sms.body,
                                                   sms.messageClass,
                                                   sms.timestamp,
                                                   Date.now(),
                                                   0,
                                                   sms.read));
          
          return false;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(context.sms.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                         context.sms.deliveryStatus,
                                         null,
                                         (function notifyResult(rv, domMessage) {
          

          if (context.requestStatusReport) {
            context.sms = domMessage;
          }

          this.broadcastSmsSystemMessage(kSmsSentObserverTopic, domMessage);
          context.request.notifyMessageSent(domMessage);
          Services.obs.notifyObservers(domMessage, kSmsSentObserverTopic, null);
        }).bind(this));

        
        return context.requestStatusReport;
      }).bind(this, context)); 
    }).bind(this); 

    let sendingMessage = {
      type: "sms",
      sender: this.getPhoneNumber(),
      receiver: number,
      body: message,
      deliveryStatusRequested: options.requestStatusReport,
      timestamp: Date.now(),
      iccId: this.getIccId()
    };

    if (silent) {
      let delivery = DOM_MOBILE_MESSAGE_DELIVERY_SENDING;
      let deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_PENDING;
      let domMessage =
        gMobileMessageService.createSmsMessage(-1, 
                                               0,  
                                               sendingMessage.iccId,
                                               delivery,
                                               deliveryStatus,
                                               sendingMessage.sender,
                                               sendingMessage.receiver,
                                               sendingMessage.body,
                                               "normal", 
                                               sendingMessage.timestamp,
                                               0,
                                               0,
                                               false);
      notifyResult(Cr.NS_OK, domMessage);
      return;
    }

    let id = gMobileMessageDatabaseService.saveSendingMessage(
      sendingMessage, notifyResult);
  },

  registerDataCallCallback: function(callback) {
    if (this._datacall_callbacks) {
      if (this._datacall_callbacks.indexOf(callback) != -1) {
        throw new Error("Already registered this callback!");
      }
    } else {
      this._datacall_callbacks = [];
    }
    this._datacall_callbacks.push(callback);
    if (DEBUG) this.debug("Registering callback: " + callback);
  },

  unregisterDataCallCallback: function(callback) {
    if (!this._datacall_callbacks) {
      return;
    }
    let index = this._datacall_callbacks.indexOf(callback);
    if (index != -1) {
      this._datacall_callbacks.splice(index, 1);
      if (DEBUG) this.debug("Unregistering callback: " + callback);
    }
  },

  _deliverDataCallCallback: function(name, args) {
    
    
    
    
    
    
    if (!this._datacall_callbacks) {
      return;
    }
    let callbacks = this._datacall_callbacks.slice();
    for (let callback of callbacks) {
      if (this._datacall_callbacks.indexOf(callback) == -1) {
        continue;
      }
      let handler = callback[name];
      if (typeof handler != "function") {
        throw new Error("No handler for " + name);
      }
      try {
        handler.apply(callback, args);
      } catch (e) {
        if (DEBUG) {
          this.debug("callback handler for " + name + " threw an exception: " + e);
        }
      }
    }
  },

  setupDataCallByType: function(apntype) {
    if (DEBUG) this.debug("setupDataCallByType: " + apntype);
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
      if (DEBUG) {
        this.debug("No apn setting for type: " + apntype);
      }
      return;
    }

    let dataInfo = this.rilContext.data;
    if (dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      return;
    }

    apnSetting.iface.connect(apntype);
    
    
    
    
    
    
    
    if (apnSetting.iface.connected) {
      if (apntype == "default" && !dataInfo.connected) {
        dataInfo.connected = true;
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, dataInfo);
      }

      
      
      if (apnSetting.iface.name in gNetworkManager.networkInterfaces) {
        gNetworkManager.unregisterNetworkInterface(apnSetting.iface);
      }
      gNetworkManager.registerNetworkInterface(apnSetting.iface);

      Services.obs.notifyObservers(apnSetting.iface,
                                   kNetworkInterfaceStateChangedTopic,
                                   null);
    }
  },

  deactivateDataCallByType: function(apntype) {
    if (DEBUG) this.debug("deactivateDataCallByType: " + apntype);
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
      if (DEBUG) {
        this.debug("No apn setting for type: " + apntype);
      }
      return;
    }

    apnSetting.iface.disconnect(apntype);
    
    
    
    
    
    
    if (apnSetting.iface.connectedTypes.length && apnSetting.iface.connected) {
      let dataInfo = this.rilContext.data;
      if (apntype == "default" && dataInfo.connected) {
        dataInfo.connected = false;
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, dataInfo);
      }

      
      
      if (apnSetting.iface.name in gNetworkManager.networkInterfaces) {
        gNetworkManager.unregisterNetworkInterface(apnSetting.iface);
      }
      gNetworkManager.registerNetworkInterface(apnSetting.iface);

      Services.obs.notifyObservers(apnSetting.iface,
                                   kNetworkInterfaceStateChangedTopic,
                                   null);
    }
  },

  getDataCallStateByType: function(apntype) {
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
       return RIL.GECKO_NETWORK_STATE_UNKNOWN;
    }
    if (!apnSetting.iface.inConnectedTypes(apntype)) {
      return RIL.GECKO_NETWORK_STATE_DISCONNECTED;
    }
    return apnSetting.iface.state;
  },

  setupDataCall: function(radioTech, apn, user, passwd, chappap, pdptype) {
    this.workerMessenger.send("setupDataCall", { radioTech: radioTech,
                                                 apn: apn,
                                                 user: user,
                                                 passwd: passwd,
                                                 chappap: chappap,
                                                 pdptype: pdptype });
  },

  deactivateDataCall: function(cid, reason) {
    this.workerMessenger.send("deactivateDataCall", { cid: cid,
                                                      reason: reason });
  },

  sendWorkerMessage: function(rilMessageType, message, callback) {
    this.workerMessenger.send(rilMessageType, message, function(response) {
      return callback.handleResponse(response);
    });
  }
};

function RILNetworkInterface(radioInterface, apnSetting) {
  this.radioInterface = radioInterface;
  this.apnSetting = apnSetting;

  this.connectedTypes = [];
}

RILNetworkInterface.prototype = {
  classID:   RILNETWORKINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILNETWORKINTERFACE_CID,
                                    classDescription: "RILNetworkInterface",
                                    interfaces: [Ci.nsINetworkInterface,
                                                 Ci.nsIRilNetworkInterface,
                                                 Ci.nsIRILDataCallback]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface,
                                         Ci.nsIRilNetworkInterface,
                                         Ci.nsIRILDataCallback]),

  

  NETWORK_STATE_UNKNOWN:       Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,
  NETWORK_STATE_CONNECTING:    Ci.nsINetworkInterface.CONNECTING,
  NETWORK_STATE_CONNECTED:     Ci.nsINetworkInterface.CONNECTED,
  NETWORK_STATE_DISCONNECTING: Ci.nsINetworkInterface.DISCONNECTING,
  NETWORK_STATE_DISCONNECTED:  Ci.nsINetworkInterface.DISCONNECTED,

  NETWORK_TYPE_WIFI:        Ci.nsINetworkInterface.NETWORK_TYPE_WIFI,
  NETWORK_TYPE_MOBILE:      Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,
  NETWORK_TYPE_MOBILE_MMS:  Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS,
  NETWORK_TYPE_MOBILE_SUPL: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,
  
  
  
  NETWORK_TYPE_MOBILE_OTHERS: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,

  



  NETWORK_APNRETRY_FACTOR: 8,
  NETWORK_APNRETRY_ORIGIN: 3,
  NETWORK_APNRETRY_MAXRETRIES: 10,

  
  timer: null,

  



  state: Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,

  get type() {
    if (this.connectedTypes.indexOf("default") != -1) {
      return this.NETWORK_TYPE_MOBILE;
    }
    if (this.connectedTypes.indexOf("mms") != -1) {
      return this.NETWORK_TYPE_MOBILE_MMS;
    }
    if (this.connectedTypes.indexOf("supl") != -1) {
      return this.NETWORK_TYPE_MOBILE_SUPL;
    }
    return this.NETWORK_TYPE_MOBILE_OTHERS;
  },

  name: null,

  ip: null,

  netmask: null,

  broadcast: null,

  dns1: null,

  dns2: null,

  get httpProxyHost() {
    return this.apnSetting.proxy || "";
  },

  get httpProxyPort() {
    return this.apnSetting.port || "";
  },

  



  get serviceId() {
    return this.radioInterface.clientId;
  },

  get iccId() {
    let iccInfo = this.radioInterface.rilContext.iccInfo;
    return iccInfo && iccInfo.iccid;
  },

  get mmsc() {
    if (!this.inConnectedTypes("mms")) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMSC.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    let mmsc = this.apnSetting.mmsc;
    if (!mmsc) {
      try {
        mmsc = Services.prefs.getCharPref("ril.mms.mmsc");
      } catch (e) {
        mmsc = "";
      }
    }

    return mmsc;
  },

  get mmsProxy() {
    if (!this.inConnectedTypes("mms")) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS proxy.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    let proxy = this.apnSetting.mmsproxy;
    if (!proxy) {
      try {
        proxy = Services.prefs.getCharPref("ril.mms.mmsproxy");
      } catch (e) {
        proxy = "";
      }
    }

    return proxy;
  },

  get mmsPort() {
    if (!this.inConnectedTypes("mms")) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS port.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    let port = this.apnSetting.mmsport;
    if (!port) {
      try {
        port = Services.prefs.getIntPref("ril.mms.mmsport");
      } catch (e) {
        port = -1;
      }
    }

    return port;
  },

  debug: function(s) {
    dump("-*- RILNetworkInterface[" + this.radioInterface.clientId + ":" +
         this.type + "]: " + s + "\n");
  },

  

  dataCallError: function(message) {
    if (message.apn != this.apnSetting.apn) {
      return;
    }
    if (DEBUG) this.debug("Data call error on APN: " + message.apn);
    this.reset();
  },

  dataCallStateChanged: function(datacall) {
    if (this.cid && this.cid != datacall.cid) {
    
    
      return;
    }
    
    
    
    if (!this.cid && datacall.apn != this.apnSetting.apn) {
      return;
    }
    if (DEBUG) {
      this.debug("Data call ID: " + datacall.cid + ", interface name: " +
                 datacall.ifname + ", APN name: " + datacall.apn);
    }
    if (this.connecting &&
        (datacall.state == RIL.GECKO_NETWORK_STATE_CONNECTING ||
         datacall.state == RIL.GECKO_NETWORK_STATE_CONNECTED)) {
      this.connecting = false;
      this.cid = datacall.cid;
      this.name = datacall.ifname;
      this.ip = datacall.ip;
      this.netmask = datacall.netmask;
      this.broadcast = datacall.broadcast;
      this.gateway = datacall.gw;
      if (datacall.dns) {
        this.dns1 = datacall.dns[0];
        this.dns2 = datacall.dns[1];
      }
      if (!this.registeredAsNetworkInterface) {
        gNetworkManager.registerNetworkInterface(this);
        this.registeredAsNetworkInterface = true;
      }
    }
    
    
    
    if (this.cid == null) {
      return;
    }

    if (this.state == datacall.state) {
      if (datacall.state != RIL.GECKO_NETWORK_STATE_CONNECTED) {
        return;
      }
      
      let changed = false;
      if (this.gateway != datacall.gw) {
        this.gateway = datacall.gw;
        changed = true;
      }
      if (datacall.dns &&
          (this.dns1 != datacall.dns[0] ||
           this.dns2 != datacall.dns[1])) {
        this.dns1 = datacall.dns[0];
        this.dns2 = datacall.dns[1];
        changed = true;
      }
      if (changed) {
        if (DEBUG) this.debug("Notify for data call minor changes.");
        Services.obs.notifyObservers(this,
                                     kNetworkInterfaceStateChangedTopic,
                                     null);
      }
      return;
    }

    this.state = datacall.state;

    Services.obs.notifyObservers(this,
                                 kNetworkInterfaceStateChangedTopic,
                                 null);

    if ((this.state == RIL.GECKO_NETWORK_STATE_UNKNOWN ||
         this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED) &&
        this.registeredAsNetworkInterface) {
      gNetworkManager.unregisterNetworkInterface(this);
      this.registeredAsNetworkInterface = false;
      this.cid = null;
      this.connectedTypes = [];
    }

    
    
    
    if (this.radioInterface.apnSettings.byType.default &&
        (this.radioInterface.apnSettings.byType.default.apn ==
         this.apnSetting.apn)) {
      this.radioInterface.updateRILNetworkInterface();
    }
  },

  receiveDataCallList: function(dataCalls, length) {
  },

  

  cid: null,
  registeredAsDataCallCallback: false,
  registeredAsNetworkInterface: false,
  connecting: false,
  apnSetting: null,

  
  apnRetryCounter: 0,

  connectedTypes: null,

  inConnectedTypes: function(type) {
    return this.connectedTypes.indexOf(type) != -1;
  },

  get connected() {
    return this.state == RIL.GECKO_NETWORK_STATE_CONNECTED;
  },

  connect: function(apntype) {
    if (apntype && !this.inConnectedTypes(apntype)) {
      this.connectedTypes.push(apntype);
    }

    if (this.connecting || this.connected) {
      return;
    }

    
    
    if (!this.connectedTypes.length) {
      return;
    }

    if (!this.registeredAsDataCallCallback) {
      this.radioInterface.registerDataCallCallback(this);
      this.registeredAsDataCallCallback = true;
    }

    if (!this.apnSetting.apn) {
      if (DEBUG) this.debug("APN name is empty, nothing to do.");
      return;
    }

    if (DEBUG) {
      this.debug("Going to set up data connection with APN " +
                 this.apnSetting.apn);
    }
    let radioTechType = this.radioInterface.rilContext.data.type;
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);
    let authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(this.apnSetting.authtype);
    
    
    if (authType == -1) {
      if (DEBUG) {
        this.debug("Invalid authType " + this.apnSetting.authtype);
      }
      authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(RIL.GECKO_DATACALL_AUTH_DEFAULT);
    }
    this.radioInterface.setupDataCall(radioTechnology,
                                      this.apnSetting.apn,
                                      this.apnSetting.user,
                                      this.apnSetting.password,
                                      authType,
                                      "IP");
    this.connecting = true;
  },

  reset: function() {
    let apnRetryTimer;
    this.connecting = false;
    
    
    if (this.apnRetryCounter >= this.NETWORK_APNRETRY_MAXRETRIES) {
      this.apnRetryCounter = 0;
      this.timer = null;
      this.connectedTypes = [];
      if (DEBUG) this.debug("Too many APN Connection retries - STOP retrying");
      return;
    }

    apnRetryTimer = this.NETWORK_APNRETRY_FACTOR *
                    (this.apnRetryCounter * this.apnRetryCounter) +
                    this.NETWORK_APNRETRY_ORIGIN;
    this.apnRetryCounter++;
    if (DEBUG) {
      this.debug("Data call - APN Connection Retry Timer (secs-counter): " +
                 apnRetryTimer + "-" + this.apnRetryCounter);
    }

    if (this.timer == null) {
      
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this.timer.initWithCallback(this, apnRetryTimer * 1000,
                                Ci.nsITimer.TYPE_ONE_SHOT);
  },

  disconnect: function(apntype) {
    let index = this.connectedTypes.indexOf(apntype);
    if (index != -1) {
      this.connectedTypes.splice(index, 1);
    }

    if (this.connectedTypes.length) {
      return;
    }

    if (this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTING ||
        this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED ||
        this.state == RIL.GECKO_NETWORK_STATE_UNKNOWN) {
      return;
    }
    let reason = RIL.DATACALL_DEACTIVATE_NO_REASON;
    if (DEBUG) this.debug("Going to disconnet data connection " + this.cid);
    this.radioInterface.deactivateDataCall(this.cid, reason);
  },

  
  notify: function(timer) {
    this.connect();
  },

  shutdown: function() {
    this.timer = null;
  }

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RadioInterfaceLayer]);
