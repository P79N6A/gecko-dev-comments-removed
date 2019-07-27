














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Sntp.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});


let RILQUIRKS_DATA_REGISTRATION_ON_DEMAND =
  libcutils.property_get("ro.moz.ril.data_reg_on_demand", "false") == "true";


let RILQUIRKS_SUBSCRIPTION_CONTROL =
  libcutils.property_get("ro.moz.ril.subscription_control", "false") == "true";



let RILQUIRKS_RADIO_OFF_WO_CARD =
  libcutils.property_get("ro.moz.ril.radio_off_wo_card", "false") == "true";


let RILQUIRKS_HAVE_IPV6 =
  libcutils.property_get("ro.moz.ril.ipv6", "false") == "true";

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");
const ICCINFO_CID =
  Components.ID("{52eec7f0-26a4-11e4-8c21-0800200c9a66}");
const GSMICCINFO_CID =
  Components.ID("{d90c4261-a99d-47bc-8b05-b057bb7e8f8a}");
const CDMAICCINFO_CID =
  Components.ID("{39ba3c08-aacc-46d0-8c04-9b619c387061}");
const NEIGHBORINGCELLINFO_CID =
  Components.ID("{f9dfe26a-851e-4a8b-a769-cbb1baae7ded}");
const GSMCELLINFO_CID =
  Components.ID("{41f6201e-7263-42e3-b31f-38a9dc8a280a}");
const WCDMACELLINFO_CID =
  Components.ID("{eeaaf307-df6e-4c98-b121-e3302b1fc468}");
const CDMACELLINFO_CID =
  Components.ID("{b497d6e4-4cb8-4d6e-b673-840c7d5ddf25}");
