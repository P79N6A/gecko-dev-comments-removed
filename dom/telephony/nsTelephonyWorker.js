





































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const DEBUG = true; 

const TELEPHONYWORKER_CONTRACTID = "@mozilla.org/telephony/worker;1";
const TELEPHONYWORKER_CID        = Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");

const DOM_CALL_READYSTATE_DIALING        = "dialing";
const DOM_CALL_READYSTATE_RINGING        = "ringing";
const DOM_CALL_READYSTATE_BUSY           = "busy";
const DOM_CALL_READYSTATE_CONNECTING     = "connecting";
const DOM_CALL_READYSTATE_CONNECTED      = "connected";
const DOM_CALL_READYSTATE_DISCONNECTING  = "disconnecting";
const DOM_CALL_READYSTATE_DISCONNECTED   = "disconnected";
const DOM_CALL_READYSTATE_INCOMING       = "incoming";
const DOM_CALL_READYSTATE_HOLDING        = "holding";
const DOM_CALL_READYSTATE_HELD           = "held";






let FakeAudioManager = {
  microphoneMuted: false,
  masterVolume: 1.0,
  masterMuted: false,
  phoneState: Ci.nsIAudioManager.PHONE_STATE_CURRENT,
  _forceForUse: {},
  setForceForUse: function setForceForUse(usage, force) {
    this._forceForUse[usage] = force;
  },
  getForceForUse: function setForceForUse(usage) {
    return this._forceForUse[usage] || Ci.nsIAudioManager.FORCE_NONE;
  }
};

XPCOMUtils.defineLazyGetter(this, "gAudioManager", function getAudioManager() {
  try {
    return Cc["@mozilla.org/telephony/audiomanager;1"]
             .getService(Ci.nsIAudioManager);
  } catch (ex) {
    
    debug("Using fake audio manager.");
    return FakeAudioManager;
  }
});


function nsTelephonyWorker() {
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  this._callbacks = [];
  this.currentState = {
    signalStrength: null,
    operator:       null,
    radioState:     null,
    cardState:      null,
    currentCalls:   {}
  };
}
nsTelephonyWorker.prototype = {

  classID:   TELEPHONYWORKER_CID,
  classInfo: XPCOMUtils.generateCI({classID: TELEPHONYWORKER_CID,
                                    contractID: TELEPHONYWORKER_CONTRACTID,
                                    classDescription: "TelephonyWorker",
                                    interfaces: [Ci.nsIRadioWorker,
                                                 Ci.nsITelephone]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioWorker,
                                         Ci.nsITelephone]),

  onerror: function onerror(event) {
    
    
    
    
    
    
    
    
    event.preventDefault();

    debug("Got an error: " + event.filename + ":" +
          event.lineno + ": " + event.message + "\n");
  },

  







  onmessage: function onmessage(event) {
    let message = event.data;
    debug("Received message: " + JSON.stringify(message));
    let value;
    switch (message.type) {
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
      case "callstatechange":
        this.handleCallState(message);
        break;
      default:
        
        return;
    }
    let methodname = "on" + message.type;
    this._callbacks.forEach(function (callback) {
      let method = callback[methodname];
      if (typeof method != "function") {
        return;
      }
      method.call(callback, message);
    });
  },

  



  handleCallState: function handleCallState(message) {
    let currentCalls = this.currentState.currentCalls;
    let oldState = currentCalls[message.callIndex];

    
    if (message.callState == DOM_CALL_READYSTATE_DISCONNECTED) {
      delete currentCalls[message.callIndex];
    } else {
      currentCalls[message.callIndex] = message;
    }

    
    
    switch (message.callState) {
      case DOM_CALL_READYSTATE_DIALING:
        this.worker.postMessage({type: "setMute", mute: false});
        gAudioManager.phoneState = Ci.nsIAudioManager.PHONE_STATE_IN_CALL;
        gAudioManager.setForceForUse(Ci.nsIAudioManager.USE_COMMUNICATION,
                                     Ci.nsIAudioManager.FORCE_NONE);
        break;
      case DOM_CALL_READYSTATE_INCOMING:
        gAudioManager.phoneState = Ci.nsIAudioManager.PHONE_STATE_RINGTONE;
        break;
      case DOM_CALL_READYSTATE_CONNECTED:
        if (!oldState ||
            oldState.callState == DOM_CALL_READYSTATE_INCOMING ||
            oldState.callState == DOM_CALL_READYSTATE_CONNECTING) {
          
          
          this.worker.postMessage({type: "setMute", mute: false});
          gAudioManager.phoneState = Ci.nsIAudioManager.PHONE_STATE_IN_CALL;
          gAudioManager.setForceForUse(Ci.nsIAudioManager.USE_COMMUNICATION,
                                       Ci.nsIAudioManager.FORCE_NONE);
        }
        break;
      case DOM_CALL_READYSTATE_DISCONNECTED:
        this.worker.postMessage({type: "setMute", mute: true});
        gAudioManager.phoneState = Ci.nsIAudioManager.PHONE_STATE_NORMAL;
        break;
    }
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

  answerCall: function answerCall() {
    this.worker.postMessage({type: "answerCall"});
  },

  rejectCall: function rejectCall() {
    this.worker.postMessage({type: "rejectCall"});
  },

  _callbacks: null,

  registerCallback: function registerCallback(callback) {
    this._callbacks.push(callback);
  },

  unregisterCallback: function unregisterCallback(callback) {
    let index = this._callbacks.indexOf(callback);
    if (index == -1) {
      throw "Callback not registered!";
    }
    this._callbacks.splice(index, 1);
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
