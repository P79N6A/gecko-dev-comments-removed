



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PC_CONTRACT = "@mozilla.org/dom/peerconnection;1";
const PC_ICE_CONTRACT = "@mozilla.org/dom/rtcicecandidate;1";
const PC_SESSION_CONTRACT = "@mozilla.org/dom/rtcsessiondescription;1";
const PC_MANAGER_CONTRACT = "@mozilla.org/dom/peerconnectionmanager;1";
const PC_ICEEVENT_CONTRACT = "@mozilla.org/dom/rtcpeerconnectioniceevent;1";
const MSEVENT_CONTRACT = "@mozilla.org/dom/mediastreamevent;1";
const DCEVENT_CONTRACT = "@mozilla.org/dom/datachannelevent;1";

const PC_CID = Components.ID("{9878b414-afaa-4176-a887-1e02b3b047c2}");
const PC_ICE_CID = Components.ID("{02b9970c-433d-4cc2-923d-f7028ac66073}");
const PC_SESSION_CID = Components.ID("{1775081b-b62d-4954-8ffe-a067bbf508a7}");
const PC_MANAGER_CID = Components.ID("{7293e901-2be3-4c02-b4bd-cbef6fc24f78}");
const PC_ICEEVENT_CID = Components.ID("{b9cd25a7-9859-4f9e-8f84-ef5181ff36c0}");
const MSEVENT_CID = Components.ID("{a722a8a9-2290-4e99-a5ed-07b504292d08}");
const DCEVENT_CID = Components.ID("{d5ed7fbf-01a8-4b18-af6c-861cf2aac920}");



function GlobalPCList() {
  this._list = [];
  this._networkdown = false; 
  Services.obs.addObserver(this, "inner-window-destroyed", true);
  Services.obs.addObserver(this, "profile-change-net-teardown", true);
  Services.obs.addObserver(this, "network:offline-about-to-go-offline", true);
  Services.obs.addObserver(this, "network:offline-status-changed", true);
}
GlobalPCList.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.IPeerConnectionManager]),
  classID: PC_MANAGER_CID,
  _xpcom_factory: {
    createInstance: function(outer, iid) {
      if (outer) {
        throw Cr.NS_ERROR_NO_AGGREGATION;
      }
      return _globalPCList.QueryInterface(iid);
    }
  },

  addPC: function(pc) {
    let winID = pc._winID;
    if (this._list[winID]) {
      this._list[winID].push(Cu.getWeakReference(pc));
    } else {
      this._list[winID] = [Cu.getWeakReference(pc)];
    }
    this.removeNullRefs(winID);
  },

  removeNullRefs: function(winID) {
    if (this._list === undefined || this._list[winID] === undefined) {
      return;
    }
    this._list[winID] = this._list[winID].filter(
      function (e,i,a) { return e.get() !== null; });
  },

  hasActivePeerConnection: function(winID) {
    this.removeNullRefs(winID);
    return this._list[winID] ? true : false;
  },

  observe: function(subject, topic, data) {
    if (topic == "inner-window-destroyed") {
      let winID = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (this._list[winID]) {
        this._list[winID].forEach(function(pcref) {
          let pc = pcref.get();
          if (pc !== null) {
            pc._pc.close();
            delete pc._observer;
            pc._pc = null;
          }
        });
        delete this._list[winID];
      }
    } else if (topic == "profile-change-net-teardown" ||
               topic == "network:offline-about-to-go-offline") {
      
      
      
      
      
      
      let array;
      while ((array = this._list.pop()) != undefined) {
        array.forEach(function(pcref) {
          let pc = pcref.get();
          if (pc !== null) {
            pc._pc.close();
            delete pc._observer;
            pc._pc = null;
          }
        });
      };
      this._networkdown = true;
    }
    else if (topic == "network:offline-status-changed") {
      if (data == "offline") {
	
        this._networkdown = true;
      } else if (data == "online") {
        this._networkdown = false;
      }
    }
  },
};
let _globalPCList = new GlobalPCList();