const LTECELLINFO_CID =
  Components.ID("{c7e0a78a-4e99-42f5-9251-e6172c5ed8d8}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const kNetworkConnStateChangedTopic      = "network-connection-state-changed";
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

const kSettingsCellBroadcastDisabled = "ril.cellbroadcast.disabled";
const kSettingsCellBroadcastSearchList = "ril.cellbroadcast.searchlist";
const kSettingsClockAutoUpdateEnabled = "time.clock.automatic-update.enabled";
const kSettingsClockAutoUpdateAvailable = "time.clock.automatic-update.available";
const kSettingsTimezoneAutoUpdateEnabled = "time.timezone.automatic-update.enabled";
const kSettingsTimezoneAutoUpdateAvailable = "time.timezone.automatic-update.available";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

const RADIO_POWER_OFF_TIMEOUT = 30000;
const SMS_HANDLED_WAKELOCK_TIMEOUT = 5000;
const HW_DEFAULT_CLIENT_ID = 0;

const INT32_MAX = 2147483647;
const UNKNOWN_RSSI = 99;

const RIL_IPC_ICCMANAGER_MSG_NAMES = [
  "RIL:GetRilContext",
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


var DEBUG = RIL.DEBUG_RIL;

function updateDebugFlag() {
  
  let debugPref;
  try {
    debugPref = Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
  } catch (e) {
    debugPref = false;
  }
  DEBUG = RIL.DEBUG_RIL || debugPref;
}
updateDebugFlag();

function debug(s) {
  dump("-*- RadioInterfaceLayer: " + s + "\n");
}

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

XPCOMUtils.defineLazyServiceGetter(this, "gTelephonyService",
                                   "@mozilla.org/telephony/telephonyservice;1",
                                   "nsIGonkTelephonyService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIGonkMobileConnectionService");

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

      if (RIL_IPC_ICCMANAGER_MSG_NAMES.indexOf(msg.name) != -1) {
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
  let _ril = null;
  let _pendingMessages = [];  
  let _isProcessingPending = false;
  let _timer = null;
  let _request = null;
  let _deactivatingDeferred = {};
  let _initializedCardState = {};
  let _allCardStateInitialized = !RILQUIRKS_RADIO_OFF_WO_CARD;

  return {
    init: function(ril) {
      _ril = ril;
    },

    receiveCardState: function(clientId) {
      if (_allCardStateInitialized) {
        return;
      }

      if (DEBUG) debug("RadioControl: receive cardState from " + clientId);
      _initializedCardState[clientId] = true;
      if (Object.keys(_initializedCardState).length == _ril.numRadioInterfaces) {
        _allCardStateInitialized = true;
        this._startProcessingPending();
      }
    },

    setRadioEnabled: function(clientId, data, callback) {
      if (DEBUG) debug("setRadioEnabled: " + clientId + ": " + JSON.stringify(data));
      let message = {
        clientId: clientId,
        data: data,
        callback: callback
      };
      _pendingMessages.push(message);
      this._startProcessingPending();
    },

    isDeactivatingDataCalls: function() {
      return _request !== null;
    },

    finishDeactivatingDataCalls: function(clientId) {
      if (DEBUG) debug("RadioControl: finishDeactivatingDataCalls: " + clientId);
      let deferred = _deactivatingDeferred[clientId];
      if (deferred) {
        deferred.resolve();
      }
    },

    notifyRadioStateChanged: function(clientId, radioState) {
      gMobileConnectionService.notifyRadioStateChanged(clientId, radioState);
    },

    _startProcessingPending: function() {
      if (!_isProcessingPending) {
        if (DEBUG) debug("RadioControl: start dequeue");
        _isProcessingPending = true;
        this._processNextMessage();
      }
    },

    _processNextMessage: function() {
      if (_pendingMessages.length === 0 || !_allCardStateInitialized) {
        if (DEBUG) debug("RadioControl: stop dequeue");
        _isProcessingPending = false;
        return;
      }

      let msg = _pendingMessages.shift();
      this._handleMessage(msg);
    },

    _getNumCards: function() {
      let numCards = 0;
      for (let i = 0, N = _ril.numRadioInterfaces; i < N; ++i) {
        if (this._isCardPresentAtClient(i)) {
          numCards++;
        }
      }
      return numCards;
    },

    _isCardPresentAtClient: function(clientId) {
      let cardState = _ril.getRadioInterface(clientId).rilContext.cardState;
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

    _isValidStateForSetRadioEnabled: function(clientId) {
      let radioState = gMobileConnectionService.getRadioState(clientId);
      return radioState == RIL.GECKO_RADIOSTATE_ENABLED ||
             radioState == RIL.GECKO_RADIOSTATE_DISABLED;
    },

    _isDummyForSetRadioEnabled: function(clientId, data) {
      let radioState = gMobileConnectionService.getRadioState(clientId);
      return (radioState == RIL.GECKO_RADIOSTATE_ENABLED && data.enabled) ||
             (radioState == RIL.GECKO_RADIOSTATE_DISABLED && !data.enabled);
    },

    _handleMessage: function(message) {
      if (DEBUG) debug("RadioControl: handleMessage: " + JSON.stringify(message));
      let clientId = message.clientId || 0;
      let radioInterface = _ril.getRadioInterface(clientId);

      if (!this._isValidStateForSetRadioEnabled(clientId)) {
        message.data.errorMsg = "InvalidStateError";
        message.callback(message.data);
        this._processNextMessage();
        return;
      }

      if (this._isDummyForSetRadioEnabled(clientId, message.data)) {
        message.callback(message.data);
        this._processNextMessage();
        return;
      }

      if (message.data.enabled) {
        if (this._isRadioAbleToEnableAtClient(clientId)) {
          this._setRadioEnabledInternal(message);
        } else {
          
          message.callback(message.data);
        }

        this._processNextMessage();
      } else {
        _request = this._setRadioEnabledInternal.bind(this, message);

        
        
        
        
        
        for (let i = 0, N = _ril.numRadioInterfaces; i < N; ++i) {
          let iface = _ril.getRadioInterface(i);
          iface.workerMessenger.send("hangUpAll");
        }

        
        
        
        this._deactivateDataCalls().then(() => {
          if (DEBUG) debug("RadioControl: deactivation done");
          this._executeRequest();
        });

        this._createTimer();
      }
    },

    _setRadioEnabledInternal: function(message) {
      let clientId = message.clientId || 0;
      let enabled = message.data.enabled || false;
      let radioInterface = _ril.getRadioInterface(clientId);

      this.notifyRadioStateChanged(clientId,
                                   enabled ? RIL.GECKO_RADIOSTATE_ENABLING
                                           : RIL.GECKO_RADIOSTATE_DISABLING);
      radioInterface.workerMessenger.send("setRadioEnabled", message.data,
                                          (function(response) {
        if (response.errorMsg) {
          
          this.notifyRadioStateChanged(clientId,
                                       enabled ? RIL.GECKO_RADIOSTATE_DISABLED
                                               : RIL.GECKO_RADIOSTATE_ENABLED);
        }
        message.callback(response);
        return false;
      }).bind(this));
    },

    _deactivateDataCalls: function() {
      if (DEBUG) debug("RadioControl: deactivating data calls...");
      _deactivatingDeferred = {};

      let promise = Promise.resolve();
      for (let i = 0, N = _ril.numRadioInterfaces; i < N; ++i) {
        promise = promise.then(this._deactivateDataCallsForClient(i));
      }

      return promise;
    },

    _deactivateDataCallsForClient: function(clientId) {
      return function() {
        let deferred = _deactivatingDeferred[clientId] = Promise.defer();
        let dataConnectionHandler = gDataConnectionManager.getConnectionHandler(clientId);
        dataConnectionHandler.deactivateDataCalls();
        return deferred.promise;
      };
    },

    _createTimer: function() {
      if (!_timer) {
        _timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      }
      _timer.initWithCallback(this._executeRequest.bind(this),
                              RADIO_POWER_OFF_TIMEOUT,
                              Ci.nsITimer.TYPE_ONE_SHOT);
    },

    _cancelTimer: function() {
      if (_timer) {
        _timer.cancel();
      }
    },

    _executeRequest: function() {
      if (typeof _request === "function") {
        if (DEBUG) debug("RadioControl: executeRequest");
        this._cancelTimer();
        _request();
        _request = null;
      }
      this._processNextMessage();
    },
  };
});

XPCOMUtils.defineLazyGetter(this, "gDataConnectionManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                           Ci.nsISettingsServiceCallback]),

    _connectionHandlers: null,

    
    
    _dataEnabled: false,

    
    
    _dataDefaultClientId: -1,

    
    
    
    _currentDataClientId: -1,

    
    
    _pendingDataCallRequest: null,

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

      let lock = gSettingsService.createLock();
      
      lock.get("ril.data.apnSettings", this);
      
      lock.get("ril.data.enabled", this);
      lock.get("ril.data.roaming_enabled", this);
      
      lock.get("ril.data.defaultServiceId", this);

      Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
      Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
    },

    getConnectionHandler: function(clientId) {
      return this._connectionHandlers[clientId];
    },

    isSwitchingDataClientId: function() {
      return this._pendingDataCallRequest !== null;
    },

    notifyDataCallStateChange: function(clientId) {
      if (!this.isSwitchingDataClientId() ||
          clientId != this._currentDataClientId) {
        return;
      }

      let connHandler = this._connectionHandlers[this._currentDataClientId];
      if (connHandler.allDataDisconnected() &&
          typeof this._pendingDataCallRequest === "function") {
        if (DEBUG) {
          this.debug("All data calls disconnected, process pending data settings.");
        }
        this._pendingDataCallRequest();
        this._pendingDataCallRequest = null;
      }
    },

    _handleDataClientIdChange: function(newDefault) {
      if (this._dataDefaultClientId === newDefault) {
         return;
      }
      this._dataDefaultClientId = newDefault;

      
      if (this._currentDataClientId == -1) {
        this._currentDataClientId = this._dataDefaultClientId;
        let connHandler = this._connectionHandlers[this._currentDataClientId];
        let radioInterface = connHandler.radioInterface;
        if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND ||
            RILQUIRKS_SUBSCRIPTION_CONTROL) {
          radioInterface.setDataRegistration(true);
        }
        if (this._dataEnabled) {
          let settings = connHandler.dataCallSettings;
          settings.oldEnabled = settings.enabled;
          settings.enabled = true;
          connHandler.updateRILNetworkInterface();
        }
        return;
      }

      let oldConnHandler = this._connectionHandlers[this._currentDataClientId];
      let oldIface = oldConnHandler.radioInterface;
      let oldSettings = oldConnHandler.dataCallSettings;
      let newConnHandler = this._connectionHandlers[this._dataDefaultClientId];
      let newIface = newConnHandler.radioInterface;
      let newSettings = newConnHandler.dataCallSettings;

      let applyPendingDataSettings = (function() {
        if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND ||
            RILQUIRKS_SUBSCRIPTION_CONTROL) {
          oldIface.setDataRegistration(false)
            .then(() => {
              if (this._dataEnabled) {
                newSettings.oldEnabled = newSettings.enabled;
                newSettings.enabled = true;
              }
              this._currentDataClientId = this._dataDefaultClientId;
              return newIface.setDataRegistration(true);
            })
            .then(() => newConnHandler.updateRILNetworkInterface());
          return;
        }

        if (this._dataEnabled) {
          newSettings.oldEnabled = newSettings.enabled;
          newSettings.enabled = true;
        }
        this._currentDataClientId = this._dataDefaultClientId;
        newConnHandler.updateRILNetworkInterface();
      }).bind(this);

      if (this._dataEnabled) {
        oldSettings.oldEnabled = oldSettings.enabled;
        oldSettings.enabled = false;
      }

      if (oldConnHandler.anyDataConnected()) {
        this._pendingDataCallRequest = applyPendingDataSettings;
        if (DEBUG) {
          this.debug("_handleDataClientIdChange: existing data call(s) active" +
                     ", wait for them to get disconnected.");
        }
        oldConnHandler.deactivateDataCalls();
        return;
      }

      applyPendingDataSettings();
    },

    _shutdown: function() {
      for (let handler of this._connectionHandlers) {
        handler.shutdown();
      }
      this._connectionHandlers = null;
      Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
    },

    


    handle: function(name, result) {
      switch(name) {
        case "ril.data.apnSettings":
          if (DEBUG) {
            this.debug("'ril.data.apnSettings' is now " +
                       JSON.stringify(result));
          }
          if (!result) {
            break;
          }
          for (let clientId in this._connectionHandlers) {
            let handler = this._connectionHandlers[clientId];
            let apnSetting = result[clientId];
            if (handler && apnSetting) {
              handler.updateApnSettings(apnSetting);
              handler.updateRILNetworkInterface();
            }
          }
          break;
        case "ril.data.enabled":
          if (DEBUG) {
            this.debug("'ril.data.enabled' is now " + result);
          }
          if (this._dataEnabled === result) {
            break;
          }
          this._dataEnabled = result;

          if (DEBUG) {
            this.debug("Default id for data call: " + this._dataDefaultClientId);
          }
          if (this._dataDefaultClientId === -1) {
            
            break;
          }

          let connHandler = this._connectionHandlers[this._dataDefaultClientId];
          let settings = connHandler.dataCallSettings;
          settings.oldEnabled = settings.enabled;
          settings.enabled = result;
          connHandler.updateRILNetworkInterface();
          break;
        case "ril.data.roaming_enabled":
          if (DEBUG) {
            this.debug("'ril.data.roaming_enabled' is now " + result);
            this.debug("Default id for data call: " + this._dataDefaultClientId);
          }
          for (let clientId = 0; clientId < this._connectionHandlers.length; clientId++) {
            let connHandler = this._connectionHandlers[clientId];
            let settings = connHandler.dataCallSettings;
            settings.roamingEnabled = Array.isArray(result) ? result[clientId] : result;
          }
          if (this._dataDefaultClientId === -1) {
            
            break;
          }
          this._connectionHandlers[this._dataDefaultClientId].updateRILNetworkInterface();
          break;
        case "ril.data.defaultServiceId":
          result = result || 0;
          if (DEBUG) {
            this.debug("'ril.data.defaultServiceId' is now " + result);
          }
          this._handleDataClientIdChange(result);
          break;
      }
    },

    handleError: function(errorMessage) {
      if (DEBUG) {
        this.debug("There was an error while reading RIL settings.");
      }
    },

    


    observe: function(subject, topic, data) {
      switch (topic) {
        case kMozSettingsChangedObserverTopic:
          let setting = JSON.parse(data);
          this.handle(setting.key, setting.value);
          break;
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
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozIccInfo]),
  classID: ICCINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          ICCINFO_CID,
    classDescription: "MozIccInfo",
    flags:            Ci.nsIClassInfo.DOM_OBJECT,
    interfaces:       [Ci.nsIDOMMozIccInfo]
  }),

  

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

  

  mdn: null,
  prlVersion: 0
};

function NeighboringCellInfo() {}
NeighboringCellInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINeighboringCellInfo]),
  classID:        NEIGHBORINGCELLINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          NEIGHBORINGCELLINFO_CID,
    classDescription: "NeighboringCellInfo",
    interfaces:       [Ci.nsINeighboringCellInfo]
  }),

  

  networkType: null,
  gsmLocationAreaCode: -1,
  gsmCellId: -1,
  wcdmaPsc: -1,
  signalStrength: UNKNOWN_RSSI
};

function CellInfo() {}
CellInfo.prototype = {
  type: null,
  registered: false,
  timestampType: Ci.nsICellInfo.TIMESTAMP_TYPE_UNKNOWN,
  timestamp: 0
};

function GsmCellInfo() {}
GsmCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGsmCellInfo]),
  classID: GSMCELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          GSMCELLINFO_CID,
    classDescription: "GsmCellInfo",
    interfaces:       [Ci.nsIGsmCellInfo]
  }),

  

  mcc: INT32_MAX,
  mnc: INT32_MAX,
  lac: INT32_MAX,
  cid: INT32_MAX,
  signalStrength: UNKNOWN_RSSI,
  bitErrorRate: UNKNOWN_RSSI
};

