






































"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const DEBUG = false; 

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const DATACALLINFO_CID =
  Components.ID("{ef474cd9-94f7-4c05-a31b-29b9de8a10d2}");

const nsIAudioManager = Ci.nsIAudioManager;
const nsIRadioInterfaceLayer = Ci.nsIRadioInterfaceLayer;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSmsDeliveredObserverTopic         = "sms-delivered";
const DOM_SMS_DELIVERY_RECEIVED          = "received";
const DOM_SMS_DELIVERY_SENT              = "sent";

const RIL_IPC_MSG_NAMES = [
  "RIL:GetRadioState",
  "RIL:EnumerateCalls",
  "RIL:GetMicrophoneMuted",
  "RIL:SetMicrophoneMuted",
  "RIL:GetSpeakerEnabled",
  "RIL:SetSpeakerEnabled",
  "RIL:StartTone",
  "RIL:StopTone",
  "RIL:Dial",
  "RIL:HangUp",
  "RIL:AnswerCall",
  "RIL:RejectCall",
  "RIL:HoldCall",
  "RIL:ResumeCall",
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
                                   "nsIFrameMessageManager");

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


function DataCallInfo(state, cid, apn) {
  this.callState = state;
  this.cid = cid;
  this.apn = apn;
}
DataCallInfo.protoptype = {
  classID:      DATACALLINFO_CID,
  classInfo:    XPCOMUtils.generateCI({classID: DATACALLINFO_CID,
                                       classDescription: "DataCallInfo",
                                       interfaces: [Ci.nsIDataCallInfo]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallInfo]),
};


