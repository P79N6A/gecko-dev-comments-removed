














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

let RILQUIRKS_SIGNAL_EXTRA_INT32 =
  libcutils.property_get("ro.moz.ril.signal_extra_int", "false") == "true";

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const kNetworkConnStateChangedTopic      = "network-connection-state-changed";
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const kSysClockChangeObserverTopic       = "system-clock-change";
const kScreenStateChangedTopic           = "screen-state-changed";

const kSettingsClockAutoUpdateEnabled = "time.clock.automatic-update.enabled";
const kSettingsClockAutoUpdateAvailable = "time.clock.automatic-update.available";
const kSettingsTimezoneAutoUpdateEnabled = "time.timezone.automatic-update.enabled";
const kSettingsTimezoneAutoUpdateAvailable = "time.timezone.automatic-update.available";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";

const RADIO_POWER_OFF_TIMEOUT = 30000;
const HW_DEFAULT_CLIENT_ID = 0;

const INT32_MAX = 2147483647;

const NETWORK_TYPE_UNKNOWN     = Ci.nsINetworkInterface.NETWORK_TYPE_UNKNOWN;
const NETWORK_TYPE_WIFI        = Ci.nsINetworkInterface.NETWORK_TYPE_WIFI;
const NETWORK_TYPE_MOBILE      = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;
const NETWORK_TYPE_MOBILE_MMS  = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS;
const NETWORK_TYPE_MOBILE_SUPL = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL;
const NETWORK_TYPE_MOBILE_IMS  = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS;
const NETWORK_TYPE_MOBILE_DUN  = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN;


const RIL_IPC_ICCMANAGER_MSG_NAMES = [
  "RIL:GetRilContext",
  "RIL:SendStkResponse",
  "RIL:SendStkMenuSelection",
  "RIL:SendStkTimerExpiration",
  "RIL:SendStkEventDownload",
  "RIL:GetCardLockEnabled",
  "RIL:UnlockCardLock",
  "RIL:SetCardLockEnabled",
  "RIL:ChangeCardLockPassword",
  "RIL:GetCardLockRetryCount",
  "RIL:IccOpenChannel",
  "RIL:IccExchangeAPDU",
  "RIL:IccCloseChannel",
  "RIL:ReadIccContacts",
  "RIL:UpdateIccContact",
  "RIL:RegisterIccMsg",
  "RIL:MatchMvno",
  "RIL:GetServiceState"
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

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/gonkiccservice;1",
                                   "nsIGonkIccService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/gonksmsservice;1",
                                   "nsIGonkSmsService");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

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

XPCOMUtils.defineLazyServiceGetter(this, "gCellBroadcastService",
                                   "@mozilla.org/cellbroadcast/gonkservice;1",
                                   "nsIGonkCellBroadcastService");

XPCOMUtils.defineLazyServiceGetter(this, "gIccMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsIIccMessenger");

XPCOMUtils.defineLazyGetter(this, "gStkCmdFactory", function() {
  let stk = {};
  Cu.import("resource://gre/modules/StkProactiveCmdFactory.jsm", stk);
  return stk.StkProactiveCmdFactory;
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
    },

    _unregisterMessageListeners: function() {
      ppmm.removeMessageListener("child-process-shutdown", this);
      for (let msgName of RIL_IPC_ICCMANAGER_MSG_NAMES) {
        ppmm.removeMessageListener(msgName, this);
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
      } else {
        if (DEBUG) debug("Ignoring unknown message type: " + msg.name);
        return null;
      }

      switch (msg.name) {
        case "RIL:RegisterIccMsg":
          this._registerMessageTarget("icc", msg.target);
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
        if (_ril.getRadioInterface(i).isCardPresent()) {
          numCards++;
        }
      }
      return numCards;
    },

    _isRadioAbleToEnableAtClient: function(clientId, numCards) {
      if (!RILQUIRKS_RADIO_OFF_WO_CARD) {
        return true;
      }

      
      
      

      if (_ril.getRadioInterface(clientId).isCardPresent()) {
        return true;
      }

      numCards = numCards == null ? this._getNumCards() : numCards;
      if (clientId === HW_DEFAULT_CLIENT_ID && numCards === 0) {
        return true;
      }

      return false;
    },

    _handleMessage: function(message) {
      if (DEBUG) debug("RadioControl: handleMessage: " + JSON.stringify(message));
      let clientId = message.clientId || 0;
      let connection =
        gMobileConnectionService.getItemByServiceId(clientId);
      let radioState = connection && connection.radioState;

      if (message.data.enabled) {
        if (this._isRadioAbleToEnableAtClient(clientId)) {
          this._setRadioEnabledInternal(message);
        } else {
          
          message.callback(message.data);
        }

        this._processNextMessage();
      } else {
        _request = this._setRadioEnabledInternal.bind(this, message);

        
        
        
        
        
        let hangUpCallback = {
          QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyCallback]),
          notifySuccess: function() {},
          notifyError: function() {}
        };

        gTelephonyService.enumerateCalls({
          QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyListener]),
          enumerateCallState: function(aInfo) {
            gTelephonyService.hangUpCall(aInfo.clientId, aInfo.callIndex,
                                         hangUpCallback);
          },
          enumerateCallStateComplete: function() {}
        });

        
        
        
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

      radioInterface.workerMessenger.send("setRadioEnabled", message.data,
                                          (function(response) {
        if (response.errorMsg) {
          
          
          this.notifyRadioStateChanged(clientId,
                                       Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN);
        }
        return message.callback(response);
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

      if (oldConnHandler.deactivateDataCalls()) {
        this._pendingDataCallRequest = applyPendingDataSettings;
        if (DEBUG) {
          this.debug("_handleDataClientIdChange: existing data call(s) active" +
                     ", wait for them to get disconnected.");
        }
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
          if ("wrappedJSObject" in subject) {
            subject = subject.wrappedJSObject;
          }
          this.handle(subject.key, subject.value);
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
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccInfo]),

  

  iccType: null,
  iccid: null,
  mcc: null,
  mnc: null,
  spn: null,
  isDisplayNetworkNameRequired: false,
  isDisplaySpnRequired: false
};

function GsmIccInfo() {}
GsmIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGsmIccInfo,
                                         Ci.nsIIccInfo]),

  

  msisdn: null
};