function WcdmaCellInfo() {}
WcdmaCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWcdmaCellInfo]),
  classID: WCDMACELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          WCDMACELLINFO_CID,
    classDescription: "WcdmaCellInfo",
    interfaces:       [Ci.nsIWcdmaCellInfo]
  }),

  

  mcc: INT32_MAX,
  mnc: INT32_MAX,
  lac: INT32_MAX,
  cid: INT32_MAX,
  psc: INT32_MAX,
  signalStrength: UNKNOWN_RSSI,
  bitErrorRate: UNKNOWN_RSSI
};

function LteCellInfo() {}
LteCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsILteCellInfo]),
  classID: LTECELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          LTECELLINFO_CID,
    classDescription: "LteCellInfo",
    interfaces:       [Ci.nsILteCellInfo]
  }),

  

  mcc: INT32_MAX,
  mnc: INT32_MAX,
  cid: INT32_MAX,
  pcid: INT32_MAX,
  tac: INT32_MAX,
  signalStrength: UNKNOWN_RSSI,
  rsrp: INT32_MAX,
  rsrq: INT32_MAX,
  rssnr: INT32_MAX,
  cqi: INT32_MAX,
  timingAdvance: INT32_MAX
};

function CdmaCellInfo() {}
CdmaCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICdmaCellInfo]),
  classID: CDMACELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          CDMACELLINFO_CID,
    classDescription: "CdmaCellInfo",
    interfaces:       [Ci.nsICdmaCellInfo]
  }),

  

  networkId: INT32_MAX,
  systemId: INT32_MAX,
  baseStationId: INT32_MAX,
  longitude: INT32_MAX,
  latitude: INT32_MAX,
  cdmaDbm: INT32_MAX,
  cdmaEcio: INT32_MAX,
  evdoDbm: INT32_MAX,
  evdoEcio: INT32_MAX,
  evdoSnr: INT32_MAX
};

function DataConnectionHandler(clientId, radioInterface) {
  
  this.clientId = clientId;
  this.radioInterface = radioInterface;
  this.dataCallSettings = {
    oldEnabled: false,
    enabled: false,
    roamingEnabled: false
  };
  this._dataCalls = [];

  
  
  this.dataNetworkInterfaces = new Map();
}
DataConnectionHandler.prototype = {
  clientId: 0,
  radioInterface: null,
  
  dataCallSettings: null,
  dataNetworkInterfaces: null,
  _dataCalls: null,

  
  _pendingApnSettings: null,

  debug: function(s) {
    dump("-*- DataConnectionHandler[" + this.clientId + "]: " + s + "\n");
  },

  shutdown: function() {
    
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      gNetworkManager.unregisterNetworkInterface(networkInterface);
      networkInterface.shutdown();
      networkInterface = null;
    });
    this.dataNetworkInterfaces.clear();
    this._dataCalls = [];
    this.clientId = null;
    this.radioInterface = null;
  },

  


  _validateApnSetting: function(apnSetting) {
    return (apnSetting &&
            apnSetting.apn &&
            apnSetting.types &&
            apnSetting.types.length);
  },

  _convertApnType: function(apnType) {
    switch(apnType) {
      case "default":
        return Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;
      case "mms":
        return Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS;
      case "supl":
        return Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL;
      case "ims":
        return Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS;
      case "dun":
        return Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN;
      default:
        return Ci.nsINetworkInterface.NETWORK_TYPE_UNKNOWN;
     }
  },

  _deliverDataCallMessage: function(name, args) {
    for (let i = 0; i < this._dataCalls.length; i++) {
      let datacall = this._dataCalls[i];
      
      
      if (!args[0].apn || args[0].apn != datacall.apnProfile.apn) {
        continue;
      }
      
      
      if (args[0].cid && datacall.linkInfo.cid &&
          args[0].cid != datacall.linkInfo.cid) {
        continue;
      }

      try {
        let handler = datacall[name];
        if (typeof handler !== "function") {
          throw new Error("No handler for " + name);
        }
        handler.apply(datacall, args);
      } catch (e) {
        if (DEBUG) {
          this.debug("Handler for " + name + " threw an exception: " + e);
        }
      }
    }
  },

  








  _setupApnSettings: function(newApnSettings) {
    if (!newApnSettings) {
      return;
    }
    if (DEBUG) this.debug("setupApnSettings: " + JSON.stringify(newApnSettings));

    
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      gNetworkManager.unregisterNetworkInterface(networkInterface);
      networkInterface.shutdown();
      networkInterface = null;
    });
    this.dataNetworkInterfaces.clear();
    this._dataCalls = [];

    
    for (let inputApnSetting of newApnSettings) {
      if (!this._validateApnSetting(inputApnSetting)) {
        continue;
      }

      
      
      for (let i = 0; i < inputApnSetting.types.length; i++) {
        let apnType = inputApnSetting.types[i];
        let networkType = this._convertApnType(apnType);
        if (networkType === Ci.nsINetworkInterface.NETWORK_TYPE_UNKNOWN) {
          if (DEBUG) this.debug("Invalid apn type: " + apnType);
          continue;
        }

        if (DEBUG) this.debug("Preparing RILNetworkInterface for type: " + apnType);
        
        let dataCall;
        for (let i = 0; i < this._dataCalls.length; i++) {
          if (this._dataCalls[i].canHandleApn(inputApnSetting)) {
            if (DEBUG) this.debug("Found shareable DataCall, reusing it.");
            dataCall = this._dataCalls[i];
            break;
          }
        }

        if (!dataCall) {
          if (DEBUG) this.debug("No shareable DataCall found, creating one.");
          dataCall = new DataCall(this.clientId, inputApnSetting);
          this._dataCalls.push(dataCall);
        }

        try {
          let networkInterface = new RILNetworkInterface(this, networkType,
                                                         inputApnSetting,
                                                         dataCall);
          gNetworkManager.registerNetworkInterface(networkInterface);
          this.dataNetworkInterfaces.set(apnType, networkInterface);
        } catch (e) {
          if (DEBUG) {
            this.debug("Error setting up RILNetworkInterface for type " +
                        apnType + ": " + e);
          }
        }
      }
    }
  },

  


  allDataDisconnected: function() {
    for (let i = 0; i < this._dataCalls.length; i++) {
      let dataCall = this._dataCalls[i];
      if (dataCall.state != RIL.GECKO_NETWORK_STATE_UNKNOWN &&
          dataCall.state != RIL.GECKO_NETWORK_STATE_DISCONNECTED) {
        return false;
      }
    }
    return true;
  },

  


  anyDataConnected: function() {
    for (let i = 0; i < this._dataCalls.length; i++) {
      if (this._dataCalls[i].state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
        return true;
      }
    }
    return false;
  },

  updateApnSettings: function(newApnSettings) {
    if (!newApnSettings) {
      return;
    }
    if (this._pendingApnSettings) {
      
      this._pengingApnSettings = newApnSettings;
      return;
    }

    let isDeactivatingDataCalls = false;
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      
      if (networkInterface.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
        networkInterface.disconnect();
        isDeactivatingDataCalls = true;
      }
    });

    if (isDeactivatingDataCalls) {
      
      this._pendingApnSettings = newApnSettings;
      return;
    }
    this._setupApnSettings(newApnSettings);
  },

  updateRILNetworkInterface: function() {
    let networkInterface = this.dataNetworkInterfaces.get("default");
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for default data.");
      }
      return;
    }

    
    
    let radioState = gMobileConnectionService.getRadioState(this.clientId);
    if (radioState != RIL.GECKO_RADIOSTATE_ENABLED) {
      if (DEBUG) {
        this.debug("RIL is not ready for data connection: radio's not ready");
      }
      return;
    }

    
    
    
    
    
    if (this.dataCallSettings.oldEnabled === this.dataCallSettings.enabled) {
      if (DEBUG) {
        this.debug("No changes for ril.data.enabled flag. Nothing to do.");
      }
      return;
    }

    let dataInfo = gMobileConnectionService.getDataConnectionInfo(this.clientId);
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

    let defaultDataCallConnected = networkInterface.connected;

    
    
    if (networkInterface.enabled &&
        (!this.dataCallSettings.enabled ||
         (dataInfo.roaming && !this.dataCallSettings.roamingEnabled))) {
      if (DEBUG) {
        this.debug("Data call settings: disconnect data call.");
      }
      networkInterface.disconnect();
      return;
    }

    if (networkInterface.enabled && wifi_active) {
      if (DEBUG) {
        this.debug("Disconnect data call when Wifi is connected.");
      }
      networkInterface.disconnect();
      return;
    }

    if (!this.dataCallSettings.enabled || defaultDataCallConnected) {
      if (DEBUG) {
        this.debug("Data call settings: nothing to do.");
      }
      return;
    }
    if (dataInfo.roaming && !this.dataCallSettings.roamingEnabled) {
      if (DEBUG) {
        this.debug("We're roaming, but data roaming is disabled.");
      }
      return;
    }
    if (wifi_active) {
      if (DEBUG) {
        this.debug("Don't connect data call when Wifi is connected.");
      }
      return;
    }
    if (this._pendingApnSettings) {
      if (DEBUG) this.debug("We're changing apn settings, ignore any changes.");
      return;
    }

    if (gRadioEnabledController.isDeactivatingDataCalls() ||
        radioState == RIL.GECKO_RADIOSTATE_ENABLING ||
        radioState == RIL.GECKO_RADIOSTATE_DISABLING) {
      
      return;
    }

    if (DEBUG) {
      this.debug("Data call settings: connect data call.");
    }
    networkInterface.connect();
  },

  getDataCallStateByType: function(apnType) {
    let networkInterface = this.dataNetworkInterfaces.get(apnType);
    if (!networkInterface) {
      return RIL.GECKO_NETWORK_STATE_UNKNOWN;
    }
    return networkInterface.state;
  },

  setupDataCallByType: function(apnType) {
    if (DEBUG) {
      this.debug("setupDataCallByType: " + apnType);
    }
    let networkInterface = this.dataNetworkInterfaces.get(apnType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + apnType);
      }
      return;
    }

    networkInterface.connect();
  },

  deactivateDataCallByType: function(apnType) {
    if (DEBUG) {
      this.debug("deactivateDataCallByType: " + apnType);
    }
    let networkInterface = this.dataNetworkInterfaces.get(apnType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + apnType);
      }
      return;
    }

    networkInterface.disconnect();
  },

  deactivateDataCalls: function() {
    let dataDisconnecting = false;
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      if (networkInterface.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
        networkInterface.disconnect();
        dataDisconnecting = true;
      }
    });

    
    
    if (gRadioEnabledController.isDeactivatingDataCalls() && !dataDisconnecting) {
      gRadioEnabledController.finishDeactivatingDataCalls(this.clientId);
    }
  },

  


  handleDataCallError: function(message) {
    
    let networkInterface = this.dataNetworkInterfaces.get("default");
    if (networkInterface && networkInterface.enabled) {
      let apnSetting = networkInterface.apnSetting;
      if (message.apn == apnSetting.apn) {
        gMobileConnectionService.notifyDataError(this.clientId, message);
      }
    }

    this._deliverDataCallMessage("dataCallError", [message]);
  },

  


  handleDataCallState: function(datacall) {
    this._deliverDataCallMessage("dataCallStateChanged", [datacall]);

    
    
    if (datacall.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED &&
        this.allDataDisconnected()) {
      if (gRadioEnabledController.isDeactivatingDataCalls()) {
        if (DEBUG) {
          this.debug("All data connections are disconnected.");
        }
        gRadioEnabledController.finishDeactivatingDataCalls(this.clientId);
      }

      if (this._pendingApnSettings) {
        if (DEBUG) {
          this.debug("Setup pending apn settings.");
        }
        this._setupApnSettings(this._pendingApnSettings);
        this._pendingApnSettings = null;
        this.updateRILNetworkInterface();
      }

      if (gDataConnectionManager.isSwitchingDataClientId()) {
        gDataConnectionManager.notifyDataCallStateChange(this.clientId);
      }
    }
  },
};

