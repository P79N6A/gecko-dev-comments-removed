





































"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const DEBUG = true; 

const TELEPHONYWORKER_CONTRACTID = "@mozilla.org/telephony/worker;1";
const TELEPHONYWORKER_CID        = Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");

const nsIAudioManager = Ci.nsIAudioManager;
const nsITelephone = Ci.nsITelephone;

const kSmsReceivedObserverTopic          = "sms-received";
const DOM_SMS_DELIVERY_RECEIVED          = "received";

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");

function convertRILCallState(state) {
  switch (state) {
    case RIL.CALL_STATE_ACTIVE:
      return nsITelephone.CALL_STATE_CONNECTED;
    case RIL.CALL_STATE_HOLDING:
      return nsITelephone.CALL_STATE_HELD;
    case RIL.CALL_STATE_DIALING:
      return nsITelephone.CALL_STATE_DIALING;
    case RIL.CALL_STATE_ALERTING:
      return nsITelephone.CALL_STATE_RINGING;
    case RIL.CALL_STATE_INCOMING:
      return nsITelephone.CALL_STATE_INCOMING;
    case RIL.CALL_STATE_WAITING:
      return nsITelephone.CALL_STATE_HELD; 
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


function nsTelephonyWorker() {
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);
  debug("Starting Worker\n");
  this.currentState = {
    signalStrength: null,
    operator:       null,
    radioState:     null,
    cardState:      null
  };
}
nsTelephonyWorker.prototype = {

  classID:   TELEPHONYWORKER_CID,
  classInfo: XPCOMUtils.generateCI({classID: TELEPHONYWORKER_CID,
                                    contractID: TELEPHONYWORKER_CONTRACTID,
                                    classDescription: "Telephone",
                                    interfaces: [Ci.nsIRadioWorker,
                                                 Ci.nsITelephone]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioWorker,
                                         Ci.nsITelephone]),

  onerror: function onerror(event) {
    debug("Got an error: " + event.filename + ":" +
          event.lineno + ": " + event.message + "\n");
    event.preventDefault();
  },

  







  onmessage: function onmessage(event) {
    let message = event.data;
    debug("Received message: " + JSON.stringify(message));
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
      case "signalstrengthchange":
        this.currentState.signalStrength = message.signalStrength;
        break;
      case "operatorchange":
        this.currentState.operator = message.operator;
        break;
      case "radiostatechange":
        this.currentState.radioState = message.radioState;
        break;
      case "cardstatechange":
        this.currentState.cardState = message.cardState;
        break;
      case "sms-received":
        this.handleSmsReceived(message);
        return;
      default:
        throw new Error("Don't know about this message type: " + message.type);
    }
  },

  




  _activeCall: null,
  get activeCall() {
    return this._activeCall;
  },
  set activeCall(val) {
    if (val && !this._activeCall) {
      
      switch (val.state) {
        case nsITelephone.CALL_STATE_INCOMING:
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_RINGTONE;
          break;
        case nsITelephone.CALL_STATE_DIALING: 
        case nsITelephone.CALL_STATE_CONNECTED:
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
          gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                       nsIAudioManager.FORCE_NONE);
          break;
        default:
          throw new Error("Invalid call state for active call: " + val.state);
      }
    } else if (!val && this._activeCall) {
      
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
    }
    this._activeCall = val;
  },

  



  handleCallStateChange: function handleCallStateChange(call) {
    debug("handleCallStateChange: " + JSON.stringify(call));
    call.state = convertRILCallState(call.state);
    if (call.state == nsITelephone.CALL_STATE_INCOMING ||
        call.state == nsITelephone.CALL_STATE_DIALING ||
        call.state == nsITelephone.CALL_STATE_CONNECTED) {
      
      this.activeCall = call;
    }
    this._deliverCallback("callStateChanged",
                          [call.callIndex, call.state, call.number]);
  },

  


  handleCallDisconnected: function handleCallStateChange(call) {
    debug("handleCallDisconnected: " + JSON.stringify(call));
    if (this.activeCall == call) {
      
      this.activeCall = null;
    }
    this._deliverCallback("callStateChanged",
                          [call.callIndex, nsITelephone.CALL_STATE_DISCONNECTED,
                           call.number]);
  },

  


  handleEnumerateCalls: function handleEnumerateCalls(calls) {
    debug("handleEnumerateCalls: " + JSON.stringify(calls));
    let callback = this._enumerationCallbacks.shift();
    let activeCallIndex = this.activeCall ? this.activeCall.callIndex : -1;
    for (let i in calls) {
      let call = calls[i];
      let state = convertRILCallState(call.state);
      let keepGoing;
      try {
        keepGoing =
          callback.enumerateCallState(call.callIndex, state, call.number,
                                      call.callIndex == activeCallIndex);
      } catch (e) {
        debug("callback handler for 'enumerateCallState' threw an " +
              " exception: " + e);
        keepGoing = true;
      }
      if (!keepGoing) {
        break;
      }
    }
  },

  handleSmsReceived: function handleSmsReceived(message) {
    
    let sms = gSmsService.createSmsMessage(-1,
                                           DOM_SMS_DELIVERY_RECEIVED,
                                           message.sender || null,
                                           message.receiver || null,
                                           message.body || null,
                                           message.timestamp);
    Services.obs.notifyObservers(sms, kSmsReceivedObserverTopic, null);
  },

  

  worker: null,

  

  currentState: null,

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
    gAudioManager.setForceUse(nsIAudioManager.USE_COMMUNICATION, force);
  },

  getNumberOfMessagesForText: function getNumberOfMessagesForText(text) {
    
    
    
    return Math.ceil(text.length / 160);
  },

  sendSMS: function sendSMS(number, message) {
    this.worker.postMessage({type: "sendSMS",
                             number: number,
                             body: message});
  },

  _callbacks: null,
  _enumerationCallbacks: null,

  registerCallback: function registerCallback(callback) {
    debug("Registering callback: " + callback);
    if (this._callbacks) {
      if (this._callbacks.indexOf(callback) != -1) {
        throw new Error("Already registered this callback!");
      }
    } else {
      this._callbacks = [];
    }
    this._callbacks.push(callback);
  },

  unregisterCallback: function unregisterCallback(callback) {
    debug("Unregistering callback: " + callback);
    let index;
    if (this._callbacks && (index = this._callbacks.indexOf(callback) != -1)) {
      this._callbacks.splice(index, 1);
    }
  },

  enumerateCalls: function enumerateCalls(callback) {
    debug("Requesting enumeration of calls for callback: " + callback);
    this.worker.postMessage({type: "enumerateCalls"});
    if (!this._enumerationCallbacks) {
      this._enumerationCallbacks = [];
    }
    this._enumerationCallbacks.push(callback);
  },

  _deliverCallback: function _deliverCallback(name, args) {
    
    
    
    
    
    
    if (!this._callbacks) {
      return;
    }
    let callbacks = this._callbacks.slice();
    for each (let callback in callbacks) {
      if (this._callbacks.indexOf(callback) == -1) {
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
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([nsTelephonyWorker]);

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- TelephonyWorker component: " + s + "\n");
  };
} else {
  debug = function (s) {};
}