function RadioInterfaceLayer() {
  debug("Starting RIL Worker");
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  this.radioState = {
    radioState:     RIL.GECKO_RADIOSTATE_UNAVAILABLE,
    cardState:      RIL.GECKO_CARDSTATE_UNAVAILABLE,
    icc:            null,

    
    
    voice:          {connected: false,
                     emergencyCallsOnly: false,
                     roaming: false,
                     operator: null,
                     type: null,
                     signalStrength: null,
                     relSignalStrength: null},
    data:          {connected: false,
                     emergencyCallsOnly: false,
                     roaming: false,
                     operator: null,
                     type: null,
                     signalStrength: null,
                     relSignalStrength: null},
  };
  for each (let msgname in RIL_IPC_MSG_NAMES) {
    ppmm.addMessageListener(msgname, this);
  }
  Services.obs.addObserver(this, "xpcom-shutdown", false);

  this._sentSmsEnvelopes = {};
  this.portAddressedSmsApps = {};
}
RadioInterfaceLayer.prototype = {

  classID:   RADIOINTERFACELAYER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RADIOINTERFACELAYER_CID,
                                    classDescription: "RadioInterfaceLayer",
                                    interfaces: [Ci.nsIWorkerHolder,
                                                 Ci.nsIRadioInterfaceLayer]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWorkerHolder,
                                         Ci.nsIRadioInterfaceLayer]),

  


  receiveMessage: function receiveMessage(msg) {
    debug("Received '" + msg.name + "' message from content process");
    switch (msg.name) {
      case "RIL:GetRadioState":
        
        return this.radioState;
      case "RIL:EnumerateCalls":
        this.enumerateCalls();
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
    switch (message.type) {
      case "callStateChange":
        
        this.handleCallStateChange(message.call);
        break;
      case "callDisconnected":
        
        this.handleCallDisconnected(message.call);
        break;
      case "enumerateCalls":
        
        this.handleEnumerateCalls(message.calls);
        break;
      case "voiceregistrationstatechange":
        this.updateVoiceConnection(message);
        break;
      case "dataregistrationstatechange":
        this.updateDataConnection(message);
        break;
      case "signalstrengthchange":
        this.handleSignalStrengthChange(message);
        break;
      case "operatorchange":
        this.handleOperatorChange(message);
        break;
      case "radiostatechange":
        this.radioState.radioState = message.radioState;
        break;
      case "cardstatechange":
        this.radioState.cardState = message.cardState;
        ppmm.sendAsyncMessage("RIL:CardStateChange", message);
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
        this.handleDataCallState(message.datacall);
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
        this.radioState.icc = message;
        break;
      default:
        throw new Error("Don't know about this message type: " + message.type);
    }
  },

  updateVoiceConnection: function updateVoiceConnection(state) {
    let voiceInfo = this.radioState.voice;
    voiceInfo.type = "gsm"; 
    if (!state || state.regState == RIL.NETWORK_CREG_STATE_UNKNOWN) {
      voiceInfo.connected = false;
      voiceInfo.emergencyCallsOnly = false;
      voiceInfo.roaming = false;
      voiceInfo.operator = null;
      voiceInfo.type = null;
      voiceInfo.signalStrength = null;
      voiceInfo.relSignalStrength = null;
      ppmm.sendAsyncMessage("RIL:VoiceThis.RadioState.VoiceChanged",
                            voiceInfo);
      return;
    }
    
    voiceInfo.connected =
      (state.regState == RIL.NETWORK_CREG_STATE_REGISTERED_HOME) ||
      (state.regState == RIL.NETWORK_CREG_STATE_REGISTERED_ROAMING);
    voiceInfo.roaming =
      voiceInfo.connected &&
      (state == RIL.NETWORK_CREG_STATE_REGISTERED_ROAMING);    
    voiceInfo.type =
      RIL.GECKO_RADIO_TECH[state.radioTech] || null;
    ppmm.sendAsyncMessage("RIL:VoiceInfoChanged", voiceInfo);

  },

  _isDataEnabled: function _isDataEnabled() {
    try {
      return Services.prefs.getBoolPref("ril.data.enabled");
    } catch(ex) {
      return false;
    }
  },

  _isDataRoamingEnabled: function _isDataRoamingEnabled() {
    try {
      return Services.prefs.getBoolPref("ril.data.roaming.enabled");
    } catch(ex) {
      return false;
    }
  },

  updateDataConnection: function updateDataConnection(state) {
    if (!this._isDataEnabled()) {
      return;
    }

    let isRegistered =
      state.regState == RIL.NETWORK_CREG_STATE_REGISTERED_HOME ||
        (this._isDataRoamingEnabled() &&
         state.regState == RIL.NETWORK_CREG_STATE_REGISTERED_ROAMING);
    let haveDataConnection =
      state.radioTech != RIL.NETWORK_CREG_TECH_UNKNOWN;

    if (isRegistered && haveDataConnection) {
      debug("Radio is ready for data connection.");
      
      RILNetworkInterface.connect();
    }
    
    
    
  },

  handleSignalStrengthChange: function handleSignalStrengthChange(message) {
    
    this.radioState.voice.signalStrength = message.gsmDBM;
    this.radioState.voice.relSignalStrength = message.gsmRelative;
    ppmm.sendAsyncMessage("RIL:VoiceInfoChanged", this.radioState.voice);

    this.radioState.data.signalStrength = message.gsmDBM;
    this.radioState.data.relSignalStrength = message.gsmRelative;
    ppmm.sendAsyncMessage("RIL:DataInfoChanged", this.radioState.data);
  },

  handleOperatorChange: function handleOperatorChange(message) {
    let operator = message.alphaLong;
    if (operator != this.radioState.voice.operator) {
      this.radioState.voice.operator = operator;
      ppmm.sendAsyncMessage("RIL:VoiceInfoChanged", this.radioState.voice);
    }
    if (operator != this.radioState.data.operator) {
      this.radioState.data.operator = operator;
      ppmm.sendAsyncMessage("RIL:DataInfoChanged", this.radioState.data);
    }
  },

  




  _activeCall: null,
  updateCallAudioState: function updateCallAudioState() {
    if (!this._activeCall) {
      
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
      debug("No active call, put audio system into PHONE_STATE_NORMAL.");
      return;
    }
    switch (this._activeCall.state) {
      case nsIRadioInterfaceLayer.CALL_STATE_INCOMING:
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_RINGTONE;
        debug("Incoming call, put audio system into PHONE_STATE_RINGTONE.");
        break;
      case nsIRadioInterfaceLayer.CALL_STATE_DIALING: 
      case nsIRadioInterfaceLayer.CALL_STATE_CONNECTED:
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
        gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                     nsIAudioManager.FORCE_NONE);
        debug("Active call, put audio system into PHONE_STATE_IN_CALL.");
        break;
    }
  },

  



  handleCallStateChange: function handleCallStateChange(call) {
    debug("handleCallStateChange: " + JSON.stringify(call));
    call.state = convertRILCallState(call.state);
    if (call.state == nsIRadioInterfaceLayer.CALL_STATE_DIALING ||
        call.state == nsIRadioInterfaceLayer.CALL_STATE_ALERTING ||
        call.state == nsIRadioInterfaceLayer.CALL_STATE_CONNECTED) {
      
      this._activeCall = call;
    }
    this.updateCallAudioState();
    ppmm.sendAsyncMessage("RIL:CallStateChanged", call);
  },

  


  handleCallDisconnected: function handleCallDisconnected(call) {
    debug("handleCallDisconnected: " + JSON.stringify(call));
    if (this._activeCall && this._activeCall.callIndex == call.callIndex) {
      this._activeCall = null;
    }
    this.updateCallAudioState();
    call.state = nsIRadioInterfaceLayer.CALL_STATE_DISCONNECTED;
    ppmm.sendAsyncMessage("RIL:CallStateChanged", call);
  },

  


  handleEnumerateCalls: function handleEnumerateCalls(calls) {
    debug("handleEnumerateCalls: " + JSON.stringify(calls));
    let activeCallIndex = this._activeCall ? this._activeCall.callIndex : -1;
    for (let i in calls) {
      calls[i].state = convertRILCallState(calls[i].state);
    }
    ppmm.sendAsyncMessage("RIL:EnumerateCalls",
                          {calls: calls, activeCallIndex: activeCallIndex});
  },

  portAddressedSmsApps: null,
  handleSmsReceived: function handleSmsReceived(message) {
    debug("handleSmsReceived: " + JSON.stringify(message));

    
    
    
    if (message.header.destinationPort != null) {
      let handler = this.portAddressedSmsApps[message.header.destinationPort];
      if (handler) {
        handler(message);
      }
      return;
    }

    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      return;
    }

    let id = gSmsDatabaseService.saveReceivedMessage(message.sender || null,
                                                     message.fullBody || null,
                                                     message.timestamp);
    let sms = gSmsService.createSmsMessage(id,
                                           DOM_SMS_DELIVERY_RECEIVED,
                                           message.sender || null,
                                           message.receiver || null,
                                           message.fullBody || null,
                                           message.timestamp);
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
                                           timestamp);

    if (!options.requestStatusReport) {
      
      delete this._sentSmsEnvelopes[message.envelopeId];
    } else {
      options.sms = sms;
    }

    gSmsRequestManager.notifySmsSent(options.requestId, sms);
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
    this._deliverDataCallCallback("dataCallStateChanged",
                                  [datacall.cid, datacall.ifname, datacall.state]);
  },

  


  handleDataCallList: function handleDataCallList(message) {
    let datacalls = [];
    for each (let datacall in message.datacalls) {
      datacalls.push(new DataCallInfo(datacall.state,
                                      datacall.cid,
                                      datacall.apn));
    }
    this._deliverDataCallCallback("receiveDataCallList",
                                  [datacalls, datacalls.length]);
  },

  

  observe: function observe(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      for each (let msgname in RIL_IPC_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      Services.obs.removeObserver(this, "xpcom-shutdown");
      ppmm = null;
    }
  },

  

  worker: null,

  

  radioState: null,

  

  enumerateCalls: function enumerateCalls() {
    debug("Requesting enumeration of calls for callback");
    this.worker.postMessage({type: "enumerateCalls"});
  },

  dial: function dial(number) {
    debug("Dialing " + number);
    this.worker.postMessage({type: "dial", number: number});
  },

  hangUp: function hangUp(callIndex) {
    debug("Hanging up call no. " + callIndex);
    this.worker.postMessage({type: "hangUp", callIndex: callIndex});
  },

  startTone: function startTone(dtmfChar) {
    debug("Sending Tone for " + dtmfChar);
    this.worker.postMessage({type: "startTone", dtmfChar: dtmfChar});
  },

  stopTone: function stopTone() {
    debug("Stopping Tone");
    this.worker.postMessage({type: "stopTone"});
  },

  answerCall: function answerCall(callIndex) {
    this.worker.postMessage({type: "answerCall", callIndex: callIndex});
  },

  rejectCall: function rejectCall(callIndex) {
    this.worker.postMessage({type: "rejectCall", callIndex: callIndex});
  },
 
  holdCall: function holdCall(callIndex) {
    this.worker.postMessage({type: "holdCall", callIndex: callIndex});
  },

  resumeCall: function resumeCall(callIndex) {
    this.worker.postMessage({type: "resumeCall", callIndex: callIndex});
  },

  get microphoneMuted() {
    return gAudioManager.microphoneMuted;
  },
  set microphoneMuted(value) {
    if (value == this.microphoneMuted) {
      return;
    }
    gAudioManager.phoneState = value ?
      nsIAudioManager.PHONE_STATE_IN_COMMUNICATION :
      nsIAudioManager.PHONE_STATE_IN_CALL;  
    gAudioManager.microphoneMuted = value;
  },

  get speakerEnabled() {
    return (gAudioManager.getForceForUse(nsIAudioManager.USE_COMMUNICATION) ==
            nsIAudioManager.FORCE_SPEAKER);
  },
  set speakerEnabled(value) {
    if (value == this.speakerEnabled) {
      return;
    }
    gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL; 
    let force = value ? nsIAudioManager.FORCE_SPEAKER :
                        nsIAudioManager.FORCE_NONE;
    gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION, force);
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
    options.type = "sendSMS";
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

  setupDataCall: function setupDataCall(radioTech, apn, user, passwd, chappap, pdptype) {
    this.worker.postMessage({type: "setupDataCall",
                             radioTech: radioTech,
                             apn: apn,
                             user: user,
                             passwd: passwd,
                             chappap: chappap,
                             pdptype: pdptype});
  },

  deactivateDataCall: function deactivateDataCall(cid, reason) {
    this.worker.postMessage({type: "deactivateDataCall",
                             cid: cid,
                             reason: reason});
  },

  getDataCallList: function getDataCallList() {
    this.worker.postMessage({type: "getDataCallList"});
  },

};


