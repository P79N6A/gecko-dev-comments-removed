




































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const TELEPHONY_CID = Components.ID("{37e248d2-02ff-469b-bb31-eef5a4a4bee3}");
const TELEPHONY_CONTRACTID = "@mozilla.org/telephony;1";

const TELEPHONY_CALL_CID = Components.ID("{6b9b3daf-e5ea-460b-89a5-641ee20dd577}");
const TELEPHONY_CALL_CONTRACTID = "@mozilla.org/telephony-call;1";


const DOM_RADIOSTATE_UNAVAILABLE   = "unavailable";
const DOM_RADIOSTATE_OFF           = "off";
const DOM_RADIOSTATE_READY         = "ready";

const DOM_CARDSTATE_UNAVAILABLE    = "unavailable";
const DOM_CARDSTATE_ABSENT         = "absent";
const DOM_CARDSTATE_PIN_REQUIRED   = "pin_required";
const DOM_CARDSTATE_PUK_REQUIRED   = "puk_required";
const DOM_CARDSTATE_NETWORK_LOCKED = "network_locked";
const DOM_CARDSTATE_NOT_READY      = "not_ready";
const DOM_CARDSTATE_READY          = "ready";

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

const CALLINDEX_TEMPORARY_DIALING = -1;









function defineEventListenerSlot(object, event_type) {
  let property_name = "on" + event_type;
  let hidden_name = "_on" + event_type;
  let bound_name = "_bound_on" + event_type;
  object.__defineGetter__(property_name, function getter() {
    return this[hidden_name];
  });
  object.__defineSetter__(property_name, function setter(handler) {
    let old_handler = this[bound_name];
    if (old_handler) {
      this.removeEventListener(event_type, old_handler);
    }
    
    let bound_handler = handler.bind(this);
    this.addEventListener(event_type, bound_handler);
    this[hidden_name] = handler;
    this[bound_name] = bound_handler;
  });
}





function EventTarget() {}
EventTarget.prototype = {

  addEventListener: function addEventListener(type, handler) {
    
    if (!this._listeners) {
      this._listeners = {};
    }
    if (!this._listeners[type]) {
      this._listeners[type] = [];
    }
    if (this._listeners[type].indexOf(handler) != -1) {
      
      return;
    }
    this._listeners[type].push(handler);
  },

  removeEventListener: function removeEventListener(type, handler) {
     let list, index;
     if (this._listeners &&
         (list = this._listeners[type]) &&
         (index = list.indexOf(handler) != -1)) {
       list.splice(index, 1);
       return;
     }
  },

  dispatchEvent: function dispatchEvent(event) {
    
    
    
    if (!this._listeners) {
      return;
    }
    let handlerList = this._listeners[event.type];
    if (!handlerList) {
      return;
    }
    event.target = this;

    
    
    
    
    
    
    let handlers = handlerList.slice();
    handlers.forEach(function (handler) {
      if (handlerList.indexOf(handler) == -1) {
        return;
      }
      switch (typeof handler) {
        case "function":
          handler(event);
          break;
        case "object":
          handler.handleEvent(event);
          break;
      }
    });
  }
};






function TelephoneCallback(telephony) {
  this.telephony = telephony;
}
TelephoneCallback.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephoneCallback]),

  

  onsignalstrengthchange: function onsignalstrengthchange(event) {
    this.telephony.signalStrength = event.signalStrength;
    this.telephony._dispatchEventByType("signalstrengthchange");
  },

  onoperatorchange: function onoperatorchange(event) {
    this.telephony.operator = event.operator;
    this.telephony._dispatchEventByType("operatorchange");
  },

  onradiostatechange: function onradiostatechange(event) {
    this.telephony.radioState = event.radioState;
    this.telephony._dispatchEventByType("radiostatechange");
  },

  oncardstatechange: function oncardstatechange(event) {
    this.telephony.cardState = event.cardState;
    this.telephony._dispatchEventByType("cardstatechange");
  },

  oncallstatechange: function oncallstatechange(event) {
    this.telephony._processCallState(event);
  },

};




