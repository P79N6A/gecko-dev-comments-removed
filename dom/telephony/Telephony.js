




































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const TELEPHONY_CID = Components.ID("{37e248d2-02ff-469b-bb31-eef5a4a4bee3}");
const TELEPHONY_CONTRACTID = "@mozilla.org/telephony;1";

const TELEPHONY_CALL_CID = Components.ID("{6b9b3daf-e5ea-460b-89a5-641ee20dd577}");
const TELEPHONY_CALL_CONTRACTID = "@mozilla.org/telephony-call;1";










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
    
    
    
    let handlerList = this._listeners[event.type];
    if (!handlerList) {
      return;
    }
    event.target = this;

    
    
    
    
    
    
    let handlers = handlerList.slice();
    handlers.forEach(function (handler) {
      if (handerList.indexOf(handler) == -1) {
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

const DOM_RADIOSTATE_UNAVAILABLE = "unavailable";
const DOM_RADIOSTATE_OFF         = "off";
const DOM_RADIOSTATE_READY       = "ready";
   
const DOM_CARDSTATE_UNAVAILABLE    = "unavailable";
const DOM_CARDSTATE_ABSENT         = "absent";
const DOM_CARDSTATE_PIN_REQUIRED   = "pin_required";
const DOM_CARDSTATE_PUK_REQUIRED   = "puk_required";
const DOM_CARDSTATE_NETWORK_LOCKED = "network_locked";
const DOM_CARDSTATE_NOT_READY      = "not_ready";
const DOM_CARDSTATE_READY          = "ready";






function TelephonyRadioCallback(telephony) {
  this.telephony = telephony;
}
TelephonyRadioCallback.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRadioCallback]),

  

  onsignalstrengthchange: function onsignalstrengthchange(signalStrength) {
    this.telephony.signalStrength = signalStrength;
    this.telephony._dispatchEventByType("signalstrengthchange");
  },

  onoperatorchange: function onoperatorchange(operator) {
    this.telephony.operator = operator;
    this.telephony._dispatchEventByType("operatorchange");
  },

  onradiostatechange: function onradiostatechange(radioState) {
    this.telephony.radioState = radioState;
    this.telephony._dispatchEventByType("radiostatechange");
  },

  oncardstatechange: function oncardstatechange(cardState) {
    this.telephony.cardState = cardState;
    this.telephony._dispatchEventByType("cardstatechange");
  },

  oncallstatechange: function oncallstatechange(callState) {
    this.telephony._processCallState(callState);
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
    this.radioInterface = Cc["@mozilla.org/telephony/radio-interface;1"]
                            .createInstance(Ci.nsIRadioInterface);
    this.radioCallback = new TelephonyRadioCallback(this);
    window.addEventListener("unload", function onunload(event) {
      this.radioInterface.unregisterCallback(this.radioCallback);
      this.radioCallback = null;
      this.window = null;
    }.bind(this));
    this.radioInterface.registerCallback(this.radioCallback);
    this.liveCalls = [];

    let initialState = this.radioInterface.initialState;
    this.operator        = initialState.operator;
    this.radioState      = initialState.radioState;
    this.cardState       = initialState.cardState;
    this.signalStrength  = initialState.signalStrength;
    this._processCallState(initialState.callState);
  },

  _dispatchEventByType: function _dispatchEventByType(type) {
    let event = this.window.document.createEvent("Event");
    event.initEvent(type, false, false);
    
    this.dispatchEvent(event);
  },

  _processCallState: function _processCallState(callState) {
    
  },

  

  liveCalls: null,

  dial: function dial(number) {
    this.radioInterface.dial(number);
    return new TelephonyCall(number, DOM_CALL_READYSTATE_DIALING);
  },

  

  signalStrength: null,
  operator: null,
  radioState: DOM_RADIOSTATE_UNAVAILABLE,
  cardState: DOM_CARDSTATE_UNAVAILABLE,

};
defineEventListenerSlot(Telephony.prototype, "radiostatechange");
defineEventListenerSlot(Telephony.prototype, "cardstatechange");
defineEventListenerSlot(Telephony.prototype, "signalstrengthchange");
defineEventListenerSlot(Telephony.prototype, "operatorchange");
defineEventListenerSlot(Telephony.prototype, "incoming");


const DOM_CALL_READYSTATE_DIALING   = "dialing";
const DOM_CALL_READYSTATE_DOM_CALLING   = "calling";
const DOM_CALL_READYSTATE_INCOMING  = "incoming";
const DOM_CALL_READYSTATE_CONNECTED = "connected";
const DOM_CALL_READYSTATE_CLOSED    = "closed";
const DOM_CALL_READYSTATE_BUSY      = "busy";

function TelephonyCall(number, initialState) {
  this.number = number;
  this.readyState = initialState;
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

  number: null,
  readyState: null,

  answer: function answer() {
    
  },

  disconnect: function disconnect() {
    
  },

};
defineEventListenerSlot(TelephonyCall.prototype, "connect");
defineEventListenerSlot(TelephonyCall.prototype, "disconnect");
defineEventListenerSlot(TelephonyCall.prototype, "busy");


const NSGetFactory = XPCOMUtils.generateNSGetFactory([Telephony]);
