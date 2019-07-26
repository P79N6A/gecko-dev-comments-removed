














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
};

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");

const nsIAudioManager = Ci.nsIAudioManager;
const nsITelephonyProvider = Ci.nsITelephonyProvider;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const kSysClockChangeObserverTopic       = "system-clock-change";
const kScreenStateChangedTopic           = "screen-state-changed";
const kTimeNitzAutomaticUpdateEnabled    = "time.nitz.automatic-update.enabled";
const kTimeNitzAvailable                 = "time.nitz.available";
const kCellBroadcastSearchList           = "ril.cellbroadcast.searchlist";
const kCellBroadcastDisabled             = "ril.cellbroadcast.disabled";
const kPrefenceChangedObserverTopic      = "nsPref:changed";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

const CALL_WAKELOCK_TIMEOUT              = 5000;

const RIL_IPC_TELEPHONY_MSG_NAMES = [
  "RIL:EnumerateCalls",
  "RIL:GetMicrophoneMuted",
  "RIL:SetMicrophoneMuted",
  "RIL:GetSpeakerEnabled",
  "RIL:SetSpeakerEnabled",
  "RIL:StartTone",
  "RIL:StopTone",
  "RIL:Dial",
  "RIL:DialEmergency",
  "RIL:HangUp",
  "RIL:AnswerCall",
  "RIL:RejectCall",
  "RIL:HoldCall",
  "RIL:ResumeCall",
  "RIL:RegisterTelephonyMsg"
];

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
  "RIL:SetCallWaitingOption",
  "RIL:GetCallWaitingOption",
  "RIL:SetCallingLineIdRestriction",
  "RIL:GetCallingLineIdRestriction",
  "RIL:SetRoamingPreference",
  "RIL:GetRoamingPreference"
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

function convertRILCallState(state) {
  switch (state) {
    case RIL.CALL_STATE_ACTIVE:
      return nsITelephonyProvider.CALL_STATE_CONNECTED;
    case RIL.CALL_STATE_HOLDING:
      return nsITelephonyProvider.CALL_STATE_HELD;
    case RIL.CALL_STATE_DIALING:
      return nsITelephonyProvider.CALL_STATE_DIALING;
    case RIL.CALL_STATE_ALERTING:
      return nsITelephonyProvider.CALL_STATE_ALERTING;
    case RIL.CALL_STATE_INCOMING:
    case RIL.CALL_STATE_WAITING:
      return nsITelephonyProvider.CALL_STATE_INCOMING;
    default:
      throw new Error("Unknown rilCallState: " + state);
  }
}





let FakeAudioManager = {
  microphoneMuted: false,
  masterVolume: 1.0,
  masterMuted: false,
  phoneState: nsIAudioManager.PHONE_STATE_CURRENT,
  _forceForUse: {},
  setForceForUse: function setForceForUse(usage, force) {
    this._forceForUse[usage] = force;
  },
  getForceForUse: function setForceForUse(usage) {
    return this._forceForUse[usage] || nsIAudioManager.FORCE_NONE;
  }
};

XPCOMUtils.defineLazyGetter(this, "gAudioManager", function getAudioManager() {
  try {
    return Cc["@mozilla.org/telephony/audiomanager;1"]
             .getService(nsIAudioManager);
  } catch (ex) {
    
    if (DEBUG) debug("Using fake audio manager.");
    return FakeAudioManager;
  }
});

XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    ril: null,

    targetsByRequestId: {},
    
    
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
      for (let msgname of RIL_IPC_TELEPHONY_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
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
      for (let msgname of RIL_IPC_TELEPHONY_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
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
        return;
      }

      if (RIL_IPC_TELEPHONY_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("telephony")) {
          if (DEBUG) {
            debug("Telephony message " + msg.name +
                  " from a content process with no 'telephony' privileges.");
          }
          return null;
        }
      } else if (RIL_IPC_MOBILECONNECTION_MSG_NAMES.indexOf(msg.name) != -1) {
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
        case "RIL:RegisterTelephonyMsg":
          this._registerMessageTarget("telephony", msg.target);
          return;
        case "RIL:RegisterMobileConnectionMsg":
          this._registerMessageTarget("mobileconnection", msg.target);
          return;
        case "RIL:RegisterIccMsg":
          this._registerMessageTarget("icc", msg.target);
          return;
        case "RIL:RegisterVoicemailMsg":
          this._registerMessageTarget("voicemail", msg.target);
          return;
        case "RIL:RegisterCellBroadcastMsg":
          this._registerMessageTarget("cellbroadcast", msg.target);
          return;
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

    sendTelephonyMessage: function sendTelephonyMessage(message, clientId, data) {
      this._sendTargetMessage("telephony", message, {
        clientId: clientId,
        data: data
      });
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
    },

    saveRequestTarget: function saveRequestTarget(msg) {
      let requestId = msg.json.data.requestId;
      if (!requestId) {
        
        return;
      }

      this.targetsByRequestId[requestId] = msg.target;
    },

    sendRequestResults: function sendRequestResults(requestType, options) {
      let target = this.targetsByRequestId[options.requestId];
      delete this.targetsByRequestId[options.requestId];

      if (!target) {
        return;
      }

      target.sendAsyncMessage(requestType, options);
    }
  };
});

