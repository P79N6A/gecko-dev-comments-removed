





































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const DEBUG = true; 

const TELEPHONYWORKER_CONTRACTID = "@mozilla.org/telephony/worker;1";
const TELEPHONYWORKER_CID        = Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");


function nsTelephonyWorker() {
  this.worker = new ChromeWorker("resource://gre/modules/ril_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  this._callbacks = [];
  this.initialState = {};
}
nsTelephonyWorker.prototype = {

  classID:   TELEPHONYWORKER_CID,
  classInfo: XPCOMUtils.generateCI({classID: TELEPHONYWORKER_CID,
                                    contractID: TELEPHONYWORKER_CONTRACTID,
                                    classDescription: "TelephonyWorker",
                                    interfaces: [Ci.nsITelephonyWorker,
                                                 Ci.nsIRadioInterface]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyWorker,
                                         Ci.nsIRadioInterface]),

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
        this.initialState.signalStrength = message.signalStrength;
        value = message.signalStrength;
        break;
      case "operatorchange":
        this.initialState.operator = message.operator;
        value = message.operator;
        break;
      case "onradiostatechange":
        this.initialState.radioState = message.radioState;
        value = message.radioState;
        break;
      case "cardstatechange":
        this.initialState.cardState = message.cardState;
        value = message.cardState;
        break;
      case "callstatechange":
        this.initialState.callState = message.callState;
        value = message.callState;
        break;
      default:
        
    }
    this._callbacks.forEach(function (callback) {
      let method = callback[methodname];
      if (typeof method != "function") {
        return;
      }
      method.call(callback, value);
    });
  },

  

  worker: null,

  

  initialState: null,

  dial: function dial(number) {
    debug("Dialing " + number);
    this.worker.postMessage({type: "dial", number: number});
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