let RILNetworkInterface = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface,
                                         Ci.nsIRILDataCallback]),

  

  NETWORK_STATE_UNKNOWN:       Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,
  NETWORK_STATE_CONNECTING:    Ci.nsINetworkInterface.CONNECTING,
  NETWORK_STATE_CONNECTED:     Ci.nsINetworkInterface.CONNECTED,
  NETWORK_STATE_SUSPENDED:     Ci.nsINetworkInterface.SUSPENDED,
  NETWORK_STATE_DISCONNECTING: Ci.nsINetworkInterface.DISCONNECTING,
  NETWORK_STATE_DISCONNECTED:  Ci.nsINetworkInterface.DISCONNECTED,

  state: Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN,

  NETWORK_TYPE_WIFI:       Ci.nsINetworkInterface.NETWORK_TYPE_WIFI,
  NETWORK_TYPE_MOBILE:     Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,
  NETWORK_TYPE_MOBILE_MMS: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS,

  type: Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,

  name: null,

  dhcp: false,

  

  dataCallStateChanged: function dataCallStateChanged(cid, interfaceName, callState) {
    if (this.connecting &&
        (callState == RIL.GECKO_NETWORK_STATE_CONNECTING ||
         callState == RIL.GECKO_NETWORK_STATE_CONNECTED)) {
      this.connecting = false;
      this.cid = cid;
      this.name = interfaceName;
      debug("Data call ID: " + cid + ", interface name: " + interfaceName);
      if (!this.registeredAsNetworkInterface) {
        let networkManager = Cc["@mozilla.org/network/manager;1"]
                               .getService(Ci.nsINetworkManager);
        networkManager.registerNetworkInterface(this);
        this.registeredAsNetworkInterface = true;
      }
    }
    if (this.cid != cid) {
      return;
    }
    if (this.state == callState) {
      return;
    }

    this.state = callState;
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

  get mRIL() {
    delete this.mRIL;
    return this.mRIL = Cc["@mozilla.org/telephony/system-worker-manager;1"]
                         .getService(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIRadioInterfaceLayer);
  },

  connect: function connect() {
    if (this.connecting ||
        this.state == RIL.GECKO_NETWORK_STATE_CONNECTED ||
        this.state == RIL.GECKO_NETWORK_STATE_SUSPENDED ||
        this.state == RIL.GECKO_NETWORK_STATE_DISCONNECTING) {
      return;
    }
    if (!this.registeredAsDataCallCallback) {
      this.mRIL.registerDataCallCallback(this);
      this.registeredAsDataCallCallback = true;
    }

    let apn, user, passwd;
    
    
    try {
      apn = Services.prefs.getCharPref("ril.data.apn");
      user = Services.prefs.getCharPref("ril.data.user");
      passwd = Services.prefs.getCharPref("ril.data.passwd");
    } catch (ex) {
      debug("No APN settings found, not going to set up data connection.");
      return;
    }
    debug("Going to set up data connection with APN " + apn);
    this.mRIL.setupDataCall(RIL.DATACALL_RADIOTECHNOLOGY_GSM,
                            apn, user, passwd,
                            RIL.DATACALL_AUTH_PAP_OR_CHAP, "IP");
    this.connecting = true;
  },

  disconnect: function disconnect() {
    this.mRIL.deactivateDataCall(this.cid);
  },

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