function RadioInterfaceLayer() {
  let workerMessenger = new WorkerMessenger();
  workerMessenger.init();
  this.setWorkerDebugFlag = workerMessenger.setDebugFlag.bind(workerMessenger);

  let numIfaces = this.numRadioInterfaces;
  if (DEBUG) debug(numIfaces + " interfaces");
  this.radioInterfaces = [];
  for (let clientId = 0; clientId < numIfaces; clientId++) {
    this.radioInterfaces.push(new RadioInterface(clientId, workerMessenger));
  }

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);

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

  



  observe: function(subject, topic, data) {
    switch (topic) {
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        for (let radioInterface of this.radioInterfaces) {
          radioInterface.shutdown();
        }
        this.radioInterfaces = null;
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;

      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (data === kPrefRilDebuggingEnabled) {
          updateDebugFlag();
          this.setWorkerDebugFlag(DEBUG);
        }
        break;
    }
  },

  



  getRadioInterface: function(clientId) {
    return this.radioInterfaces[clientId];
  },

  getClientIdForEmergencyCall: function() {
    for (let cid = 0; cid < this.numRadioInterfaces; ++cid) {
      if (gRadioEnabledController._isRadioAbleToEnableAtClient(cid)) {
        return cid;
      }
    }
    return -1;
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

function WorkerMessenger() {
  
  this.radioInterfaces = [];
  this.tokenCallbackMap = {};

  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);
}
WorkerMessenger.prototype = {
  radioInterfaces: null,
  worker: null,

  
  token: 1,

  
  tokenCallbackMap: null,

  init: function() {
    let options = {
      debug: DEBUG,
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
        dataRegistrationOnDemand: RILQUIRKS_DATA_REGISTRATION_ON_DEMAND,
        subscriptionControl: RILQUIRKS_SUBSCRIPTION_CONTROL
      }
    };

    this.send(null, "setInitialOptions", options);
  },

  setDebugFlag: function(aDebug) {
    let options = { debug: aDebug };
    this.send(null, "setDebugFlag", options);
  },

  debug: function(aClientId, aMessage) {
    
    dump("-*- RadioInterface[" + aClientId + "]: " + aMessage + "\n");
  },

  onerror: function(event) {
    if (DEBUG) {
      this.debug("X", "Got an error: " + event.filename + ":" +
                 event.lineno + ": " + event.message + "\n");
    }
    event.preventDefault();
  },

  


  onmessage: function(event) {
    let message = event.data;
    let clientId = message.rilMessageClientId;
    if (clientId === null) {
      return;
    }

    if (DEBUG) {
      this.debug(clientId, "Received message from worker: " + JSON.stringify(message));
    }

    let token = message.rilMessageToken;
    if (token == null) {
      
      let radioInterface = this.radioInterfaces[clientId];
      radioInterface.handleUnsolicitedWorkerMessage(message);
      return;
    }

    let callback = this.tokenCallbackMap[message.rilMessageToken];
    if (!callback) {
      if (DEBUG) this.debug(clientId, "Ignore orphan token: " + message.rilMessageToken);
      return;
    }

    let keep = false;
    try {
      keep = callback(message);
    } catch(e) {
      if (DEBUG) this.debug(clientId, "callback throws an exception: " + e);
    }

    if (!keep) {
      delete this.tokenCallbackMap[message.rilMessageToken];
    }
  },

  registerClient: function(aClientId, aRadioInterface) {
    if (DEBUG) this.debug(aClientId, "Starting RIL Worker");

    
    this.radioInterfaces[aClientId] = aRadioInterface;

    this.send(null, "registerClient", { clientId: aClientId });
    gSystemWorkerManager.registerRilWorker(aClientId, this.worker);
  },

  














  send: function(clientId, rilMessageType, message, callback) {
    message = message || {};

    message.rilMessageClientId = clientId;
    message.rilMessageToken = this.token;
    this.token++;

    if (callback) {
      
      
      
      
      
      
      
      this.tokenCallbackMap[message.rilMessageToken] = callback;
    }

    message.rilMessageType = rilMessageType;
    this.worker.postMessage(message);
  },

  











  sendWithIPCMessage: function(clientId, msg, rilMessageType, ipcType) {
    this.send(clientId, rilMessageType, msg.json.data, (function(reply) {
      ipcType = ipcType || msg.name;
      msg.target.sendAsyncMessage(ipcType, {
        clientId: clientId,
        data: reply
      });
      return false;
    }).bind(this));
  }
};

