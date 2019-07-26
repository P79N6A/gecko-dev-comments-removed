














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);


const DEBUG = RIL.DEBUG_RIL;

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{3bdd52a9-3965-4130-b569-0ac5afed045e}");

const nsIAudioManager = Ci.nsIAudioManager;
const nsIRadioInterfaceLayer = Ci.nsIRadioInterfaceLayer;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsDeliveredObserverTopic         = "sms-delivered";
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const DOM_SMS_DELIVERY_RECEIVED          = "received";
const DOM_SMS_DELIVERY_SENT              = "sent";

const RIL_IPC_MSG_NAMES = [
  "RIL:GetRilContext",
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
  "RIL:GetAvailableNetworks",
  "RIL:SelectNetwork",
  "RIL:SelectNetworkAuto",
  "RIL:GetCardLock",
  "RIL:UnlockCardLock",
  "RIL:SetCardLock",
  "RIL:SendUSSD",
  "RIL:CancelUSSD",
  "RIL:SendStkResponse",
  "RIL:SendStkMenuSelection"
];

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsRequestManager",
                                   "@mozilla.org/sms/smsrequestmanager;1",
                                   "nsISmsRequestManager");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsDatabaseService",
                                   "@mozilla.org/sms/rilsmsdatabaseservice;1",
                                   "nsISmsDatabaseService");

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

XPCOMUtils.defineLazyGetter(this, "WAP", function () {
  let WAP = {};
  Cu.import("resource://gre/modules/WapPushManager.js", WAP);
  return WAP;
});

function convertRILCallState(state) {
  switch (state) {
    case RIL.CALL_STATE_ACTIVE:
      return nsIRadioInterfaceLayer.CALL_STATE_CONNECTED;
    case RIL.CALL_STATE_HOLDING:
      return nsIRadioInterfaceLayer.CALL_STATE_HELD;
    case RIL.CALL_STATE_DIALING:
      return nsIRadioInterfaceLayer.CALL_STATE_DIALING;
    case RIL.CALL_STATE_ALERTING:
      return nsIRadioInterfaceLayer.CALL_STATE_ALERTING;
    case RIL.CALL_STATE_INCOMING:
    case RIL.CALL_STATE_WAITING:
      return nsIRadioInterfaceLayer.CALL_STATE_INCOMING; 
    case RIL.CALL_STATE_BUSY:
      return nsIRadioInterfaceLayer.CALL_STATE_BUSY;
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
    
    debug("Using fake audio manager.");
    return FakeAudioManager;
  }
});


