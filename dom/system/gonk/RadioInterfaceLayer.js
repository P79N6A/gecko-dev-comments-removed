














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

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
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const kSysClockChangeObserverTopic       = "system-clock-change";
const kScreenStateChangedTopic           = "screen-state-changed";
const kTimeNitzAutomaticUpdateEnabled    = "time.nitz.automatic-update.enabled";
const kTimeNitzAvailable                 = "time.nitz.available";
const kCellBroadcastSearchList           = "ril.cellbroadcast.searchlist";
const kCellBroadcastDisabled             = "ril.cellbroadcast.disabled";
const kPrefenceChangedObserverTopic      = "nsPref:changed";
const kClirModePreference                = "ril.clirMode";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

const RADIO_POWER_OFF_TIMEOUT = 30000;

const RIL_IPC_MOBILECONNECTION_MSG_NAMES = [
  "RIL:GetRilContext",
  "RIL:GetAvailableNetworks",
  "RIL:SelectNetwork",
  "RIL:SelectNetworkAuto",
  "RIL:SendMMI",
  "RIL:CancelMMI",
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
  "RIL:UpdateIccContact"
];

const RIL_IPC_VOICEMAIL_MSG_NAMES = [
  "RIL:GetVoicemailInfo"
];

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/rilsmsservice;1",
                                   "nsIRilSmsService");

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

XPCOMUtils.defineLazyGetter(this, "PhoneNumberUtils", function () {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  let ns = {};
  Cu.import("resource://gre/modules/RilMessageManager.jsm", ns);
  return ns.RilMessageManager;
});

function RadioInterfaceLayer() {
  let callback = this._receiveMessage.bind(this);
  gMessageManager.registerMessageListeners("icc",
                                           RIL_IPC_ICCMANAGER_MSG_NAMES,
                                           callback);
  gMessageManager.registerMessageListeners("mobileconnection",
                                           RIL_IPC_MOBILECONNECTION_MSG_NAMES,
                                           callback);
  gMessageManager.registerMessageListeners("voicemail",
                                           RIL_IPC_VOICEMAIL_MSG_NAMES,
                                           callback);

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

  _receiveMessage: function _receiveMessage(topic, msg) {
    let clientId = msg.json.clientId || 0;
    let radioInterface = this.getRadioInterface(clientId);
    if (!radioInterface) {
      if (DEBUG) debug("No such radio interface: " + clientId);
      return null;
    }

    return radioInterface.receiveMessage(msg);
  },

  



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

  
  
  lock.get(kTimeNitzAutomaticUpdateEnabled, this);

  
  this.setNitzAvailable(false);

  
  
  lock.get(kCellBroadcastSearchList, this);

  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.prefs.addObserver(kCellBroadcastDisabled, this, false);
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
        gSmsService.notifyMessageReceived(message);
        break;
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
      default:
        throw new Error("Don't know about this message type: " +
                        message.rilMessageType);
    }
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

  


  setNitzAvailable: function setNitzAvailable(value) {
    gSettingsService.createLock().set(kTimeNitzAvailable, value, null,
                                      "fromInternalSetting");
  },

  


  setNitzTime: function setNitzTime(message) {
    
    
    gTimeService.set(
      message.networkTimeInMS + (Date.now() - message.receiveTimeInMS));

    
    
    
    
    
    
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
    
    this.setNitzAvailable(true);

    
    this._lastNitzMessage = message;

    
    if (this._nitzAutomaticUpdateEnabled) {
      this.setNitzTime(message);
    }
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

    let iccInfoChanged = !oldIccInfo ||
                          oldIccInfo.iccid != message.iccid ||
                          oldIccInfo.mcc != message.mcc ||
                          oldIccInfo.mnc != message.mnc ||
                          oldIccInfo.spn != message.spn ||
                          oldIccInfo.isDisplayNetworkNameRequired != message.isDisplayNetworkNameRequired ||
                          oldIccInfo.isDisplaySpnRequired != message.isDisplaySpnRequired ||
                          oldIccInfo.msisdn != message.msisdn;
    if (!iccInfoChanged) {
      return;
    }
    
    
    gMessageManager.sendIccMessage("RIL:IccInfoChanged",
                                   this.clientId, message);

    
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
        
        for each (let apnSetting in this.apnSettings.byAPN) {
          if (apnSetting.iface) {
            apnSetting.iface.shutdown();
          }
        }
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        Services.obs.removeObserver(this, kSysClockChangeObserverTopic);
        Services.obs.removeObserver(this, kScreenStateChangedTopic);
        Services.prefs.removeObserver(kCellBroadcastDisabled, this);
        break;
      case kSysClockChangeObserverTopic:
        if (this._lastNitzMessage) {
          this._lastNitzMessage.receiveTimeInMS += parseInt(data, 10);
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

  
  
  _nitzAutomaticUpdateEnabled: null,

  
  
  _lastNitzMessage: null,

  
  _cellBroadcastSearchListStr: null,

  handleSettingsChange: function handleSettingsChange(aName, aResult, aMessage) {
    
    
    let isNitzAvailable = (this._lastNitzMessage !== null);
    if (aName === kTimeNitzAvailable && aMessage !== "fromInternalSetting" &&
        aResult !== isNitzAvailable) {
      if (DEBUG) {
        this.debug("Content processes cannot modify 'time.nitz.available'. Restore!");
      }
      
      this.setNitzAvailable(isNitzAvailable);
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
      case kTimeNitzAutomaticUpdateEnabled:
        this._nitzAutomaticUpdateEnabled = aResult;

        
        if (this._nitzAutomaticUpdateEnabled && this._lastNitzMessage) {
          this.setNitzTime(this._lastNitzMessage);
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

