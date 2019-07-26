



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PC_CONTRACT = "@mozilla.org/dom/peerconnection;1";
const PC_ICE_CONTRACT = "@mozilla.org/dom/rtcicecandidate;1";
const PC_SESSION_CONTRACT = "@mozilla.org/dom/rtcsessiondescription;1";
const PC_MANAGER_CONTRACT = "@mozilla.org/dom/peerconnectionmanager;1";

const PC_CID = Components.ID("{7cb2b368-b1ce-4560-acac-8e0dbda7d3d0}");
const PC_ICE_CID = Components.ID("{8c5dbd70-2c8e-4ecb-a5ad-2fc919099f01}");
const PC_SESSION_CID = Components.ID("{5f21ffd9-b73f-4ba0-a685-56b4667aaf1c}");
const PC_MANAGER_CID = Components.ID("{7293e901-2be3-4c02-b4bd-cbef6fc24f78}");



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
  classInfo: XPCOMUtils.generateCI({classID: PC_MANAGER_CID,
                                    contractID: PC_MANAGER_CONTRACT,
                                    classDescription: "PeerConnectionManager",
                                    interfaces: [
                                      Ci.nsIObserver,
                                      Ci.nsISupportsWeakReference,
                                      Ci.IPeerConnectionManager
                                    ]}),

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
      this._list[winID].push(Components.utils.getWeakReference(pc));
    } else {
      this._list[winID] = [Components.utils.getWeakReference(pc)];
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
            pc._pc.close(false);
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
            pc._pc.close(true);
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

function IceCandidate(candidate) {
  this.candidate = candidate;
  this.sdpMid = null;
  this.sdpMLineIndex = null;
}
IceCandidate.prototype = {
  classID: PC_ICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: PC_ICE_CID,
                                    contractID: PC_ICE_CONTRACT,
                                    classDescription: "IceCandidate",
                                    interfaces: [
                                      Ci.nsIDOMRTCIceCandidate
                                    ],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT}),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMRTCIceCandidate, Ci.nsIDOMGlobalObjectConstructor
  ]),

  constructor: function(win, candidateInitDict) {
    if (this._win) {
      throw new Error("Constructor already called");
    }
    this._win = win;
    if (candidateInitDict !== undefined) {
      this.candidate = candidateInitDict.candidate || null;
      this.sdpMid = candidateInitDict.sdpMid || null;
      this.sdpMLineIndex = candidateInitDict.sdpMLineIndex === null ?
            null : candidateInitDict.sdpMLineIndex + 1;
    } else {
      this.candidate = this.sdpMid = this.sdpMLineIndex = null;
    }
  }
};

function SessionDescription(type, sdp) {
  this.type = type;
  this.sdp = sdp;
}
SessionDescription.prototype = {
  classID: PC_SESSION_CID,

  classInfo: XPCOMUtils.generateCI({classID: PC_SESSION_CID,
                                    contractID: PC_SESSION_CONTRACT,
                                    classDescription: "SessionDescription",
                                    interfaces: [
                                      Ci.nsIDOMRTCSessionDescription
                                    ],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT}),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMRTCSessionDescription, Ci.nsIDOMGlobalObjectConstructor
  ]),

  constructor: function(win, descriptionInitDict) {
    if (this._win) {
      throw new Error("Constructor already called");
    }
    this._win = win;
    if (descriptionInitDict !== undefined) {
      this.type = descriptionInitDict.type || null;
      this.sdp = descriptionInitDict.sdp || null;
    } else {
      this.type = this.sdp = null;
    }
  },

  toString: function() {
    return JSON.stringify({
      type: this.type, sdp: this.sdp
    });
  }
};

