














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Sntp.jsm");

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

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");

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
const kClockAutoUpdateEnabled            = "time.clock.automatic-update.enabled";
const kClockAutoUpdateAvailable          = "time.clock.automatic-update.available";
const kTimezoneAutoUpdateEnabled         = "time.timezone.automatic-update.enabled";
const kTimezoneAutoUpdateAvailable       = "time.timezone.automatic-update.available";
const kCellBroadcastSearchList           = "ril.cellbroadcast.searchlist";
const kCellBroadcastDisabled             = "ril.cellbroadcast.disabled";
const kPrefenceChangedObserverTopic      = "nsPref:changed";
const kClirModePreference                = "ril.clirMode";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

const RADIO_POWER_OFF_TIMEOUT = 30000;
const SMS_HANDLED_WAKELOCK_TIMEOUT = 5000;

const RIL_IPC_MOBILECONNECTION_MSG_NAMES = [
  "RIL:GetRilContext",
  "RIL:GetAvailableNetworks",
  "RIL:SelectNetwork",
  "RIL:SelectNetworkAuto",
  "RIL:SendMMI",
  "RIL:CancelMMI",
  "RIL:RegisterMobileConnectionMsg",
  "RIL:SetCallForwardingOption",
  "RIL:GetCallForwardingOption",
  "RIL:SetCallBarringOption",
  "RIL:GetCallBarringOption",
  "RIL:ChangeCallBarringPassword",
  "RIL:SetCallWaitingOption",
  "RIL:GetCallWaitingOption",
  "RIL:SetCallingLineIdRestriction",
  "RIL:GetCallingLineIdRestriction",
  "RIL:SetRoamingPreference",
  "RIL:GetRoamingPreference",
  "RIL:ExitEmergencyCbMode",
  "RIL:SetVoicePrivacyMode",
  "RIL:GetVoicePrivacyMode"
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
  "RIL:RegisterIccMsg"
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

XPCOMUtils.defineLazyGetter(this, "WAP", function () {
  let wap = {};
  Cu.import("resource://gre/modules/WapPushManager.js", wap);
  return wap;
});

XPCOMUtils.defineLazyGetter(this, "PhoneNumberUtils", function () {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    ril: null,

    
    
    targetsByTopic: {},
    topics: [],

    targetMessageQueue: [],
    ready: false,

    init: function init(ril) {
      this.ril = ril;

      Services.obs.addObserver(this, "xpcom-shutdown", false);
      Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
      this._registerMessageListeners();
    },

    _shutdown: function _shutdown() {
      this.ril = null;

      Services.obs.removeObserver(this, "xpcom-shutdown");
      this._unregisterMessageListeners();
    },

    _registerMessageListeners: function _registerMessageListeners() {
      ppmm.addMessageListener("child-process-shutdown", this);
      for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
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

    _unregisterMessageListeners: function _unregisterMessageListeners() {
      ppmm.removeMessageListener("child-process-shutdown", this);
      for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
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

    _registerMessageTarget: function _registerMessageTarget(topic, target) {
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

    _unregisterMessageTarget: function _unregisterMessageTarget(topic, target) {
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

    _enqueueTargetMessage: function _enqueueTargetMessage(topic, message, options) {
      let msg = { topic : topic,
                  message : message,
                  options : options };
      
      
      let messageQueue = this.targetMessageQueue;
      for(let i = 0; i < messageQueue.length; i++) {
        if (messageQueue[i].message === message) {
          messageQueue.splice(i, 1);
          break;
        }
      }

      messageQueue.push(msg);
    },

    _sendTargetMessage: function _sendTargetMessage(topic, message, options) {
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

    _resendQueuedTargetMessage: function _resendQueuedTargetMessage() {
      this.ready = true;

      
      
      
      

      
      for each (let msg in this.targetMessageQueue) {
        this._sendTargetMessage(msg.topic, msg.message, msg.options);
      }
      this.targetMessageQueue = null;
    },

    



    receiveMessage: function receiveMessage(msg) {
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

      return radioInterface.receiveMessage(msg);
    },

    



    observe: function observe(subject, topic, data) {
      switch (topic) {
        case kSysMsgListenerReadyObserverTopic:
          Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);
          this._resendQueuedTargetMessage();
          break;
        case "xpcom-shutdown":
          this._shutdown();
          break;
      }
    },

    sendMobileConnectionMessage: function sendMobileConnectionMessage(message, clientId, data) {
      this._sendTargetMessage("mobileconnection", message, {
        clientId: clientId,
        data: data
      });
    },

    sendVoicemailMessage: function sendVoicemailMessage(message, clientId, data) {
      this._sendTargetMessage("voicemail", message, {
        clientId: clientId,
        data: data
      });
    },

    sendCellBroadcastMessage: function sendCellBroadcastMessage(message, clientId, data) {
      this._sendTargetMessage("cellbroadcast", message, {
        clientId: clientId,
        data: data
      });
    },

    sendIccMessage: function sendIccMessage(message, clientId, data) {
      this._sendTargetMessage("icc", message, {
        clientId: clientId,
        data: data
      });
    }
  };
});

function RadioInterfaceLayer() {
  gMessageManager.init(this);

  let options = {
    debug: debugPref,
    cellBroadcastDisabled: false,
    clirMode: RIL.CLIR_DEFAULT
  };

  try {
    options.cellBroadcastDisabled =
      Services.prefs.getBoolPref(kCellBroadcastDisabled);
  } catch(e) {}

  try {
    options.clirMode = Services.prefs.getIntPref(kClirModePreference);
  } catch(e) {}

  let numIfaces = this.numRadioInterfaces;
  debug(numIfaces + " interfaces");
  this.radioInterfaces = [];
  for (let clientId = 0; clientId < numIfaces; clientId++) {
    options.clientId = clientId;
    this.radioInterfaces.push(new RadioInterface(options));
  }
}
RadioInterfaceLayer.prototype = {

  classID:   RADIOINTERFACELAYER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACELAYER_CID,
                                    classDescription: "RadioInterfaceLayer",
                                    interfaces: [Ci.nsIRadioInterfaceLayer]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioInterfaceLayer,
                                         Ci.nsIObserver]),

  



  observe: function observe(subject, topic, data) {
    
  },

  



  getRadioInterface: function getRadioInterface(clientId) {
    return this.radioInterfaces[clientId];
  }
};

XPCOMUtils.defineLazyGetter(RadioInterfaceLayer.prototype,
                            "numRadioInterfaces", function () {
  try {
    return Services.prefs.getIntPref("ril.numRadioInterfaces");
  } catch (e) {
    return 1;
  }
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

  onerror: function onerror(event) {
    if (DEBUG) {
      this.debug("Got an error: " + event.filename + ":" +
                 event.lineno + ": " + event.message + "\n");
    }
    event.preventDefault();
  },

  


  onmessage: function onmessage(event) {
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

  














  send: function send(rilMessageType, message, callback) {
    message = message || {};

    message.rilMessageToken = this.token;
    this.token++;

    if (callback) {
      
      
      
      
      
      
      
      this.tokenCallbackMap[message.rilMessageToken] = callback;
    }

    message.rilMessageType = rilMessageType;
    this.worker.postMessage(message);
  },

  











  sendWithIPCMessage: function sendWithIPCMessage(msg, rilMessageType, ipcType) {
    this.send(rilMessageType, msg.json.data, function(reply) {
      ipcType = ipcType || msg.name;
      msg.target.sendAsyncMessage(ipcType, reply);
      return false;
    });
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
      byAPN: {}
  };

  this.rilContext = {
    radioState:     RIL.GECKO_RADIOSTATE_UNAVAILABLE,
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
  lock.get("ril.radio.disabled", this);

  
  lock.get("ril.radio.preferredNetworkType", this);

  
  lock.get("ril.data.roaming_enabled", this);
  lock.get("ril.data.enabled", this);
  lock.get("ril.data.apnSettings", this);

  
  
  lock.get(kClockAutoUpdateEnabled, this);

  
  
  lock.get(kTimezoneAutoUpdateEnabled, this);

  
  this.setClockAutoUpdateAvailable(false);

  
  this.setTimezoneAutoUpdateAvailable(false);

  
  
  lock.get(kCellBroadcastSearchList, this);

  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic, false);
  Services.prefs.addObserver(kCellBroadcastDisabled, this, false);

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] = this.handleSmsWdpPortPush.bind(this);

  this._sntp = new Sntp(this.setClockBySntp.bind(this),
                        Services.prefs.getIntPref('network.sntp.maxRetryCount'),
                        Services.prefs.getIntPref('network.sntp.refreshPeriod'),
                        Services.prefs.getIntPref('network.sntp.timeout'),
                        Services.prefs.getCharPref('network.sntp.pools').split(';'),
                        Services.prefs.getIntPref('network.sntp.port'));
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

  debug: function debug(s) {
    dump("-*- RadioInterface[" + this.clientId + "]: " + s + "\n");
  },

  



  updateInfo: function updateInfo(srcInfo, destInfo) {
    for (let key in srcInfo) {
      if (key === 'rilMessageType') {
        continue;
      }
      destInfo[key] = srcInfo[key];
    }
  },

  



  isInfoChanged: function isInfoChanged(srcInfo, destInfo) {
    if (!destInfo) {
      return true;
    }

    for (let key in srcInfo) {
      if (key === 'rilMessageType') {
        continue;
      }
      if (srcInfo[key] !== destInfo[key]) {
        return true;
      }
    }

    return false;
  },

  


  receiveMessage: function receiveMessage(msg) {
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
      case "RIL:GetAvailableNetworks":
        this.workerMessenger.sendWithIPCMessage(msg, "getAvailableNetworks");
        break;
      case "RIL:SelectNetwork":
        this.workerMessenger.sendWithIPCMessage(msg, "selectNetwork");
        break;
      case "RIL:SelectNetworkAuto":
        this.workerMessenger.sendWithIPCMessage(msg, "selectNetworkAuto");
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
      case "RIL:SetCallForwardingOption":
        this.setCallForwardingOption(msg.target, msg.json.data);
        break;
      case "RIL:GetCallForwardingOption":
        this.workerMessenger.sendWithIPCMessage(msg, "queryCallForwardStatus");
        break;
      case "RIL:SetCallBarringOption":
        this.workerMessenger.sendWithIPCMessage(msg, "setCallBarring");
        break;
      case "RIL:GetCallBarringOption":
        this.workerMessenger.sendWithIPCMessage(msg, "queryCallBarringStatus");
        break;
      case "RIL:ChangeCallBarringPassword":
        this.workerMessenger.sendWithIPCMessage(msg, "changeCallBarringPassword");
        break;
      case "RIL:SetCallWaitingOption":
        this.workerMessenger.sendWithIPCMessage(msg, "setCallWaiting");
        break;
      case "RIL:GetCallWaitingOption":
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
    }
    return null;
  },

  handleUnsolicitedWorkerMessage: function handleUnsolicitedWorkerMessage(message) {
    switch (message.rilMessageType) {
      case "callRing":
        gTelephonyProvider.notifyCallRing();
        break;
      case "callStateChange":
        gTelephonyProvider.notifyCallStateChanged(message.call);
        break;
      case "callDisconnected":
        gTelephonyProvider.notifyCallDisconnected(message.call);
        break;
      case "conferenceCallStateChanged":
        gTelephonyProvider.notifyConferenceCallStateChanged(message.state);
        break;
      case "cdmaCallWaiting":
        gTelephonyProvider.notifyCdmaCallWaiting(message.number);
        break;
      case "callError":
        gTelephonyProvider.notifyCallError(message.callIndex, message.errorMsg);
        break;
      case "suppSvcNotification":
        gTelephonyProvider.notifySupplementaryService(message.callIndex,
                                                      message.notification);
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
        if (ackOk) {
          this.workerMessenger.send("ackSMS", { result: RIL.PDU_FCS_OK });
        }
        return;
      case "cellbroadcast-received":
        message.timestamp = Date.now();
        gMessageManager.sendCellBroadcastMessage("RIL:CellBroadcastReceived",
                                                 this.clientId, message);
        break;
      case "datacallstatechange":
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
      case "setRadioEnabled":
        let lock = gSettingsService.createLock();
        lock.set("ril.radio.disabled", !message.on, null, null);
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

  getMsisdn: function getMsisdn() {
    let iccInfo = this.rilContext.iccInfo;
    let number = iccInfo ? iccInfo.msisdn : null;

    
    
    if (number === undefined || number === "undefined") {
      return null;
    }
    return number;
  },

  updateNetworkInfo: function updateNetworkInfo(message) {
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

  






  checkRoamingBetweenOperators: function checkRoamingBetweenOperators(registration) {
    let iccInfo = this.rilContext.iccInfo;
    if (!iccInfo || !registration.connected) {
      return;
    }

    let spn = iccInfo.spn && iccInfo.spn.toLowerCase();
    let operator = registration.network;
    let longName = operator.longName && operator.longName.toLowerCase();
    let shortName = operator.shortName && operator.shortName.toLowerCase();

    let equalsLongName = longName && (spn == longName);
    let equalsShortName = shortName && (spn == shortName);
    let equalsMcc = iccInfo.mcc == operator.mcc;

    registration.roaming = registration.roaming &&
                           !(equalsMcc && (equalsLongName || equalsShortName));
  },

  






  updateVoiceConnection: function updateVoiceConnection(newInfo, batch) {
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

  






  updateDataConnection: function updateDataConnection(newInfo, batch) {
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

  


  handleDataCallError: function handleDataCallError(message) {
    
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
  setPreferredNetworkType: function setPreferredNetworkType(value) {
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

  setCellBroadcastSearchList: function setCellBroadcastSearchList(newSearchListStr) {
    if (newSearchListStr == this._cellBroadcastSearchListStr) {
      return;
    }

    this.workerMessenger.send("setCellBroadcastSearchList",
                              { searchListStr: newSearchListStr },
                              (function callback(response) {
      if (!response.success) {
        let lock = gSettingsService.createLock();
        lock.set(kCellBroadcastSearchList,
                 this._cellBroadcastSearchListStr, null);
      } else {
        this._cellBroadcastSearchListStr = response.searchListStr;
      }

      return false;
    }).bind(this));
  },

  






  handleSignalStrengthChange: function handleSignalStrengthChange(message, batch) {
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

  






  handleOperatorChange: function handleOperatorChange(message, batch) {
    let operatorInfo = this.operatorInfo;
    let voice = this.rilContext.voice;
    let data = this.rilContext.data;

    if (this.isInfoChanged(message, operatorInfo)) {
      this.updateInfo(message, operatorInfo);

      
      if (message.mcc && message.mnc) {
        try {
          Services.prefs.setCharPref("ril.lastKnownNetwork",
                                     message.mcc + "-" + message.mnc);
        } catch (e) {}
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

  handleOtaStatus: function handleOtaStatus(message) {
    if (message.status < 0 ||
        RIL.CDMA_OTA_PROVISION_STATUS_TO_GECKO.length <= message.status) {
      return;
    }

    let status = RIL.CDMA_OTA_PROVISION_STATUS_TO_GECKO[message.status];

    gMessageManager.sendMobileConnectionMessage("RIL:OtaStatusChanged",
                                                this.clientId, status);
  },

  handleRadioStateChange: function handleRadioStateChange(message) {
    this._changingRadioPower = false;

    let newState = message.radioState;
    if (this.rilContext.radioState == newState) {
      return;
    }
    this.rilContext.radioState = newState;
    

    this._ensureRadioState();
  },

  _ensureRadioState: function _ensureRadioState() {
    if (DEBUG) {
      this.debug("Reported radio state is " + this.rilContext.radioState +
                 ", desired radio enabled state is " + this._radioEnabled);
    }
    if (this._radioEnabled == null) {
      
      
      return;
    }
    if (!this._sysMsgListenerReady) {
      
      
      return;
    }
    if (this.rilContext.radioState == RIL.GECKO_RADIOSTATE_UNKNOWN) {
      
      
      return;
    }
    if (this._changingRadioPower) {
      
      return;
    }

    if (this.rilContext.radioState == RIL.GECKO_RADIOSTATE_OFF &&
        this._radioEnabled) {
      this._changingRadioPower = true;
      this.setRadioEnabled(true);
    }
    if (this.rilContext.radioState == RIL.GECKO_RADIOSTATE_READY &&
        !this._radioEnabled) {
      this._changingRadioPower = true;
      this.powerOffRadioSafely();
    }
  },

  _radioOffTimer: null,
  _cancelRadioOffTimer: function _cancelRadioOffTimer() {
    if (this._radioOffTimer) {
      this._radioOffTimer.cancel();
    }
  },
  _fireRadioOffTimer: function _fireRadioOffTimer() {
    if (DEBUG) this.debug("Radio off timer expired, set radio power off right away.");
    this.setRadioEnabled(false);
  },

  


  powerOffRadioSafely: function powerOffRadioSafely() {
    let dataDisconnecting = false;
    for each (let apnSetting in this.apnSettings.byAPN) {
      for each (let type in apnSetting.types) {
        if (this.getDataCallStateByType(type) ==
            RIL.GECKO_NETWORK_STATE_CONNECTED) {
          this.deactivateDataCallByType(type);
          dataDisconnecting = true;
        }
      }
    }
    if (dataDisconnecting) {
      if (this._radioOffTimer == null) {
        this._radioOffTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      }
      this._radioOffTimer.initWithCallback(this._fireRadioOffTimer.bind(this),
                                           RADIO_POWER_OFF_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);
      return;
    }
    this.setRadioEnabled(false);
  },

  









  updateApnSettings: function updateApnSettings(allApnSettings) {
    let thisSimApnSettings = allApnSettings[this.clientId];
    if (!thisSimApnSettings) {
      return;
    }

    
    for each (let apnSetting in this.apnSettings.byAPN) {
      
      for each (let type in apnSetting.types) {
        if (this.getDataCallStateByType(type) ==
            RIL.GECKO_NETWORK_STATE_CONNECTED) {
          this.deactivateDataCallByType(type);
        }
      }
      if (apnSetting.iface.name in gNetworkManager.networkInterfaces) {
        gNetworkManager.unregisterNetworkInterface(apnSetting.iface);
      }
      this.unregisterDataCallCallback(apnSetting.iface);
      delete apnSetting.iface;
    }
    this.apnSettings.byAPN = {};
    this.apnSettings.byType = {};

    
    for (let apnIndex = 0; thisSimApnSettings[apnIndex]; apnIndex++) {
      let inputApnSetting = thisSimApnSettings[apnIndex];
      if (!this.validateApnSetting(inputApnSetting)) {
        continue;
      }

      
      
      let apnKey = inputApnSetting.apn + (inputApnSetting.user || '') +
                   (inputApnSetting.password || '');
      if (!this.apnSettings.byAPN[apnKey]) {
        this.apnSettings.byAPN[apnKey] = {};
        this.apnSettings.byAPN[apnKey] = inputApnSetting;
        this.apnSettings.byAPN[apnKey].iface =
          new RILNetworkInterface(this, this.apnSettings.byAPN[apnKey]);
      } else {
        this.apnSettings.byAPN[apnKey].types.push(inputApnSetting.types);
      }
      for each (let type in inputApnSetting.types) {
        this.apnSettings.byType[type] = {};
        this.apnSettings.byType[type] = this.apnSettings.byAPN[apnKey];
      }
    }
  },

  


  validateApnSetting: function validateApnSetting(apnSetting) {
    return (apnSetting &&
            apnSetting.apn &&
            apnSetting.types &&
            apnSetting.types.length);
  },

  updateRILNetworkInterface: function updateRILNetworkInterface() {
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
    if (this._changingRadioPower) {
      
      return;
    }

    if (DEBUG) this.debug("Data call settings: connect data call.");
    this.setupDataCallByType("default");
  },

  


  updateNetworkSelectionMode: function updateNetworkSelectionMode(message) {
    if (DEBUG) this.debug("updateNetworkSelectionMode: " + JSON.stringify(message));
    this.rilContext.networkSelectionMode = message.mode;
    gMessageManager.sendMobileConnectionMessage("RIL:NetworkSelectionModeChanged",
                                                this.clientId, message);
  },

  


  handleEmergencyCbModeChange: function handleEmergencyCbModeChange(message) {
    if (DEBUG) this.debug("handleEmergencyCbModeChange: " + JSON.stringify(message));
    gMessageManager.sendMobileConnectionMessage("RIL:EmergencyCbModeChanged",
                                                this.clientId, message);
  },

  






  handleSmsWdpPortPush: function handleSmsWdpPortPush(message) {
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
    };
    WAP.WapPushManager.receiveWdpPDU(message.fullData, message.fullData.length,
                                     0, options);
  },

  








  broadcastSmsSystemMessage: function broadcastSmsSystemMessage(aName, aDomMessage) {
    if (DEBUG) this.debug("Broadcasting the SMS system message: " + aName);

    
    
    
    gSystemMessenger.broadcastMessage(aName, {
      type:           aDomMessage.type,
      id:             aDomMessage.id,
      threadId:       aDomMessage.threadId,
      delivery:       aDomMessage.delivery,
      deliveryStatus: aDomMessage.deliveryStatus,
      sender:         aDomMessage.sender,
      receiver:       aDomMessage.receiver,
      body:           aDomMessage.body,
      messageClass:   aDomMessage.messageClass,
      timestamp:      aDomMessage.timestamp,
      read:           aDomMessage.read
    });
  },

  
  
  
  _smsHandledWakeLock: null,
  _smsHandledWakeLockTimer: null,
  _cancelSmsHandledWakeLockTimer: function _cancelSmsHandledWakeLockTimer() {
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
  handleSmsReceived: function handleSmsReceived(message) {
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
        .initWithCallback(this._cancelSmsHandledWakeLockTimer.bind(this),
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
    message.receiver = this.getMsisdn();
    message.body = message.fullBody = message.fullBody || null;
    message.timestamp = Date.now();

    if (gSmsService.isSilentNumber(message.sender)) {
      message.id = -1;
      message.threadId = 0;
      message.delivery = DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED;
      message.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      message.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(message.id,
                                               message.threadId,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
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
      return true;
    }

    let notifyReceived = function notifyReceived(rv, domMessage) {
      let success = Components.isSuccessCode(rv);

      
      this.workerMessenger.send("ackSMS", {
        result: (success ? RIL.PDU_FCS_OK
                         : RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED)
      });

      if (!success) {
        
        
        if (DEBUG) {
          this.debug("Could not store SMS " + message.id + ", error code " + rv);
        }
        return;
      }

      this.broadcastSmsSystemMessage("sms-received", domMessage);
      Services.obs.notifyObservers(domMessage, kSmsReceivedObserverTopic, null);
    }.bind(this);

    if (message.messageClass != RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0]) {
      message.id = gMobileMessageDatabaseService.saveReceivedMessage(message,
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
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.read);

      notifyReceived(Cr.NS_OK, domMessage);
    }

    
    return false;
  },

  


  handleDataCallState: function handleDataCallState(datacall) {
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
        this._changingRadioPower) {
      let anyDataConnected = false;
      for each (let apnSetting in this.apnSettings.byAPN) {
        for each (let type in apnSetting.types) {
          if (this.getDataCallStateByType(type) == RIL.GECKO_NETWORK_STATE_CONNECTED) {
            anyDataConnected = true;
            break;
          }
        }
        if (anyDataConnected) {
          break;
        }
      }
      if (!anyDataConnected) {
        if (DEBUG) this.debug("All data connections are disconnected, set radio off.");
        this._cancelRadioOffTimer();
        this.setRadioEnabled(false);
      }
    }
  },

  


  handleDataCallList: function handleDataCallList(message) {
    this._deliverDataCallCallback("receiveDataCallList",
                                  [message.datacalls, message.datacalls.length]);
  },

  


  setClockAutoUpdateAvailable: function setClockAutoUpdateAvailable(value) {
    gSettingsService.createLock().set(kClockAutoUpdateAvailable, value, null,
                                      "fromInternalSetting");
  },

  


  setTimezoneAutoUpdateAvailable: function setTimezoneAutoUpdateAvailable(value) {
    gSettingsService.createLock().set(kTimezoneAutoUpdateAvailable, value, null,
                                      "fromInternalSetting");
  },

  


  setClockByNitz: function setClockByNitz(message) {
    
    
    gTimeService.set(
      message.networkTimeInMS + (Date.now() - message.receiveTimeInMS));
  },

  


  setTimezoneByNitz: function setTimezoneByNitz(message) {
    
    
    
    
    
    
    if (message.networkTimeZoneInMinutes != (new Date()).getTimezoneOffset()) {
      let absTimeZoneInMinutes = Math.abs(message.networkTimeZoneInMinutes);
      let timeZoneStr = "UTC";
      timeZoneStr += (message.networkTimeZoneInMinutes >= 0 ? "+" : "-");
      timeZoneStr += ("0" + Math.floor(absTimeZoneInMinutes / 60)).slice(-2);
      timeZoneStr += ":";
      timeZoneStr += ("0" + absTimeZoneInMinutes % 60).slice(-2);
      gSettingsService.createLock().set("time.timezone", timeZoneStr, null);
    }
  },

  


  handleNitzTime: function handleNitzTime(message) {
    
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

  


  setClockBySntp: function setClockBySntp(offset) {
    
    this.setClockAutoUpdateAvailable(true);
    if (!this._clockAutoUpdateEnabled) {
      return;
    }
    if (this._lastNitzMessage) {
      debug("SNTP: NITZ available, discard SNTP");
      return;
    }
    gTimeService.set(Date.now() + offset);
  },

  handleIccMbdn: function handleIccMbdn(message) {
    let voicemailInfo = this.voicemailInfo;

    voicemailInfo.number = message.number;
    voicemailInfo.displayName = message.alphaId;

    gMessageManager.sendVoicemailMessage("RIL:VoicemailInfoChanged",
                                         this.clientId, voicemailInfo);
  },

  handleIccInfoChange: function handleIccInfoChange(message) {
    let oldIccInfo = this.rilContext.iccInfo;
    this.rilContext.iccInfo = message;

    if (!this.isInfoChanged(message, oldIccInfo)) {
      return;
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
      try {
        Services.prefs.setCharPref("ril.lastKnownHomeNetwork",
                                   message.mcc + "-" + message.mnc);
      } catch (e) {}
    }

    
    let oldSpn = oldIccInfo ? oldIccInfo.spn : null;
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

  handleUSSDReceived: function handleUSSDReceived(ussd) {
    if (DEBUG) this.debug("handleUSSDReceived " + JSON.stringify(ussd));
    gSystemMessenger.broadcastMessage("ussd-received", ussd);
    gMessageManager.sendMobileConnectionMessage("RIL:USSDReceived",
                                                this.clientId, ussd);
  },

  handleStkProactiveCommand: function handleStkProactiveCommand(message) {
    if (DEBUG) this.debug("handleStkProactiveCommand " + JSON.stringify(message));
    gSystemMessenger.broadcastMessage("icc-stkcommand", message);
    gMessageManager.sendIccMessage("RIL:StkCommand", this.clientId, message);
  },

  handleExitEmergencyCbMode: function handleExitEmergencyCbMode(message) {
    if (DEBUG) this.debug("handleExitEmergencyCbMode: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:ExitEmergencyCbMode", message);
  },

  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kSysMsgListenerReadyObserverTopic:
        Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);
        this._sysMsgListenerReady = true;
        this._ensureRadioState();
        break;
      case kMozSettingsChangedObserverTopic:
        let setting = JSON.parse(data);
        this.handleSettingsChange(setting.key, setting.value, setting.message);
        break;
      case kPrefenceChangedObserverTopic:
        if (data === kCellBroadcastDisabled) {
          let value = false;
          try {
            value = Services.prefs.getBoolPref(kCellBroadcastDisabled);
          } catch(e) {}
          this.workerMessenger.send("setCellBroadcastDisabled",
                                    { disabled: value });
        }
        break;
      case "xpcom-shutdown":
        
        this._cancelSmsHandledWakeLockTimer();

        
        for each (let apnSetting in this.apnSettings.byAPN) {
          if (apnSetting.iface) {
            apnSetting.iface.shutdown();
          }
        }
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        Services.obs.removeObserver(this, kSysClockChangeObserverTopic);
        Services.obs.removeObserver(this, kScreenStateChangedTopic);
        Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
        Services.prefs.removeObserver(kCellBroadcastDisabled, this);
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
        if (network.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
          
          
          if (this._sntp.isExpired()) {
            this._sntp.request();
          }
        }
        break;
      case kScreenStateChangedTopic:
        this.workerMessenger.send("setScreenState", { on: (data === "on") });
        break;
    }
  },

  
  
  _sysMsgListenerReady: false,

  
  
  _radioEnabled: null,

  
  
  _changingRadioPower: false,

  
  dataCallSettings: null,

  apnSettings: null,

  
  
  _clockAutoUpdateEnabled: null,

  
  
  _timezoneAutoUpdateEnabled: null,

  
  
  _lastNitzMessage: null,

  
  _sntp: null,

  
  _cellBroadcastSearchListStr: null,

  handleSettingsChange: function handleSettingsChange(aName, aResult, aMessage) {
    
    
    if (aName === kClockAutoUpdateAvailable &&
        aMessage !== "fromInternalSetting") {
      let isClockAutoUpdateAvailable = this._lastNitzMessage !== null ||
                                       this._sntp.isAvailable();
      if (aResult !== isClockAutoUpdateAvailable) {
        debug("Content processes cannot modify 'time.clock.automatic-update.available'. Restore!");
        
        this.setClockAutoUpdateAvailable(isClockAutoUpdateAvailable);
      }
    }

    
    
    
    if (aName === kTimezoneAutoUpdateAvailable &&
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

  
  handle: function handle(aName, aResult) {
    switch(aName) {
      case "ril.radio.disabled":
        if (DEBUG) this.debug("'ril.radio.disabled' is now " + aResult);
        this._radioEnabled = !aResult;
        this._ensureRadioState();
        break;
      case "ril.radio.preferredNetworkType":
        if (DEBUG) this.debug("'ril.radio.preferredNetworkType' is now " + aResult);
        this.setPreferredNetworkType(aResult);
        break;
      case "ril.data.enabled":
        if (DEBUG) this.debug("'ril.data.enabled' is now " + aResult);
        let enabled;
        if (Array.isArray(aResult)) {
          enabled = aResult[this.clientId];
        } else {
          
          enabled = aResult;
        }
        this.dataCallSettings.oldEnabled = this.dataCallSettings.enabled;
        this.dataCallSettings.enabled = enabled;
        this.updateRILNetworkInterface();
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
      case kClockAutoUpdateEnabled:
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
      case kTimezoneAutoUpdateEnabled:
        this._timezoneAutoUpdateEnabled = aResult;

        if (this._timezoneAutoUpdateEnabled) {
          
          if (this._timezoneAutoUpdateEnabled && this._lastNitzMessage) {
            this.setTimezoneByNitz(this._lastNitzMessage);
          }
        }
        break;
      case kCellBroadcastSearchList:
        if (DEBUG) {
          this.debug("'" + kCellBroadcastSearchList + "' is now " + aResult);
        }
        this.setCellBroadcastSearchList(aResult);
        break;
    }
  },

  handleError: function handleError(aErrorMessage) {
    if (DEBUG) this.debug("There was an error while reading RIL settings.");

    
    this._radioEnabled = true;
    this._ensureRadioState();

    
    this.dataCallSettings.oldEnabled = false;
    this.dataCallSettings.enabled = false;
    this.dataCallSettings.roamingEnabled = false;
    this.apnSettings = {
      byType: {},
      byAPN: {},
    };
  },

  

  setRadioEnabled: function setRadioEnabled(value) {
    if (DEBUG) this.debug("Setting radio power to " + value);
    this.workerMessenger.send("setRadioPower", { on: value });
  },

  rilContext: null,

  

  _sendCfStateChanged: function _sendCfStateChanged(message) {
    gMessageManager.sendMobileConnectionMessage("RIL:CfStateChanged",
                                                this.clientId, message);
  },

  _updateCallingLineIdRestrictionPref:
    function _updateCallingLineIdRestrictionPref(mode) {
    try {
      Services.prefs.setIntPref(kClirModePreference, mode);
      Services.prefs.savePrefFile(null);
      if (DEBUG) {
        this.debug(kClirModePreference + " pref is now " + mode);
      }
    } catch (e) {}
  },

  sendMMI: function sendMMI(target, message) {
    if (DEBUG) this.debug("SendMMI " + JSON.stringify(message));
    this.workerMessenger.send("sendMMI", message, (function(response) {
      if (response.isSetCallForward) {
        this._sendCfStateChanged(response);
      } else if (response.isSetCLIR && response.success) {
        this._updateCallingLineIdRestrictionPref(response.clirMode);
      }

      target.sendAsyncMessage("RIL:SendMMI", response);
      return false;
    }).bind(this));
  },

  setCallForwardingOption: function setCallForwardingOption(target, message) {
    if (DEBUG) this.debug("setCallForwardingOption: " + JSON.stringify(message));
    message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE;
    this.workerMessenger.send("setCallForward", message, (function(response) {
      this._sendCfStateChanged(response);
      target.sendAsyncMessage("RIL:SetCallForwardingOption", response);
      return false;
    }).bind(this));
  },

  setCallingLineIdRestriction: function setCallingLineIdRestriction(target,
                                                                    message) {
    if (DEBUG) {
      this.debug("setCallingLineIdRestriction: " + JSON.stringify(message));
    }
    this.workerMessenger.send("setCLIR", message, (function(response) {
      if (response.success) {
        this._updateCallingLineIdRestrictionPref(response.clirMode);
      }
      target.sendAsyncMessage("RIL:SetCallingLineIdRestriction", response);
      return false;
    }).bind(this));
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

  

















  _countGsm7BitSeptets: function _countGsm7BitSeptets(message, langTable, langShiftTable, strict7BitEncoding) {
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

        
        
        c = '*';
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

  















  _calculateUserDataLength7Bit: function _calculateUserDataLength7Bit(message, strict7BitEncoding) {
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

  










  _calculateUserDataLengthUCS2: function _calculateUserDataLengthUCS2(message) {
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

  




























  _calculateUserDataLength: function _calculateUserDataLength(message, strict7BitEncoding) {
    let options = this._calculateUserDataLength7Bit(message, strict7BitEncoding);
    if (!options) {
      options = this._calculateUserDataLengthUCS2(message);
    }

    if (DEBUG) this.debug("_calculateUserDataLength: " + JSON.stringify(options));
    return options;
  },

  
















  _fragmentText7Bit: function _fragmentText7Bit(text, langTable, langShiftTable, segmentSeptets, strict7BitEncoding) {
    let ret = [];
    let body = "", len = 0;
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

          
          
          c = '*';
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

  









  _fragmentTextUCS2: function _fragmentTextUCS2(text, segmentChars) {
    let ret = [];
    for (let offset = 0; offset < text.length; offset += segmentChars) {
      let str = text.substr(offset, segmentChars);
      ret.push({
        body: str,
        encodedBodyLength: str.length * 2,
      });
    }

    return ret;
  },

  

















  _fragmentText: function _fragmentText(text, options, strict7BitEncoding) {
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

  getSegmentInfoForText: function getSegmentInfoForText(text, request) {
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

  sendSMS: function sendSMS(number, message, silent, request) {
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
      } else if (!this._radioEnabled) {
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
                                           function notifyResult(rv, domMessage) {
            
            let topic = (response.deliveryStatus == RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
                        ? kSmsDeliverySuccessObserverTopic
                        : kSmsDeliveryErrorObserverTopic;
            Services.obs.notifyObservers(domMessage, topic, null);
          });

          
          return false;
        } 

        
        if (context.silent) {
          
          
          
          let sms = context.sms;
          context.request.notifyMessageSent(
            gMobileMessageService.createSmsMessage(sms.id,
                                                   sms.threadId,
                                                   DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                                   sms.deliveryStatus,
                                                   sms.sender,
                                                   sms.receiver,
                                                   sms.body,
                                                   sms.messageClass,
                                                   sms.timestamp,
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
          
          this.broadcastSmsSystemMessage("sms-sent", domMessage);

          if (context.requestStatusReport) {
            context.sms = domMessage;
          }

          context.request.notifyMessageSent(domMessage);
          Services.obs.notifyObservers(domMessage, kSmsSentObserverTopic, null);
        }).bind(this));

        
        return context.requestStatusReport;
      }).bind(this, context)); 
    }).bind(this); 

    let sendingMessage = {
      type: "sms",
      sender: this.getMsisdn(),
      receiver: number,
      body: message,
      deliveryStatusRequested: options.requestStatusReport,
      timestamp: Date.now()
    };

    if (silent) {
      let deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_PENDING;
      let delivery = DOM_MOBILE_MESSAGE_DELIVERY_SENDING;
      let domMessage =
        gMobileMessageService.createSmsMessage(-1, 
                                               0,  
                                               delivery,
                                               deliveryStatus,
                                               sendingMessage.sender,
                                               sendingMessage.receiver,
                                               sendingMessage.body,
                                               "normal", 
                                               sendingMessage.timestamp,
                                               false);
      notifyResult(Cr.NS_OK, domMessage);
      return;
    }

    let id = gMobileMessageDatabaseService.saveSendingMessage(
      sendingMessage, notifyResult);
  },

  registerDataCallCallback: function registerDataCallCallback(callback) {
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

  unregisterDataCallCallback: function unregisterDataCallCallback(callback) {
    if (!this._datacall_callbacks) {
      return;
    }
    let index = this._datacall_callbacks.indexOf(callback);
    if (index != -1) {
      this._datacall_callbacks.splice(index, 1);
      if (DEBUG) this.debug("Unregistering callback: " + callback);
    }
  },

  _deliverDataCallCallback: function _deliverDataCallCallback(name, args) {
    
    
    
    
    
    
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

  setupDataCallByType: function setupDataCallByType(apntype) {
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
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

  deactivateDataCallByType: function deactivateDataCallByType(apntype) {
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
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

  getDataCallStateByType: function getDataCallStateByType(apntype) {
    let apnSetting = this.apnSettings.byType[apntype];
    if (!apnSetting) {
       return RIL.GECKO_NETWORK_STATE_UNKNOWN;
    }
    if (!apnSetting.iface.inConnectedTypes(apntype)) {
      return RIL.GECKO_NETWORK_STATE_DISCONNECTED;
    }
    return apnSetting.iface.state;
  },

  setupDataCall: function setupDataCall(radioTech, apn, user, passwd, chappap, pdptype) {
    this.workerMessenger.send("setupDataCall", { radioTech: radioTech,
                                                 apn: apn,
                                                 user: user,
                                                 passwd: passwd,
                                                 chappap: chappap,
                                                 pdptype: pdptype });
  },

  deactivateDataCall: function deactivateDataCall(cid, reason) {
    this.workerMessenger.send("deactivateDataCall", { cid: cid,
                                                      reason: reason });
  },

  sendWorkerMessage: function sendWorkerMessage(rilMessageType, message,
                                                callback) {
    this.workerMessenger.send(rilMessageType, message, function (response) {
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
                                                 Ci.nsIRILDataCallback]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface,
                                         Ci.nsIRILDataCallback]),

  

  NETWORK_STATE_UNKNOWN:       Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,
  NETWORK_STATE_CONNECTING:    Ci.nsINetworkInterface.CONNECTING,
  NETWORK_STATE_CONNECTED:     Ci.nsINetworkInterface.CONNECTED,
  NETWORK_STATE_DISCONNECTING: Ci.nsINetworkInterface.DISCONNECTING,
  NETWORK_STATE_DISCONNECTED:  Ci.nsINetworkInterface.DISCONNECTED,

  state: Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,

  NETWORK_TYPE_WIFI:        Ci.nsINetworkInterface.NETWORK_TYPE_WIFI,
  NETWORK_TYPE_MOBILE:      Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,
  NETWORK_TYPE_MOBILE_MMS:  Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS,
  NETWORK_TYPE_MOBILE_SUPL: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,
  
  
  
  NETWORK_TYPE_MOBILE_OTHERS: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,

  



  NETWORK_APNRETRY_FACTOR: 8,
  NETWORK_APNRETRY_ORIGIN: 3,
  NETWORK_APNRETRY_MAXRETRIES: 10,

  
  timer: null,

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
    return this.apnSetting.proxy || '';
  },

  get httpProxyPort() {
    return this.apnSetting.port || '';
  },

  debug: function debug(s) {
    dump("-*- RILNetworkInterface[" + this.radioInterface.clientId + ":" +
         this.type + "]: " + s + "\n");
  },

  

  dataCallError: function dataCallError(message) {
    if (message.apn != this.apnSetting.apn) {
      return;
    }
    if (DEBUG) this.debug("Data call error on APN: " + message.apn);
    this.reset();
  },

  dataCallStateChanged: function dataCallStateChanged(datacall) {
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
      if (datacall.state != GECKO_NETWORK_STATE_CONNECTED) {
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

    
    
    
    if (this.radioInterface.apnSettings.byType.default &&
        (this.radioInterface.apnSettings.byType.default.apn ==
         this.apnSetting.apn)) {
      this.radioInterface.updateRILNetworkInterface();
    }

    if (this.state == RIL.GECKO_NETWORK_STATE_UNKNOWN &&
        this.registeredAsNetworkInterface) {
      gNetworkManager.unregisterNetworkInterface(this);
      this.registeredAsNetworkInterface = false;
      this.cid = null;
      this.connectedTypes = [];
      return;
    }

    Services.obs.notifyObservers(this,
                                 kNetworkInterfaceStateChangedTopic,
                                 null);
  },

  receiveDataCallList: function receiveDataCallList(dataCalls, length) {
  },

  

  cid: null,
  registeredAsDataCallCallback: false,
  registeredAsNetworkInterface: false,
  connecting: false,
  apnSetting: null,

  
  apnRetryCounter: 0,

  connectedTypes: null,

  inConnectedTypes: function inConnectedTypes(type) {
    return this.connectedTypes.indexOf(type) != -1;
  },

  get connected() {
    return this.state == RIL.GECKO_NETWORK_STATE_CONNECTED;
  },

  connect: function connect(apntype) {
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

  reset: function reset() {
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

  disconnect: function disconnect(apntype) {
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