function RTCIceCandidate() {
  this.candidate = this.sdpMid = this.sdpMLineIndex = null;
}
RTCIceCandidate.prototype = {
  classDescription: "mozRTCIceCandidate",
  classID: PC_ICE_CID,
  contractID: PC_ICE_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(dict) {
    this.candidate = dict.candidate;
    this.sdpMid = dict.sdpMid;
    this.sdpMLineIndex = ("sdpMLineIndex" in dict)? dict.sdpMLineIndex+1 : null;
  }
};

function RTCSessionDescription() {
  this.type = this.sdp = null;
}
RTCSessionDescription.prototype = {
  classDescription: "mozRTCSessionDescription",
  classID: PC_SESSION_CID,
  contractID: PC_SESSION_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(dict) {
    this.type = dict.type;
    this.sdp  = dict.sdp;
  },

  
  toJSON: function() {
    return { type: this.type, sdp: this.sdp,
             __exposedProps__: { type: "rw", sdp: "rw" } };
  }
};

function MediaStreamEvent() {
  this.type = this._stream = null;
}
MediaStreamEvent.prototype = {
  classDescription: "MediaStreamEvent",
  classID: MSEVENT_CID,
  contractID: MSEVENT_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(type, dict) {
    this.type = type;
    this.__DOM_IMPL__.initEvent(type, dict.bubbles || false,
                                dict.cancelable || false);
    this._stream = dict.stream;
  },

  get stream() { return this._stream; }
};

function RTCDataChannelEvent() {
  this.type = this._channel = null;
}
RTCDataChannelEvent.prototype = {
  classDescription: "RTCDataChannelEvent",
  classID: DCEVENT_CID,
  contractID: DCEVENT_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(type, dict) {
    this.type = type;
    this.__DOM_IMPL__.initEvent(type, dict.bubbles || false,
                                dict.cancelable || false);
    this._channel = dict.channel;
  },

  get channel() { return this._channel; }
};

function RTCPeerConnectionIceEvent() {
  this.type = this._candidate = null;
}
RTCPeerConnectionIceEvent.prototype = {
  classDescription: "RTCPeerConnectionIceEvent",
  classID: PC_ICEEVENT_CID,
  contractID: PC_ICEEVENT_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(type, dict) {
    this.type = type;
    this.__DOM_IMPL__.initEvent(type, dict.bubbles || false,
                                dict.cancelable || false);
    this._candidate = dict.candidate;
  },

  get candidate() { return this._candidate; }
};