function Telephony() {}
Telephony.prototype = {

  __proto__: EventTarget.prototype,

  classID: TELEPHONY_CID,
  classInfo: XPCOMUtils.generateCI({classID: TELEPHONY_CID,
                                    contractID: TELEPHONY_CONTRACTID,
                                    interfaces: [Ci.mozIDOMTelephony,
                                                 Ci.nsIDOMEventTarget],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Telephony"}),
  QueryInterface: XPCOMUtils.generateQI([Ci.mozIDOMTelephony,
                                         Ci.nsIDOMEventTarget,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  

  init: function init(window) {
    this.window = window;
    this.telephone = Cc["@mozilla.org/telephony/radio-interface;1"]
                       .createInstance(Ci.nsITelephone);
    this.telephoneCallback = new TelephoneCallback(this);
    
    window.addEventListener("unload", function onunload(event) {
      this.telephone.unregisterCallback(this.telephoneCallback);
      this.telephoneCallback = null;
      this.window = null;
    }.bind(this));
    this.telephone.registerCallback(this.telephoneCallback);
    this.callsByIndex = {};
    this.liveCalls = [];

    
    let currentState = this.telephone.currentState;
    let states = currentState.currentCalls;
    for (let i = 0; i < states.length; i++) {
      let state = states[i];
      let call = new TelephonyCall(this.telephone, state.callIndex);
      call.readyState = state.callState;
      call.number = state.number;
      this.liveCalls.push(call);
      this.callsByIndex[state.callIndex] = call;
    }

    this.operator        = currentState.operator;
    this.radioState      = currentState.radioState;
    this.cardState       = currentState.cardState;
    this.signalStrength  = currentState.signalStrength;
  },

  _dispatchEventByType: function _dispatchEventByType(type) {
    let event = this.window.document.createEvent("Event");
    event.initEvent(type, false, false);
    
    this.dispatchEvent(event);
  },

  _dispatchCallEvent: function _dispatchCallEvent(call, type, target) {
    let event = this.window.document.createEvent("Event");
    event.initEvent(type, false, false);
    event.call = call; 
    
    target = target || call;
    target.dispatchEvent(event);    
  },

  _processCallState: function _processCallState(state) {
    
    
    if (state.callState == DOM_CALL_READYSTATE_DIALING) {
      let call = this.callsByIndex[CALLINDEX_TEMPORARY_DIALING];
      if (call) {
        call.callIndex = state.callIndex;
        delete this.callsByIndex[CALLINDEX_TEMPORARY_DIALING];
        this.callsByIndex[call.callIndex] = call;
        
        
        return;
      }
    }

    
    
    let call = this.callsByIndex[state.callIndex];
    if (call) {
      if (call.readyState == state.callState) {
        
        return;
      }
      if (state.readyState == DOM_CALL_READYSTATE_DISCONNECTED) {
        let index = this.liveCalls.indexOf(call);
        if (index != -1) {
          this.liveCalls.splice(index, 1);
        }
        delete this.callsByIndex[call.callIndex];
      }
      call.readyState = state.callState;
      this._dispatchCallEvent(call, "readystatechange");
      this._dispatchCallEvent(call, state.callState);
      return;
    }

    
    
    if (state.readyState == DOM_CALL_READYSTATE_DISCONNECTED) {
      return;
    }
    call = new TelephonyCall(this.telephone, state.callIndex);
    call.number = state.number;
    call.readyState = state.callState;
    this.callsByIndex[state.callIndex] = call;
    this.liveCalls.push(call);

    let target;
    if (call.readyState == DOM_CALL_READYSTATE_INCOMING) {
      target = this;
    } else {
      target = call;
      this._dispatchCallEvent(call, "readystatechange");
    }
    this._dispatchCallEvent(call, state.callState, target);
  },

  callsByIndex: null,

  

  liveCalls: null,

  dial: function dial(number) {
    this.telephone.dial(number);

    
    
    
    
    let callIndex = CALLINDEX_TEMPORARY_DIALING;
    let call = new TelephonyCall(this.telephone, callIndex);
    call.readyState = DOM_CALL_READYSTATE_DIALING;
    call.number = number;
    this.callsByIndex[callIndex] = call;
    this.liveCalls.push(call);
    return call;
  },

  get muted() {
    return this.telephone.microphoneMuted;
  },
  set muted(value) {
    this.telephone.microphoneMuted = value;
  },

  get speakerOn() {
    return this.telephone.speakerEnabled;
  },
  set speakerOn(value) {
    this.telephone.speakerEnabled = value;
  },

  

  signalStrength: null,
  operator: null,
  radioState: DOM_RADIOSTATE_UNAVAILABLE,
  cardState: DOM_CARDSTATE_UNAVAILABLE,

};
defineEventListenerSlot(Telephony.prototype, DOM_CALL_READYSTATE_INCOMING);

defineEventListenerSlot(Telephony.prototype, "radiostatechange");
defineEventListenerSlot(Telephony.prototype, "cardstatechange");
defineEventListenerSlot(Telephony.prototype, "signalstrengthchange");
defineEventListenerSlot(Telephony.prototype, "operatorchange");


function TelephonyCall(telephone, callIndex) {
  this.telephone = telephone;
  this.callIndex = callIndex;
}
TelephonyCall.prototype = {

  __proto__: EventTarget.prototype,

  classID: TELEPHONY_CALL_CID,
  classInfo: XPCOMUtils.generateCI({classID: TELEPHONY_CALL_CID,
                                    contractID: TELEPHONY_CALL_CONTRACTID,
                                    interfaces: [Ci.mozIDOMTelephonyCall,
                                                 Ci.nsIDOMEventTarget],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "TelephonyCall"}),
  QueryInterface: XPCOMUtils.generateQI([Ci.mozIDOMTelephonyCall,
                                         Ci.nsIDOMEventTarget]),


  callIndex: null,

  

  number: null,
  readyState: null,

  answer: function answer() {
    if (this.readyState != DOM_CALL_READYSTATE_INCOMING) {
      throw "Can only answer an incoming call!";
    }
    this.telephone.answerCall();
  },

  disconnect: function disconnect() {
    if (this.readyState == DOM_CALL_READYSTATE_INCOMING) {
      this.telephone.rejectCall();
    } else {
      this.telephone.hangUp(this.callIndex);
    }
  },

};
defineEventListenerSlot(TelephonyCall.prototype, "readystatechange");
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_RINGING);
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_BUSY);
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_CONNECTING);
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_CONNECTED);
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_DISCONNECTING);
defineEventListenerSlot(TelephonyCall.prototype, DOM_CALL_READYSTATE_DISCONNECTED);


const NSGetFactory = XPCOMUtils.generateNSGetFactory([Telephony]);