function PeerConnection() {
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

  
  this.onaddstream = null;
  this.onopen = null;
  this.onremovestream = null;
  this.onicecandidate = null;
  this.onstatechange = null;
  this.ongatheringchange = null;
  this.onicechange = null;

  
  this.ondatachannel = null;
  this.onconnection = null;
  this.onclosedconnection = null;
}
PeerConnection.prototype = {
  classID: PC_CID,

  classInfo: XPCOMUtils.generateCI({classID: PC_CID,
                                    contractID: PC_CONTRACT,
                                    classDescription: "PeerConnection",
                                    interfaces: [
                                      Ci.nsIDOMRTCPeerConnection
                                    ],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT}),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMRTCPeerConnection,
    Ci.nsIDOMGlobalObjectConstructor,
    Ci.nsISupportsWeakReference
  ]),

  
  constructor: function(win, rtcConfig) {
    if (!Services.prefs.getBoolPref("media.peerconnection.enabled")) {
      throw new Error("PeerConnection not enabled (did you set the pref?)");
    }
    if (this._win) {
      throw new Error("RTCPeerConnection constructor already called");
    }
    if (!rtcConfig) {
      
      rtcConfig = { "iceServers": [{ url: "stun:23.21.150.121" }] };
    }
    this._mustValidateRTCConfiguration(rtcConfig,
        "RTCPeerConnection constructor passed invalid RTCConfiguration");
    if (_globalPCList._networkdown) {
      throw new Error("Can't create RTCPeerConnections when the network is down");
    }

    this._pc = Cc["@mozilla.org/peerconnection;1"].
             createInstance(Ci.IPeerConnection);
    this._observer = new PeerConnectionObserver(this);

    
    this._queueOrRun({
      func: this._pc.initialize,
      args: [this._observer, win, rtcConfig, Services.tm.currentThread],
      wait: true
    });

    this._win = win;
    this._winID = this._win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

    
    _globalPCList.addPC(this);
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
    function isObject(obj) {
      return obj && (typeof obj === "object");
    }
    function isArray(obj) {
      return isObject(obj) &&
        (Object.prototype.toString.call(obj) === "[object Array]");
    }
    function nicerNewURI(uriStr, errorMsg) {
      let ios = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);
      try {
        return ios.newURI(uriStr, null, null);
      } catch (e if (e.result == Cr.NS_ERROR_MALFORMED_URI)) {
        throw new Error(errorMsg + " - malformed URI: " + uriStr);
      }
    }
    function mustValidateServer(server) {
      let url = nicerNewURI(server.url, errorMsg);
      if (!(url.scheme in { stun:1, stuns:1, turn:1, turns:1 })) {
        throw new Error (errorMsg + " - improper scheme: " + url.scheme);
      }
      if (server.credential && isObject(server.credential)) {
        throw new Error (errorMsg + " - invalid credential");
      }
    }
    if (!isObject(rtcConfig)) {
      throw new Error (errorMsg);
    }
    if (!isArray(rtcConfig.iceServers)) {
      throw new Error (errorMsg + " - iceServers [] property not present");
    }
    for (let i=0; i < rtcConfig.iceServers.length; i++) {
      mustValidateServer (rtcConfig.iceServers[i], errorMsg);
    }
  },

  










  _validateConstraints: function(constraints) {
    function isObject(obj) {
      return obj && (typeof obj === "object");
    }
    function isArray(obj) {
      return isObject(obj) &&
        (Object.prototype.toString.call(obj) === "[object Array]");
    }

    if (!isObject(constraints)) {
      return false;
    }
    if (constraints.mandatory && !isObject(constraints.mandatory)) {
      return false;
    }
    if (constraints.optional && !isArray(constraints.optional)) {
      return false;
    }

    return true;
  },

  
  
  
  
  _checkClosed: function() {
    if (this._closed) {
      throw new Error ("Peer connection is closed");
    }
  },

  createOffer: function(onSuccess, onError, constraints) {
    if (!constraints) {
      constraints = {};
    }

    if (!this._validateConstraints(constraints)) {
      throw new Error("createOffer passed invalid constraints");
    }

    this._onCreateOfferSuccess = onSuccess;
    this._onCreateOfferFailure = onError;

    this._queueOrRun({
      func: this._pc.createOffer,
      args: [constraints],
      wait: true
    });
  },

  _createAnswer: function(onSuccess, onError, constraints, provisional) {
    this._onCreateAnswerSuccess = onSuccess;
    this._onCreateAnswerFailure = onError;

    if (!this.remoteDescription) {
      this._observer.onCreateAnswerError(3); 
      






      return;
    }

    if (this.remoteDescription.type != "offer") {
      this._observer.onCreateAnswerError(3); 
      






      return;
    }

    

    this._pc.createAnswer(constraints);
  },

  createAnswer: function(onSuccess, onError, constraints, provisional) {
    if (!constraints) {
      constraints = {};
    }

    if (!this._validateConstraints(constraints)) {
      throw new Error("createAnswer passed invalid constraints");
    }

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
      default:
        throw new Error(
          "Invalid type " + desc.type + " provided to setLocalDescription"
        );
        break;
    }

    this._queueOrRun({
      func: this._pc.setLocalDescription,
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
      default:
        throw new Error(
          "Invalid type " + desc.type + " provided to setRemoteDescription"
        );
        break;
    }

    this._queueOrRun({
      func: this._pc.setRemoteDescription,
      args: [type, desc.sdp],
      wait: true,
      type: desc.type
    });
  },

  updateIce: function(config, constraints, restart) {
    return Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  addIceCandidate: function(cand) {
    if (!cand) {
      throw new Error ("NULL candidate passed to addIceCandidate!");
    }

    if (!cand.candidate || !cand.sdpMLineIndex) {
      throw new Error ("Invalid candidate passed to addIceCandidate!");
    }

    this._queueOrRun({
      func: this._pc.addIceCandidate,
      args: [cand.candidate, cand.sdpMid || "", cand.sdpMLineIndex],
      wait: false
    });
  },

  addStream: function(stream, constraints) {
    
    this._queueOrRun({
      func: this._pc.addStream,
      args: [stream],
      wait: false
    });
  },

  removeStream: function(stream) {
    this._queueOrRun({
      func: this._pc.removeStream,
      args: [stream],
      wait: false
    });
  },

  close: function() {
    this._queueOrRun({
      func: this._pc.close,
      args: [false],
      wait: false
    });
    this._closed = true;
  },

  get localStreams() {
    this._checkClosed();
    return this._pc.localStreams;
  },

  get remoteStreams() {
    this._checkClosed();
    return this._pc.remoteStreams;
  },

  get localDescription() {
    this._checkClosed();
    let sdp = this._pc.localDescription;
    if (sdp.length == 0) {
      return null;
    }
    return {
      type: this._localType, sdp: sdp,
      __exposedProps__: { type: "rw", sdp: "rw" }
    };
  },

  get remoteDescription() {
    this._checkClosed();
    let sdp = this._pc.remoteDescription;
    if (sdp.length == 0) {
      return null;
    }
    return {
      type: this._remoteType, sdp: sdp,
      __exposedProps__: { type: "rw", sdp: "rw" }
    };
  },

  createDataChannel: function(label, dict) {
    this._checkClosed();
    if (dict &&
        dict.maxRetransmitTime != undefined &&
        dict.maxRetransmitNum != undefined) {
      throw new Error("Both maxRetransmitTime and maxRetransmitNum cannot be provided");
    }

    
    let type;
    if (dict.maxRetransmitTime != undefined) {
      type = Ci.IPeerConnection.kDataChannelPartialReliableTimed;
    } else if (dict.maxRetransmitNum != undefined) {
      type = Ci.IPeerConnection.kDataChannelPartialReliableRexmit;
    } else {
      type = Ci.IPeerConnection.kDataChannelReliable;
    }

    
    
    
    let channel = this._pc.createDataChannel(
      label, type, dict.outOfOrderAllowed, dict.maxRetransmitTime,
      dict.maxRetransmitNum
    );
    return channel;
  },

  connectDataConnection: function(localport, remoteport, numstreams) {
    if (numstreams == undefined || numstreams <= 0) {
      numstreams = 16;
    }
    this._queueOrRun({
      func: this._pc.connectDataConnection,
      args: [localport, remoteport, numstreams],
      wait: false
    });
  }
};