function RadioInterface(aClientId, aWorkerMessenger) {
  this.clientId = aClientId;
  this.workerMessenger = {
    send: aWorkerMessenger.send.bind(aWorkerMessenger, aClientId),
    sendWithIPCMessage:
      aWorkerMessenger.sendWithIPCMessage.bind(aWorkerMessenger, aClientId),
  };
  aWorkerMessenger.registerClient(aClientId, this);

  this.rilContext = {
    cardState:      RIL.GECKO_CARDSTATE_UNKNOWN,
    iccInfo:        null,
    imsi:           null
  };

  this.voicemailInfo = {
    number: null,
    displayName: null
  };

  this.operatorInfo = {};

  let lock = gSettingsService.createLock();

  
  
  lock.get(kSettingsClockAutoUpdateEnabled, this);

  
  
  lock.get(kSettingsTimezoneAutoUpdateEnabled, this);

  
  this.setClockAutoUpdateAvailable(false);

  
  this.setTimezoneAutoUpdateAvailable(false);

  







  lock.get(kSettingsCellBroadcastDisabled, this);

  













  lock.get(kSettingsCellBroadcastSearchList, this);

  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.obs.addObserver(this, kNetworkConnStateChangedTopic, false);

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] = this.handleSmsWdpPortPush.bind(this);

  this._receivedSmsSegmentsMap = {};

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

  shutdown: function() {
    
    this._releaseSmsHandledWakeLock();

    Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
    Services.obs.removeObserver(this, kSysClockChangeObserverTopic);
    Services.obs.removeObserver(this, kScreenStateChangedTopic);
    Services.obs.removeObserver(this, kNetworkConnStateChangedTopic);
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

  


  receiveMessage: function(msg) {
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
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
      case "RIL:GetVoicemailInfo":
        
        return this.voicemailInfo;
    }
    return null;
  },

  handleUnsolicitedWorkerMessage: function(message) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    switch (message.rilMessageType) {
      case "callRing":
        gTelephonyService.notifyCallRing();
        break;
      case "callStateChange":
        gTelephonyService.notifyCallStateChanged(this.clientId, message.call);
        break;
      case "callDisconnected":
        gTelephonyService.notifyCallDisconnected(this.clientId, message.call);
        break;
      case "conferenceCallStateChanged":
        gTelephonyService.notifyConferenceCallStateChanged(message.state);
        break;
      case "cdmaCallWaiting":
        gTelephonyService.notifyCdmaCallWaiting(this.clientId, message.waitingCall);
        break;
      case "suppSvcNotification":
        gTelephonyService.notifySupplementaryService(this.clientId,
                                                     message.callIndex,
                                                     message.notification);
        break;
      case "datacallerror":
        connHandler.handleDataCallError(message);
        break;
      case "datacallstatechange":
        let addresses = [];
        for (let i = 0; i < message.addresses.length; i++) {
          let [address, prefixLength] = message.addresses[i].split("/");
          
          
          addresses.push({
            address: address,
            prefixLength: prefixLength ? parseInt(prefixLength, 10) : 0
          });
        }
        message.addresses = addresses;
        connHandler.handleDataCallState(message);
        break;
      case "emergencyCbModeChange":
        gMobileConnectionService.notifyEmergencyCallbackModeChanged(this.clientId,
                                                                    message);
        break;
      case "networkinfochanged":
        gMobileConnectionService.notifyNetworkInfoChanged(this.clientId,
                                                          message);
        connHandler.updateRILNetworkInterface();
        break;
      case "networkselectionmodechange":
        gMobileConnectionService.notifyNetworkSelectModeChanged(this.clientId,
                                                                message.mode);
        break;
      case "voiceregistrationstatechange":
        gMobileConnectionService.notifyVoiceInfoChanged(this.clientId, message);
        break;
      case "dataregistrationstatechange":
        gMobileConnectionService.notifyDataInfoChanged(this.clientId, message);
        connHandler.updateRILNetworkInterface();
        break;
      case "signalstrengthchange":
        gMobileConnectionService.notifySignalStrengthChanged(this.clientId,
                                                             message);
        break;
      case "operatorchange":
        gMobileConnectionService.notifyOperatorChanged(this.clientId, message);
        break;
      case "otastatuschange":
        gMobileConnectionService.notifyOtaStatusChanged(this.clientId, message.status);
        break;
      case "radiostatechange":
        
        
        gRadioEnabledController.notifyRadioStateChanged(this.clientId,
                                                        message.radioState);
        break;
      case "ussdreceived":
        gMobileConnectionService.notifyUssdReceived(this.clientId,
                                                    message.message,
                                                    message.sessionEnded);
        break;
      case "cardstatechange":
        this.rilContext.cardState = message.cardState;
        gRadioEnabledController.receiveCardState(this.clientId);
        gMessageManager.sendIccMessage("RIL:CardStateChanged",
                                       this.clientId, message);
        break;
      case "sms-received":
        this.handleSmsMultipart(message);
        break;
      case "cellbroadcast-received":
        message.timestamp = Date.now();
        this.broadcastCbsSystemMessage(message);
        gMessageManager.sendCellBroadcastMessage("RIL:CellBroadcastReceived",
                                                 this.clientId, message);
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
      case "stkcommand":
        this.handleStkProactiveCommand(message);
        break;
      case "stksessionend":
        gMessageManager.sendIccMessage("RIL:StkSessionEnd", this.clientId, null);
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

    if (!iccInfo) {
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

    if (!message.errorMsg) {
      switch (message.mvnoType) {
        case "imsi":
          if (!this.rilContext.imsi) {
            message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
            break;
          }
          message.result = this.isImsiMatches(message.mvnoData);
          break;
        case "spn":
          let spn = this.rilContext.iccInfo && this.rilContext.iccInfo.spn;
          if (!spn) {
            message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
            break;
          }
          message.result = spn == message.mvnoData;
          break;
        case "gid":
          this.workerMessenger.send("getGID1", null, (function(response) {
            let gid = response.gid1;
            let mvnoDataLength = message.mvnoData.length;

            if (!gid) {
              message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
            } else if (mvnoDataLength > gid.length) {
              message.result = false;
            } else {
              message.result =
                gid.substring(0, mvnoDataLength).toLowerCase() ==
                message.mvnoData.toLowerCase();
            }

            target.sendAsyncMessage("RIL:MatchMvno", {
              clientId: this.clientId,
              data: message
            });
          }).bind(this));
          return;
        default:
          message.errorMsg = RIL.GECKO_ERROR_MODE_NOT_SUPPORTED;
      }
    }

    target.sendAsyncMessage("RIL:MatchMvno", {
      clientId: this.clientId,
      data: message
    });
  },

  setCellBroadcastSearchList: function(settings) {
    let newSearchList =
      Array.isArray(settings) ? settings[this.clientId] : settings;
    let oldSearchList =
      Array.isArray(this._cellBroadcastSearchList) ?
        this._cellBroadcastSearchList[this.clientId] :
        this._cellBroadcastSearchList;

    if ((newSearchList == oldSearchList) ||
          (newSearchList && oldSearchList &&
            newSearchList.gsm == oldSearchList.gsm &&
            newSearchList.cdma == oldSearchList.cdma)) {
      return;
    }

    this.workerMessenger.send("setCellBroadcastSearchList",
                              { searchList: newSearchList },
                              (function callback(response) {
      if (!response.success) {
        let lock = gSettingsService.createLock();
        lock.set(kSettingsCellBroadcastSearchList,
                 this._cellBroadcastSearchList, null);
      } else {
        this._cellBroadcastSearchList = settings;
      }

      return false;
    }).bind(this));
  },

  setDataRegistration: function(attach) {
    let deferred = Promise.defer();
    this.workerMessenger.send("setDataRegistration",
                              {attach: attach},
                              (function(response) {
      
      deferred.resolve(response.errorMsg ? response.errorMsg : null);
    }).bind(this));

    return deferred.promise;
  },

  



  updateRILNetworkInterface: function() {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.updateRILNetworkInterface();
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
      sourcePort: message.originatorPort,
      destinationAddress: this.rilContext.iccInfo.msisdn,
      destinationPort: message.destinationPort,
      serviceId: this.clientId
    };
    WAP.WapPushManager.receiveWdpPDU(message.fullData, message.fullData.length,
                                     0, options);
  },

  








  broadcastSmsSystemMessage: function(aName, aDomMessage) {
    if (DEBUG) this.debug("Broadcasting the SMS system message: " + aName);

    
    
    
    gSystemMessenger.broadcastMessage(aName, {
      iccId:             aDomMessage.iccId,
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

  _acquireSmsHandledWakeLock: function() {
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
  },

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

  




  _receivedSmsSegmentsMap: null,

  





  _processReceivedSmsSegment: function(aSegment) {

    
    if (!(aSegment.segmentMaxSeq && (aSegment.segmentMaxSeq > 1))) {
      if (aSegment.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
        aSegment.fullData = aSegment.data;
      } else {
        aSegment.fullBody = aSegment.body;
      }
      return aSegment;
    }

    
    let hash = aSegment.sender + ":" +
               aSegment.segmentRef + ":" +
               aSegment.segmentMaxSeq;
    let seq = aSegment.segmentSeq;

    let options = this._receivedSmsSegmentsMap[hash];
    if (!options) {
      options = aSegment;
      this._receivedSmsSegmentsMap[hash] = options;

      options.receivedSegments = 0;
      options.segments = [];
    } else if (options.segments[seq]) {
      
      if (DEBUG) {
        this.debug("Got duplicated segment no." + seq +
                           " of a multipart SMS: " + JSON.stringify(aSegment));
      }
      return null;
    }

    if (options.receivedSegments > 0) {
      
      options.timestamp = aSegment.timestamp;
    }

    if (options.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      options.segments[seq] = aSegment.data;
    } else {
      options.segments[seq] = aSegment.body;
    }
    options.receivedSegments++;

    
    
    
    
    
    if (aSegment.teleservice === RIL.PDU_CDMA_MSG_TELESERIVCIE_ID_WAP
        && seq === 1) {
      if (!options.originatorPort && aSegment.originatorPort) {
        options.originatorPort = aSegment.originatorPort;
      }

      if (!options.destinationPort && aSegment.destinationPort) {
        options.destinationPort = aSegment.destinationPort;
      }
    }

    if (options.receivedSegments < options.segmentMaxSeq) {
      if (DEBUG) {
        this.debug("Got segment no." + seq + " of a multipart SMS: " +
                           JSON.stringify(options));
      }
      return null;
    }

    
    delete this._receivedSmsSegmentsMap[hash];

    
    if (options.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      
      let fullDataLen = 0;
      for (let i = 1; i <= options.segmentMaxSeq; i++) {
        fullDataLen += options.segments[i].length;
      }

      options.fullData = new Uint8Array(fullDataLen);
      for (let d= 0, i = 1; i <= options.segmentMaxSeq; i++) {
        let data = options.segments[i];
        for (let j = 0; j < data.length; j++) {
          options.fullData[d++] = data[j];
        }
      }
    } else {
      options.fullBody = options.segments.join("");
    }

    
    delete options.receivedSegments;
    delete options.segments;

    if (DEBUG) {
      this.debug("Got full multipart SMS: " + JSON.stringify(options));
    }

    return options;
  },

  


  _createSavableSmsSegment: function(aMessage) {
    
    
    let segment = {};
    segment.messageType = aMessage.messageType;
    segment.teleservice = aMessage.teleservice;
    segment.SMSC = aMessage.SMSC;
    segment.sentTimestamp = aMessage.sentTimestamp;
    segment.timestamp = Date.now();
    segment.sender = aMessage.sender;
    segment.pid = aMessage.pid;
    segment.encoding = aMessage.encoding;
    segment.messageClass = aMessage.messageClass;
    segment.iccId = this.getIccId();
    if (aMessage.header) {
      segment.segmentRef = aMessage.header.segmentRef;
      segment.segmentSeq = aMessage.header.segmentSeq;
      segment.segmentMaxSeq = aMessage.header.segmentMaxSeq;
      segment.originatorPort = aMessage.header.originatorPort;
      segment.destinationPort = aMessage.header.destinationPort;
    }
    segment.mwiPresent = (aMessage.mwi)? true: false;
    segment.mwiDiscard = (segment.mwiPresent)? aMessage.mwi.discard: false;
    segment.mwiMsgCount = (segment.mwiPresent)? aMessage.mwi.msgCount: 0;
    segment.mwiActive = (segment.mwiPresent)? aMessage.mwi.active: false;
    segment.serviceCategory = aMessage.serviceCategory;
    segment.language = aMessage.language;
    segment.data = aMessage.data;
    segment.body = aMessage.body;

    return segment;
  },

  





  _purgeCompleteSmsMessage: function(aMessage) {
    
    delete aMessage.segmentRef;
    delete aMessage.segmentSeq;
    delete aMessage.segmentMaxSeq;

    
    delete aMessage.data;
    delete aMessage.body;
  },

  


  handleSmsMultipart: function(aMessage) {
    if (DEBUG) this.debug("handleSmsMultipart: " + JSON.stringify(aMessage));

    this._acquireSmsHandledWakeLock();

    let segment = this._createSavableSmsSegment(aMessage);

    let isMultipart = (segment.segmentMaxSeq && (segment.segmentMaxSeq > 1));
    let messageClass = segment.messageClass;

    let handleReceivedAndAck = function(aRvOfIncompleteMsg, aCompleteMessage) {
      if (aCompleteMessage) {
        this._purgeCompleteSmsMessage(aCompleteMessage);
        if (this.handleSmsReceived(aCompleteMessage)) {
          this.sendAckSms(Cr.NS_OK, aCompleteMessage);
        }
        
      } else {
        this.sendAckSms(aRvOfIncompleteMsg, segment);
      }
    }.bind(this);

    
    if (!isMultipart ||
        (messageClass == RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0])) {
      
      
      
      
      
      
      

      handleReceivedAndAck(Cr.NS_OK,  
                           this._processReceivedSmsSegment(segment));
    } else {
      gMobileMessageDatabaseService
        .saveSmsSegment(segment, function notifyResult(aRv, aCompleteMessage) {
        handleReceivedAndAck(aRv,  
                             aCompleteMessage);
      });
    }
  },

  portAddressedSmsApps: null,
  handleSmsReceived: function(message) {
    if (DEBUG) this.debug("handleSmsReceived: " + JSON.stringify(message));

    if (message.messageType == RIL.PDU_CDMA_MSG_TYPE_BROADCAST) {
      gMessageManager.sendCellBroadcastMessage("RIL:CellBroadcastReceived",
                                               this.clientId, message);
      return true;
    }

    
    
    
    if (message.destinationPort != null) {
      let handler = this.portAddressedSmsApps[message.destinationPort];
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

    if (message.mwiPresent) {
      let mwi = {
        discard: message.mwiDiscard,
        msgCount: message.mwiMsgCount,
        active: message.mwiActive
      };
      this.workerMessenger.send("updateMwis", { mwi: mwi });

      mwi.returnNumber = message.sender;
      mwi.returnMessage = message.fullBody;
      gMessageManager.sendVoicemailMessage("RIL:VoicemailNotification",
                                           this.clientId, mwi);

      
      
      if (message.mwiDiscard) {
        return true;
      }
    }

    let notifyReceived = function notifyReceived(rv, domMessage) {
      let success = Components.isSuccessCode(rv);

      this.sendAckSms(rv, message);

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

  


  sendAckSms: function(aRv, aMessage) {
    if (aMessage.messageClass === RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_2]) {
      return;
    }

    let result = RIL.PDU_FCS_OK;
    if (!Components.isSuccessCode(aRv)) {
      if (DEBUG) this.debug("Failed to handle received sms: " + aRv);
      result = (aRv === Cr.NS_ERROR_FILE_NO_DEVICE_SPACE)
                ? RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED
                : RIL.PDU_FCS_UNSPECIFIED;
    }

    this.workerMessenger.send("ackSMS", { result: result });

  },

  








  broadcastCbsSystemMessage: function(aMessage) {
    
    
    let etws = (aMessage.etws != null)
               ? {
                    warningType: (aMessage.etws.warningType != null)
                                 ? RIL.CB_ETWS_WARNING_TYPE_NAMES[aMessage.etws.warningType]
                                 : null,
                    emergencyUserAlert: aMessage.etws.emergencyUserAlert,
                    popup: aMessage.etws.popup
                 }
               : null;

    let systemMessage = {
      serviceId: this.clientId,
      gsmGeographicalScope: RIL.CB_GSM_GEOGRAPHICAL_SCOPE_NAMES[aMessage.geographicalScope],
      messageCode: aMessage.messageCode,
      messageId: aMessage.messageId,
      language: aMessage.language,
      body: aMessage.fullBody,
      messageClass: aMessage.messageClass,
      timestamp: aMessage.timestamp,
      etws: etws,
      cdmaServiceCategory: aMessage.serviceCategory
    };

    if (DEBUG) {
      this.debug("CBS system message to be broadcasted: " + JSON.stringify(systemMessage));
    }

    gSystemMessenger.broadcastMessage("cellbroadcast-received", systemMessage);
  },

  


  setClockAutoUpdateAvailable: function(value) {
    gSettingsService.createLock().set(kSettingsClockAutoUpdateAvailable, value, null);
  },

  


  setTimezoneAutoUpdateAvailable: function(value) {
    gSettingsService.createLock().set(kSettingsTimezoneAutoUpdateAvailable, value, null);
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

    if (!message || !message.iccid) {
      
      this.rilContext.iccInfo = null;
    } else {
      if (!this.rilContext.iccInfo) {
        if (message.iccType === "ruim" || message.iccType === "csim") {
          this.rilContext.iccInfo = new CdmaIccInfo();
        } else if (message.iccType === "sim" || message.iccType === "usim") {
          this.rilContext.iccInfo = new GsmIccInfo();
        } else {
          this.rilContext.iccInfo = new IccInfo();
        }
      }

      if (!this.isInfoChanged(message, this.rilContext.iccInfo)) {
        return;
      }

      this.updateInfo(message, this.rilContext.iccInfo);
    }

    
    
    gMessageManager.sendIccMessage("RIL:IccInfoChanged",
                                   this.clientId,
                                   message.iccid ? message : null);

    
    
    gMobileConnectionService.notifyIccChanged(this.clientId,
                                              message.iccid || null);

    
    if (message.mcc) {
      try {
        Services.prefs.setCharPref("ril.lastKnownSimMcc",
                                   message.mcc.toString());
      } catch (e) {}
    }

    
    if (message.mcc && message.mnc) {
      let lastKnownHomeNetwork = message.mcc + "-" + message.mnc;
      
      if (message.spn) {
        lastKnownHomeNetwork += "-" + message.spn;
      }

      gMobileConnectionService.notifyLastHomeNetworkChanged(this.clientId,
                                                            lastKnownHomeNetwork);
    }

    
    if (!oldSpn && message.spn) {
      gMobileConnectionService.notifySpnAvailable(this.clientId);
    }
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

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        let setting = JSON.parse(data);
        this.handleSettingsChange(setting.key, setting.value, setting.isInternalChange);
        break;
      case kSysClockChangeObserverTopic:
        let offset = parseInt(data, 10);
        if (this._lastNitzMessage) {
          this._lastNitzMessage.receiveTimeInMS += offset;
        }
        this._sntp.updateOffset(offset);
        break;
      case kNetworkConnStateChangedTopic:
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

  
  
  _clockAutoUpdateEnabled: null,

  
  
  _timezoneAutoUpdateEnabled: null,

  
  
  _lastNitzMessage: null,

  
  _sntp: null,

  
  _cellBroadcastSearchList: null,

  handleSettingsChange: function(aName, aResult, aIsInternalSetting) {
    
    
    if (aName === kSettingsClockAutoUpdateAvailable &&
        !aIsInternalSetting) {
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
        !aIsInternalSetting) {
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
        } else {
          
          let buildTime = libcutils.property_get("ro.build.date.utc", "0") * 1000;
          let file = FileUtils.File("/system/b2g/b2g");
          if (file.lastModifiedTime > buildTime) {
            buildTime = file.lastModifiedTime;
          }
          if (buildTime > Date.now()) {
            gTimeService.set(buildTime);
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
          this.debug("'" + kSettingsCellBroadcastSearchList +
            "' is now " + JSON.stringify(aResult));
        }

        this.setCellBroadcastSearchList(aResult);
        break;
      case kSettingsCellBroadcastDisabled:
        if (DEBUG) {
          this.debug("'" + kSettingsCellBroadcastDisabled +
            "' is now " + JSON.stringify(aResult));
        }

        let setCbsDisabled =
          Array.isArray(aResult) ? aResult[this.clientId] : aResult;
        this.workerMessenger.send("setCellBroadcastDisabled",
                                  { disabled: setCbsDisabled });
        break;
    }
  },

  handleError: function(aErrorMessage) {
    if (DEBUG) {
      this.debug("There was an error while reading RIL settings.");
    }
  },

  

  rilContext: null,

  




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

    request.notifySegmentInfoForTextGot(options.segmentMaxSeq,
                                        options.segmentChars,
                                        options.segmentChars - charsInLastSegment);
  },

  getSmscAddress: function(request) {
    this.workerMessenger.send("getSmscAddress",
                              null,
                              (function(response) {
      if (!response.errorMsg) {
        request.notifyGetSmscAddress(response.smscAddress);
      } else {
        request.notifyGetSmscAddressFailed(Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR);
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
      if (!Components.isSuccessCode(rv)) {
        if (DEBUG) this.debug("Error! Fail to save sending message! rv = " + rv);
        request.notifySendMessageFailed(
          gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(rv),
          domMessage);
        Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
        return;
      }

      if (!silent) {
        Services.obs.notifyObservers(domMessage, kSmsSendingObserverTopic, null);
      }

      
      
      let errorCode;
      let radioState = gMobileConnectionService.getRadioState(this.clientId);
      if (!PhoneNumberUtils.isPlainPhoneNumber(options.number)) {
        if (DEBUG) this.debug("Error! Address is invalid when sending SMS: " +
                              options.number);
        errorCode = Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
      } else if (radioState == RIL.GECKO_RADIOSTATE_DISABLED) {
        if (DEBUG) this.debug("Error! Radio is disabled when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
      } else if (this.rilContext.cardState != "ready") {
        if (DEBUG) this.debug("Error! SIM card is not ready when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
      }
      if (errorCode) {
        if (silent) {
          request.notifySendMessageFailed(errorCode, domMessage);
          return;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(domMessage.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                         RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                         null,
                                         function notifyResult(rv, domMessage) {
          
          request.notifySendMessageFailed(errorCode, domMessage);
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
            
            
            
            let sms = context.sms;
            context.request.notifySendMessageFailed(
              error,
              gMobileMessageService.createSmsMessage(sms.id,
                                                     sms.threadId,
                                                     sms.iccId,
                                                     DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                                     RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                                     sms.sender,
                                                     sms.receiver,
                                                     sms.body,
                                                     sms.messageClass,
                                                     sms.timestamp,
                                                     0,
                                                     0,
                                                     sms.read));
            return false;
          }

          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(context.sms.id,
                                           null,
                                           DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                           RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                           null,
                                           function notifyResult(rv, domMessage) {
            
            context.request.notifySendMessageFailed(error, domMessage);
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

  
  
  setupDataCallByType: function(apntype) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.setupDataCallByType(apntype);
  },

  
  
  deactivateDataCallByType: function(apntype) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.deactivateDataCallByType(apntype);
  },

  
  getDataCallStateByType: function(apntype) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    return connHandler.getDataCallStateByType(apntype);
  },

  sendWorkerMessage: function(rilMessageType, message, callback) {
    
    if (rilMessageType === "setRadioEnabled") {
      
      gRadioEnabledController.setRadioEnabled(this.clientId, message,
                                              callback.handleResponse);
      return;
    }

    if (callback) {
      this.workerMessenger.send(rilMessageType, message, function(response) {
        return callback.handleResponse(response);
      });
    } else {
      this.workerMessenger.send(rilMessageType, message);
    }
  },

  getCellInfoList: function(callback) {
    this.workerMessenger.send("getCellInfoList",
                              null,
                              function(response) {
      if (response.errorMsg) {
        callback.notifyGetCellInfoListFailed(response.errorMsg);
        return;
      }

      let cellInfoList = [];
      let count = response.result.length;
      for (let i = 0; i < count; i++) {
        let srcCellInfo = response.result[i];
        let cellInfo;
        switch (srcCellInfo.type) {
          case RIL.CELL_INFO_TYPE_GSM:
            cellInfo = new GsmCellInfo();
            break;
          case RIL.CELL_INFO_TYPE_WCDMA:
            cellInfo = new WcdmaCellInfo();
            break;
          case RIL.CELL_INFO_TYPE_LTE:
            cellInfo = new LteCellInfo();
            break;
          case RIL.CELL_INFO_TYPE_CDMA:
            cellInfo = new CdmaCellInfo();
            break;
        }

        if (!cellInfo) {
          continue;
        }
        this.updateInfo(srcCellInfo, cellInfo);
        cellInfoList.push(cellInfo);
      }
      callback.notifyGetCellInfoList(cellInfoList);
    }.bind(this));
  },

  getNeighboringCellIds: function(callback) {
    this.workerMessenger.send("getNeighboringCellIds",
                              null,
                              function(response) {
      if (response.errorMsg) {
        callback.notifyGetNeighboringCellIdsFailed(response.errorMsg);
        return;
      }

      let neighboringCellIds = [];
      let count = response.result.length;
      for (let i = 0; i < count; i++) {
        let srcCellInfo = response.result[i];
        let cellInfo = new NeighboringCellInfo();
        this.updateInfo(srcCellInfo, cellInfo);
        neighboringCellIds.push(cellInfo);
      }
      callback.notifyGetNeighboringCellIds(neighboringCellIds);

    }.bind(this));
  }
};

function DataCall(clientId, apnSetting) {
  this.clientId = clientId;
  this.apnProfile = {
    apn: apnSetting.apn,
    user: apnSetting.user,
    password: apnSetting.password,
    authType: apnSetting.authtype,
    protocol: apnSetting.protocol,
    roaming_protocol: apnSetting.roaming_protocol
  };
  this.linkInfo = {
    cid: null,
    ifname: null,
    ips: [],
    prefixLengths: [],
    dnses: [],
    gateways: []
  };
  this.state = RIL.GECKO_NETWORK_STATE_UNKNOWN;
  this.requestedNetworkIfaces = [];
}

DataCall.prototype = {
  



  NETWORK_APNRETRY_FACTOR: 8,
  NETWORK_APNRETRY_ORIGIN: 3,
  NETWORK_APNRETRY_MAXRETRIES: 10,

  
  timer: null,

  
  apnRetryCounter: 0,

  
  requestedNetworkIfaces: null,

  dataCallError: function(message) {
    if (DEBUG) this.debug("Data call error on APN: " + message.apn);
    this.state = RIL.GECKO_NETWORK_STATE_DISCONNECTED;
    this.retry();
  },

  dataCallStateChanged: function(datacall) {
    if (DEBUG) {
      this.debug("Data call ID: " + datacall.cid + ", interface name: " +
                 datacall.ifname + ", APN name: " + datacall.apn + ", state: " +
                 datacall.state);
    }

    if (this.state == datacall.state &&
        datacall.state != RIL.GECKO_NETWORK_STATE_CONNECTED) {
      return;
    }

    switch (datacall.state) {
      case RIL.GECKO_NETWORK_STATE_CONNECTED:
        if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTING) {
          this.linkInfo.cid = datacall.cid;

          if (this.requestedNetworkIfaces.length === 0) {
            if (DEBUG) {
              this.debug("State is connected, but no network interface requested" +
                         " this DataCall");
            }
            this.deactivate();
            return;
          }

          this.linkInfo.ifname = datacall.ifname;
          for (let entry of datacall.addresses) {
            this.linkInfo.ips.push(entry.address);
            this.linkInfo.prefixLengths.push(entry.prefixLength);
          }
          this.linkInfo.gateways = datacall.gateways.slice();
          this.linkInfo.dnses = datacall.dnses.slice();

        } else if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          
          let changed = false;
          if (this.linkInfo.ips.length != datacall.addresses.length) {
            changed = true;
            this.linkInfo.ips = [];
            this.linkInfo.prefixLengths = [];
            for (let entry of datacall.addresses) {
              this.linkInfo.ips.push(entry.address);
              this.linkInfo.prefixLengths.push(entry.prefixLength);
            }
          }

          let reduceFunc = function(aRhs, aChanged, aElement, aIndex) {
            return aChanged || (aElement != aRhs[aIndex]);
          };
          for (let field of ["gateways", "dnses"]) {
            let lhs = this.linkInfo[field], rhs = datacall[field];
            if (lhs.length != rhs.length ||
                lhs.reduce(reduceFunc.bind(null, rhs), false)) {
              changed = true;
              this.linkInfo[field] = rhs.slice();
            }
          }
          if (!changed) {
            return;
          }
        }
        break;
      case RIL.GECKO_NETWORK_STATE_DISCONNECTED:
      case RIL.GECKO_NETWORK_STATE_UNKNOWN:
        if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          
          this.state = datacall.state;
          for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
            this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
          }
        }
        this.reset();

        if (this.requestedNetworkIfaces.length > 0) {
          if (DEBUG) {
            this.debug("State is disconnected/unknown, but this DataCall is" +
                       " requested.");
          }
          this.setup();
          return;
        }
        break;
    }

    this.state = datacall.state;
    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
    }
  },

  

  debug: function(s) {
    dump("-*- DataCall[" + this.clientId + ":" + this.apnProfile.apn + "]: " +
      s + "\n");
  },

  get connected() {
    return this.state == RIL.GECKO_NETWORK_STATE_CONNECTED;
  },

  inRequestedTypes: function(type) {
    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      if (this.requestedNetworkIfaces[i].type == type) {
        return true;
      }
    }
    return false;
  },

  canHandleApn: function(apnSetting) {
    
    return (this.apnProfile.apn == apnSetting.apn &&
            (this.apnProfile.user || '') == (apnSetting.user || '') &&
            (this.apnProfile.password || '') == (apnSetting.password || ''));
  },

  reset: function() {
    this.linkInfo.cid = null;
    this.linkInfo.ifname = null;
    this.linkInfo.ips = [];
    this.linkInfo.prefixLengths = [];
    this.linkInfo.dnses = [];
    this.linkInfo.gateways = [];

    this.state = RIL.GECKO_NETWORK_STATE_UNKNOWN;
  },

  connect: function(networkInterface) {
    if (DEBUG) this.debug("connect: " + networkInterface.type);

    if (this.requestedNetworkIfaces.indexOf(networkInterface) == -1) {
      this.requestedNetworkIfaces.push(networkInterface);
    }

    if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTING ||
        this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTING) {
      return;
    }
    if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
      networkInterface.notifyRILNetworkInterface();
      return;
    }

    
    
    if (this.timer) {
      this.timer.cancel();
    }

    this.setup();
  },

  setup: function() {
    if (DEBUG) {
      this.debug("Going to set up data connection with APN " +
                 this.apnProfile.apn);
    }

    let radioInterface = this.gRIL.getRadioInterface(this.clientId);
    let dataInfo = gMobileConnectionService.getDataConnectionInfo(this.clientId);
    if (dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      return;
    }

    let radioTechType = dataInfo.type;
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);
    let authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(this.apnProfile.authtype);
    
    
    if (authType == -1) {
      if (DEBUG) {
        this.debug("Invalid authType " + this.apnProfile.authtype);
      }
      authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(RIL.GECKO_DATACALL_AUTH_DEFAULT);
    }
    let pdpType = RIL.GECKO_DATACALL_PDP_TYPE_IP;
    if (RILQUIRKS_HAVE_IPV6) {
      pdpType = !dataInfo.roaming
              ? this.apnProfile.protocol
              : this.apnProfile.roaming_protocol;
      if (RIL.RIL_DATACALL_PDP_TYPES.indexOf(pdpType) < 0) {
        if (DEBUG) {
          this.debug("Invalid pdpType '" + pdpType + "', using '" +
                     RIL.GECKO_DATACALL_PDP_TYPE_DEFAULT + "'");
        }
        pdpType = RIL.GECKO_DATACALL_PDP_TYPE_DEFAULT;
      }
    }
    radioInterface.sendWorkerMessage("setupDataCall", {
      radioTech: radioTechnology,
      apn: this.apnProfile.apn,
      user: this.apnProfile.user,
      passwd: this.apnProfile.password,
      chappap: authType,
      pdptype: pdpType
    });
    this.state = RIL.GECKO_NETWORK_STATE_CONNECTING;
  },

  retry: function() {
    let apnRetryTimer;

    
    
    if (this.apnRetryCounter >= this.NETWORK_APNRETRY_MAXRETRIES) {
      this.apnRetryCounter = 0;
      this.timer = null;
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

  disconnect: function(networkInterface) {
    if (DEBUG) this.debug("disconnect: " + networkInterface.type);
    let index = this.requestedNetworkIfaces.indexOf(networkInterface);
    if (index != -1) {
      this.requestedNetworkIfaces.splice(index, 1);

      if (this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED ||
          this.state == RIL.GECKO_NETWORK_STATE_UNKNOWN) {
        if (this.timer) {
          this.timer.cancel();
        }
        this.reset();
        return;
      }

      
      
      
      networkInterface.notifyRILNetworkInterface();
    }

    
    
    
    if (this.requestedNetworkIfaces.length > 0 ||
        this.state != RIL.GECKO_NETWORK_STATE_CONNECTED) {
      return;
    }

    this.deactivate();
  },

  deactivate: function() {
    let reason = RIL.DATACALL_DEACTIVATE_NO_REASON;
    if (DEBUG) {
      this.debug("Going to disconnet data connection cid " + this.linkInfo.cid);
    }
    let radioInterface = this.gRIL.getRadioInterface(this.clientId);
    radioInterface.sendWorkerMessage("deactivateDataCall", {
      cid: this.linkInfo.cid,
      reason: reason
    });
    this.state = RIL.GECKO_NETWORK_STATE_DISCONNECTING;
  },

  
  notify: function(timer) {
    this.setup();
  },

  shutdown: function() {
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }
  }
};

function RILNetworkInterface(dataConnectionHandler, type, apnSetting, dataCall) {
  if (!dataCall) {
    throw new Error("No dataCall for RILNetworkInterface: " + type);
  }

  this.dataConnectionHandler = dataConnectionHandler;
  this.type = type;
  this.apnSetting = apnSetting;
  this.dataCall = dataCall;

  this.enabled = false;
}

RILNetworkInterface.prototype = {
  classID:   RILNETWORKINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILNETWORKINTERFACE_CID,
                                    classDescription: "RILNetworkInterface",
                                    interfaces: [Ci.nsINetworkInterface,
                                                 Ci.nsIRilNetworkInterface]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface,
                                         Ci.nsIRilNetworkInterface]),

  
  dataCall: null,

  
  enabled: null,

  



  get state() {
    if (!this.dataCall.inRequestedTypes(this.type)) {
      return Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED;
    }
    return this.dataCall.state;
  },

  type: null,

  get name() {
    return this.dataCall.linkInfo.ifname;
  },

  get httpProxyHost() {
    return this.apnSetting.proxy || "";
  },

  get httpProxyPort() {
    return this.apnSetting.port || "";
  },

  getAddresses: function(ips, prefixLengths) {
    let linkInfo = this.dataCall.linkInfo;

    ips.value = linkInfo.ips.slice();
    prefixLengths.value = linkInfo.prefixLengths.slice();

    return linkInfo.ips.length;
  },

  getGateways: function(count) {
    let linkInfo = this.dataCall.linkInfo;

    if (count) {
      count.value = linkInfo.gateways.length;
    }
    return linkInfo.gateways.slice();
  },

  getDnses: function(count) {
    let linkInfo = this.dataCall.linkInfo;

    if (count) {
      count.value = linkInfo.dnses.length;
    }
    return linkInfo.dnses.slice();
  },

  



  get serviceId() {
    return this.dataConnectionHandler.clientId;
  },

  get iccId() {
    let iccInfo = this.dataConnectionHandler.radioInterface.rilContext.iccInfo;
    return iccInfo && iccInfo.iccid;
  },

  get mmsc() {
    if (this.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS) {
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
    if (this.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS) {
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
    if (this.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS) {
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
    dump("-*- RILNetworkInterface[" + this.dataConnectionHandler.clientId + ":" +
         this.type + "]: " + s + "\n");
  },

  apnSetting: null,

  get connected() {
    return this.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;
  },

  notifyRILNetworkInterface: function() {
    if (DEBUG) {
      this.debug("notifyRILNetworkInterface type: " + this.type + ", state: " +
                 this.state);
    }

    gNetworkManager.updateNetworkInterface(this);
  },

  connect: function() {
    this.enabled = true;

    this.dataCall.connect(this);
  },

  disconnect: function() {
    if (!this.enabled) {
      return;
    }
    this.enabled = false;

    this.dataCall.disconnect(this);
  },

  shutdown: function() {
    this.dataCall.shutdown();
    this.dataCall = null;
  }

};

XPCOMUtils.defineLazyServiceGetter(DataCall.prototype, "gRIL",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RadioInterfaceLayer]);