function CdmaIccInfo() {}
CdmaIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICdmaIccInfo,
                                         Ci.nsIIccInfo]),

  

  mdn: null,
  prlVersion: 0
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
        return NETWORK_TYPE_MOBILE;
      case "mms":
        return NETWORK_TYPE_MOBILE_MMS;
      case "supl":
        return NETWORK_TYPE_MOBILE_SUPL;
      case "ims":
        return NETWORK_TYPE_MOBILE_IMS;
      case "dun":
        return NETWORK_TYPE_MOBILE_DUN;
      default:
        return NETWORK_TYPE_UNKNOWN;
     }
  },

  _compareDataCallOptions: function(dataCall, newDataCall) {
    return dataCall.apnProfile.apn == newDataCall.apnProfile.apn &&
           dataCall.apnProfile.user == newDataCall.apnProfile.user &&
           dataCall.apnProfile.password == newDataCall.apnProfile.passwd &&
           dataCall.chappap == newDataCall.chappap &&
           dataCall.pdptype == newDataCall.pdptype;
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
        if (networkType === NETWORK_TYPE_UNKNOWN) {
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
          this.dataNetworkInterfaces.set(networkType, networkInterface);
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
    let networkInterface = this.dataNetworkInterfaces.get(NETWORK_TYPE_MOBILE);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for default data.");
      }
      return;
    }

    let connection =
      gMobileConnectionService.getItemByServiceId(this.clientId);

    
    
    let radioState = connection && connection.radioState;
    if (radioState != Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
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

    let dataInfo = connection && connection.data;
    let isRegistered =
      dataInfo &&
      dataInfo.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;
    let haveDataConnection =
      dataInfo &&
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
        gNetworkManager.active.type == NETWORK_TYPE_WIFI) {
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

    if (gRadioEnabledController.isDeactivatingDataCalls()) {
      
      return;
    }

    if (DEBUG) {
      this.debug("Data call settings: connect data call.");
    }
    networkInterface.connect();
  },

  _isMobileNetworkType: function(networkType) {
    if (networkType === NETWORK_TYPE_MOBILE ||
        networkType === NETWORK_TYPE_MOBILE_MMS ||
        networkType === NETWORK_TYPE_MOBILE_SUPL ||
        networkType === NETWORK_TYPE_MOBILE_IMS ||
        networkType === NETWORK_TYPE_MOBILE_DUN) {
      return true;
    }

    return false;
  },

  getDataCallStateByType: function(networkType) {
    if (!this._isMobileNetworkType(networkType)) {
      if (DEBUG) this.debug(networkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(networkType);
    if (!networkInterface) {
      return RIL.GECKO_NETWORK_STATE_UNKNOWN;
    }
    return networkInterface.state;
  },

  setupDataCallByType: function(networkType) {
    if (DEBUG) {
      this.debug("setupDataCallByType: " + networkType);
    }

    if (!this._isMobileNetworkType(networkType)) {
      if (DEBUG) this.debug(networkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(networkType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + networkType);
      }
      return;
    }

    networkInterface.connect();
  },

  deactivateDataCallByType: function(networkType) {
    if (DEBUG) {
      this.debug("deactivateDataCallByType: " + networkType);
    }

    if (!this._isMobileNetworkType(networkType)) {
      if (DEBUG) this.debug(networkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(networkType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + networkType);
      }
      return;
    }

    networkInterface.disconnect();
  },

  deactivateDataCalls: function() {
    let dataDisconnecting = false;
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      if (networkInterface.enabled) {
        if (networkInterface.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          dataDisconnecting = true;
        }
        networkInterface.disconnect();
      }
    });

    
    
    if (gRadioEnabledController.isDeactivatingDataCalls() && !dataDisconnecting) {
      gRadioEnabledController.finishDeactivatingDataCalls(this.clientId);
    }

    return dataDisconnecting;
  },

  _findDataCallByCid: function(cid) {
    if (cid === undefined || cid < 0) {
      return -1;
    }

    for (let i = 0; i < this._dataCalls.length; i++) {
      let datacall = this._dataCalls[i];
      if (datacall.linkInfo.cid != null &&
          datacall.linkInfo.cid === cid) {
        return i;
      }
    }

    return -1;
  },

  


  handleDataCallListChanged: function(dataCallList) {
    let currentDataCalls = this._dataCalls.slice();
    for (let i = 0; i < dataCallList.length; i++) {
      let dataCall = dataCallList[i];
      let index = this._findDataCallByCid(dataCall.cid);
      if (index == -1) {
        if (DEBUG) {
          this.debug("Unexpected new data call: " + JSON.stringify(dataCall));
        }
        continue;
      }
      currentDataCalls[index].onDataCallChanged(dataCall);
      currentDataCalls[index] = null;
    }

    
    
    
    for (let i = 0; i < currentDataCalls.length; i++) {
      let currentDataCall = currentDataCalls[i];
      if (currentDataCall && currentDataCall.linkInfo.cid != null &&
          currentDataCall.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
        if (DEBUG) {
          this.debug("Expected data call missing: " + JSON.stringify(
            currentDataCall.apnProfile) + ", must have been DISCONNECTED.");
        }
        currentDataCall.onDataCallChanged({
          state: RIL.GECKO_NETWORK_STATE_DISCONNECTED
        });
      }
    }
  },

  


  notifyDataCallError: function(message) {
    
    let networkInterface = this.dataNetworkInterfaces.get(NETWORK_TYPE_MOBILE);
    if (networkInterface && networkInterface.enabled) {
      let dataCall = networkInterface.dataCall;
      
      
      if (message.cid !== undefined) {
        if (message.linkInfo.cid == dataCall.linkInfo.cid) {
          gMobileConnectionService.notifyDataError(this.clientId, message);
        }
      } else {
        if (this._compareDataCallOptions(dataCall, message)) {
          gMobileConnectionService.notifyDataError(this.clientId, message);
        }
      }
    }
  },

  


  notifyDataCallChanged: function(updatedDataCall) {
    
    
    if (updatedDataCall.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED ||
        updatedDataCall.state == RIL.GECKO_NETWORK_STATE_UNKNOWN &&
        this.allDataDisconnected()) {
      if (gRadioEnabledController.isDeactivatingDataCalls()) {
        if (DEBUG) {
          this.debug("All data calls are disconnected.");
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
                                         Ci.nsIRadioInterfaceLayer_new, 
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
      if (this.getRadioInterface(cid).isCardPresent()) {
        return cid;
      }
    }

    
    return HW_DEFAULT_CLIENT_ID;
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
        subscriptionControl: RILQUIRKS_SUBSCRIPTION_CONTROL,
        signalExtraInt: RILQUIRKS_SIGNAL_EXTRA_INT32
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
    cardState:      Ci.nsIIcc.CARD_STATE_UNKNOWN,
    iccInfo:        null,
    imsi:           null
  };

  this.operatorInfo = {};

  let lock = gSettingsService.createLock();

  
  
  lock.get(kSettingsClockAutoUpdateEnabled, this);

  
  
  lock.get(kSettingsTimezoneAutoUpdateEnabled, this);

  
  this.setClockAutoUpdateAvailable(false);

  
  this.setTimezoneAutoUpdateAvailable(false);

  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.obs.addObserver(this, kNetworkConnStateChangedTopic, false);

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

  isCardPresent: function() {
    let cardState = this.rilContext.cardState;
    return cardState !== Ci.nsIIcc.CARD_STATE_UNDETECTED &&
      cardState !== Ci.nsIIcc.CARD_STATE_UNKNOWN;
  },

  




  receiveMessage: function(msg) {
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
      case "RIL:GetCardLockEnabled":
        this.workerMessenger.sendWithIPCMessage(msg, "iccGetCardLockEnabled",
                                                "RIL:GetCardLockResult");
        break;
      case "RIL:UnlockCardLock":
        this.workerMessenger.sendWithIPCMessage(msg, "iccUnlockCardLock",
                                                "RIL:SetUnlockCardLockResult");
        break;
      case "RIL:SetCardLockEnabled":
        this.workerMessenger.sendWithIPCMessage(msg, "iccSetCardLockEnabled",
                                                "RIL:SetUnlockCardLockResult");
        break;
      case "RIL:ChangeCardLockPassword":
        this.workerMessenger.sendWithIPCMessage(msg, "iccChangeCardLockPassword",
                                                "RIL:SetUnlockCardLockResult");
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
      case "RIL:GetServiceState":
        this.workerMessenger.sendWithIPCMessage(msg, "getIccServiceState");
        break;
    }
    return null;
  },

  handleUnsolicitedWorkerMessage: function(message) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    switch (message.rilMessageType) {
      case "callRing":
        gTelephonyService.notifyCallRing();
        break;
      case "currentCalls":
        gTelephonyService.notifyCurrentCalls(this.clientId, message.calls);
        break;
      case "cdmaCallWaiting":
        gTelephonyService.notifyCdmaCallWaiting(this.clientId,
                                                message.waitingCall);
        break;
      case "suppSvcNotification":
        gTelephonyService.notifySupplementaryService(this.clientId,
                                                     message.number,
                                                     message.notification);
        break;
      case "ussdreceived":
        gTelephonyService.notifyUssdReceived(this.clientId, message.message,
                                             message.sessionEnded);
        break;
      case "datacalllistchanged":
        connHandler.handleDataCallListChanged(message.datacalls);
        break;
      case "emergencyCbModeChange":
        gMobileConnectionService.notifyEmergencyCallbackModeChanged(this.clientId,
                                                                    message.active,
                                                                    message.timeoutMs);
        break;
      case "networkinfochanged":
        gMobileConnectionService.notifyNetworkInfoChanged(this.clientId,
                                                          message);
        if (message[RIL.NETWORK_INFO_DATA_REGISTRATION_STATE]) {
          connHandler.updateRILNetworkInterface();
        }
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
      case "cardstatechange":
        this.rilContext.cardState = message.cardState;
        gRadioEnabledController.receiveCardState(this.clientId);
        gIccService.notifyCardStateChanged(this.clientId,
                                           this.rilContext.cardState);
        
        gMessageManager.sendIccMessage("RIL:CardStateChanged",
                                       this.clientId, message);
        break;
      case "sms-received":
        this.handleSmsReceived(message);
        break;
      case "cellbroadcast-received":
        this.handleCellbroadcastMessageReceived(message);
        break;
      case "nitzTime":
        this.handleNitzTime(message);
        break;
      case "iccinfochange":
        this.handleIccInfoChange(message);
        break;
      case "iccimsi":
        this.rilContext.imsi = message.imsi;
        gIccService.notifyImsiChanged(this.clientId, this.rilContext.imsi);
        break;
      case "iccmbdn":
        this.handleIccMbdn(message);
        break;
      case "iccmwis":
        this.handleIccMwis(message.mwi);
        break;
      case "stkcommand":
        this.handleStkProactiveCommand(message);
        break;
      case "stksessionend":
        
        gMessageManager.sendIccMessage("RIL:StkSessionEnd", this.clientId, null);
        break;
      case "cdma-info-rec-received":
        this.handleCdmaInformationRecords(message.records);
        break;
      default:
        throw new Error("Don't know about this message type: " +
                        message.rilMessageType);
    }
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

    if (!message || !message.mvnoData) {
      message.errorMsg = RIL.GECKO_ERROR_INVALID_PARAMETER;
    }

    if (!message.errorMsg) {
      switch (message.mvnoType) {
        case RIL.GECKO_CARDMVNO_TYPE_IMSI:
          if (!this.rilContext.imsi) {
            message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
            break;
          }
          message.result = this.isImsiMatches(message.mvnoData);
          break;
        case RIL.GECKO_CARDMVNO_TYPE_SPN:
          let spn = this.rilContext.iccInfo && this.rilContext.iccInfo.spn;
          if (!spn) {
            message.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
            break;
          }
          message.result = spn == message.mvnoData;
          break;
        case RIL.GECKO_CARDMVNO_TYPE_GID:
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

  


  handleSmsReceived: function(aMessage) {
    let header = aMessage.header;
    
    
    
    
    
    
    let segmentRef = (header && header.segmentRef !== undefined)
      ? header.segmentRef : 1;
    let segmentSeq = header && header.segmentSeq || 1;
    let segmentMaxSeq = header && header.segmentMaxSeq || 1;
    
    
    
    let originatorPort = (header && header.originatorPort !== undefined)
      ? header.originatorPort
      : Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID;
    let destinationPort = (header && header.destinationPort !== undefined)
      ? header.destinationPort
      : Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID;
    
    let mwiPresent = (aMessage.mwi)? true : false;
    let mwiDiscard = (mwiPresent)? aMessage.mwi.discard: false;
    let mwiMsgCount = (mwiPresent)? aMessage.mwi.msgCount: 0;
    let mwiActive = (mwiPresent)? aMessage.mwi.active: false;
    
    let cdmaMessageType = aMessage.messageType || 0;
    let cdmaTeleservice = aMessage.teleservice || 0;
    let cdmaServiceCategory = aMessage.serviceCategory || 0;

    gSmsService
      .notifyMessageReceived(this.clientId,
                             aMessage.SMSC || null,
                             aMessage.sentTimestamp,
                             aMessage.sender,
                             aMessage.pid,
                             aMessage.encoding,
                             RIL.GECKO_SMS_MESSAGE_CLASSES
                               .indexOf(aMessage.messageClass),
                             aMessage.language || null,
                             segmentRef,
                             segmentSeq,
                             segmentMaxSeq,
                             originatorPort,
                             destinationPort,
                             mwiPresent,
                             mwiDiscard,
                             mwiMsgCount,
                             mwiActive,
                             cdmaMessageType,
                             cdmaTeleservice,
                             cdmaServiceCategory,
                             aMessage.body || null,
                             aMessage.data || [],
                             (aMessage.data) ? aMessage.data.length : 0);
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
    let service = Cc["@mozilla.org/voicemail/voicemailservice;1"]
                  .getService(Ci.nsIGonkVoicemailService);
    service.notifyInfoChanged(this.clientId, message.number, message.alphaId);
  },

  handleIccMwis: function(mwi) {
    let service = Cc["@mozilla.org/voicemail/voicemailservice;1"]
                  .getService(Ci.nsIGonkVoicemailService);
    
    service.notifyStatusChanged(this.clientId, mwi.active, mwi.msgCount,
                                null, null);
  },

  handleIccInfoChange: function(message) {
    let oldSpn = this.rilContext.iccInfo ? this.rilContext.iccInfo.spn : null;

    
    
    if (!message || !message.iccid) {
      
      
      if (!this.rilContext.iccInfo) {
        return;
      }

      
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
    gIccService.notifyIccInfoChanged(this.clientId,
                                     message.iccid ? message : null);

    
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
      gIccMessenger
        .notifyStkProactiveCommand(iccId,
                                   gStkCmdFactory.createCommand(message));
    }
    
    gMessageManager.sendIccMessage("RIL:StkCommand", this.clientId, message);
  },

  _convertCbGsmGeographicalScope: function(aGeographicalScope) {
    return (aGeographicalScope != null)
      ? aGeographicalScope
      : Ci.nsICellBroadcastService.GSM_GEOGRAPHICAL_SCOPE_INVALID;
  },

  _convertCbMessageClass: function(aMessageClass) {
    let index = RIL.GECKO_SMS_MESSAGE_CLASSES.indexOf(aMessageClass);
    return (index != -1)
      ? index
      : Ci.nsICellBroadcastService.GSM_MESSAGE_CLASS_NORMAL;
  },

  _convertCbEtwsWarningType: function(aWarningType) {
    return (aWarningType != null)
      ? aWarningType
      : Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID;
  },

  handleCellbroadcastMessageReceived: function(aMessage) {
    let etwsInfo = aMessage.etws;
    let hasEtwsInfo = etwsInfo != null;
    let serviceCategory = (aMessage.serviceCategory)
      ? aMessage.serviceCategory
      : Ci.nsICellBroadcastService.CDMA_SERVICE_CATEGORY_INVALID;

    gCellBroadcastService
      .notifyMessageReceived(this.clientId,
                             this._convertCbGsmGeographicalScope(aMessage.geographicalScope),
                             aMessage.messageCode,
                             aMessage.messageId,
                             aMessage.language,
                             aMessage.fullBody,
                             this._convertCbMessageClass(aMessage.messageClass),
                             Date.now(),
                             serviceCategory,
                             hasEtwsInfo,
                             (hasEtwsInfo)
                               ? this._convertCbEtwsWarningType(etwsInfo.warningType)
                               : Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID,
                             hasEtwsInfo ? etwsInfo.emergencyUserAlert : false,
                             hasEtwsInfo ? etwsInfo.popup : false);
  },

  handleCdmaInformationRecords: function(aRecords) {
    if (DEBUG) this.debug("cdma-info-rec-received: " + JSON.stringify(aRecords));

    let clientId = this.clientId;

    aRecords.forEach(function(aRecord) {
      if (aRecord.display) {
        gMobileConnectionService
          .notifyCdmaInfoRecDisplay(clientId, aRecord.display);
        return;
      }

      if (aRecord.calledNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecCalledPartyNumber(clientId,
                                              aRecord.calledNumber.type,
                                              aRecord.calledNumber.plan,
                                              aRecord.calledNumber.number,
                                              aRecord.calledNumber.pi,
                                              aRecord.calledNumber.si);
        return;
      }

      if (aRecord.callingNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecCallingPartyNumber(clientId,
                                               aRecord.callingNumber.type,
                                               aRecord.callingNumber.plan,
                                               aRecord.callingNumber.number,
                                               aRecord.callingNumber.pi,
                                               aRecord.callingNumber.si);
        return;
      }

      if (aRecord.connectedNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecConnectedPartyNumber(clientId,
                                                 aRecord.connectedNumber.type,
                                                 aRecord.connectedNumber.plan,
                                                 aRecord.connectedNumber.number,
                                                 aRecord.connectedNumber.pi,
                                                 aRecord.connectedNumber.si);
        return;
      }

      if (aRecord.signal) {
        gMobileConnectionService
          .notifyCdmaInfoRecSignal(clientId,
                                   aRecord.signal.type,
                                   aRecord.signal.alertPitch,
                                   aRecord.signal.signal);
        return;
      }

      if (aRecord.redirect) {
        gMobileConnectionService
          .notifyCdmaInfoRecRedirectingNumber(clientId,
                                              aRecord.redirect.type,
                                              aRecord.redirect.plan,
                                              aRecord.redirect.number,
                                              aRecord.redirect.pi,
                                              aRecord.redirect.si,
                                              aRecord.redirect.reason);
        return;
      }

      if (aRecord.lineControl) {
        gMobileConnectionService
          .notifyCdmaInfoRecLineControl(clientId,
                                        aRecord.lineControl.polarityIncluded,
                                        aRecord.lineControl.toggle,
                                        aRecord.lineControl.reverse,
                                        aRecord.lineControl.powerDenial);
        return;
      }

      if (aRecord.clirCause) {
        gMobileConnectionService
          .notifyCdmaInfoRecClir(clientId,
                                 aRecord.clirCause);
        return;
      }

      if (aRecord.audioControl) {
        gMobileConnectionService
          .notifyCdmaInfoRecAudioControl(clientId,
                                         aRecord.audioControl.upLink,
                                         aRecord.audioControl.downLink);
        return;
      }
    });
  },

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        if ("wrappedJSObject" in subject) {
          subject = subject.wrappedJSObject;
        }
        this.handleSettingsChange(subject.key, subject.value, subject.isInternalChange);
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

        
        if (network.type != NETWORK_TYPE_WIFI &&
            network.type != NETWORK_TYPE_MOBILE) {
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
    }
  },

  handleError: function(aErrorMessage) {
    if (DEBUG) {
      this.debug("There was an error while reading RIL settings.");
    }
  },

  

  rilContext: null,

  
  
  setupDataCallByType: function(networkType) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.setupDataCallByType(networkType);
  },

  
  
  deactivateDataCallByType: function(networkType) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.deactivateDataCallByType(networkType);
  },

  
  getDataCallStateByType: function(networkType) {
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    return connHandler.getDataCallStateByType(networkType);
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
    addresses: [],
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

  
  pdptype: null,

  
  chappap: null,

  




  _compareDataCallLink: function(updatedDataCall, currentDataCall) {
    
    if (updatedDataCall.ifname != currentDataCall.ifname) {
      return "deactivate";
    }

    
    for (let i = 0; i < currentDataCall.addresses.length; i++) {
      let address = currentDataCall.addresses[i];
      if (updatedDataCall.addresses.indexOf(address) < 0) {
        return "deactivate";
      }
    }

    if (currentDataCall.addresses.length != updatedDataCall.addresses.length) {
      
      
      
      return "changed";
    }

    let fields = ["gateways", "dnses"];
    for (let i = 0; i < fields.length; i++) {
      
      let field = fields[i];
      let lhs = updatedDataCall[field], rhs = currentDataCall[field];
      if (lhs.length != rhs.length) {
        return "changed";
      }
      for (let i = 0; i < lhs.length; i++) {
        if (lhs[i] != rhs[i]) {
          return "changed";
        }
      }
    }

    return "identical";
  },

  onSetupDataCallResult: function(dataCall) {
    if (dataCall.status && dataCall.status != RIL.DATACALL_FAIL_NONE) {
      dataCall.errorMsg =
        RIL.RIL_DATACALL_FAILCAUSE_TO_GECKO_DATACALL_ERROR[dataCall.status];
    }

    if (dataCall.errorMsg) {
      if (DEBUG) {
        this.debug("SetupDataCall error for apn " + dataCall.apn + ": " +
                   dataCall.errorMsg + " (" + dataCall.status + "), retry time: " +
                   dataCall.suggestedRetryTime);
      }

      this.state = RIL.GECKO_NETWORK_STATE_DISCONNECTED;

      if (this.requestedNetworkIfaces.length === 0) {
        if (DEBUG) this.debug("This DataCall is not requested anymore.");
        return;
      }

      
      let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
      connHandler.notifyDataCallError(this);

      
      if (dataCall.suggestedRetryTime === INT32_MAX ||
          this.isPermanentFail(dataCall.status, dataCall.errorMsg)) {
        if (DEBUG) this.debug("Data call error: no retry needed.");
        return;
      }

      this.retry(dataCall.suggestedRetryTime);
      return;
    }

    this.apnRetryCounter = 0;
    this.linkInfo.cid = dataCall.cid;

    if (this.requestedNetworkIfaces.length === 0) {
      if (DEBUG) {
        this.debug("State is connected, but no network interface requested" +
                   " this DataCall");
      }
      this.deactivate();
      return;
    }

    this.linkInfo.ifname = dataCall.ifname;
    this.linkInfo.addresses = dataCall.addresses.slice();
    this.linkInfo.gateways = dataCall.gateways.slice();
    this.linkInfo.dnses = dataCall.dnses.slice();
    this.state = dataCall.state;

    
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.notifyDataCallChanged(this);

    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
    }
  },

  onDeactivateDataCallResult: function() {
    this.reset();

    if (this.requestedNetworkIfaces.length > 0) {
      if (DEBUG) {
        this.debug("State is disconnected/unknown, but this DataCall is" +
                   " requested.");
      }
      this.setup();
      return;
    }

    
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.notifyDataCallChanged(this);
  },

  onDataCallChanged: function(updatedDataCall) {
    if (DEBUG) {
      this.debug("onDataCallChanged: " + JSON.stringify(updatedDataCall));
    }

    if (this.state == updatedDataCall.state &&
        updatedDataCall.state != RIL.GECKO_NETWORK_STATE_CONNECTED) {
      return;
    }

    switch (updatedDataCall.state) {
      case RIL.GECKO_NETWORK_STATE_CONNECTED:
        if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          let result =
            this._compareDataCallLink(updatedDataCall, this.linkInfo);

          if (result == "identical") {
            if (DEBUG) this.debug("No changes in data call.");
            return;
          }
          if (result == "deactivate") {
            if (DEBUG) this.debug("Data link changed, cleanup.");
            this.deactivate();
            return;
          }
          
          if (DEBUG) {
            this.debug("Data link minor change, just update and notify.");
          }

          this.linkInfo.addresses = updatedDataCall.addresses.slice();
          this.linkInfo.gateways = updatedDataCall.gateways.slice();
          this.linkInfo.dnses = updatedDataCall.dnses.slice();
        }
        break;
      case RIL.GECKO_NETWORK_STATE_DISCONNECTED:
      case RIL.GECKO_NETWORK_STATE_UNKNOWN:
        if (this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          
          this.state = updatedDataCall.state;
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

    this.state = updatedDataCall.state;

    
    let connHandler = gDataConnectionManager.getConnectionHandler(this.clientId);
    connHandler.notifyDataCallChanged(this);

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

  isPermanentFail: function(dataFailCause, errorMsg) {
    
    if (errorMsg === RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE ||
        errorMsg === RIL.GECKO_ERROR_INVALID_PARAMETER ||
        dataFailCause === RIL.DATACALL_FAIL_OPERATOR_BARRED ||
        dataFailCause === RIL.DATACALL_FAIL_MISSING_UKNOWN_APN ||
        dataFailCause === RIL.DATACALL_FAIL_UNKNOWN_PDP_ADDRESS_TYPE ||
        dataFailCause === RIL.DATACALL_FAIL_USER_AUTHENTICATION ||
        dataFailCause === RIL.DATACALL_FAIL_ACTIVATION_REJECT_GGSN ||
        dataFailCause === RIL.DATACALL_FAIL_SERVICE_OPTION_NOT_SUPPORTED ||
        dataFailCause === RIL.DATACALL_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED ||
        dataFailCause === RIL.DATACALL_FAIL_NSAPI_IN_USE ||
        dataFailCause === RIL.DATACALL_FAIL_ONLY_IPV4_ALLOWED ||
        dataFailCause === RIL.DATACALL_FAIL_ONLY_IPV6_ALLOWED ||
        dataFailCause === RIL.DATACALL_FAIL_PROTOCOL_ERRORS ||
        dataFailCause === RIL.DATACALL_FAIL_RADIO_POWER_OFF ||
        dataFailCause === RIL.DATACALL_FAIL_TETHERED_CALL_ACTIVE) {
      return true;
    }

    return false;
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
    let isIdentical = this.apnProfile.apn == apnSetting.apn &&
                      (this.apnProfile.user || '') == (apnSetting.user || '') &&
                      (this.apnProfile.password || '') == (apnSetting.password || '') &&
                      (this.apnProfile.authType || '') == (apnSetting.authtype || '');

    if (RILQUIRKS_HAVE_IPV6) {
      isIdentical = isIdentical &&
                    (this.apnProfile.protocol || '') == (apnSetting.protocol || '') &&
                    (this.apnProfile.roaming_protocol || '') == (apnSetting.roaming_protocol || '');
    }

    return isIdentical;
  },

  resetLinkInfo: function() {
    this.linkInfo.cid = null;
    this.linkInfo.ifname = null;
    this.linkInfo.addresses = [];
    this.linkInfo.dnses = [];
    this.linkInfo.gateways = [];
  },

  reset: function() {
    this.resetLinkInfo();

    this.state = RIL.GECKO_NETWORK_STATE_UNKNOWN;

    this.chappap = null;
    this.pdptype = null;
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
      
      
      Services.tm.currentThread.dispatch(function(state) {
        
        
        if (networkInterface.state == state) {
          networkInterface.notifyRILNetworkInterface();
        }
      }.bind(null, RIL.GECKO_NETWORK_STATE_CONNECTED), Ci.nsIEventTarget.DISPATCH_NORMAL);
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

    let connection =
      gMobileConnectionService.getItemByServiceId(this.clientId);
    let dataInfo = connection && connection.data;
    if (dataInfo == null ||
        dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      return;
    }

    let radioTechType = dataInfo.type;
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);
    let authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(this.apnProfile.authType);
    
    
    if (authType == -1) {
      if (DEBUG) {
        this.debug("Invalid authType " + this.apnProfile.authtype);
      }
      authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(RIL.GECKO_DATACALL_AUTH_DEFAULT);
    }
    this.chappap = authType;

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
    this.pdptype = pdpType;

    let radioInterface = this.gRIL.getRadioInterface(this.clientId);
    radioInterface.sendWorkerMessage("setupDataCall", {
      radioTech: radioTechnology,
      apn: this.apnProfile.apn,
      user: this.apnProfile.user,
      passwd: this.apnProfile.password,
      chappap: authType,
      pdptype: pdpType
    }, this.onSetupDataCallResult.bind(this));
    this.state = RIL.GECKO_NETWORK_STATE_CONNECTING;
  },

  retry: function(suggestedRetryTime) {
    let apnRetryTimer;

    
    
    if (this.apnRetryCounter >= this.NETWORK_APNRETRY_MAXRETRIES) {
      this.apnRetryCounter = 0;
      this.timer = null;
      if (DEBUG) this.debug("Too many APN Connection retries - STOP retrying");
      return;
    }

    
    if (suggestedRetryTime !== undefined && suggestedRetryTime >= 0) {
      apnRetryTimer = suggestedRetryTime / 1000;
    } else {
      apnRetryTimer = this.NETWORK_APNRETRY_FACTOR *
                      (this.apnRetryCounter * this.apnRetryCounter) +
                      this.NETWORK_APNRETRY_ORIGIN;
    }
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

      
      
      
      Services.tm.currentThread.dispatch(function(state) {
        
        
        if (networkInterface.state == state) {
          networkInterface.notifyRILNetworkInterface();
        }
      }.bind(null, RIL.GECKO_NETWORK_STATE_DISCONNECTED), Ci.nsIEventTarget.DISPATCH_NORMAL);
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
      this.debug("Going to disconnect data connection cid " + this.linkInfo.cid);
    }
    let radioInterface = this.gRIL.getRadioInterface(this.clientId);
    radioInterface.sendWorkerMessage("deactivateDataCall", {
      cid: this.linkInfo.cid,
      reason: reason
    }, this.onDeactivateDataCallResult.bind(this));

    this.state = RIL.GECKO_NETWORK_STATE_DISCONNECTING;
    this.resetLinkInfo();
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

  getAddresses: function(aIps, aPrefixLengths) {
    let addresses = this.dataCall.linkInfo.addresses;

    let ips = [];
    let prefixLengths = [];
    for (let i = 0; i < addresses.length; i++) {
      let [ip, prefixLength] = addresses[i].split("/");
      ips.push(ip);
      prefixLengths.push(prefixLength);
    }

    aIps.value = ips.slice();
    aPrefixLengths.value = prefixLengths.slice();

    return ips.length;
  },

  getGateways: function(aCount) {
    let linkInfo = this.dataCall.linkInfo;

    if (aCount) {
      aCount.value = linkInfo.gateways.length;
    }
    return linkInfo.gateways.slice();
  },

  getDnses: function(aCount) {
    let linkInfo = this.dataCall.linkInfo;

    if (aCount) {
      aCount.value = linkInfo.dnses.length;
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
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMSC.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return this.apnSetting.mmsc || "";
  },

  get mmsProxy() {
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS proxy.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return this.apnSetting.mmsproxy || "";
  },

  get mmsPort() {
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS port.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    
    
    return this.apnSetting.mmsport || -1;
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