function PeerConnectionObserver(dompc) {
  this._dompc = dompc;
}
PeerConnectionObserver.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.IPeerConnectionObserver,
                                         Ci.nsISupportsWeakReference]),

  onCreateOfferSuccess: function(offer) {
    if (this._dompc._onCreateOfferSuccess) {
      try {
        this._dompc._onCreateOfferSuccess.onCallback({
          type: "offer", sdp: offer,
          __exposedProps__: { type: "rw", sdp: "rw" }
        });
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onCreateOfferError: function(code) {
    if (this._dompc._onCreateOfferFailure) {
      try {
        this._dompc._onCreateOfferFailure.onCallback(code);
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onCreateAnswerSuccess: function(answer) {
    if (this._dompc._onCreateAnswerSuccess) {
      try {
        this._dompc._onCreateAnswerSuccess.onCallback({
          type: "answer", sdp: answer,
          __exposedProps__: { type: "rw", sdp: "rw" }
        });
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onCreateAnswerError: function(code) {
    if (this._dompc._onCreateAnswerFailure) {
      try {
        this._dompc._onCreateAnswerFailure.onCallback(code);
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onSetLocalDescriptionSuccess: function(code) {
    this._dompc._localType = this._dompc._pendingType;
    this._dompc._pendingType = null;
    if (this._dompc._onSetLocalDescriptionSuccess) {
      try {
        this._dompc._onSetLocalDescriptionSuccess.onCallback(code);
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onSetRemoteDescriptionSuccess: function(code) {
    this._dompc._remoteType = this._dompc._pendingType;
    this._dompc._pendingType = null;
    if (this._dompc._onSetRemoteDescriptionSuccess) {
      try {
        this._dompc._onSetRemoteDescriptionSuccess.onCallback(code);
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onSetLocalDescriptionError: function(code) {
    this._dompc._pendingType = null;
    if (this._dompc._onSetLocalDescriptionFailure) {
      try {
        this._dompc._onSetLocalDescriptionFailure.onCallback(code);
      } catch(e) {}
    }
    this._dompc._executeNext();
  },

  onSetRemoteDescriptionError: function(code) {
    this._dompc._pendingType = null;
    if (this._dompc._onSetRemoteDescriptionFailure) {
      this._dompc._onSetRemoteDescriptionFailure.onCallback(code);
    }
    this._dompc._executeNext();
  },

  onStateChange: function(state) {
    if (state != Ci.IPeerConnectionObserver.kIceState) {
      return;
    }

    let self = this;
    let iceCb = function() {};
    let iceGatherCb = function() {};
    if (this._dompc.onicechange) {
      iceCb = function(args) {
        try {
          self._dompc.onicechange(args);
        } catch(e) {}
      };
    }
    if (this._dompc.ongatheringchange) {
      iceGatherCb = function(args) {
        try {
          self._dompc.ongatheringchange(args);
        } catch(e) {}
      };
    }

    switch (this._dompc._pc.iceState) {
      case Ci.IPeerConnection.kIceGathering:
        iceGatherCb("gathering");
        break;
      case Ci.IPeerConnection.kIceWaiting:
        iceCb("starting");
        this._dompc._executeNext();
        break;
      case Ci.IPeerConnection.kIceChecking:
        iceCb("checking");
        break;
      case Ci.IPeerConnection.kIceConnected:
        
        iceCb("connected");
        iceGatherCb("complete");
        break;
      case Ci.IPeerConnection.kIceFailed:
        iceCb("failed");
        break;
      default:
        
        break;
    }
  },

  onAddStream: function(stream, type) {
    if (this._dompc.onaddstream) {
      try {
        this._dompc.onaddstream.onCallback({
          stream: stream, type: type,
          __exposedProps__: { stream: "r", type: "r" }
        });
      } catch(e) {}
    }
  },

  onRemoveStream: function(stream, type) {
    if (this._dompc.onremovestream) {
      try {
        this._dompc.onremovestream.onCallback({
          stream: stream, type: type,
          __exposedProps__: { stream: "r", type: "r" }
        });
      } catch(e) {}
    }
  },

  foundIceCandidate: function(cand) {
    if (this._dompc.onicecandidate) {
      try {
        this._dompc.onicecandidate.onCallback({
          candidate: cand,
          __exposedProps__: { candidate: "rw" }
        });
      } catch(e) {}
    }
  },

  notifyDataChannel: function(channel) {
    if (this._dompc.ondatachannel) {
      try {
        this._dompc.ondatachannel.onCallback(channel);
      } catch(e) {}
    }
  },

  notifyConnection: function() {
    if (this._dompc.onconnection) {
      try {
        this._dompc.onconnection.onCallback();
      } catch(e) {}
    }
  },

  notifyClosedConnection: function() {
    if (this._dompc.onclosedconnection) {
      try {
        this._dompc.onclosedconnection.onCallback();
      } catch(e) {}
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory(
  [GlobalPCList, IceCandidate, SessionDescription, PeerConnection]
);