function RadioInterfaceLayer() {
  gMessageManager.init(this);

  let options = {
    debug: debugPref,
    cellBroadcastDisabled: false
  };

  try {
    options.cellBroadcastDisabled =
      Services.prefs.getBoolPref(kCellBroadcastDisabled);
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

function RadioInterface(options) {
  this.clientId = options.clientId;

  this.dataCallSettings = {
    oldEnabled: false,
    enabled: false,
    roamingEnabled: false
  };

  
  
  
  
  this.apnSettings = {
      byType: {},
      byAPN: {}
  };

  if (DEBUG) this.debug("Starting RIL Worker[" + this.clientId + "]");
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  
  options.rilMessageType = "setInitialOptions";
  this.worker.postMessage(options);

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

  this._sentSmsEnvelopes = {};

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] = this.handleSmsWdpPortPush.bind(this);

  gSystemWorkerManager.registerRilWorker(this.clientId, this.worker);
}
RadioInterface.prototype = {

  classID:   RADIOINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACE_CID,
                                    classDescription: "RadioInterface",
                                    interfaces: [Ci.nsIRadioInterface]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioInterface,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  debug: function debug(s) {
    dump("-*- RadioInterface[" + this.clientId + "]: " + s + "\n");
  },

  


  receiveMessage: function receiveMessage(msg) {
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
      case "RIL:EnumerateCalls":
        gMessageManager.saveRequestTarget(msg);
        this.enumerateCalls(msg.json.data);
        break;
      case "RIL:GetMicrophoneMuted":
        
        return this.microphoneMuted;
      case "RIL:SetMicrophoneMuted":
        this.microphoneMuted = msg.json.data;
        break;
      case "RIL:GetSpeakerEnabled":
        
        return this.speakerEnabled;
      case "RIL:SetSpeakerEnabled":
        this.speakerEnabled = msg.json.data;
        break;
      case "RIL:StartTone":
        this.startTone(msg.json.data);
        break;
      case "RIL:StopTone":
        this.stopTone();
        break;
      case "RIL:Dial":
        this.dial(msg.json.data);
        break;
      case "RIL:DialEmergency":
        this.dialEmergency(msg.json.data);
        break;
      case "RIL:HangUp":
        this.hangUp(msg.json.data);
        break;
      case "RIL:AnswerCall":
        this.answerCall(msg.json.data);
        break;
      case "RIL:RejectCall":
        this.rejectCall(msg.json.data);
        break;
      case "RIL:HoldCall":
        this.holdCall(msg.json.data);
        break;
      case "RIL:ResumeCall":
        this.resumeCall(msg.json.data);
        break;
      case "RIL:GetAvailableNetworks":
        gMessageManager.saveRequestTarget(msg);
        this.getAvailableNetworks(msg.json.data.requestId);
        break;
      case "RIL:SelectNetwork":
        gMessageManager.saveRequestTarget(msg);
        this.selectNetwork(msg.json.data);
        break;
      case "RIL:SelectNetworkAuto":
        gMessageManager.saveRequestTarget(msg);
        this.selectNetworkAuto(msg.json.data.requestId);
        break;
      case "RIL:GetCardLockState":
        gMessageManager.saveRequestTarget(msg);
        this.getCardLockState(msg.json.data);
        break;
      case "RIL:UnlockCardLock":
        gMessageManager.saveRequestTarget(msg);
        this.unlockCardLock(msg.json.data);
        break;
      case "RIL:SetCardLock":
        gMessageManager.saveRequestTarget(msg);
        this.setCardLock(msg.json.data);
        break;
      case "RIL:GetCardLockRetryCount":
        gMessageManager.saveRequestTarget(msg);
        this.getCardLockRetryCount(msg.json.data);
        break;
      case "RIL:SendMMI":
        gMessageManager.saveRequestTarget(msg);
        this.sendMMI(msg.json.data);
        break;
      case "RIL:CancelMMI":
        gMessageManager.saveRequestTarget(msg);
        this.cancelMMI(msg.json.data);
        break;
      case "RIL:SendStkResponse":
        this.sendStkResponse(msg.json.data);
        break;
      case "RIL:SendStkMenuSelection":
        this.sendStkMenuSelection(msg.json.data);
        break;
      case "RIL:SendStkTimerExpiration":
        this.sendStkTimerExpiration(msg.json.data);
        break;
      case "RIL:SendStkEventDownload":
        this.sendStkEventDownload(msg.json.data);
        break;
      case "RIL:IccOpenChannel":
        gMessageManager.saveRequestTarget(msg);
        this.iccOpenChannel(msg.json.data);
        break;
      case "RIL:IccCloseChannel":
        gMessageManager.saveRequestTarget(msg);
        this.iccCloseChannel(msg.json.data);
        break;
      case "RIL:IccExchangeAPDU":
        gMessageManager.saveRequestTarget(msg);
        this.iccExchangeAPDU(msg.json.data);
        break;
      case "RIL:ReadIccContacts":
        gMessageManager.saveRequestTarget(msg);
        this.readIccContacts(msg.json.data);
        break;
      case "RIL:UpdateIccContact":
        gMessageManager.saveRequestTarget(msg);
        this.updateIccContact(msg.json.data);
        break;
      case "RIL:SetCallForwardingOption":
        gMessageManager.saveRequestTarget(msg);
        this.setCallForwardingOption(msg.json.data);
        break;
      case "RIL:GetCallForwardingOption":
        gMessageManager.saveRequestTarget(msg);
        this.getCallForwardingOption(msg.json.data);
        break;
      case "RIL:SetCallBarringOption":
        gMessageManager.saveRequestTarget(msg);
        this.setCallBarringOption(msg.json.data);
        break;
      case "RIL:GetCallBarringOption":
        gMessageManager.saveRequestTarget(msg);
        this.getCallBarringOption(msg.json.data);
        break;
      case "RIL:SetCallWaitingOption":
        gMessageManager.saveRequestTarget(msg);
        this.setCallWaitingOption(msg.json.data);
        break;
      case "RIL:GetCallWaitingOption":
        gMessageManager.saveRequestTarget(msg);
        this.getCallWaitingOption(msg.json.data);
        break;
      case "RIL:SetCallingLineIdRestriction":
        gMessageManager.saveRequestTarget(msg);
        this.setCallingLineIdRestriction(msg.json.data);
        break;
      case "RIL:GetCallingLineIdRestriction":
        gMessageManager.saveRequestTarget(msg);
        this.getCallingLineIdRestriction(msg.json.data);
        break;
      case "RIL:GetVoicemailInfo":
        
        return this.voicemailInfo;
      case "RIL:SetRoamingPreference":
        gMessageManager.saveRequestTarget(msg);
        this.setRoamingPreference(msg.json.data);
        break;
      case "RIL:GetRoamingPreference":
        gMessageManager.saveRequestTarget(msg);
        this.getRoamingPreference(msg.json.data);
        break;
    }
  },

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
    switch (message.rilMessageType) {
      case "callRing":
        this.handleCallRing();
        break;
      case "callStateChange":
        
        this.handleCallStateChange(message.call);
        break;
      case "callDisconnected":
        
        this.handleCallDisconnected(message.call);
        break;
      case "enumerateCalls":
        
        this.handleEnumerateCalls(message);
        break;
      case "callError":
        this.handleCallError(message);
        break;
      case "iccOpenChannel":
        this.handleIccOpenChannel(message);
        break;
      case "iccCloseChannel":
        this.handleIccCloseChannel(message);
        break;
      case "iccExchangeAPDU":
        this.handleIccExchangeAPDU(message);
        break;
      case "getAvailableNetworks":
        this.handleGetAvailableNetworks(message);
        break;
      case "selectNetwork":
        this.handleSelectNetwork(message);
        break;
      case "selectNetworkAuto":
        this.handleSelectNetworkAuto(message);
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
          this.worker.postMessage({
            rilMessageType: "ackSMS",
            result: RIL.PDU_FCS_OK
          });
        }
        return;
      case "sms-sent":
        this.handleSmsSent(message);
        return;
      case "sms-delivery":
        this.handleSmsDelivery(message);
        return;
      case "sms-send-failed":
        this.handleSmsSendFailed(message);
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
      case "iccGetCardLockState":
      case "iccSetCardLock":
      case "iccUnlockCardLock":
        this.handleIccCardLockResult(message);
        break;
      case "iccGetCardLockRetryCount":
        this.handleIccCardLockRetryCount(message);
        break;
      case "icccontacts":
        this.handleReadIccContacts(message);
        break;
      case "icccontactupdate":
        this.handleUpdateIccContact(message);
        break;
      case "iccmbdn":
        this.handleIccMbdn(message);
        break;
      case "USSDReceived":
        if (DEBUG) this.debug("USSDReceived " + JSON.stringify(message));
        this.handleUSSDReceived(message);
        break;
      case "sendMMI":
      case "sendUSSD":
        this.handleSendMMI(message);
        break;
      case "cancelMMI":
      case "cancelUSSD":
        this.handleCancelMMI(message);
        break;
      case "stkcommand":
        this.handleStkProactiveCommand(message);
        break;
      case "stksessionend":
        gMessageManager.sendIccMessage("RIL:StkSessionEnd", this.clientId, null);
        break;
      case "setPreferredNetworkType":
        this.handleSetPreferredNetworkType(message);
        break;
      case "queryCallForwardStatus":
        this.handleQueryCallForwardStatus(message);
        break;
      case "setCallForward":
        this.handleSetCallForward(message);
        break;
      case "queryCallBarringStatus":
        this.handleQueryCallBarringStatus(message);
        break;
      case "setCallBarring":
        this.handleSetCallBarring(message);
        break;
      case "queryCallWaiting":
        this.handleQueryCallWaiting(message);
        break;
      case "setCallWaiting":
        this.handleSetCallWaiting(message);
        break;
      case "getCLIR":
        this.handleGetCLIR(message);
        break;
      case "setCLIR":
        this.handleSetCLIR(message);
        break;
      case "setCellBroadcastSearchList":
        this.handleSetCellBroadcastSearchList(message);
        break;
      case "setRadioEnabled":
        let lock = gSettingsService.createLock();
        lock.set("ril.radio.disabled", !message.on, null, null);
        break;
      case "setRoamingPreference":
        this.handleSetRoamingPreference(message);
        break;
      case "queryRoamingPreference":
        this.handleQueryRoamingPreference(message);
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

    
    if (voiceMessage) {
      voiceMessage.batch = true;
      this.updateVoiceConnection(voiceMessage);
    }

    if (dataMessage) {
      dataMessage.batch = true;
      this.updateDataConnection(dataMessage);
    }

    if (operatorMessage) {
      operatorMessage.batch = true;
      this.handleOperatorChange(operatorMessage);
    }

    let voice = this.rilContext.voice;
    let data = this.rilContext.data;

    this.checkRoamingBetweenOperators(voice);
    this.checkRoamingBetweenOperators(data);

    if (voiceMessage || operatorMessage) {
      gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                  this.clientId, voice);
    }
    if (dataMessage || operatorMessage) {
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

  






  updateVoiceConnection: function updateVoiceConnection(newInfo) {
    let voiceInfo = this.rilContext.voice;
    voiceInfo.state = newInfo.state;
    voiceInfo.connected = newInfo.connected;
    voiceInfo.roaming = newInfo.roaming;
    voiceInfo.emergencyCallsOnly = newInfo.emergencyCallsOnly;
    voiceInfo.type = newInfo.type;

    
    
    if (newInfo.regState !== RIL.NETWORK_CREG_STATE_REGISTERED_HOME &&
        newInfo.regState !== RIL.NETWORK_CREG_STATE_REGISTERED_ROAMING) {
      voiceInfo.cell = null;
      voiceInfo.network = null;
      voiceInfo.signalStrength = null;
      voiceInfo.relSignalStrength = null;
    } else {
      voiceInfo.cell = newInfo.cell;
    }

    if (!newInfo.batch) {
      gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                  this.clientId, voiceInfo);
    }
  },

  updateDataConnection: function updateDataConnection(newInfo) {
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

    
    
    if (newInfo.regState !== RIL.NETWORK_CREG_STATE_REGISTERED_HOME &&
        newInfo.regState !== RIL.NETWORK_CREG_STATE_REGISTERED_ROAMING) {
      dataInfo.cell = null;
      dataInfo.network = null;
      dataInfo.signalStrength = null;
      dataInfo.relSignalStrength = null;
    } else {
      dataInfo.cell = newInfo.cell;
    }

    if (!newInfo.batch) {
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

    this.worker.postMessage({rilMessageType: "setPreferredNetworkType",
                             networkType: networkType});
  },

  handleSetPreferredNetworkType: function handleSetPreferredNetworkType(message) {
    if ((this._preferredNetworkType != null) && !message.success) {
      gSettingsService.createLock().set("ril.radio.preferredNetworkType",
                                        this._preferredNetworkType,
                                        null);
      return;
    }

    this._preferredNetworkType = message.networkType;
    if (DEBUG) {
      this.debug("_preferredNetworkType is now " +
                 RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[this._preferredNetworkType]);
    }
  },

  setCellBroadcastSearchList: function setCellBroadcastSearchList(newSearchListStr) {
    if (newSearchListStr == this._cellBroadcastSearchListStr) {
      return;
    }

    this.worker.postMessage({
      rilMessageType: "setCellBroadcastSearchList",
      searchListStr: newSearchListStr
    });
  },

  handleSetCellBroadcastSearchList: function handleSetCellBroadcastSearchList(message) {
    if (message.rilRequestError != RIL.ERROR_SUCCESS) {
      let lock = gSettingsService.createLock();
      lock.set(kCellBroadcastSearchList, this._cellBroadcastSearchListStr, null);
      return;
    }

    this._cellBroadcastSearchListStr = message.searchListStr;
  },

  handleSignalStrengthChange: function handleSignalStrengthChange(message) {
    let voiceInfo = this.rilContext.voice;
    
    if (voiceInfo.signalStrength != message.gsmDBM ||
        voiceInfo.relSignalStrength != message.gsmRelative) {
      voiceInfo.signalStrength = message.gsmDBM;
      voiceInfo.relSignalStrength = message.gsmRelative;
      gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                  this.clientId, voiceInfo);
    }

    let dataInfo = this.rilContext.data;
    if (dataInfo.signalStrength != message.gsmDBM ||
        dataInfo.relSignalStrength != message.gsmRelative) {
      dataInfo.signalStrength = message.gsmDBM;
      dataInfo.relSignalStrength = message.gsmRelative;
      gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                  this.clientId, dataInfo);
    }
  },

  networkChanged: function networkChanged(srcNetwork, destNetwork) {
    return !destNetwork ||
      destNetwork.longName != srcNetwork.longName ||
      destNetwork.shortName != srcNetwork.shortName ||
      destNetwork.mnc != srcNetwork.mnc ||
      destNetwork.mcc != srcNetwork.mcc;
  },

  handleOperatorChange: function handleOperatorChange(message) {
    let voice = this.rilContext.voice;
    let data = this.rilContext.data;

    if (this.networkChanged(message, voice.network)) {
      
      if (message.mcc && message.mnc) {
        try {
          Services.prefs.setCharPref("ril.lastKnownNetwork",
                                     message.mcc + "-" + message.mnc);
        } catch (e) {}
      }

      voice.network = message;
      if (!message.batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:VoiceInfoChanged",
                                                    this.clientId, voice);
      }
    }

    if (this.networkChanged(message, data.network)) {
      data.network = message;
      if (!message.batch) {
        gMessageManager.sendMobileConnectionMessage("RIL:DataInfoChanged",
                                                    this.clientId, data);
      }
    }
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
      this.setRadioEnabled(true);
    }
    if (this.rilContext.radioState == RIL.GECKO_RADIOSTATE_READY &&
        !this._radioEnabled) {
      this.setRadioEnabled(false);
    }
  },

  









  updateApnSettings: function updateApnSettings(allApnSettings) {
    
    let simNumber = 1;
    for (let simId = 0; simId < simNumber; simId++) {
      let thisSimApnSettings = allApnSettings[simId];
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

  


  _activeCall: null,
  updateCallAudioState: function updateCallAudioState(call) {
    switch (call.state) {
      case nsITelephonyProvider.CALL_STATE_DIALING: 
      case nsITelephonyProvider.CALL_STATE_ALERTING:
      case nsITelephonyProvider.CALL_STATE_CONNECTED:
        call.isActive = true;
        this._activeCall = call;
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
        if (this.speakerEnabled) {
          gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                       nsIAudioManager.FORCE_SPEAKER);
        }
        if (DEBUG) {
          this.debug("Active call, put audio system into PHONE_STATE_IN_CALL: "
                     + gAudioManager.phoneState);
        }
        break;
      case nsITelephonyProvider.CALL_STATE_INCOMING:
        call.isActive = false;
        if (!this._activeCall) {
          
          
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_RINGTONE;
          if (DEBUG) {
            this.debug("Incoming call, put audio system into " +
                       "PHONE_STATE_RINGTONE: " + gAudioManager.phoneState);
          }
        }
        break;
      case nsITelephonyProvider.CALL_STATE_HELD: 
      case nsITelephonyProvider.CALL_STATE_DISCONNECTED:
        call.isActive = false;
        if (this._activeCall &&
            this._activeCall.callIndex == call.callIndex) {
          
          this._activeCall = null;
        }

        if (!this._activeCall) {
          
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
          if (DEBUG) {
            this.debug("No active call, put audio system into " +
                       "PHONE_STATE_NORMAL: " + gAudioManager.phoneState);
          }
        }
        break;
    }
  },

  _callRingWakeLock: null,
  _callRingWakeLockTimer: null,
  _cancelCallRingWakeLockTimer: function _cancelCallRingWakeLockTimer() {
    if (this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer.cancel();
    }
    if (this._callRingWakeLock) {
      this._callRingWakeLock.unlock();
      this._callRingWakeLock = null;
    }
  },

  





  handleCallRing: function handleCallRing() {
    if (!this._callRingWakeLock) {
      this._callRingWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer =
        Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this._callRingWakeLockTimer
        .initWithCallback(this._cancelCallRingWakeLockTimer.bind(this),
                          CALL_WAKELOCK_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

    gSystemMessenger.broadcastMessage("telephony-new-call", {});
  },

  



  handleCallStateChange: function handleCallStateChange(call) {
    if (DEBUG) this.debug("handleCallStateChange: " + JSON.stringify(call));
    call.state = convertRILCallState(call.state);

    if (call.state == nsITelephonyProvider.CALL_STATE_DIALING) {
      gSystemMessenger.broadcastMessage("telephony-new-call", {});
    }
    this.updateCallAudioState(call);
    gMessageManager.sendTelephonyMessage("RIL:CallStateChanged",
                                         this.clientId, call);
  },

  


  handleCallDisconnected: function handleCallDisconnected(call) {
    if (DEBUG) this.debug("handleCallDisconnected: " + JSON.stringify(call));
    call.state = nsITelephonyProvider.CALL_STATE_DISCONNECTED;
    let duration = ("started" in call && typeof call.started == "number") ?
      new Date().getTime() - call.started : 0;
    let data = {
      number: call.number,
      duration: duration,
      direction: call.isOutgoing ? "outgoing" : "incoming"
    };
    gSystemMessenger.broadcastMessage("telephony-call-ended", data);
    this.updateCallAudioState(call);
    gMessageManager.sendTelephonyMessage("RIL:CallStateChanged",
                                         this.clientId, call);
  },

  


  handleEnumerateCalls: function handleEnumerateCalls(options) {
    if (DEBUG) this.debug("handleEnumerateCalls: " + JSON.stringify(options));
    for (let i in options.calls) {
      options.calls[i].state = convertRILCallState(options.calls[i].state);
      options.calls[i].isActive = this._activeCall ?
        options.calls[i].callIndex == this._activeCall.callIndex : false;
    }
    gMessageManager.sendRequestResults("RIL:EnumerateCalls", options);
  },

  handleReadIccContacts: function handleReadIccContacts(message) {
    if (DEBUG) this.debug("handleReadIccContacts: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:ReadIccContacts", message);
  },

  handleUpdateIccContact: function handleUpdateIccContact(message) {
    if (DEBUG) this.debug("handleUpdateIccContact: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:UpdateIccContact", message);
  },

  


  handleIccOpenChannel: function handleIccOpenChannel(message) {
    if (DEBUG) this.debug("handleIccOpenChannel: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:IccOpenChannel", message);
  },

  


  handleIccCloseChannel: function handleIccCloseChannel(message) {
    if (DEBUG) this.debug("handleIccCloseChannel: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:IccCloseChannel", message);
  },

  


  handleIccExchangeAPDU: function handleIccExchangeAPDU(message) {
    if (DEBUG) this.debug("handleIccExchangeAPDU: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:IccExchangeAPDU", message);
  },

  


  handleGetAvailableNetworks: function handleGetAvailableNetworks(message) {
    if (DEBUG) this.debug("handleGetAvailableNetworks: " + JSON.stringify(message));

    gMessageManager.sendRequestResults("RIL:GetAvailableNetworks", message);
  },

  


  updateNetworkSelectionMode: function updateNetworkSelectionMode(message) {
    if (DEBUG) this.debug("updateNetworkSelectionMode: " + JSON.stringify(message));
    this.rilContext.networkSelectionMode = message.mode;
    gMessageManager.sendMobileConnectionMessage("RIL:NetworkSelectionModeChanged",
                                                this.clientId, message);
  },

  


  handleSelectNetwork: function handleSelectNetwork(message) {
    if (DEBUG) this.debug("handleSelectNetwork: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SelectNetwork", message);
  },

  


  handleSelectNetworkAuto: function handleSelectNetworkAuto(message) {
    if (DEBUG) this.debug("handleSelectNetworkAuto: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SelectNetworkAuto", message);
  },

  


  handleCallError: function handleCallError(message) {
    gMessageManager.sendTelephonyMessage("RIL:CallError",
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

  portAddressedSmsApps: null,
  handleSmsReceived: function handleSmsReceived(message) {
    if (DEBUG) this.debug("handleSmsReceived: " + JSON.stringify(message));

    
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

      
      message.rilMessageType = "ackSMS";
      if (!success) {
        message.result = RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED;
      }
      this.worker.postMessage(message);

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

  


  _sentSmsEnvelopes: null,
  createSmsEnvelope: function createSmsEnvelope(options) {
    let i;
    for (i = 1; this._sentSmsEnvelopes[i]; i++) {
      
    }

    if (DEBUG) this.debug("createSmsEnvelope: assigned " + i);
    this._sentSmsEnvelopes[i] = options;
    return i;
  },

  handleSmsSent: function handleSmsSent(message) {
    if (DEBUG) this.debug("handleSmsSent: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }

    gMobileMessageDatabaseService.setMessageDelivery(options.sms.id,
                                                     null,
                                                     DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                                     options.sms.deliveryStatus,
                                                     function notifyResult(rv, domMessage) {
      
      this.broadcastSmsSystemMessage("sms-sent", domMessage);

      if (!options.requestStatusReport) {
        
        delete this._sentSmsEnvelopes[message.envelopeId];
      } else {
        options.sms = domMessage;
      }

      options.request.notifyMessageSent(domMessage);
      Services.obs.notifyObservers(domMessage, kSmsSentObserverTopic, null);
    }.bind(this));
  },

  handleSmsDelivery: function handleSmsDelivery(message) {
    if (DEBUG) this.debug("handleSmsDelivery: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }
    delete this._sentSmsEnvelopes[message.envelopeId];

    gMobileMessageDatabaseService.setMessageDelivery(options.sms.id,
                                                     null,
                                                     options.sms.delivery,
                                                     message.deliveryStatus,
                                                     function notifyResult(rv, domMessage) {
      
      let topic = (message.deliveryStatus == RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
                  ? kSmsDeliverySuccessObserverTopic
                  : kSmsDeliveryErrorObserverTopic;
      Services.obs.notifyObservers(domMessage, topic, null);
    }.bind(this));
  },

  handleSmsSendFailed: function handleSmsSendFailed(message) {
    if (DEBUG) this.debug("handleSmsSendFailed: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }
    delete this._sentSmsEnvelopes[message.envelopeId];

    let error = Ci.nsIMobileMessageCallback.UNKNOWN_ERROR;
    switch (message.errorMsg) {
      case RIL.ERROR_RADIO_NOT_AVAILABLE:
        error = Ci.nsIMobileMessageCallback.NO_SIGNAL_ERROR;
        break;
    }

    gMobileMessageDatabaseService.setMessageDelivery(options.sms.id,
                                                     null,
                                                     DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                                     RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                                     function notifyResult(rv, domMessage) {
      
      options.request.notifySendMessageFailed(error);
      Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
    }.bind(this));
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

  handleIccCardLockResult: function handleIccCardLockResult(message) {
    gMessageManager.sendRequestResults("RIL:CardLockResult", message);
  },

  handleIccCardLockRetryCount: function handleIccCardLockRetryCount(message) {
    gMessageManager.sendRequestResults("RIL:CardLockRetryCount", message);
  },

  handleUSSDReceived: function handleUSSDReceived(ussd) {
    if (DEBUG) this.debug("handleUSSDReceived " + JSON.stringify(ussd));
    gSystemMessenger.broadcastMessage("ussd-received", ussd);
    gMessageManager.sendMobileConnectionMessage("RIL:USSDReceived",
                                                this.clientId, ussd);
  },

  handleSendMMI: function handleSendMMI(message) {
    if (DEBUG) this.debug("handleSendMMI " + JSON.stringify(message));
    let messageType = message.success ? "RIL:SendMMI:Return:OK" :
                                        "RIL:SendMMI:Return:KO";
    gMessageManager.sendRequestResults(messageType, message);
  },

  handleCancelMMI: function handleCancelMMI(message) {
    if (DEBUG) this.debug("handleCancelMMI " + JSON.stringify(message));
    let messageType = message.success ? "RIL:CancelMMI:Return:OK" :
                                        "RIL:CancelMMI:Return:KO";
    gMessageManager.sendRequestResults(messageType, message);
  },

  handleStkProactiveCommand: function handleStkProactiveCommand(message) {
    if (DEBUG) this.debug("handleStkProactiveCommand " + JSON.stringify(message));
    gSystemMessenger.broadcastMessage("icc-stkcommand", message);
    gMessageManager.sendIccMessage("RIL:StkCommand", this.clientId, message);
  },

  handleQueryCallForwardStatus: function handleQueryCallForwardStatus(message) {
    if (DEBUG) this.debug("handleQueryCallForwardStatus: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:GetCallForwardingOption", message);
  },

  handleSetCallForward: function handleSetCallForward(message) {
    if (DEBUG) this.debug("handleSetCallForward: " + JSON.stringify(message));
    gMessageManager.sendMobileConnectionMessage("RIL:CfStateChanged",
                                                this.clientId, message);

    let messageType;
    if (message.isSendMMI) {
      messageType = message.success ? "RIL:SendMMI:Return:OK" :
                                      "RIL:SendMMI:Return:KO";
    } else {
      messageType = "RIL:SetCallForwardingOption";
    }
    gMessageManager.sendRequestResults(messageType, message);
  },

  handleQueryCallBarringStatus: function handleQueryCallBarringStatus(message) {
    if (DEBUG) this.debug("handleQueryCallBarringStatus: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:GetCallBarringOption", message);
  },

  handleSetCallBarring: function handleSetCallBarring(message) {
    if (DEBUG) this.debug("handleSetCallBarring: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SetCallBarringOption", message);
  },

  handleQueryCallWaiting: function handleQueryCallWaiting(message) {
    if (DEBUG) this.debug("handleQueryCallWaiting: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:GetCallWaitingOption", message);
  },

  handleSetCallWaiting: function handleSetCallWaiting(message) {
    if (DEBUG) this.debug("handleSetCallWaiting: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SetCallWaitingOption", message);
  },

  handleGetCLIR: function handleGetCLIR(message) {
    if (DEBUG) this.debug("handleGetCLIR: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:GetCallingLineIdRestriction",
                                       message);
  },

  handleSetCLIR: function handleSetCLIR(message) {
    if (DEBUG) this.debug("handleSetCLIR: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SetCallingLineIdRestriction",
                                       message);
  },

  handleSetRoamingPreference: function handleSetRoamingPreference(message) {
    if (DEBUG) this.debug("handleSetRoamingPreference: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:SetRoamingPreference", message);
  },

  handleQueryRoamingPreference: function handleQueryRoamingPreference(message) {
    if (DEBUG) this.debug("handleQueryRoamingPreference: " + JSON.stringify(message));
    gMessageManager.sendRequestResults("RIL:GetRoamingPreference", message);
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
          this.worker.postMessage({
            rilMessageType: "setCellBroadcastDisabled",
            disabled: value
          });
        }
        break;
      case "xpcom-shutdown":
        
        this._cancelCallRingWakeLockTimer();
        
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
        this.setScreenState(data);
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
        this.dataCallSettings.oldEnabled = this.dataCallSettings.enabled;
        this.dataCallSettings.enabled = aResult;
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

  

  worker: null,

  

  setRadioEnabled: function setRadioEnabled(value) {
    if (DEBUG) this.debug("Setting radio power to " + value);
    this._changingRadioPower = true;
    this.worker.postMessage({rilMessageType: "setRadioPower", on: value});
  },

  rilContext: null,

  

  enumerateCalls: function enumerateCalls(message) {
    if (DEBUG) this.debug("Requesting enumeration of calls for callback");
    message.rilMessageType = "enumerateCalls";
    this.worker.postMessage(message);
  },

  _validateNumber: function _validateNumber(number) {
    
    if (PhoneNumberUtils.isPlainPhoneNumber(number)) {
      return true;
    }

    this.handleCallError({
      callIndex: -1,
      errorMsg: RIL.RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[RIL.CALL_FAIL_UNOBTAINABLE_NUMBER]
    });
    if (DEBUG) {
      this.debug("Number '" + number + "' doesn't seem to be a viable number." +
                 " Drop.");
    }

    return false;
  },

  dial: function dial(number) {
    if (DEBUG) this.debug("Dialing " + number);
    number = PhoneNumberUtils.normalize(number);
    if (this._validateNumber(number)) {
      this.worker.postMessage({rilMessageType: "dial",
                              number: number,
                              isDialEmergency: false});
    }
  },

  dialEmergency: function dialEmergency(number) {
    if (DEBUG) this.debug("Dialing emergency " + number);
    
    
    if (this._validateNumber(number)) {
      this.worker.postMessage({rilMessageType: "dial",
                              number: number,
                              isDialEmergency: true});
    }
  },

  hangUp: function hangUp(callIndex) {
    if (DEBUG) this.debug("Hanging up call no. " + callIndex);
    this.worker.postMessage({rilMessageType: "hangUp",
                             callIndex: callIndex});
  },

  startTone: function startTone(dtmfChar) {
    if (DEBUG) this.debug("Sending Tone for " + dtmfChar);
    this.worker.postMessage({rilMessageType: "startTone",
                             dtmfChar: dtmfChar});
  },

  stopTone: function stopTone() {
    if (DEBUG) this.debug("Stopping Tone");
    this.worker.postMessage({rilMessageType: "stopTone"});
  },

  answerCall: function answerCall(callIndex) {
    this.worker.postMessage({rilMessageType: "answerCall",
                             callIndex: callIndex});
  },

  rejectCall: function rejectCall(callIndex) {
    this.worker.postMessage({rilMessageType: "rejectCall",
                             callIndex: callIndex});
  },

  holdCall: function holdCall(callIndex) {
    this.worker.postMessage({rilMessageType: "holdCall",
                             callIndex: callIndex});
  },

  resumeCall: function resumeCall(callIndex) {
    this.worker.postMessage({rilMessageType: "resumeCall",
                             callIndex: callIndex});
  },

  getAvailableNetworks: function getAvailableNetworks(requestId) {
    this.worker.postMessage({rilMessageType: "getAvailableNetworks",
                             requestId: requestId});
  },

  setScreenState: function setScreenState(state) {
    if (DEBUG) this.debug("setScreenState: " + JSON.stringify(state));
    this.worker.postMessage({
      rilMessageType: "setScreenState",
      on: (state === "on")
    });
  },

  sendMMI: function sendMMI(message) {
    if (DEBUG) this.debug("SendMMI " + JSON.stringify(message));
    message.rilMessageType = "sendMMI";
    this.worker.postMessage(message);
  },

  cancelMMI: function cancelMMI(message) {
    
    
    
    
    if (DEBUG) this.debug("Cancel pending USSD");
    message.rilMessageType = "cancelUSSD";
    this.worker.postMessage(message);
  },

  selectNetworkAuto: function selectNetworkAuto(requestId) {
    this.worker.postMessage({rilMessageType: "selectNetworkAuto",
                             requestId: requestId});
  },

  selectNetwork: function selectNetwork(message) {
    message.rilMessageType = "selectNetwork";
    this.worker.postMessage(message);
  },

  sendStkResponse: function sendStkResponse(message) {
    message.rilMessageType = "sendStkTerminalResponse";
    this.worker.postMessage(message);
  },

  sendStkMenuSelection: function sendStkMenuSelection(message) {
    message.rilMessageType = "sendStkMenuSelection";
    this.worker.postMessage(message);
  },

  sendStkTimerExpiration: function sendStkTimerExpiration(message) {
    message.rilMessageType = "sendStkTimerExpiration";
    this.worker.postMessage(message);
  },

  sendStkEventDownload: function sendStkEventDownload(message) {
    message.rilMessageType = "sendStkEventDownload";
    this.worker.postMessage(message);
  },

  iccOpenChannel: function iccOpenChannel(message) {
    if (DEBUG) this.debug("ICC Open Channel");
    message.rilMessageType = "iccOpenChannel";
    this.worker.postMessage(message);
  },

  iccCloseChannel: function iccCloseChannel(message) {
    if (DEBUG) this.debug("ICC Close Channel");
    message.rilMessageType = "iccCloseChannel";
    this.worker.postMessage(message);
  },

  iccExchangeAPDU: function iccExchangeAPDU(message) {
    if (DEBUG) this.debug("ICC Exchange APDU");
    message.rilMessageType = "iccExchangeAPDU";
    this.worker.postMessage(message);
  },

  setCallForwardingOption: function setCallForwardingOption(message) {
    if (DEBUG) this.debug("setCallForwardingOption: " + JSON.stringify(message));
    message.rilMessageType = "setCallForward";
    message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE;
    this.worker.postMessage(message);
  },

  getCallForwardingOption: function getCallForwardingOption(message) {
    if (DEBUG) this.debug("getCallForwardingOption: " + JSON.stringify(message));
    message.rilMessageType = "queryCallForwardStatus";
    message.serviceClass = RIL.ICC_SERVICE_CLASS_NONE;
    message.number = null;
    this.worker.postMessage(message);
  },

  setCallBarringOption: function setCallBarringingOption(message) {
    if (DEBUG) this.debug("setCallBarringOption: " + JSON.stringify(message));
    message.rilMessageType = "setCallBarring";
    this.worker.postMessage(message);
  },

  getCallBarringOption: function getCallBarringOption(message) {
    if (DEBUG) this.debug("getCallBarringOption: " + JSON.stringify(message));
    message.rilMessageType = "queryCallBarringStatus";
    this.worker.postMessage(message);
  },

  setCallWaitingOption: function setCallWaitingOption(message) {
    if (DEBUG) this.debug("setCallWaitingOption: " + JSON.stringify(message));
    message.rilMessageType = "setCallWaiting";
    this.worker.postMessage(message);
  },

  getCallWaitingOption: function getCallWaitingOption(message) {
    if (DEBUG) this.debug("getCallWaitingOption: " + JSON.stringify(message));
    message.rilMessageType = "queryCallWaiting";
    this.worker.postMessage(message);
  },

  setCallingLineIdRestriction: function setCallingLineIdRestriction(message) {
    if (DEBUG) {
      this.debug("setCallingLineIdRestriction: " + JSON.stringify(message));
    }
    message.rilMessageType = "setCLIR";
    this.worker.postMessage(message);
  },

  getCallingLineIdRestriction: function getCallingLineIdRestriction(message) {
    if (DEBUG) {
      this.debug("getCallingLineIdRestriction: " + JSON.stringify(message));
    }
    message.rilMessageType = "getCLIR";
    this.worker.postMessage(message);
  },

  getRoamingPreference: function getRoamingPreference(message) {
    if (DEBUG) this.debug("getRoamingPreference: " + JSON.stringify(message));
    message.rilMessageType = "queryRoamingPreference";
    this.worker.postMessage(message);
  },

  setRoamingPreference: function setRoamingPreference(message) {
    if (DEBUG) this.debug("setRoamingPreference: " + JSON.stringify(message));
    message.rilMessageType = "setRoamingPreference";
    this.worker.postMessage(message);
  },

  get microphoneMuted() {
    return gAudioManager.microphoneMuted;
  },
  set microphoneMuted(value) {
    if (value == this.microphoneMuted) {
      return;
    }
    gAudioManager.microphoneMuted = value;

    if (!this._activeCall) {
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
    }
  },

  get speakerEnabled() {
    return (gAudioManager.getForceForUse(nsIAudioManager.USE_COMMUNICATION) ==
            nsIAudioManager.FORCE_SPEAKER);
  },
  set speakerEnabled(value) {
    if (value == this.speakerEnabled) {
      return;
    }
    let force = value ? nsIAudioManager.FORCE_SPEAKER :
                        nsIAudioManager.FORCE_NONE;
    gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION, force);

    if (!this._activeCall) {
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
    }
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

  getSegmentInfoForText: function getSegmentInfoForText(text) {
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

    let result = gMobileMessageService.createSmsSegmentInfo(options.segmentMaxSeq,
                                                            options.segmentChars,
                                                            options.segmentChars - charsInLastSegment);
    return result;
  },

  sendSMS: function sendSMS(number, message, request) {
    let strict7BitEncoding;
    try {
      strict7BitEncoding = Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      strict7BitEncoding = false;
    }

    let options = this._fragmentText(message, null, strict7BitEncoding);
    options.rilMessageType = "sendSMS";
    options.number = PhoneNumberUtils.normalize(number);
    let requestStatusReport;
    try {
      requestStatusReport =
        Services.prefs.getBoolPref("dom.sms.requestStatusReport");
    } catch (e) {
      requestStatusReport = true;
    }
    options.requestStatusReport = requestStatusReport;
    if (options.segmentMaxSeq > 1) {
      options.segmentRef16Bit = this.segmentRef16Bit;
      options.segmentRef = this.nextSegmentRef;
    }

    let sendingMessage = {
      type: "sms",
      sender: this.getMsisdn(),
      receiver: number,
      body: message,
      deliveryStatusRequested: options.requestStatusReport,
      timestamp: Date.now()
    };

    let id = gMobileMessageDatabaseService.saveSendingMessage(
      sendingMessage,
      function notifyResult(rv, domMessage) {

        
        Services.obs.notifyObservers(domMessage, kSmsSendingObserverTopic, null);

        
        
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
          gMobileMessageDatabaseService
            .setMessageDelivery(domMessage.id,
                                null,
                                DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                function notifyResult(rv, domMessage) {
            
            request.notifySendMessageFailed(errorCode);
            Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
          });
          return;
        }

        
        options.envelopeId = this.createSmsEnvelope({
          request: request,
          sms: domMessage,
          requestStatusReport: options.requestStatusReport
        });

        
        this.worker.postMessage(options);

      }.bind(this));
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
    this.worker.postMessage({rilMessageType: "setupDataCall",
                             radioTech: radioTech,
                             apn: apn,
                             user: user,
                             passwd: passwd,
                             chappap: chappap,
                             pdptype: pdptype});
  },

  deactivateDataCall: function deactivateDataCall(cid, reason) {
    this.worker.postMessage({rilMessageType: "deactivateDataCall",
                             cid: cid,
                             reason: reason});
  },

  getDataCallList: function getDataCallList() {
    this.worker.postMessage({rilMessageType: "getDataCallList"});
  },

  getCardLockState: function getCardLockState(message) {
    message.rilMessageType = "iccGetCardLockState";
    this.worker.postMessage(message);
  },

  unlockCardLock: function unlockCardLock(message) {
    message.rilMessageType = "iccUnlockCardLock";
    this.worker.postMessage(message);
  },

  setCardLock: function setCardLock(message) {
    message.rilMessageType = "iccSetCardLock";
    this.worker.postMessage(message);
  },

  getCardLockRetryCount: function getCardLockRetryCount(message) {
    message.rilMessageType = "iccGetCardLockRetryCount";
    this.worker.postMessage(message);
  },

  readIccContacts: function readIccContacts(message) {
    message.rilMessageType = "readICCContacts";
    this.worker.postMessage(message);
  },

  updateIccContact: function updateIccContact(message) {
    message.rilMessageType = "updateICCContact";
    this.worker.postMessage(message);
  },
};

function RILNetworkInterface(radioInterface, apnSetting) {
  this.radioInterface = radioInterface;
  this.apnSetting = apnSetting;
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

  dhcp: false,

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
  apnSetting: {},

  
  apnRetryCounter: 0,

  connectedTypes: [],

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

