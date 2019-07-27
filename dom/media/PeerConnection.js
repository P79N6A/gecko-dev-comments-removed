




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PeerConnectionIdp",
  "resource://gre/modules/media/PeerConnectionIdp.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "convertToRTCStatsReport",
  "resource://gre/modules/media/RTCStatsReport.jsm");

const PC_CONTRACT = "@mozilla.org/dom/peerconnection;1";
const PC_OBS_CONTRACT = "@mozilla.org/dom/peerconnectionobserver;1";
const PC_ICE_CONTRACT = "@mozilla.org/dom/rtcicecandidate;1";
const PC_SESSION_CONTRACT = "@mozilla.org/dom/rtcsessiondescription;1";
const PC_MANAGER_CONTRACT = "@mozilla.org/dom/peerconnectionmanager;1";
const PC_STATS_CONTRACT = "@mozilla.org/dom/rtcstatsreport;1";
const PC_IDENTITY_CONTRACT = "@mozilla.org/dom/rtcidentityassertion;1";
const PC_STATIC_CONTRACT = "@mozilla.org/dom/peerconnectionstatic;1";
const PC_SENDER_CONTRACT = "@mozilla.org/dom/rtpsender;1";
const PC_RECEIVER_CONTRACT = "@mozilla.org/dom/rtpreceiver;1";

const PC_CID = Components.ID("{00e0e20d-1494-4776-8e0e-0f0acbea3c79}");
const PC_OBS_CID = Components.ID("{d1748d4c-7f6a-4dc5-add6-d55b7678537e}");
const PC_ICE_CID = Components.ID("{02b9970c-433d-4cc2-923d-f7028ac66073}");
const PC_SESSION_CID = Components.ID("{1775081b-b62d-4954-8ffe-a067bbf508a7}");
const PC_MANAGER_CID = Components.ID("{7293e901-2be3-4c02-b4bd-cbef6fc24f78}");
const PC_STATS_CID = Components.ID("{7fe6e18b-0da3-4056-bf3b-440ef3809e06}");
const PC_IDENTITY_CID = Components.ID("{1abc7499-3c54-43e0-bd60-686e2703f072}");
const PC_STATIC_CID = Components.ID("{0fb47c47-a205-4583-a9fc-cbadf8c95880}");
const PC_SENDER_CID = Components.ID("{4fff5d46-d827-4cd4-a970-8fd53977440e}");
const PC_RECEIVER_CID = Components.ID("{d974b814-8fde-411c-8c45-b86791b81030}");