function RTCPeerConnection() {
  this._queue = [];

  this._pc = null;
  this._observer = null;
  this._closed = false;

  this._onCreateOfferSuccess = null;
  this._onCreateOfferFailure = null;
  this._onCreateAnswerSuccess = null;
  this._onCreateAnswerFailure = null;

  this._pendingType = null;
  this._localType = null;
  this._remoteType = null;

  







  this._pending = false;

  
  this._iceGatheringState = this._iceConnectionState = "new";

  
  this._ongatheringchange = null;
  this._onicechange = null;
}
RTCPeerConnection.prototype = {
  classDescription: "mozRTCPeerConnection",
  classID: PC_CID,
  contractID: PC_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsISupportsWeakReference]),
  init: function(win) { this._win = win; },

  __init: function(rtcConfig) {
    if (!rtcConfig.iceServers ||
        !Services.prefs.getBoolPref("media.peerconnection.use_document_iceservers")) {
      rtcConfig = {iceServers:
        JSON.parse(Services.prefs.getCharPref("media.peerconnection.default_iceservers"))};
    }
    this._mustValidateRTCConfiguration(rtcConfig,
        "RTCPeerConnection constructor passed invalid RTCConfiguration");
    if (_globalPCList._networkdown) {
      throw new Components.Exception("Can't create RTCPeerConnections when the network is down");
    }

    this.makeGetterSetterEH("onaddstream");
    this.makeGetterSetterEH("onicecandidate");
    this.makeGetterSetterEH("onnegotiationneeded");
    this.makeGetterSetterEH("onsignalingstatechange");
    this.makeGetterSetterEH("onremovestream");
    this.makeGetterSetterEH("ondatachannel");
    this.makeGetterSetterEH("onconnection");
    this.makeGetterSetterEH("onclosedconnection");
    this.makeGetterSetterEH("oniceconnectionstatechange");

    this._pc = Cc["@mozilla.org/peerconnection;1"].
             createInstance(Ci.IPeerConnection);
    this._observer = new PeerConnectionObserver(this);

    
    this._queueOrRun({
      func: this._getPC().initialize,
      args: [this._observer, this._win, rtcConfig, Services.tm.currentThread],
      wait: true
    });

    this._winID = this._win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

    
    _globalPCList.addPC(this);
  },

  _getPC: function() {
    if (!this._pc) {
      throw new Components.Exception("RTCPeerConnection is gone (did you enter Offline mode?)");
    }
    return this._pc;
  },

  





  _queueOrRun: function(obj) {
    this._checkClosed();
    if (!this._pending) {
      if (obj.type !== undefined) {
        this._pendingType = obj.type;
      }
      obj.func.apply(this, obj.args);
      if (obj.wait) {
        this._pending = true;
      }
    } else {
      this._queue.push(obj);
    }
  },

  
  _executeNext: function() {
    if (this._queue.length) {
      let obj = this._queue.shift();
      if (obj.type !== undefined) {
        this._pendingType = obj.type;
      }
      obj.func.apply(this, obj.args);
      if (!obj.wait) {
        this._executeNext();
      }
    } else {
      this._pending = false;
    }
  },

  










  _mustValidateRTCConfiguration: function(rtcConfig, errorMsg) {
    function nicerNewURI(uriStr, errorMsg) {
      let ios = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);
      try {
        return ios.newURI(uriStr, null, null);
      } catch (e if (e.result == Cr.NS_ERROR_MALFORMED_URI)) {
        throw new Components.Exception(errorMsg + " - malformed URI: " + uriStr,
                                       Cr.NS_ERROR_MALFORMED_URI);
      }
    }
    function mustValidateServer(server) {
      let url = nicerNewURI(server.url, errorMsg);
      if (!(url.scheme in { stun:1, stuns:1, turn:1, turns:1 })) {
        throw new Components.Exception(errorMsg + " - improper scheme: " + url.scheme,
                                       Cr.NS_ERROR_MALFORMED_URI);
      }
    }
    if (rtcConfig.iceServers) {
      let len = rtcConfig.iceServers.length;
      for (let i=0; i < len; i++) {
        mustValidateServer (rtcConfig.iceServers[i], errorMsg);
      }
    }
  },

  











  _mustValidateConstraints: function(constraints, errorMsg) {
    function isObject(obj) {
      return obj && (typeof obj === "object");
    }
    function isArraylike(obj) {
      return isObject(obj) && ("length" in obj);
    }
    const SUPPORTED_CONSTRAINTS = {
      OfferToReceiveAudio:1,
      OfferToReceiveVideo:1,
      MozDontOfferDataChannel:1
    };
    const OTHER_KNOWN_CONSTRAINTS = {
      VoiceActivityDetection:1,
      IceTransports:1,
      RequestIdentity:1
    };
    
    
    if (!isObject(constraints) || Array.isArray(constraints)) {
      throw new Components.Exception(errorMsg);
    }
    if (constraints.mandatory) {
      
      
      if (!isObject(constraints.mandatory) || Array.isArray(constraints.mandatory)) {
        throw new Components.Exception(errorMsg + " - malformed mandatory constraints");
      }
      for (let constraint in constraints.mandatory) {
        if (!(constraint in SUPPORTED_CONSTRAINTS) &&
            constraints.mandatory.hasOwnProperty(constraint)) {
          throw new Components.Exception(errorMsg + " - " +
                                         ((constraint in OTHER_KNOWN_CONSTRAINTS)?
                                          "unsupported" : "unknown") +
                                         " mandatory constraint: " + constraint);
        }
      }
    }
    if (constraints.optional) {
      if (!isArraylike(constraints.optional)) {
        throw new Components.Exception(errorMsg +
                                       " - malformed optional constraint array");
      }
      let len = constraints.optional.length;
      for (let i = 0; i < len; i += 1) {
        if (!isObject(constraints.optional[i])) {
          throw new Components.Exception(errorMsg +
                                         " - malformed optional constraint: " +
                                         constraints.optional[i]);
        }
        let constraints_per_entry = 0;
        for (let constraint in constraints.optional[i]) {
          if (constraints.optional[i].hasOwnProperty(constraint)) {
            if (constraints_per_entry) {
              throw new Components.Exception(errorMsg +
                  " - optional constraint must be single key/value pair");
            }
            constraints_per_entry += 1;
          }
        }
      }
    }
  },

  
  
  
  
  _checkClosed: function() {
    if (this._closed) {
      throw new Components.Exception("Peer connection is closed");
    }
  },

  dispatchEvent: function(event) {
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  
  reportError: function(msg, file, line) {
    this.reportMsg(msg, file, line, Ci.nsIScriptError.exceptionFlag);
  },

  reportWarning: function(msg, file, line) {
    this.reportMsg(msg, file, line, Ci.nsIScriptError.warningFlag);
  },

  reportMsg: function(msg, file, line, flag) {
    let scriptErrorClass = Cc["@mozilla.org/scripterror;1"];
    let scriptError = scriptErrorClass.createInstance(Ci.nsIScriptError);
    scriptError.initWithWindowID(msg, file, null, line, 0, flag,
                                 "content javascript", this._winID);
    let console = Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService);
    console.logMessage(scriptError);

    if (flag != Ci.nsIScriptError.warningFlag) {
      
      try {
        if (typeof this._win.onerror === "function") {
          this._win.onerror(msg, file, line);
        }
      } catch(e) {
        
        try {
          let scriptError = scriptErrorClass.createInstance(Ci.nsIScriptError);
          scriptError.initWithWindowID(e.message, e.fileName, null, e.lineNumber,
                                       0, Ci.nsIScriptError.exceptionFlag,
                                       "content javascript",
                                       this._winID);
          console.logMessage(scriptError);
        } catch(e) {}
      }
    }
  },

  getEH: function(type) {
    return this.__DOM_IMPL__.getEventHandler(type);
  },

  setEH: function(type, handler) {
    this.__DOM_IMPL__.setEventHandler(type, handler);
  },

  makeGetterSetterEH: function(name) {
    Object.defineProperty(this, name,
                          {
                            get:function()  { return this.getEH(name); },
                            set:function(h) { return this.setEH(name, h); }
                          });
  },

  get onicechange()       { return this._onicechange; },
  get ongatheringchange() { return this._ongatheringchange; },

  set onicechange(cb) {
    this.deprecated("onicechange");
    this._onicechange = cb;
  },
  set ongatheringchange(cb) {
    this.deprecated("ongatheringchange");
    this._ongatheringchange = cb;
  },

  deprecated: function(name) {
    this.reportWarning(name + " is deprecated!", null, 0);
  },

  createOffer: function(onSuccess, onError, constraints) {
    if (!constraints) {
      constraints = {};
    }
    if (!onError) {
      this.deprecated("calling createOffer without failureCallback");
    }
    this._mustValidateConstraints(constraints, "createOffer passed invalid constraints");
    this._onCreateOfferSuccess = onSuccess;
    this._onCreateOfferFailure = onError;

    this._queueOrRun({
      func: this._getPC().createOffer,
      args: [constraints],
      wait: true
    });
  },

  _createAnswer: function(onSuccess, onError, constraints, provisional) {
    if (!onError) {
      this.deprecated("calling createAnswer without failureCallback");
    }
    this._onCreateAnswerSuccess = onSuccess;
    this._onCreateAnswerFailure = onError;

    if (!this.remoteDescription) {

      this._observer.onCreateAnswerError(Ci.IPeerConnection.kInvalidState,
                                         "setRemoteDescription not called");
      return;
    }

    if (this.remoteDescription.type != "offer") {

      this._observer.onCreateAnswerError(Ci.IPeerConnection.kInvalidState,
                                         "No outstanding offer");
      return;
    }

    

    this._getPC().createAnswer(constraints);
  },

  createAnswer: function(onSuccess, onError, constraints, provisional) {
    if (!constraints) {
      constraints = {};
    }

    this._mustValidateConstraints(constraints, "createAnswer passed invalid constraints");

    if (!provisional) {
      provisional = false;
    }

    this._queueOrRun({
      func: this._createAnswer,
      args: [onSuccess, onError, constraints, provisional],
      wait: true
    });
  },

  setLocalDescription: function(desc, onSuccess, onError) {
    
    
    
    this._onSetLocalDescriptionSuccess = onSuccess;
    this._onSetLocalDescriptionFailure = onError;

    let type;
    switch (desc.type) {
      case "offer":
        type = Ci.IPeerConnection.kActionOffer;
        break;
      case "answer":
        type = Ci.IPeerConnection.kActionAnswer;
        break;
      case "pranswer":
        throw new Components.Exception("pranswer not yet implemented",
                                       Cr.NS_ERROR_NOT_IMPLEMENTED);
      default:
        throw new Components.Exception("Invalid type " + desc.type +
                                       " provided to setLocalDescription");
    }

    this._queueOrRun({
      func: this._getPC().setLocalDescription,
      args: [type, desc.sdp],
      wait: true,
      type: desc.type
    });
  },

  setRemoteDescription: function(desc, onSuccess, onError) {
    
    
    
    this._onSetRemoteDescriptionSuccess = onSuccess;
    this._onSetRemoteDescriptionFailure = onError;

    let type;
    switch (desc.type) {
      case "offer":
        type = Ci.IPeerConnection.kActionOffer;
        break;
      case "answer":
        type = Ci.IPeerConnection.kActionAnswer;
        break;
      case "pranswer":
        throw new Components.Exception("pranswer not yet implemented",
                                       Cr.NS_ERROR_NOT_IMPLEMENTED);
      default:
        throw new Components.Exception("Invalid type " + desc.type +
                                       " provided to setRemoteDescription");
    }

    this._queueOrRun({
      func: this._getPC().setRemoteDescription,
      args: [type, desc.sdp],
      wait: true,
      type: desc.type
    });
  },

  updateIce: function(config, constraints) {
    throw new Components.Exception("updateIce not yet implemented",
                                   Cr.NS_ERROR_NOT_IMPLEMENTED);
  },

  addIceCandidate: function(cand, onSuccess, onError) {
    if (!cand.candidate && !cand.sdpMLineIndex) {
      throw new Components.Exception("Invalid candidate passed to addIceCandidate!");
    }
    this._onAddIceCandidateSuccess = onSuccess || null;
    this._onAddIceCandidateError = onError || null;

    this._queueOrRun({
      func: this._getPC().addIceCandidate,
      args: [cand.candidate, cand.sdpMid || "", cand.sdpMLineIndex],
      wait: true
    });
  },

  addStream: function(stream, constraints) {
    if (stream.currentTime === undefined) {
      throw new Components.Exception("Invalid stream passed to addStream!");
    }
    
    this._queueOrRun({
      func: this._getPC().addStream,
      args: [stream],
      wait: false
    });
  },

  removeStream: function(stream) {
     
     throw new Components.Exception("removeStream not yet implemented",
                                    Cr.NS_ERROR_NOT_IMPLEMENTED);
  },

  getStreamById: function(id) {
    throw new Components.Exception("getStreamById not yet implemented",
                                   Cr.NS_ERROR_NOT_IMPLEMENTED);
  },

  close: function() {
    this._queueOrRun({
      func: this._getPC().close,
      args: [false],
      wait: false
    });
    this._closed = true;
    this.changeIceConnectionState("closed");
  },

  getLocalStreams: function() {
    this._checkClosed();
    return this._getPC().localStreams;
  },

  getRemoteStreams: function() {
    this._checkClosed();
    return this._getPC().remoteStreams;
  },

  
  get localStreams() {
    this.deprecated("localStreams");
    return this.getLocalStreams();
  },
  get remoteStreams() {
    this.deprecated("remoteStreams");
    return this.getRemoteStreams();
  },

  get localDescription() {
    this._checkClosed();
    let sdp = this._getPC().localDescription;
    if (sdp.length == 0) {
      return null;
    }
    return new this._win.mozRTCSessionDescription({ type: this._localType,
                                                    sdp: sdp });
  },

  get remoteDescription() {
    this._checkClosed();
    let sdp = this._getPC().remoteDescription;
    if (sdp.length == 0) {
      return null;
    }
    return new this._win.mozRTCSessionDescription({ type: this._remoteType,
                                                    sdp: sdp });
  },

  get signalingState()     { return "stable"; }, 
  get iceGatheringState()  { return this._iceGatheringState; },
  get iceConnectionState() { return this._iceConnectionState; },

  changeIceGatheringState: function(state) {
    this._iceGatheringState = state;
  },

  changeIceConnectionState: function(state) {
    this._iceConnectionState = state;
    this.dispatchEvent(new this._win.Event("iceconnectionstatechange"));
  },

  get readyState() {
    this.deprecated("readyState");
    
    
    if(this._closed) {
      return "closed";
    }

    var state="undefined";
    switch (this._getPC().readyState) {
      case Ci.IPeerConnection.kNew:
        state = "new";
        break;
      case Ci.IPeerConnection.kNegotiating:
        state = "negotiating";
        break;
      case Ci.IPeerConnection.kActive:
        state = "active";
        break;
      case Ci.IPeerConnection.kClosing:
        state = "closing";
        break;
      case Ci.IPeerConnection.kClosed:
        state = "closed";
        break;
    }
    return state;
  },

  createDataChannel: function(label, dict) {
    this._checkClosed();
    if (dict == undefined) {
      dict = {};
    }
    if (dict.maxRetransmitTime != undefined &&
        dict.maxRetransmitNum != undefined) {
      throw new Components.Exception("Both maxRetransmitTime and maxRetransmitNum cannot be provided");
    }
    let protocol;
    if (dict.protocol == undefined) {
      protocol = "";
    } else {
      protocol = dict.protocol;
    }

    
    let type;
    if (dict.maxRetransmitTime != undefined) {
      type = Ci.IPeerConnection.kDataChannelPartialReliableTimed;
    } else if (dict.maxRetransmitNum != undefined) {
      type = Ci.IPeerConnection.kDataChannelPartialReliableRexmit;
    } else {
      type = Ci.IPeerConnection.kDataChannelReliable;
    }

    
    let channel = this._getPC().createDataChannel(
      label, protocol, type, dict.outOfOrderAllowed, dict.maxRetransmitTime,
      dict.maxRetransmitNum, dict.preset ? true : false,
      dict.stream != undefined ? dict.stream : 0xFFFF
    );
    return channel;
  },

  connectDataConnection: function(localport, remoteport, numstreams) {
    if (numstreams == undefined || numstreams <= 0) {
      numstreams = 16;
    }
    this._queueOrRun({
      func: this._getPC().connectDataConnection,
      args: [localport, remoteport, numstreams],
      wait: false
    });
  }
};