function RadioInterfaceLayer() {
  this.dataNetworkInterface = new RILNetworkInterface(this, Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE);
  this.mmsNetworkInterface = new RILNetworkInterface(this, Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS);
  this.suplNetworkInterface = new RILNetworkInterface(this, Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL);

  debug("Starting RIL Worker");
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  this.rilContext = {
    radioState:     RIL.GECKO_RADIOSTATE_UNAVAILABLE,
    cardState:      RIL.GECKO_CARDSTATE_UNAVAILABLE,
    icc:            null,

    
    
    
    
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

  this.callWaitingStatus = null;

  
  
  let lock = gSettingsService.createLock();
  lock.get("ril.radio.disabled", this);

  
  lock.get("ril.data.apn", this);
  lock.get("ril.data.user", this);
  lock.get("ril.data.passwd", this);
  lock.get("ril.data.httpProxyHost", this);
  lock.get("ril.data.httpProxyPort", this);
  lock.get("ril.data.roaming_enabled", this);
  lock.get("ril.data.enabled", this);
  this._dataCallSettingsToRead = ["ril.data.enabled",
                                  "ril.data.roaming_enabled",
                                  "ril.data.apn",
                                  "ril.data.user",
                                  "ril.data.passwd",
                                  "ril.data.httpProxyHost",
                                  "ril.data.httpProxyPort"];

  
  lock.get("ril.mms.apn", this);
  lock.get("ril.mms.user", this);
  lock.get("ril.mms.passwd", this);
  lock.get("ril.mms.httpProxyHost", this);
  lock.get("ril.mms.httpProxyPort", this);
  lock.get("ril.mms.mmsc", this);
  lock.get("ril.mms.mmsproxy", this);
  lock.get("ril.mms.mmsport", this);
  lock.get("ril.supl.apn", this);
  lock.get("ril.supl.user", this);
  lock.get("ril.supl.passwd", this);
  lock.get("ril.supl.httpProxyHost", this);
  lock.get("ril.supl.httpProxyPort", this);

  
  lock.get("ril.callwaiting.enabled", this);

  this._messageManagerByRequest = {};

  for each (let msgname in RIL_IPC_MSG_NAMES) {
    ppmm.addMessageListener(msgname, this);
  }
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);

  this._sentSmsEnvelopes = {};

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] = this.handleSmsWdpPortPush.bind(this);
}
RadioInterfaceLayer.prototype = {

  classID:   RADIOINTERFACELAYER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACELAYER_CID,
                                    classDescription: "RadioInterfaceLayer",
                                    interfaces: [Ci.nsIWorkerHolder,
                                                 Ci.nsIRadioInterfaceLayer]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWorkerHolder,
                                         Ci.nsIRadioInterfaceLayer,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  


  receiveMessage: function receiveMessage(msg) {
    debug("Received '" + msg.name + "' message from content process");
    switch (msg.name) {
      case "RIL:GetRilContext":
        
        return this.rilContext;
      case "RIL:EnumerateCalls":
        this.saveRequestTarget(msg);
        this.enumerateCalls(msg.json);
        break;
      case "RIL:GetMicrophoneMuted":
        
        return this.microphoneMuted;
      case "RIL:SetMicrophoneMuted":
        this.microphoneMuted = msg.json;
        break;
      case "RIL:GetSpeakerEnabled":
        
        return this.speakerEnabled;
      case "RIL:SetSpeakerEnabled":
        this.speakerEnabled = msg.json;
        break;
      case "RIL:StartTone":
        this.startTone(msg.json);
        break;
      case "RIL:StopTone":
        this.stopTone();
        break;
      case "RIL:Dial":
        this.dial(msg.json);
        break;
      case "RIL:DialEmergency":
        this.dialEmergency(msg.json);
        break;
      case "RIL:HangUp":
        this.hangUp(msg.json);
        break;
      case "RIL:AnswerCall":
        this.answerCall(msg.json);
        break;
      case "RIL:RejectCall":
        this.rejectCall(msg.json);
        break;
      case "RIL:HoldCall":
        this.holdCall(msg.json);
        break;
      case "RIL:ResumeCall":
        this.resumeCall(msg.json);
        break;
      case "RIL:GetAvailableNetworks":
        this.saveRequestTarget(msg);
        this.getAvailableNetworks(msg.json.requestId);
        break;
      case "RIL:SelectNetwork":
        this.saveRequestTarget(msg);
        this.selectNetwork(msg.json);
        break;
      case "RIL:SelectNetworkAuto":
        this.saveRequestTarget(msg);
        this.selectNetworkAuto(msg.json.requestId);
      case "RIL:GetCardLock":
        this.saveRequestTarget(msg);
        this.getCardLock(msg.json);
        break;
      case "RIL:UnlockCardLock":
        this.saveRequestTarget(msg);
        this.unlockCardLock(msg.json);
        break;
      case "RIL:SetCardLock":
        this.saveRequestTarget(msg);
        this.setCardLock(msg.json);
        break;
      case "RIL:SendUSSD":
        this.saveRequestTarget(msg);
        this.sendUSSD(msg.json);
        break;
      case "RIL:CancelUSSD":
        this.saveRequestTarget(msg);
        this.cancelUSSD(msg.json);
        break;
      case "RIL:SendStkResponse":
        this.sendStkResponse(msg.json);
        break;
      case "RIL:SendStkMenuSelection":
        this.sendStkMenuSelection(msg.json);
        break;
    }
  },

  onerror: function onerror(event) {
    debug("Got an error: " + event.filename + ":" +
          event.lineno + ": " + event.message + "\n");
    event.preventDefault();
  },

  






  onmessage: function onmessage(event) {
    let message = event.data;
    debug("Received message from worker: " + JSON.stringify(message));
    switch (message.rilMessageType) {
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
        ppmm.broadcastAsyncMessage("RIL:CardStateChanged", message);
        break;
      case "setCallWaiting":
        this.handleCallWaitingStatusChange(message);
        break;
      case "sms-received":
        this.handleSmsReceived(message);
        return;
      case "sms-sent":
        this.handleSmsSent(message);
        return;
      case "sms-delivered":
        this.handleSmsDelivered(message);
        return;
      case "sms-send-failed":
        this.handleSmsSendFailed(message);
        return;
      case "datacallstatechange":
        this.handleDataCallState(message);
        break;
      case "datacalllist":
        this.handleDataCallList(message);
        break;
      case "nitzTime":
        
        
        
        
        
        
        
        debug("nitzTime networkTime=" + message.networkTimeInSeconds +
              " timezone=" + message.networkTimeZoneInMinutes +
              " dst=" + message.dstFlag +
              " timestamp=" + message.localTimeStampInMS);
        break;
      case "iccinfochange":
        this.handleICCInfoChange(message);
        break;
      case "iccGetCardLock":
      case "iccSetCardLock":
      case "iccUnlockCardLock":
        this.handleICCCardLockResult(message);
        break;
      case "icccontacts":
        if (!this._contactsCallbacks) {
          return;
        }
        let callback = this._contactsCallbacks[message.requestId];
        if (callback) {
          delete this._contactsCallbacks[message.requestId];
          callback.receiveContactsList(message.errorMsg,
                                       message.contactType,
                                       message.contacts);
        }
        break;
      case "iccmbdn":
        ppmm.broadcastAsyncMessage("RIL:VoicemailNumberChanged", message);
        break;
      case "ussdreceived":
        debug("ussdreceived " + JSON.stringify(message));
        this.handleUSSDReceived(message);
        break;
      case "sendussd":
        this.handleSendUSSD(message);
        break;
      case "cancelussd":
        this.handleCancelUSSD(message);
        break;
      case "stkcommand":
        this.handleStkProactiveCommand(message);
        break;
      case "stksessionend":
        ppmm.broadcastAsyncMessage("RIL:StkSessionEnd", null);
        break;
      default:
        throw new Error("Don't know about this message type: " +
                        message.rilMessageType);
    }
  },

  _messageManagerByRequest: null,
  saveRequestTarget: function saveRequestTarget(msg) {
    let requestId = msg.json.requestId;
    if (!requestId) {
      
      return;
    }

    this._messageManagerByRequest[requestId] = msg.target;
  },

  _sendRequestResults: function _sendRequestResults(requestType, options) {
    let target = this._messageManagerByRequest[options.requestId];
    delete this._messageManagerByRequest[options.requestId];

    if (!target) {
      return;
    }

    target.sendAsyncMessage(requestType, options);
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

    let dataInfoChanged = false;
    if (dataMessage) {
      dataMessage.batch = true;
      this.updateDataConnection(dataMessage);
    }

    let voice = this.rilContext.voice;
    let data = this.rilContext.data;
    if (operatorMessage) {
      if (this.networkChanged(operatorMessage, voice.network)) {
        voice.network = operatorMessage;
      }
      if (this.networkChanged(operatorMessage, data.network)) {
        data.network = operatorMessage;
      }
    }

    this.checkRoamingBetweenOperators(voice);
    this.checkRoamingBetweenOperators(data);

    if (voiceMessage || operatorMessage) {
      ppmm.broadcastAsyncMessage("RIL:VoiceInfoChanged", voice);
    }
    if (dataMessage || operatorMessage) {
      ppmm.broadcastAsyncMessage("RIL:DataInfoChanged", data);
    }

    if (selectionMessage) {
      this.updateNetworkSelectionMode(selectionMessage);
    }
  },

  






  checkRoamingBetweenOperators: function checkRoamingBetweenOperators(registration) {
    let icc = this.rilContext.icc;
    if (!icc || !registration.connected) {
      return;
    }

    let spn = icc.spn;
    let operator = registration.network;
    let longName = operator.longName;
    let shortName = operator.shortName;

    let equalsLongName = longName && (spn == longName);
    let equalsShortName = shortName && (spn == shortName);
    let equalsMcc = icc.mcc == operator.mcc;

    registration.roaming = registration.roaming &&
                           !(equalsMcc && (equalsLongName || equalsShortName));
  },

  






  updateVoiceConnection: function updateVoiceConnection(newInfo) {
    let voiceInfo = this.rilContext.voice;
    voiceInfo.state = newInfo.state;
    voiceInfo.connected = newInfo.connected;
    voiceInfo.roaming = newInfo.roaming;
    voiceInfo.emergencyCallsOnly = newInfo.emergencyCallsOnly;
    
    
    
    voiceInfo.type = "gsm";

    
    if (voiceInfo.connected && this.callWaitingStatus == null) {
      
      this.setCallWaitingEnabled(this._callWaitingEnabled);
    }

    
    
    if (newInfo.regState == RIL.NETWORK_CREG_STATE_UNKNOWN) {
      voiceInfo.network = null;
      voiceInfo.signalStrength = null;
      voiceInfo.relSignalStrength = null;
    }

    let newCell = newInfo.cell;
    if ((newCell.gsmLocationAreaCode < 0) || (newCell.gsmCellId < 0)) {
      voiceInfo.cell = null;
    } else {
      voiceInfo.cell = newCell;
    }

    if (!newInfo.batch) {
      ppmm.broadcastAsyncMessage("RIL:VoiceInfoChanged", voiceInfo);
    }
  },

  updateDataConnection: function updateDataConnection(newInfo) {
    let dataInfo = this.rilContext.data;
    dataInfo.state = newInfo.state;
    dataInfo.roaming = newInfo.roaming;
    dataInfo.emergencyCallsOnly = newInfo.emergencyCallsOnly;
    dataInfo.type = newInfo.type;
    
    
    dataInfo.connected = this.dataNetworkInterface.connected;

    
    
    if (newInfo.regState == RIL.NETWORK_CREG_STATE_UNKNOWN) {
      dataInfo.network = null;
      dataInfo.signalStrength = null;
      dataInfo.relSignalStrength = null;
    }

    let newCell = newInfo.cell;
    if ((newCell.gsmLocationAreaCode < 0) || (newCell.gsmCellId < 0)) {
      dataInfo.cell = null;
    } else {
      dataInfo.cell = newCell;
    }

    if (!newInfo.batch) {
      ppmm.broadcastAsyncMessage("RIL:DataInfoChanged", dataInfo);
    }
    this.updateRILNetworkInterface();
  },

  


  handleDataCallError: function handleDataCallError(message) {
    
    if (message.apn == this.dataCallSettings["apn"]) {
      ppmm.broadcastAsyncMessage("RIL:DataError", message);
    }

    this._deliverDataCallCallback("dataCallError", [message]);
  },

  handleSignalStrengthChange: function handleSignalStrengthChange(message) {
    let voiceInfo = this.rilContext.voice;
    
    if (voiceInfo.signalStrength != message.gsmDBM ||
        voiceInfo.relSignalStrength != message.gsmRelative) {
      voiceInfo.signalStrength = message.gsmDBM;
      voiceInfo.relSignalStrength = message.gsmRelative;
      ppmm.broadcastAsyncMessage("RIL:VoiceInfoChanged", voiceInfo);
    }

    let dataInfo = this.rilContext.data;
    if (dataInfo.signalStrength != message.gsmDBM ||
        dataInfo.relSignalStrength != message.gsmRelative) {
      dataInfo.signalStrength = message.gsmDBM;
      dataInfo.relSignalStrength = message.gsmRelative;
      ppmm.broadcastAsyncMessage("RIL:DataInfoChanged", dataInfo);
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
      voice.network = message;
      ppmm.broadcastAsyncMessage("RIL:VoiceInfoChanged", voice);
    }

    if (this.networkChanged(message, data.network)) {
      data.network = message;
      ppmm.broadcastAsyncMessage("RIL:DataInfoChanged", data);
    }
  },

  handleRadioStateChange: function handleRadioStateChange(message) {
    let newState = message.radioState;
    if (this.rilContext.radioState == newState) {
      return;
    }
    this.rilContext.radioState = newState;
    

    this._ensureRadioState();
  },

  _ensureRadioState: function _ensureRadioState() {
    debug("Reported radio state is " + this.rilContext.radioState +
          ", desired radio enabled state is " + this._radioEnabled);
    if (this._radioEnabled == null) {
      
      
      return;
    }
    if (!this._sysMsgListenerReady) {
      
      
      return;
    }
    if (this.rilContext.radioState == RIL.GECKO_RADIOSTATE_UNKNOWN) {
      
      
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

  handleCallWaitingStatusChange: function handleCallWaitingStatusChange(message) {
    let newStatus = message.enabled;

    
    
    if (!message.success) {
      newStatus = !newStatus;
      let lock = gSettingsService.createLock();
      lock.set("ril.callwaiting.enabled", newStatus, null);
      return;
    }

    this.callWaitingStatus = newStatus;
  },

  setCallWaitingEnabled: function setCallWaitingEnabled(value) {
    debug("Current call waiting status is " + this.callWaitingStatus +
          ", desired call waiting status is " + value);
    if (!this.rilContext.voice.connected) {
      
      return;
    }

    if (value == null) {
      
      
      return;
    }

    if (this.callWaitingStatus != value) {
      debug("Setting call waiting status to " + value);
      this.worker.postMessage({rilMessageType: "setCallWaiting", enabled: value});
    }
  },

  updateRILNetworkInterface: function updateRILNetworkInterface() {
    if (this._dataCallSettingsToRead.length) {
      debug("We haven't read completely the APN data from the " +
            "settings DB yet. Wait for that.");
      return;
    } 

    
    
    if (this.rilContext.radioState != RIL.GECKO_RADIOSTATE_READY) {
      debug("RIL is not ready for data connection: radio's not ready");
      return; 
    }

    
    
    
    
    
    if (this._oldRilDataEnabledState == this.dataCallSettings["enabled"]) {
      debug("No changes for ril.data.enabled flag. Nothing to do.");
      return;
    }

    if (!this.dataCallSettings["enabled"] && this.dataNetworkInterface.connected) {
      debug("Data call settings: disconnect data call.");
      this.dataNetworkInterface.disconnect();
      return;
    }
    if (!this.dataCallSettings["enabled"] || this.dataNetworkInterface.connected) {
      debug("Data call settings: nothing to do.");
      return;
    }
    let dataInfo = this.rilContext.data;
    let isRegistered =
      dataInfo.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;
    let haveDataConnection =
      dataInfo.type != RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN;
    if (!isRegistered || !haveDataConnection) {
      debug("RIL is not ready for data connection: Phone's not registered " +
            "or doesn't have data connection.");
      return;
    }
    if (dataInfo.roaming && !this.dataCallSettings["roaming_enabled"]) {
      debug("We're roaming, but data roaming is disabled.");
      return;
    }

    debug("Data call settings: connect data call.");
    this.dataNetworkInterface.connect(this.dataCallSettings);
  },

  


  _activeCall: null,
  updateCallAudioState: function updateCallAudioState() {
    if (!this._activeCall) {
      
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
      debug("No active call, put audio system into PHONE_STATE_NORMAL: "
            + gAudioManager.phoneState);
      return;
    }
    switch (this._activeCall.state) {
      case nsIRadioInterfaceLayer.CALL_STATE_INCOMING:
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_RINGTONE;
        debug("Incoming call, put audio system into PHONE_STATE_RINGTONE: "
              + gAudioManager.phoneState);
        break;
      case nsIRadioInterfaceLayer.CALL_STATE_DIALING: 
      case nsIRadioInterfaceLayer.CALL_STATE_CONNECTED:
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
        let force = this.speakerEnabled ? nsIAudioManager.FORCE_SPEAKER
                                        : nsIAudioManager.FORCE_NONE;
        gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION, force);
        debug("Active call, put audio system into PHONE_STATE_IN_CALL: "
              + gAudioManager.phoneState);
        break;
    }
  },

  



  handleCallStateChange: function handleCallStateChange(call) {
    debug("handleCallStateChange: " + JSON.stringify(call));
    call.state = convertRILCallState(call.state);

    if (call.state == nsIRadioInterfaceLayer.CALL_STATE_INCOMING) {
      gSystemMessenger.broadcastMessage("telephony-incoming", {number: call.number});
    }

    if (call.isActive) {
      this._activeCall = call;
    } else if (this._activeCall &&
               this._activeCall.callIndex == call.callIndex) {
      
      this._activeCall = null;
    }
    this.updateCallAudioState();
    ppmm.broadcastAsyncMessage("RIL:CallStateChanged", call);
  },

  


  handleCallDisconnected: function handleCallDisconnected(call) {
    debug("handleCallDisconnected: " + JSON.stringify(call));
    if (call.isActive) {
      this._activeCall = null;
    }
    this.updateCallAudioState();
    call.state = nsIRadioInterfaceLayer.CALL_STATE_DISCONNECTED;
    ppmm.broadcastAsyncMessage("RIL:CallStateChanged", call);
  },

  


  handleEnumerateCalls: function handleEnumerateCalls(options) {
    debug("handleEnumerateCalls: " + JSON.stringify(options));
    for (let i in options.calls) {
      options.calls[i].state = convertRILCallState(options.calls[i].state);
    }
    this._sendRequestResults("RIL:EnumerateCalls", options);
  },

  


  handleGetAvailableNetworks: function handleGetAvailableNetworks(message) {
    debug("handleGetAvailableNetworks: " + JSON.stringify(message));

    this._sendRequestResults("RIL:GetAvailableNetworks", message);
  },

  


  updateNetworkSelectionMode: function updateNetworkSelectionMode(message) {
    debug("updateNetworkSelectionMode: " + JSON.stringify(message));
    ppmm.broadcastAsyncMessage("RIL:NetworkSelectionModeChanged", message);
  },

  


  handleSelectNetwork: function handleSelectNetwork(message) {
    debug("handleSelectNetwork: " + JSON.stringify(message));
    this._sendRequestResults("RIL:SelectNetwork", message);
  },

  


  handleSelectNetworkAuto: function handleSelectNetworkAuto(message) {
    debug("handleSelectNetworkAuto: " + JSON.stringify(message));
    this._sendRequestResults("RIL:SelectNetworkAuto", message);
  },

  


  handleCallError: function handleCallError(message) {
    ppmm.broadcastAsyncMessage("RIL:CallError", message);   
  },

  






  handleSmsWdpPortPush: function handleSmsWdpPortPush(message) {
    if (message.encoding != RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      debug("Got port addressed SMS but not encoded in 8-bit alphabet. Drop!");
      return;
    }

    let options = {
      bearer: WAP.WDP_BEARER_GSM_SMS_GSM_MSISDN,
      sourceAddress: message.sender,
      sourcePort: message.header.originatorPort,
      destinationAddress: this.rilContext.icc.msisdn,
      destinationPort: message.header.destinationPort,
    };
    WAP.WapPushManager.receiveWdpPDU(message.fullData, message.fullData.length,
                                     0, options);
  },

  portAddressedSmsApps: null,
  handleSmsReceived: function handleSmsReceived(message) {
    debug("handleSmsReceived: " + JSON.stringify(message));

    
    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      message.fullData = new Uint8Array(message.fullData);
    }

    
    
    
    if (message.header && message.header.destinationPort != null) {
      let handler = this.portAddressedSmsApps[message.header.destinationPort];
      if (handler) {
        handler(message);
      }
      return;
    }

    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      return;
    }

    
    
    
    

    let mwi = message.mwi;
    if (mwi) {
      mwi.returnNumber = message.sender || null;
      mwi.returnMessage = message.fullBody || null;
      ppmm.broadcastAsyncMessage("RIL:VoicemailNotification", mwi);
      return;
    }

    let id = -1;
    if (message.messageClass != RIL.PDU_DCS_MSG_CLASS_0) {
      id = gSmsDatabaseService.saveReceivedMessage(message.sender || null,
                                                   message.fullBody || null,
                                                   message.timestamp);
    }
    let sms = gSmsService.createSmsMessage(id,
                                           DOM_SMS_DELIVERY_RECEIVED,
                                           message.sender || null,
                                           message.receiver || null,
                                           message.fullBody || null,
                                           message.timestamp,
                                           false);

    gSystemMessenger.broadcastMessage("sms-received",
                                      {id: id,
                                       delivery: DOM_SMS_DELIVERY_RECEIVED,
                                       sender: message.sender || null,
                                       receiver: message.receiver || null,
                                       body: message.fullBody || null,
                                       timestamp: message.timestamp,
                                       read: false});
    Services.obs.notifyObservers(sms, kSmsReceivedObserverTopic, null);
  },

  


  _sentSmsEnvelopes: null,
  createSmsEnvelope: function createSmsEnvelope(options) {
    let i;
    for (i = 1; this._sentSmsEnvelopes[i]; i++) {
      
    }

    debug("createSmsEnvelope: assigned " + i);
    options.envelopeId = i;
    this._sentSmsEnvelopes[i] = options;
  },

  handleSmsSent: function handleSmsSent(message) {
    debug("handleSmsSent: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }

    let timestamp = Date.now();
    let id = gSmsDatabaseService.saveSentMessage(options.number,
                                                 options.fullBody,
                                                 timestamp);
    let sms = gSmsService.createSmsMessage(id,
                                           DOM_SMS_DELIVERY_SENT,
                                           null,
                                           options.number,
                                           options.fullBody,
                                           timestamp,
                                           true);

    if (!options.requestStatusReport) {
      
      delete this._sentSmsEnvelopes[message.envelopeId];
    } else {
      options.sms = sms;
    }

    gSmsRequestManager.notifySmsSent(options.requestId, sms);

    Services.obs.notifyObservers(options.sms, kSmsSentObserverTopic, null);
  },

  handleSmsDelivered: function handleSmsDelivered(message) {
    debug("handleSmsDelivered: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }
    delete this._sentSmsEnvelopes[message.envelopeId];

    Services.obs.notifyObservers(options.sms, kSmsDeliveredObserverTopic, null);
  },

  handleSmsSendFailed: function handleSmsSendFailed(message) {
    debug("handleSmsSendFailed: " + JSON.stringify(message));

    let options = this._sentSmsEnvelopes[message.envelopeId];
    if (!options) {
      return;
    }
    delete this._sentSmsEnvelopes[message.envelopeId];

    let error = gSmsRequestManager.UNKNOWN_ERROR;
    switch (message.error) {
      case RIL.ERROR_RADIO_NOT_AVAILABLE:
        error = gSmsRequestManager.NO_SIGNAL_ERROR;
        break;
    }

    gSmsRequestManager.notifySmsSendFailed(options.requestId, error);
  },

  


  handleDataCallState: function handleDataCallState(datacall) {
    let data = this.rilContext.data;

    if (datacall.ifname &&
        datacall.apn == this.dataCallSettings["apn"]) {
      data.connected = (datacall.state == RIL.GECKO_NETWORK_STATE_CONNECTED);
      ppmm.broadcastAsyncMessage("RIL:DataInfoChanged", data);
    }

    this._deliverDataCallCallback("dataCallStateChanged",
                                  [datacall]);
  },

  


  handleDataCallList: function handleDataCallList(message) {
    this._deliverDataCallCallback("receiveDataCallList",
                                  [message.datacalls, message.datacalls.length]);
  },

  handleICCInfoChange: function handleICCInfoChange(message) {
    let oldIcc = this.rilContext.icc;
    this.rilContext.icc = message;
   
    let iccInfoChanged = !oldIcc ||
                         oldIcc.iccid != message.iccid ||
                         oldIcc.mcc != message.mcc || 
                         oldIcc.mnc != message.mnc;
    if (!iccInfoChanged) {
      return;
    }
    
    
    ppmm.broadcastAsyncMessage("RIL:IccInfoChanged", message);
  },

  handleICCCardLockResult: function handleICCCardLockResult(message) {
    this._sendRequestResults("RIL:CardLockResult", message);
  },

  handleUSSDReceived: function handleUSSDReceived(ussd) {
    debug("handleUSSDReceived " + JSON.stringify(ussd));
    ppmm.broadcastAsyncMessage("RIL:UssdReceived", ussd);
  },

  handleSendUSSD: function handleSendUSSD(message) {
    debug("handleSendUSSD " + JSON.stringify(message));
    let messageType = message.success ? "RIL:SendUssd:Return:OK" :
                                        "RIL:SendUssd:Return:KO";
    this._sendRequestResults(messageType, message);
  },

  handleCancelUSSD: function handleCancelUSSD(message) {
    debug("handleCancelUSSD " + JSON.stringify(message));
    let messageType = message.success ? "RIL:CancelUssd:Return:OK" :
                                        "RIL:CancelUssd:Return:KO";
    this._sendRequestResults(messageType, message);
  },

  handleStkProactiveCommand: function handleStkProactiveCommand(message) {
    debug("handleStkProactiveCommand " + JSON.stringify(message));
    gSystemMessenger.broadcastMessage("icc-stkcommand", message);
    ppmm.broadcastAsyncMessage("RIL:StkCommand", message);
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
        this.handle(setting.key, setting.value);
        break;
      case "xpcom-shutdown":
        for each (let msgname in RIL_IPC_MSG_NAMES) {
          ppmm.removeMessageListener(msgname, this);
        }
        
        this.dataNetworkInterface.shutdown();
        this.mmsNetworkInterface.shutdown();
        this.suplNetworkInterface.shutdown();
        ppmm = null;
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        break;
    }
  },

  
  
  _sysMsgListenerReady: false,

  
  
  _radioEnabled: null,

  
  
  
  _callWaitingEnabled: null,

  
  dataCallSettings: {},
  dataCallSettingsMMS: {},
  dataCallSettingsSUPL: {},
  _dataCallSettingsToRead: [],
  _oldRilDataEnabledState: null,

  

  handle: function handle(aName, aResult) {
    switch(aName) {
      case "ril.radio.disabled":
        debug("'ril.radio.disabled' is now " + aResult);
        this._radioEnabled = !aResult;
        this._ensureRadioState();
        break;
      case "ril.data.enabled":
        this._oldRilDataEnabledState = this.dataCallSettings["enabled"];
        
      case "ril.data.roaming_enabled":
      case "ril.data.apn":
      case "ril.data.user":
      case "ril.data.passwd":
      case "ril.data.httpProxyHost":
      case "ril.data.httpProxyPort":
        let key = aName.slice(9);
        this.dataCallSettings[key] = aResult;
        debug("'" + aName + "'" + " is now " + this.dataCallSettings[key]);
        let index = this._dataCallSettingsToRead.indexOf(aName);
        if (index != -1) {
          this._dataCallSettingsToRead.splice(index, 1);
        }
        this.updateRILNetworkInterface();
        break;
      case "ril.mms.apn":
      case "ril.mms.user":
      case "ril.mms.passwd":
      case "ril.mms.httpProxyHost":
      case "ril.mms.httpProxyPort":
      case "ril.mms.mmsc":
      case "ril.mms.mmsproxy":
      case "ril.mms.mmsport":
        key = aName.slice(8);
        this.dataCallSettingsMMS[key] = aResult;
        break;
      case "ril.supl.apn":
      case "ril.supl.user":
      case "ril.supl.passwd":
      case "ril.supl.httpProxyHost":
      case "ril.supl.httpProxyPort":
        key = aName.slice(9);
        this.dataCallSettingsSUPL[key] = aResult;
        break;
      case "ril.callwaiting.enabled":
        this._callWaitingEnabled = aResult;
        this.setCallWaitingEnabled(this._callWaitingEnabled);
        break;
    };
  },

  handleError: function handleError(aErrorMessage) {
    debug("There was an error while reading RIL settings.");

    
    this._radioEnabled = true;
    this._ensureRadioState();

    
    this.dataCallSettings = {};
    this.dataCallSettings["enabled"] = false; 
  },

  

  worker: null,

  

  setRadioEnabled: function setRadioEnabled(value) {
    debug("Setting radio power to " + value);
    this.worker.postMessage({rilMessageType: "setRadioPower", on: value});
  },

  rilContext: null,

  

  enumerateCalls: function enumerateCalls(message) {
    debug("Requesting enumeration of calls for callback");
    message.rilMessageType = "enumerateCalls";
    this.worker.postMessage(message);
  },

  dial: function dial(number) {
    debug("Dialing " + number);
    this.worker.postMessage({rilMessageType: "dial",
                             number: number,
                             isDialEmergency: false});
  },

  dialEmergency: function dialEmergency(number) {
    debug("Dialing emergency " + number);
    this.worker.postMessage({rilMessageType: "dial",
                             number: number,
                             isDialEmergency: true});
  },

  hangUp: function hangUp(callIndex) {
    debug("Hanging up call no. " + callIndex);
    this.worker.postMessage({rilMessageType: "hangUp",
                             callIndex: callIndex});
  },

  startTone: function startTone(dtmfChar) {
    debug("Sending Tone for " + dtmfChar);
    this.worker.postMessage({rilMessageType: "startTone",
                             dtmfChar: dtmfChar});
  },

  stopTone: function stopTone() {
    debug("Stopping Tone");
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

  sendUSSD: function sendUSSD(message) {
    debug("SendUSSD " + JSON.stringify(message));
    message.rilMessageType = "sendUSSD";
    this.worker.postMessage(message);
  },

  cancelUSSD: function cancelUSSD(message) {
    debug("Cancel pending USSD");
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

  














  _countGsm7BitSeptets: function _countGsm7BitSeptets(message, langTable, langShiftTable) {
    let length = 0;
    for (let msgIndex = 0; msgIndex < message.length; msgIndex++) {
      let septet = langTable.indexOf(message.charAt(msgIndex));

      
      
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        length++;
        continue;
      }

      septet = langShiftTable.indexOf(message.charAt(msgIndex));
      if (septet < 0) {
        return -1;
      }

      
      
      
      if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
        continue;
      }

      
      
      
      
      
      length += 2;
    }

    return length;
  },

  












  _calculateUserDataLength7Bit: function _calculateUserDataLength7Bit(message) {
    let options = null;
    let minUserDataSeptets = Number.MAX_VALUE;
    for (let i = 0; i < this.enabledGsmTableTuples.length; i++) {
      let [langIndex, langShiftIndex] = this.enabledGsmTableTuples[i];

      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];

      let bodySeptets = this._countGsm7BitSeptets(message,
                                                  langTable,
                                                  langShiftTable);
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
      let userDataSeptets = bodySeptets + headerSeptets;
      let segments = bodySeptets ? 1 : 0;
      if (userDataSeptets > RIL.PDU_MAX_USER_DATA_7BIT) {
        if (this.segmentRef16Bit) {
          headerLen += 6;
        } else {
          headerLen += 5;
        }

        headerSeptets = Math.ceil((headerLen + 1) * 8 / 7);
        let segmentSeptets = RIL.PDU_MAX_USER_DATA_7BIT - headerSeptets;
        segments = Math.ceil(bodySeptets / segmentSeptets);
        userDataSeptets = bodySeptets + headerSeptets * segments;
      }

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
      };
    }

    return options;
  },

  










  _calculateUserDataLengthUCS2: function _calculateUserDataLengthUCS2(message) {
    let bodyChars = message.length;
    let headerLen = 0;
    let headerChars = Math.ceil((headerLen ? headerLen + 1 : 0) / 2);
    let segments = bodyChars ? 1 : 0;
    if ((bodyChars + headerChars) > RIL.PDU_MAX_USER_DATA_UCS2) {
      if (this.segmentRef16Bit) {
        headerLen += 6;
      } else {
        headerLen += 5;
      }

      headerChars = Math.ceil((headerLen + 1) / 2);
      let segmentChars = RIL.PDU_MAX_USER_DATA_UCS2 - headerChars;
      segments = Math.ceil(bodyChars / segmentChars);
    }

    return {
      dcs: RIL.PDU_DCS_MSG_CODING_16BITS_ALPHABET,
      encodedFullBodyLength: bodyChars * 2,
      userDataHeaderLength: headerLen,
      segmentMaxSeq: segments,
    };
  },

  



























  _calculateUserDataLength: function _calculateUserDataLength(message) {
    let options = this._calculateUserDataLength7Bit(message);
    if (!options) {
      options = this._calculateUserDataLengthUCS2(message);
    }

    if (options) {
      options.fullBody = message;
    }

    debug("_calculateUserDataLength: " + JSON.stringify(options));
    return options;
  },

  













  _fragmentText7Bit: function _fragmentText7Bit(text, langTable, langShiftTable, headerLen) {
    const headerSeptets = Math.ceil((headerLen ? headerLen + 1 : 0) * 8 / 7);
    const segmentSeptets = RIL.PDU_MAX_USER_DATA_7BIT - headerSeptets;
    let ret = [];
    let begin = 0, len = 0;
    for (let i = 0, inc = 0; i < text.length; i++) {
      let septet = langTable.indexOf(text.charAt(i));
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        inc = 1;
      } else {
        septet = langShiftTable.indexOf(text.charAt(i));
        if (septet < 0) {
          throw new Error("Given text cannot be encoded with GSM 7-bit Alphabet!");
        }

        if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
          continue;
        }

        inc = 2;
      }

      if ((len + inc) > segmentSeptets) {
        ret.push({
          body: text.substring(begin, i),
          encodedBodyLength: len,
        });
        begin = i;
        len = 0;
      }

      len += inc;
    }

    if (len) {
      ret.push({
        body: text.substring(begin),
        encodedBodyLength: len,
      });
    }

    return ret;
  },

  









  _fragmentTextUCS2: function _fragmentTextUCS2(text, headerLen) {
    const headerChars = Math.ceil((headerLen ? headerLen + 1 : 0) / 2);
    const segmentChars = RIL.PDU_MAX_USER_DATA_UCS2 - headerChars;
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

  














  _fragmentText: function _fragmentText(text, options) {
    if (!options) {
      options = this._calculateUserDataLength(text);
    }

    if (options.segmentMaxSeq <= 1) {
      options.segments = null;
      return options;
    }

    if (options.dcs == RIL.PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[options.langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[options.langShiftIndex];
      options.segments = this._fragmentText7Bit(options.fullBody,
                                                langTable, langShiftTable,
                                                options.userDataHeaderLength);
    } else {
      options.segments = this._fragmentTextUCS2(options.fullBody,
                                                options.userDataHeaderLength);
    }

    
    options.segmentMaxSeq = options.segments.length;

    return options;
  },

  getNumberOfMessagesForText: function getNumberOfMessagesForText(text) {
    return this._fragmentText(text).segmentMaxSeq;
  },

  sendSMS: function sendSMS(number, message, requestId, processId) {
    let options = this._calculateUserDataLength(message);
    options.rilMessageType = "sendSMS";
    options.number = number;
    options.requestId = requestId;
    options.processId = processId;
    options.requestStatusReport = true;

    this._fragmentText(message, options);
    if (options.segmentMaxSeq > 1) {
      options.segmentRef16Bit = this.segmentRef16Bit;
      options.segmentRef = this.nextSegmentRef;
    }

    
    this.createSmsEnvelope(options);

    this.worker.postMessage(options);
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
    debug("Registering callback: " + callback);
  },

  unregisterDataCallCallback: function unregisterDataCallCallback(callback) {
    if (!this._datacall_callbacks) {
      return;
    }
    let index = this._datacall_callbacks.indexOf(callback);
    if (index != -1) {
      this._datacall_callbacks.splice(index, 1);
      debug("Unregistering callback: " + callback);
    }
  },

  _deliverDataCallCallback: function _deliverDataCallCallback(name, args) {
    
    
    
    
    
    
    if (!this._datacall_callbacks) {
      return;
    }
    let callbacks = this._datacall_callbacks.slice();
    for each (let callback in callbacks) {
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
        debug("callback handler for " + name + " threw an exception: " + e);
      }
    }
  },

  


  usingDefaultAPN: function usingDefaultAPN(apntype) {
    switch (apntype) {
      case "mms":
        return (this.dataCallSettingsMMS["apn"] == this.dataCallSettings["apn"]);
      case "supl":
        return (this.dataCallSettingsSUPL["apn"] == this.dataCallSettings["apn"]);
      return false;
    }
  },

  setupDataCallByType: function setupDataCallByType(apntype) {
    if (apntype != "default" && this.usingDefaultAPN(apntype)) {
      debug("Secondary APN type " + apntype + " goes through default APN, nothing to do.");
      return;
    }
    switch (apntype) {
      case "default":
        this.dataNetworkInterface.connect(this.dataCallSettings);
        break;
      case "mms":
        this.mmsNetworkInterface.connect(this.dataCallSettingsMMS);
        break;
      case "supl":
        this.suplNetworkInterface.connect(this.dataCallSettingsSUPL);
        break;
      default:
        debug("Unsupported APN type " + apntype);
        break;
    }
  },

  deactivateDataCallByType: function deactivateDataCallByType(apntype) {
    if (apntype != "default" && this.usingDefaultAPN(apntype)) {
      debug("Secondary APN type " + apntype + " goes through default APN, nothing to do.");
      return;
    }
    switch (apntype) {
      case "default":
        this.dataNetworkInterface.disconnect();
        break;
      case "mms":
        this.mmsNetworkInterface.disconnect();
        break;
      case "supl":
        this.suplNetworkInterface.disconnect();
        break;
      default:
        debug("Unsupported APN type " + apntype);
        break;
    }
  },

  getDataCallStateByType: function getDataCallStateByType(apntype) {
    switch (apntype) {
      case "default":
        return this.dataNetworkInterface.state;
      case "mms":
        return this.mmsNetworkInterface.state;
      case "supl":
        return this.suplNetworkInterface.state;
      default:
        return RIL.GECKO_NETWORK_STATE_UNKNOWN;
    }
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

  getCardLock: function getCardLock(message) {
    message.rilMessageType = "iccGetCardLock";
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

  _contactsCallbacks: null,
  getICCContacts: function getICCContacts(contactType, callback) {
    if (!this._contactsCallbacks) {
      this._contactsCallbacks = {};
    }
    let requestId = Math.floor(Math.random() * 1000);
    this._contactsCallbacks[requestId] = callback;
    this.worker.postMessage({rilMessageType: "getICCContacts",
                             contactType: contactType,
                             requestId: requestId});
  }
};

function RILNetworkInterface(ril, type)
{
  this.mRIL = ril;
  this.type = type;
}

RILNetworkInterface.prototype = {
  classID:   RILNETWORKINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILNETWORKINTERFACE_CID,
                                    classDescription: "RILNetworkInterface",
                                    interfaces: [Ci.nsINetworkInterface,
                                                 Ci.nsIRIODataCallback]}),
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

  



  NETWORK_APNRETRY_FACTOR: 8,
  NETWORK_APNRETRY_ORIGIN: 3,
  NETWORK_APNRETRY_MAXRETRIES: 10,

  
  timer: null,

  type: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,

  name: null,

  dhcp: false,

  ip: null,

  netmask: null,

  broadcast: null,

  dns1: null,

  dns2: null,

  httpProxyHost: null,

  httpProxyPort: null,

  

  dataCallError: function dataCallError(message) {
    if (message.apn != this.dataCallSettings["apn"]) {
      return;
    }
    debug("Data call error on APN: " + message.apn);
    this.reset();
  },

  dataCallStateChanged: function dataCallStateChanged(datacall) {
    if (datacall.apn != this.dataCallSettings["apn"]) {
      return;
    }
    debug("Data call ID: " + datacall.cid + ", interface name: " +
          datacall.ifname + ", APN name: " + datacall.apn);
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
    if (this.cid != datacall.cid) {
      return;
    }
    if (this.state == datacall.state) {
      return;
    }

    this.state = datacall.state;

    if (this.state == RIL.GECKO_NETWORK_STATE_UNKNOWN &&
       this.registeredAsNetworkInterface) {
      gNetworkManager.unregisterNetworkInterface(this);
      this.registeredAsNetworkInterface = false;
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
  dataCallSettings: {},

  
  apnRetryCounter: 0,

  get connected() {
    return this.state == RIL.GECKO_NETWORK_STATE_CONNECTED;
  },

  connect: function connect(options) {
    if (this.connecting || this.connected) {
      return;
    }

    if (!this.registeredAsDataCallCallback) {
      this.mRIL.registerDataCallCallback(this);
      this.registeredAsDataCallCallback = true;
    }

    if (options) {
      
      this.dataCallSettings = options;
    }

    if (!this.dataCallSettings["apn"]) {
      debug("APN name is empty, nothing to do.");
      return;
    }

    this.httpProxyHost = this.dataCallSettings["httpProxyHost"];
    this.httpProxyPort = this.dataCallSettings["httpProxyPort"];

    debug("Going to set up data connection with APN " + this.dataCallSettings["apn"]);
    this.mRIL.setupDataCall(RIL.DATACALL_RADIOTECHNOLOGY_GSM,
                            this.dataCallSettings["apn"], 
                            this.dataCallSettings["user"], 
                            this.dataCallSettings["passwd"],
                            RIL.DATACALL_AUTH_PAP_OR_CHAP, "IP");
    this.connecting = true;
  },

  reset: function reset() {
    let apnRetryTimer;
    this.connecting = false;
    
    
    if (this.apnRetryCounter >= this.NETWORK_APNRETRY_MAXRETRIES) {
      this.apnRetryCounter = 0;
      this.timer = null;
      debug("Too many APN Connection retries - STOP retrying");
      return;
    }

    apnRetryTimer = this.NETWORK_APNRETRY_FACTOR *
                    (this.apnRetryCounter * this.apnRetryCounter) +
                    this.NETWORK_APNRETRY_ORIGIN;
    this.apnRetryCounter++;
    debug("Data call - APN Connection Retry Timer (secs-counter): " +
          apnRetryTimer + "-" + this.apnRetryCounter);

    if (this.timer == null) {
      
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this.timer.initWithCallback(this, apnRetryTimer * 1000,
                                Ci.nsITimer.TYPE_ONE_SHOT);
  },

  disconnect: function disconnect() {
    if (this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTING ||
        this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED) {
      return;
    }
    let reason = RIL.DATACALL_DEACTIVATE_NO_REASON;
    debug("Going to disconnet data connection " + this.cid);
    this.mRIL.deactivateDataCall(this.cid, reason);
  },

  
  notify: function(timer) {
    this.connect();
  },

  shutdown: function() {
    this.timer = null;
  }

};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([RadioInterfaceLayer]);

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- RadioInterfaceLayer: " + s + "\n");
  };
} else {
  debug = function (s) {};
}