function GlobalPCList() {
  this._list = {};
  this._networkdown = false; 
  this._lifecycleobservers = {};
  Services.obs.addObserver(this, "inner-window-destroyed", true);
  Services.obs.addObserver(this, "profile-change-net-teardown", true);
  Services.obs.addObserver(this, "network:offline-about-to-go-offline", true);
  Services.obs.addObserver(this, "network:offline-status-changed", true);
  Services.obs.addObserver(this, "gmp-plugin-crash", true);
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

  notifyLifecycleObservers: function(pc, type) {
    for (var key of Object.keys(this._lifecycleobservers)) {
      this._lifecycleobservers[key](pc, pc._winID, type);
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
    if (this._list[winID] === undefined) {
      return;
    }
    this._list[winID] = this._list[winID].filter(
      function (e,i,a) { return e.get() !== null; });

    if (this._list[winID].length === 0) {
      delete this._list[winID];
    }
  },

  hasActivePeerConnection: function(winID) {
    this.removeNullRefs(winID);
    return this._list[winID] ? true : false;
  },

  observe: function(subject, topic, data) {
    let cleanupPcRef = function(pcref) {
      let pc = pcref.get();
      if (pc) {
        pc._pc.close();
        delete pc._observer;
        pc._pc = null;
      }
    };

    let cleanupWinId = function(list, winID) {
      if (list.hasOwnProperty(winID)) {
        list[winID].forEach(cleanupPcRef);
        delete list[winID];
      }
    };

    let broadcastPluginCrash = function(list, winID, pluginID, name, crashReportID) {
      if (list.hasOwnProperty(winID)) {
        list[winID].forEach(function(pcref) {
          let pc = pcref.get();
          if (pc) {
            pc._pc.pluginCrash(pluginID, name, crashReportID);
          }
        });
      }
    };

    if (topic == "inner-window-destroyed") {
      let winID = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
      cleanupWinId(this._list, winID);

      if (this._lifecycleobservers.hasOwnProperty(winID)) {
        delete this._lifecycleobservers[winID];
      }
    } else if (topic == "profile-change-net-teardown" ||
               topic == "network:offline-about-to-go-offline") {
      
      
      
      
      
      
      for (let winId in this._list) {
        cleanupWinId(this._list, winId);
      }
      this._networkdown = true;
    }
    else if (topic == "network:offline-status-changed") {
      if (data == "offline") {
        
        this._networkdown = true;
      } else if (data == "online") {
        this._networkdown = false;
      }
    } else if (topic == "gmp-plugin-crash") {
      
      
      let sep = data.indexOf(' ');
      let pluginId = data.slice(0, sep);
      let rest = data.slice(sep+1);
      
      sep = rest.indexOf(' ');
      let name = rest.slice(0, sep);
      let crashId = rest.slice(sep+1);
      for (let winId in this._list) {
        broadcastPluginCrash(this._list, winId, pluginId, name, crashId);
      }
    }
  },

  _registerPeerConnectionLifecycleCallback: function(winID, cb) {
    this._lifecycleobservers[winID] = cb;
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
    this.sdpMLineIndex = ("sdpMLineIndex" in dict)? dict.sdpMLineIndex : null;
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
  }
};

function RTCStatsReport(win, dict) {
  this._win = win;
  this._pcid = dict.pcid;
  this._report = convertToRTCStatsReport(dict);
}
RTCStatsReport.prototype = {
  classDescription: "RTCStatsReport",
  classID: PC_STATS_CID,
  contractID: PC_STATS_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports]),

  
  
  
  
  

  makeStatsPublic: function() {
    let props = {};
    this.forEach(function(stat) {
        props[stat.id] = { enumerable: true, configurable: false,
                           writable: false, value: stat };
      });
    Object.defineProperties(this.__DOM_IMPL__.wrappedJSObject, props);
  },

  forEach: function(cb, thisArg) {
    for (var key in this._report) {
      cb.call(thisArg || this._report, this.get(key), key, this._report);
    }
  },

  get: function(key) {
    function publifyReadonly(win, obj) {
      let props = {};
      for (let k in obj) {
        props[k] = {enumerable:true, configurable:false, writable:false, value:obj[k]};
      }
      let pubobj = Cu.createObjectIn(win);
      Object.defineProperties(pubobj, props);
      return pubobj;
    }

    
    return publifyReadonly(this._win, this._report[key]);
  },

  has: function(key) {
    return this._report[key] !== undefined;
  },

  get mozPcid() { return this._pcid; }
};

function RTCIdentityAssertion() {}
RTCIdentityAssertion.prototype = {
  classDescription: "RTCIdentityAssertion",
  classID: PC_IDENTITY_CID,
  contractID: PC_IDENTITY_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function(idp, name) {
    this.idp = idp;
    this.name  = name;
  }
};