function RTCError(code, message) {
  this.name = this.reasonName[Math.min(code, this.reasonName.length - 1)];
  this.message = (typeof message === "string")? message : this.name;
  this.__exposedProps__ = { name: "rw", message: "rw" };
}
RTCError.prototype = {
  
  reasonName: [
    "NO_ERROR", 
    "INVALID_CONSTRAINTS_TYPE",
    "INVALID_CANDIDATE_TYPE",
    "INVALID_MEDIASTREAM_TRACK",
    "INVALID_STATE",
    "INVALID_SESSION_DESCRIPTION",
    "INCOMPATIBLE_SESSION_DESCRIPTION",
    "INCOMPATIBLE_CONSTRAINTS",
    "INCOMPATIBLE_MEDIASTREAMTRACK",
    "INTERNAL_ERROR"
  ]
};


function PeerConnectionObserver(dompc) {
  this._dompc = dompc;
}
PeerConnectionObserver.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.IPeerConnectionObserver,
                                         Ci.nsISupportsWeakReference]),

  dispatchEvent: function(event) {
    this._dompc.dispatchEvent(event);
  },

  callCB: function(callback, arg) {
    if (callback) {
      try {
        callback(arg);
      } catch(e) {
        
        
        
        this._dompc.reportError(e.message, e.fileName, e.lineNumber);
      }
    }
  },

  onCreateOfferSuccess: function(sdp) {
    this.callCB(this._dompc._onCreateOfferSuccess,
                new this._dompc._win.mozRTCSessionDescription({ type: "offer",
                                                                sdp: sdp }));
    this._dompc._executeNext();
  },

  onCreateOfferError: function(code, message) {
    this.callCB(this._dompc._onCreateOfferFailure, new RTCError(code, message));
    this._dompc._executeNext();
  },

  onCreateAnswerSuccess: function(sdp) {
    this.callCB (this._dompc._onCreateAnswerSuccess,
                 new this._dompc._win.mozRTCSessionDescription({ type: "answer",
                                                                 sdp: sdp }));
    this._dompc._executeNext();
  },

  onCreateAnswerError: function(code, message) {
    this.callCB(this._dompc._onCreateAnswerFailure, new RTCError(code, message));
    this._dompc._executeNext();
  },

  onSetLocalDescriptionSuccess: function() {
    this._dompc._localType = this._dompc._pendingType;
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onSetLocalDescriptionSuccess);

    
    
    
    
    
    
    this.foundIceCandidate(null);

    this._dompc._executeNext();
  },

  onSetRemoteDescriptionSuccess: function() {
    this._dompc._remoteType = this._dompc._pendingType;
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onSetRemoteDescriptionSuccess);
    this._dompc._executeNext();
  },

  onSetLocalDescriptionError: function(code, message) {
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onSetLocalDescriptionFailure,
                new RTCError(code, message));
    this._dompc._executeNext();
  },

  onSetRemoteDescriptionError: function(code, message) {
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onSetRemoteDescriptionFailure,
                new RTCError(code, message));
    this._dompc._executeNext();
  },

  onAddIceCandidateSuccess: function() {
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onAddIceCandidateSuccess);
    this._dompc._executeNext();
  },

  onAddIceCandidateError: function(code, message) {
    this._dompc._pendingType = null;
    this.callCB(this._dompc._onAddIceCandidateError, new RTCError(code, message));
    this._dompc._executeNext();
  },

  onStateChange: function(state) {
    if (state != Ci.IPeerConnectionObserver.kIceState) {
      return;
    }

    switch (this._dompc._pc.iceState) {
      case Ci.IPeerConnection.kIceWaiting:
        this._dompc.changeIceConnectionState("completed");
        this.callCB(this._dompc.ongatheringchange, "complete");
        this.callCB(this._onicechange, "starting");
        
        this._dompc._executeNext();
        break;
      case Ci.IPeerConnection.kIceChecking:
        this._dompc.changeIceConnectionState("checking");
        this.callCB(this._onicechange, "checking");
        break;
      case Ci.IPeerConnection.kIceGathering:
        this._dompc.changeIceGatheringState("gathering");
        this.callCB(this._ongatheringchange, "gathering");
        break;
      case Ci.IPeerConnection.kIceConnected:
        
        this._dompc.changeIceConnectionState("connected");
        this.callCB(this._onicechange, "connected");
        break;
      case Ci.IPeerConnection.kIceFailed:
        this._dompc.changeIceConnectionState("failed");
        this.callCB(this._onicechange, "failed");
        break;
      default:
        
        break;
    }
  },

  onAddStream: function(stream) {
    this.dispatchEvent(new this._dompc._win.MediaStreamEvent("addstream",
                                                             { stream: stream }));
  },

  onRemoveStream: function(stream, type) {
    this.dispatchEvent(new this._dompc._win.MediaStreamEvent("removestream",
                                                             { stream: stream }));
  },

  foundIceCandidate: function(c) {
    this.dispatchEvent(new this._dompc._win.RTCPeerConnectionIceEvent("icecandidate",
                                                                      { candidate: c }));
  },

  notifyDataChannel: function(channel) {
    this.dispatchEvent(new this._dompc._win.RTCDataChannelEvent("datachannel",
                                                                { channel: channel }));
  },

  notifyConnection: function() {
    this.dispatchEvent(new this._dompc._win.Event("connection"));
  },

  notifyClosedConnection: function() {
    this.dispatchEvent(new this._dompc._win.Event("closedconnection"));
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory(
  [GlobalPCList, RTCIceCandidate, RTCSessionDescription, RTCPeerConnection,
   RTCPeerConnectionIceEvent, MediaStreamEvent, RTCDataChannelEvent]
);