function RTCPeerConnection() {
  this._queue = [];
  this._senders = [];
  this._receivers = [];

  this._pc = null;
  this._observer = null;
  this._closed = false;

  this._onCreateOfferSuccess = null;
  this._onCreateOfferFailure = null;
  this._onCreateAnswerSuccess = null;
  this._onCreateAnswerFailure = null;
  this._onGetStatsSuccess = null;
  this._onGetStatsFailure = null;
  this._onReplaceTrackSender = null;
  this._onReplaceTrackWithTrack = null;
  this._onReplaceTrackSuccess = null;
  this._onReplaceTrackFailure = null;

  this._localType = null;
  this._remoteType = null;
  this._trickleIce = false;
  this._peerIdentity = null;

  







  this._pending = false;

  
  this._iceGatheringState = this._iceConnectionState = "new";
}
RTCPeerConnection.prototype = {
  classDescription: "mozRTCPeerConnection",
  classID: PC_CID,
  contractID: PC_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
  init: function(win) { this._win = win; },

  __init: function(rtcConfig) {
    this._trickleIce = Services.prefs.getBoolPref("media.peerconnection.trickle_ice");
    if (!rtcConfig.iceServers ||
        !Services.prefs.getBoolPref("media.peerconnection.use_document_iceservers")) {
      rtcConfig.iceServers =
        JSON.parse(Services.prefs.getCharPref("media.peerconnection.default_iceservers"));
    }
    this._mustValidateRTCConfiguration(rtcConfig,
        "RTCPeerConnection constructor passed invalid RTCConfiguration");
    if (_globalPCList._networkdown) {
      throw new this._win.DOMError("",
          "Can't create RTCPeerConnections when the network is down");
    }

    this.makeGetterSetterEH("onaddstream");
    this.makeGetterSetterEH("onaddtrack");
    this.makeGetterSetterEH("onicecandidate");
    this.makeGetterSetterEH("onnegotiationneeded");
    this.makeGetterSetterEH("onsignalingstatechange");
    this.makeGetterSetterEH("onremovestream");
    this.makeGetterSetterEH("ondatachannel");
    this.makeGetterSetterEH("oniceconnectionstatechange");
    this.makeGetterSetterEH("onidentityresult");
    this.makeGetterSetterEH("onpeeridentity");
    this.makeGetterSetterEH("onidpassertionerror");
    this.makeGetterSetterEH("onidpvalidationerror");

    this._pc = new this._win.PeerConnectionImpl();

    this.__DOM_IMPL__._innerObject = this;
    this._observer = new this._win.PeerConnectionObserver(this.__DOM_IMPL__);

    
    this._winID = this._win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
    _globalPCList.addPC(this);

    this._queueOrRun({
      func: this._initialize,
      args: [rtcConfig],
      
      wait: !this._trickleIce
    });
  },

  _initialize: function(rtcConfig) {
    this._impl.initialize(this._observer, this._win, rtcConfig,
                          Services.tm.currentThread);
    this._initIdp();
    _globalPCList.notifyLifecycleObservers(this, "initialized");
  },

  get _impl() {
    if (!this._pc) {
      throw new this._win.DOMError("",
          "RTCPeerConnection is gone (did you enter Offline mode?)");
    }
    return this._pc;
  },

  callCB: function(callback, arg) {
    if (callback) {
      this._win.setTimeout(() => {
        try {
          callback(arg);
        } catch(e) {
          
          
          
          this.logErrorAndCallOnError(e.message, e.fileName, e.lineNumber);
        }
      }, 0);
    }
  },

  _initIdp: function() {
    let prefName = "media.peerconnection.identity.timeout";
    let idpTimeout = Services.prefs.getIntPref(prefName);
    let warningFunc = this.logWarning.bind(this);
    this._localIdp = new PeerConnectionIdp(this._win, idpTimeout, warningFunc,
                                           this.dispatchEvent.bind(this));
    this._remoteIdp = new PeerConnectionIdp(this._win, idpTimeout, warningFunc,
                                            this.dispatchEvent.bind(this));
  },

  





  _queueOrRun: function(obj) {
    this._checkClosed();

    if (this._pending) {
      
      this._queue.push(obj);
    } else {
      this._pending = obj.wait;
      obj.func.apply(this, obj.args);
    }
  },

  
  _executeNext: function() {

    
    
    this._pending = false;

    while (this._queue.length && !this._pending) {
      let obj = this._queue.shift();
      
      this._queueOrRun(obj);
    }
  },

  










  _mustValidateRTCConfiguration: function(rtcConfig, errorMsg) {
    var errorCtor = this._win.DOMError;
    function nicerNewURI(uriStr, errorMsg) {
      let ios = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);
      try {
        return ios.newURI(uriStr, null, null);
      } catch (e if (e.result == Cr.NS_ERROR_MALFORMED_URI)) {
        throw new errorCtor("", errorMsg + " - malformed URI: " + uriStr);
      }
    }
    function mustValidateServer(server) {
      if (!server.url) {
        throw new errorCtor("", errorMsg + " - missing url");
      }
      let url = nicerNewURI(server.url, errorMsg);
      if (url.scheme in { turn:1, turns:1 }) {
        if (!server.username) {
          throw new errorCtor("", errorMsg + " - missing username: " + server.url);
        }
        if (!server.credential) {
          throw new errorCtor("", errorMsg + " - missing credential: " +
                              server.url);
        }
      }
      else if (!(url.scheme in { stun:1, stuns:1 })) {
        throw new errorCtor("", errorMsg + " - improper scheme: " + url.scheme);
      }
    }
    if (rtcConfig.iceServers) {
      let len = rtcConfig.iceServers.length;
      for (let i=0; i < len; i++) {
        mustValidateServer (rtcConfig.iceServers[i], errorMsg);
      }
    }
  },

  
  
  
  
  _checkClosed: function() {
    if (this._closed) {
      throw new this._win.DOMError("", "Peer connection is closed");
    }
  },

  dispatchEvent: function(event) {
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  
  logErrorAndCallOnError: function(msg, file, line) {
    this.logMsg(msg, file, line, Ci.nsIScriptError.exceptionFlag);

    
    try {
      if (typeof this._win.onerror === "function") {
        this._win.onerror(msg, file, line);
      }
    } catch(e) {
      
      try {
        this.logError(e.message, e.fileName, e.lineNumber);
      } catch(e) {}
    }
  },

  logError: function(msg, file, line) {
    this.logMsg(msg, file, line, Ci.nsIScriptError.errorFlag);
  },

  logWarning: function(msg, file, line) {
    this.logMsg(msg, file, line, Ci.nsIScriptError.warningFlag);
  },

  logMsg: function(msg, file, line, flag) {
    let scriptErrorClass = Cc["@mozilla.org/scripterror;1"];
    let scriptError = scriptErrorClass.createInstance(Ci.nsIScriptError);
    scriptError.initWithWindowID(msg, file, null, line, 0, flag,
                                 "content javascript", this._winID);
    let console = Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService);
    console.logMessage(scriptError);
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

  createOffer: function(onSuccess, onError, options) {
    options = options || {};
    this._queueOrRun({
      func: this._createOffer,
      args: [onSuccess, onError, options],
      wait: true
    });
  },

  _createOffer: function(onSuccess, onError, options) {
    this._onCreateOfferSuccess = onSuccess;
    this._onCreateOfferFailure = onError;
    this._impl.createOffer(options);
  },

  _createAnswer: function(onSuccess, onError) {
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
    this._impl.createAnswer();
  },

  createAnswer: function(onSuccess, onError) {
    this._queueOrRun({
      func: this._createAnswer,
      args: [onSuccess, onError],
      wait: true
    });
  },

  setLocalDescription: function(desc, onSuccess, onError) {
    this._localType = desc.type;

    let type;
    switch (desc.type) {
      case "offer":
        type = Ci.IPeerConnection.kActionOffer;
        break;
      case "answer":
        type = Ci.IPeerConnection.kActionAnswer;
        break;
      case "pranswer":
        throw new this._win.DOMError("", "pranswer not yet implemented");
      default:
        throw new this._win.DOMError("",
            "Invalid type " + desc.type + " provided to setLocalDescription");
    }

    this._queueOrRun({
      func: this._setLocalDescription,
      args: [type, desc.sdp, onSuccess, onError],
      wait: true
    });
  },

  _setLocalDescription: function(type, sdp, onSuccess, onError) {
    this._onSetLocalDescriptionSuccess = onSuccess;
    this._onSetLocalDescriptionFailure = onError;
    this._impl.setLocalDescription(type, sdp);
  },

  setRemoteDescription: function(desc, onSuccess, onError) {
    this._remoteType = desc.type;

    let type;
    switch (desc.type) {
      case "offer":
        type = Ci.IPeerConnection.kActionOffer;
        break;
      case "answer":
        type = Ci.IPeerConnection.kActionAnswer;
        break;
      case "pranswer":
        throw new this._win.DOMError("", "pranswer not yet implemented");
      default:
        throw new this._win.DOMError("",
            "Invalid type " + desc.type + " provided to setRemoteDescription");
    }

    this._queueOrRun({
      func: this._setRemoteDescription,
      args: [type, desc.sdp, onSuccess, onError],
      wait: true
    });
  },

  





  _processIdpResult: function(message) {
    let good = !!message;
    
    
    if (good && this._impl.peerIdentity) {
      good = (message.identity === this._impl.peerIdentity);
    }
    if (good) {
      this._impl.peerIdentity = message.identity;
      this._peerIdentity = new this._win.RTCIdentityAssertion(
        this._remoteIdp.provider, message.identity);
      this.dispatchEvent(new this._win.Event("peeridentity"));
    }
    return good;
  },

  _setRemoteDescription: function(type, sdp, onSuccess, onError) {
    let idpComplete = false;
    let setRemoteComplete = false;
    let idpError = null;

    
    
    
    let allDone = () => {
      if (!setRemoteComplete || !idpComplete || !onSuccess) {
        return;
      }
      this.callCB(onSuccess);
      onSuccess = null;
      this._executeNext();
    };

    let setRemoteDone = () => {
      setRemoteComplete = true;
      allDone();
    };

    
    
    let idpDone;
    if (!this._impl.peerIdentity) {
      idpDone = this._processIdpResult.bind(this);
      idpComplete = true; 
    } else {
      idpDone = message => {
        let idpGood = this._processIdpResult(message);
        if (!idpGood) {
          
          
          idpError = "Peer Identity mismatch, expected: " +
            this._impl.peerIdentity;
          this.callCB(onError, idpError);
          this.close();
        } else {
          idpComplete = true;
          allDone();
        }
      };
    }

    try {
      this._remoteIdp.verifyIdentityFromSDP(sdp, idpDone);
    } catch (e) {
      
      this.logWarning(e.message, e.fileName, e.lineNumber);
      idpDone(null);
    }

    this._onSetRemoteDescriptionSuccess = setRemoteDone;
    this._onSetRemoteDescriptionFailure = onError;
    this._impl.setRemoteDescription(type, sdp);
  },

  setIdentityProvider: function(provider, protocol, username) {
    this._checkClosed();
    this._localIdp.setIdentityProvider(provider, protocol, username);
  },

  _gotIdentityAssertion: function(assertion){
    let args = { assertion: assertion };
    let ev = new this._win.RTCPeerConnectionIdentityEvent("identityresult", args);
    this.dispatchEvent(ev);
  },

  getIdentityAssertion: function() {
    this._checkClosed();

    var gotAssertion = assertion => {
      if (assertion) {
        this._gotIdentityAssertion(assertion);
      }
    };

    this._localIdp.getIdentityAssertion(this._impl.fingerprint,
                                        gotAssertion);
  },

  updateIce: function(config) {
    throw new this._win.DOMError("", "updateIce not yet implemented");
  },

  addIceCandidate: function(cand, onSuccess, onError) {
    if (!cand.candidate && !cand.sdpMLineIndex) {
      throw new this._win.DOMError("",
          "Invalid candidate passed to addIceCandidate!");
    }
    this._onAddIceCandidateSuccess = onSuccess || null;
    this._onAddIceCandidateError = onError || null;

    this._queueOrRun({ func: this._addIceCandidate, args: [cand], wait: false });
  },

  _addIceCandidate: function(cand) {
    this._impl.addIceCandidate(cand.candidate, cand.sdpMid || "",
                               (cand.sdpMLineIndex === null) ? 0 :
                                 cand.sdpMLineIndex + 1);
  },

  addStream: function(stream) {
    stream.getTracks().forEach(track => this.addTrack(track, stream));
  },

  removeStream: function(stream) {
     
     throw new this._win.DOMError("", "removeStream not yet implemented");
  },

  getStreamById: function(id) {
    throw new this._win.DOMError("", "getStreamById not yet implemented");
  },

  addTrack: function(track, stream) {
    if (stream.currentTime === undefined) {
      throw new this._win.DOMError("", "invalid stream.");
    }
    if (stream.getTracks().indexOf(track) == -1) {
      throw new this._win.DOMError("", "track is not in stream.");
    }
    this._checkClosed();
    this._impl.addTrack(track, stream);
    let sender = this._win.RTCRtpSender._create(this._win,
                                                new RTCRtpSender(this, track,
                                                                 stream));
    this._senders.push({ sender: sender, stream: stream });
    return sender;
  },

  removeTrack: function(sender) {
     
     throw new this._win.DOMError("", "removeTrack not yet implemented");
  },

  _replaceTrack: function(sender, withTrack, onSuccess, onError) {
    
    
    
    
    
    
    
    
    
    

    this._onReplaceTrackSender = sender;
    this._onReplaceTrackWithTrack = withTrack;
    this._onReplaceTrackSuccess = onSuccess;
    this._onReplaceTrackFailure = onError;
    this._impl.replaceTrack(sender.track, withTrack, sender._stream);
  },

  close: function() {
    if (this._closed) {
      return;
    }
    this.changeIceConnectionState("closed");
    this._queueOrRun({ func: this._close, args: [false], wait: false });
    this._closed = true;
  },

  _close: function() {
    this._localIdp.close();
    this._remoteIdp.close();
    this._impl.close();
  },

  getLocalStreams: function() {
    this._checkClosed();
    return this._impl.getLocalStreams();
  },

  getRemoteStreams: function() {
    this._checkClosed();
    return this._impl.getRemoteStreams();
  },

  getSenders: function() {
    this._checkClosed();
    let streams = this._impl.getLocalStreams();
    let senders = [];
    
    for (let i = this._senders.length - 1; i >= 0; i--) {
      if (streams.indexOf(this._senders[i].stream) != -1) {
        senders.push(this._senders[i].sender);
      } else {
        this._senders.splice(i,1);
      }
    }
    return senders;
  },

  getReceivers: function() {
    this._checkClosed();
    let streams = this._impl.getRemoteStreams();
    let receivers = [];
    
    for (let i = this._receivers.length - 1; i >= 0; i--) {
      if (streams.indexOf(this._receivers[i].stream) != -1) {
        receivers.push(this._receivers[i].receiver);
      } else {
        this._receivers.splice(i,1);
      }
    }
    return receivers;
  },

  get localDescription() {
    this._checkClosed();
    let sdp = this._impl.localDescription;
    if (sdp.length == 0) {
      return null;
    }

    sdp = this._localIdp.wrapSdp(sdp);
    return new this._win.mozRTCSessionDescription({ type: this._localType,
                                                    sdp: sdp });
  },

  get remoteDescription() {
    this._checkClosed();
    let sdp = this._impl.remoteDescription;
    if (sdp.length == 0) {
      return null;
    }
    return new this._win.mozRTCSessionDescription({ type: this._remoteType,
                                                    sdp: sdp });
  },

  get peerIdentity() { return this._peerIdentity; },
  get id() { return this._impl.id; },
  get iceGatheringState()  { return this._iceGatheringState; },
  get iceConnectionState() { return this._iceConnectionState; },

  get signalingState() {
    
    
    if (this._closed) {
      return "closed";
    }
    return {
      "SignalingInvalid":            "",
      "SignalingStable":             "stable",
      "SignalingHaveLocalOffer":     "have-local-offer",
      "SignalingHaveRemoteOffer":    "have-remote-offer",
      "SignalingHaveLocalPranswer":  "have-local-pranswer",
      "SignalingHaveRemotePranswer": "have-remote-pranswer",
      "SignalingClosed":             "closed"
    }[this._impl.signalingState];
  },

  changeIceGatheringState: function(state) {
    this._iceGatheringState = state;
    _globalPCList.notifyLifecycleObservers(this, "icegatheringstatechange");
  },

  changeIceConnectionState: function(state) {
    this._iceConnectionState = state;
    _globalPCList.notifyLifecycleObservers(this, "iceconnectionstatechange");
    this.dispatchEvent(new this._win.Event("iceconnectionstatechange"));
  },

  getStats: function(selector, onSuccess, onError) {
    this._queueOrRun({
      func: this._getStats,
      args: [selector, onSuccess, onError],
      wait: false
    });
  },

  _getStats: function(selector, onSuccess, onError) {
    this._onGetStatsSuccess = onSuccess;
    this._onGetStatsFailure = onError;

    this._impl.getStats(selector);
  },

  createDataChannel: function(label, dict) {
    this._checkClosed();
    if (dict == undefined) {
      dict = {};
    }
    if (dict.maxRetransmitNum != undefined) {
      dict.maxRetransmits = dict.maxRetransmitNum;
      this.logWarning("Deprecated RTCDataChannelInit dictionary entry maxRetransmitNum used!", null, 0);
    }
    if (dict.outOfOrderAllowed != undefined) {
      dict.ordered = !dict.outOfOrderAllowed; 
                                              
      this.logWarning("Deprecated RTCDataChannelInit dictionary entry outOfOrderAllowed used!", null, 0);
    }
    if (dict.preset != undefined) {
      dict.negotiated = dict.preset;
      this.logWarning("Deprecated RTCDataChannelInit dictionary entry preset used!", null, 0);
    }
    if (dict.stream != undefined) {
      dict.id = dict.stream;
      this.logWarning("Deprecated RTCDataChannelInit dictionary entry stream used!", null, 0);
    }

    if (dict.maxRetransmitTime != undefined &&
        dict.maxRetransmits != undefined) {
      throw new this._win.DOMError("",
          "Both maxRetransmitTime and maxRetransmits cannot be provided");
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
    } else if (dict.maxRetransmits != undefined) {
      type = Ci.IPeerConnection.kDataChannelPartialReliableRexmit;
    } else {
      type = Ci.IPeerConnection.kDataChannelReliable;
    }

    
    let channel = this._impl.createDataChannel(
      label, protocol, type, !dict.ordered, dict.maxRetransmitTime,
      dict.maxRetransmits, dict.negotiated ? true : false,
      dict.id != undefined ? dict.id : 0xFFFF
    );
    return channel;
  },

  connectDataConnection: function(localport, remoteport, numstreams) {
    if (numstreams == undefined || numstreams <= 0) {
      numstreams = 16;
    }
    this._queueOrRun({
      func: this._connectDataConnection,
      args: [localport, remoteport, numstreams],
      wait: false
    });
  },

  _connectDataConnection: function(localport, remoteport, numstreams) {
    this._impl.connectDataConnection(localport, remoteport, numstreams);
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


function PeerConnectionObserver() {
  this._dompc = null;
}
PeerConnectionObserver.prototype = {
  classDescription: "PeerConnectionObserver",
  classID: PC_OBS_CID,
  contractID: PC_OBS_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
  init: function(win) { this._win = win; },

  __init: function(dompc) {
    this._dompc = dompc._innerObject;
  },

  dispatchEvent: function(event) {
    this._dompc.dispatchEvent(event);
  },

  onCreateOfferSuccess: function(sdp) {
    let pc = this._dompc;
    let fp = pc._impl.fingerprint;
    pc._localIdp.appendIdentityToSDP(sdp, fp, function(sdp, assertion) {
      if (assertion) {
        pc._gotIdentityAssertion(assertion);
      }
      pc.callCB(pc._onCreateOfferSuccess,
                new pc._win.mozRTCSessionDescription({ type: "offer",
                                                       sdp: sdp }));
      pc._executeNext();
    }.bind(this));
  },

  onCreateOfferError: function(code, message) {
    this._dompc.callCB(this._dompc._onCreateOfferFailure, new RTCError(code, message));
    this._dompc._executeNext();
  },

  onCreateAnswerSuccess: function(sdp) {
    let pc = this._dompc;
    let fp = pc._impl.fingerprint;
    pc._localIdp.appendIdentityToSDP(sdp, fp, function(sdp, assertion) {
      if (assertion) {
        pc._gotIdentityAssertion(assertion);
      }
      pc.callCB(pc._onCreateAnswerSuccess,
                new pc._win.mozRTCSessionDescription({ type: "answer",
                                                       sdp: sdp }));
      pc._executeNext();
    }.bind(this));
  },

  onCreateAnswerError: function(code, message) {
    this._dompc.callCB(this._dompc._onCreateAnswerFailure,
                       new RTCError(code, message));
    this._dompc._executeNext();
  },

  onSetLocalDescriptionSuccess: function() {
    this._dompc.callCB(this._dompc._onSetLocalDescriptionSuccess);
    this._dompc._executeNext();
  },

  onSetRemoteDescriptionSuccess: function() {
    
    this._dompc._onSetRemoteDescriptionSuccess();
  },

  onSetLocalDescriptionError: function(code, message) {
    this._localType = null;
    this._dompc.callCB(this._dompc._onSetLocalDescriptionFailure,
                       new RTCError(code, message));
    this._dompc._executeNext();
  },

  onSetRemoteDescriptionError: function(code, message) {
    this._remoteType = null;
    this._dompc.callCB(this._dompc._onSetRemoteDescriptionFailure,
                       new RTCError(code, message));
    this._dompc._executeNext();
  },

  onAddIceCandidateSuccess: function() {
    this._dompc.callCB(this._dompc._onAddIceCandidateSuccess);
    this._dompc._executeNext();
  },

  onAddIceCandidateError: function(code, message) {
    this._dompc.callCB(this._dompc._onAddIceCandidateError,
                       new RTCError(code, message));
    this._dompc._executeNext();
  },

  onIceCandidate: function(level, mid, candidate) {
    if (candidate == "") {
      this.foundIceCandidate(null);
    } else {
      this.foundIceCandidate(new this._dompc._win.mozRTCIceCandidate(
          {
              candidate: candidate,
              sdpMid: mid,
              sdpMLineIndex: level - 1
          }
      ));
    }
  },


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  handleIceConnectionStateChange: function(iceConnectionState) {
    var histogram = Services.telemetry.getHistogramById("WEBRTC_ICE_SUCCESS_RATE");

    if (iceConnectionState === 'failed') {
      histogram.add(false);
      this._dompc.logError("ICE failed, see about:webrtc for more details", null, 0);
    }
    if (this._dompc.iceConnectionState === 'checking' &&
        (iceConnectionState === 'completed' ||
         iceConnectionState === 'connected')) {
          histogram.add(true);
    }
    this._dompc.changeIceConnectionState(iceConnectionState);
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  handleIceGatheringStateChange: function(gatheringState) {
    this._dompc.changeIceGatheringState(gatheringState);
  },

  onStateChange: function(state) {
    switch (state) {
      case "SignalingState":
        this._dompc.callCB(this._dompc.onsignalingstatechange,
                           this._dompc.signalingState);
        break;

      case "IceConnectionState":
        this.handleIceConnectionStateChange(this._dompc._pc.iceConnectionState);
        break;

      case "IceGatheringState":
        this.handleIceGatheringStateChange(this._dompc._pc.iceGatheringState);
        break;

      case "SdpState":
        
        break;

      case "ReadyState":
        
        break;

      case "SipccState":
        
        break;

      default:
        this._dompc.logWarning("Unhandled state type: " + state, null, 0);
        break;
    }
  },

  onGetStatsSuccess: function(dict) {
    let chromeobj = new RTCStatsReport(this._dompc._win, dict);
    let webidlobj = this._dompc._win.RTCStatsReport._create(this._dompc._win,
                                                            chromeobj);
    chromeobj.makeStatsPublic();
    this._dompc.callCB(this._dompc._onGetStatsSuccess, webidlobj);
    this._dompc._executeNext();
  },

  onGetStatsError: function(code, message) {
    this._dompc.callCB(this._dompc._onGetStatsFailure,
                       new RTCError(code, message));
    this._dompc._executeNext();
  },

  onAddStream: function(stream) {
    let ev = new this._dompc._win.MediaStreamEvent("addstream",
                                                   { stream: stream });
    this._dompc.dispatchEvent(ev);
  },

  onRemoveStream: function(stream, type) {
    this.dispatchEvent(new this._dompc._win.MediaStreamEvent("removestream",
                                                             { stream: stream }));
  },

  onAddTrack: function(track) {
    let ev = new this._dompc._win.MediaStreamTrackEvent("addtrack",
                                                        { track: track });
    this._dompc.dispatchEvent(ev);
  },

  onRemoveTrack: function(track, type) {
    this.dispatchEvent(new this._dompc._win.MediaStreamTrackEvent("removetrack",
                                                                  { track: track }));
  },

  onReplaceTrackSuccess: function() {
    var pc = this._dompc;
    pc._onReplaceTrackSender.track = pc._onReplaceTrackWithTrack;
    pc._onReplaceTrackWithTrack = null;
    pc._onReplaceTrackSender = null;
    pc.callCB(pc._onReplaceTrackSuccess);
  },

  onReplaceTrackError: function(code, message) {
    var pc = this._dompc;
    pc._onReplaceTrackWithTrack = null;
    pc._onReplaceTrackSender = null;
    pc.callCB(pc._onReplaceTrackError, new RTCError(code, message));
  },

  foundIceCandidate: function(cand) {
    this.dispatchEvent(new this._dompc._win.RTCPeerConnectionIceEvent("icecandidate",
                                                                      { candidate: cand } ));
  },

  notifyDataChannel: function(channel) {
    this.dispatchEvent(new this._dompc._win.RTCDataChannelEvent("datachannel",
                                                                { channel: channel }));
  }
};

function RTCPeerConnectionStatic() {
}
RTCPeerConnectionStatic.prototype = {
  classDescription: "mozRTCPeerConnectionStatic",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  classID: PC_STATIC_CID,
  contractID: PC_STATIC_CONTRACT,

  init: function(win) {
    this._winID = win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
  },

  registerPeerConnectionLifecycleCallback: function(cb) {
    _globalPCList._registerPeerConnectionLifecycleCallback(this._winID, cb);
  },
};

function RTCRtpSender(pc, track, stream) {
  this._pc = pc;
  this.track = track;
  this._stream = stream;
}
RTCRtpSender.prototype = {
  classDescription: "RTCRtpSender",
  classID: PC_SENDER_CID,
  contractID: PC_SENDER_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports]),

  replaceTrack: function(withTrack, onSuccess, onError) {
    this._pc._checkClosed();
    this._pc._queueOrRun({
      func: this._pc._replaceTrack,
      args: [this, withTrack, onSuccess, onError],
      wait: false
    });
  }
};

function RTCRtpReceiver(pc, track) {
  this.pc = pc;
  this.track = track;
}
RTCRtpReceiver.prototype = {
  classDescription: "RTCRtpReceiver",
  classID: PC_RECEIVER_CID,
  contractID: PC_RECEIVER_CONTRACT,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory(
  [GlobalPCList,
   RTCIceCandidate,
   RTCSessionDescription,
   RTCPeerConnection,
   RTCPeerConnectionStatic,
   RTCRtpReceiver,
   RTCRtpSender,
   RTCStatsReport,
   RTCIdentityAssertion,
   PeerConnectionObserver]
);
