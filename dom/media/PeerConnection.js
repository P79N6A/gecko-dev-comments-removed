




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
const PC_STATIC_CONTRACT = "@mozilla.org/dom/peerconnectionstatic;1";
const PC_SENDER_CONTRACT = "@mozilla.org/dom/rtpsender;1";
const PC_RECEIVER_CONTRACT = "@mozilla.org/dom/rtpreceiver;1";

const PC_CID = Components.ID("{bdc2e533-b308-4708-ac8e-a8bfade6d851}");
const PC_OBS_CID = Components.ID("{d1748d4c-7f6a-4dc5-add6-d55b7678537e}");
const PC_ICE_CID = Components.ID("{02b9970c-433d-4cc2-923d-f7028ac66073}");
const PC_SESSION_CID = Components.ID("{1775081b-b62d-4954-8ffe-a067bbf508a7}");
const PC_MANAGER_CID = Components.ID("{7293e901-2be3-4c02-b4bd-cbef6fc24f78}");
const PC_STATS_CID = Components.ID("{7fe6e18b-0da3-4056-bf3b-440ef3809e06}");
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
    } else if (topic == "network:app-offline-status-changed") {
      
      
      if (!this._networkdown && !this._win.navigator.onLine) {
        for (let winId in this._list) {
          cleanupWinId(this._list, winId);
        }
      }
      this._networkdown = !this._win.navigator.onLine;
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

function RTCPeerConnection() {
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
    this._winID = this._win.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

    if (!rtcConfig.iceServers ||
        !Services.prefs.getBoolPref("media.peerconnection.use_document_iceservers")) {
      rtcConfig.iceServers =
        JSON.parse(Services.prefs.getCharPref("media.peerconnection.default_iceservers"));
    }
    
    rtcConfig.iceServers.forEach(server => {
      if (typeof server.urls === "string") {
        server.urls = [server.urls];
      } else if (!server.urls && server.url) {
        
        server.urls = [server.url];
        this.logWarning("RTCIceServer.url is deprecated! Use urls instead.", null, 0);
      }
    });
    this._mustValidateRTCConfiguration(rtcConfig,
        "RTCPeerConnection constructor passed invalid RTCConfiguration");
    if (_globalPCList._networkdown || !this._win.navigator.onLine) {
      throw new this._win.DOMException(
          "Can't create RTCPeerConnections when the network is down",
          "InvalidStateError");
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
    this._operationsChain = this._win.Promise.resolve();

    this.__DOM_IMPL__._innerObject = this;
    this._observer = new this._win.PeerConnectionObserver(this.__DOM_IMPL__);

    
    _globalPCList.addPC(this);

    this._impl.initialize(this._observer, this._win, rtcConfig,
                          Services.tm.currentThread);
    this._initIdp();
    _globalPCList.notifyLifecycleObservers(this, "initialized");
  },

  get _impl() {
    if (!this._pc) {
      throw new this._win.DOMException(
          "RTCPeerConnection is gone (did you enter Offline mode?)",
          "InvalidStateError");
    }
    return this._pc;
  },

  _initIdp: function() {
    this._peerIdentity = new this._win.Promise((resolve, reject) => {
      this._resolvePeerIdentity = resolve;
      this._rejectPeerIdentity = reject;
    });
    this._lastIdentityValidation = this._win.Promise.resolve();

    let prefName = "media.peerconnection.identity.timeout";
    let idpTimeout = Services.prefs.getIntPref(prefName);
    this._localIdp = new PeerConnectionIdp(this._win, idpTimeout);
    this._remoteIdp = new PeerConnectionIdp(this._win, idpTimeout);
  },

  

  _chain: function(func) {
    this._checkClosed(); 
    let p = this._operationsChain.then(() => {
      
      
      
      
      
      if (!this._closed) {
        return func();
      }
    });
    
    this._operationsChain = p.catch(() => {});
    return p;
  },

  
  
  

  _legacyCatch: function(onSuccess, onError, func) {
    if (!onSuccess) {
      return func();
    }
    try {
      return func().then(this._wrapLegacyCallback(onSuccess),
                         this._wrapLegacyCallback(onError));
    } catch (e) {
      this._wrapLegacyCallback(onError)(e);
      return this._win.Promise.resolve(); 
    }
  },

  _wrapLegacyCallback: function(func) {
    return result => {
      try {
        func && func(result);
      } catch (e) {
        this.logErrorAndCallOnError(e);
      }
    };
  },

  












  _mustValidateRTCConfiguration: function(rtcConfig, msg) {
    let ios = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);

    let nicerNewURI = uriStr => {
      try {
        return ios.newURI(uriStr, null, null);
      } catch (e if (e.result == Cr.NS_ERROR_MALFORMED_URI)) {
        throw new this._win.DOMException(msg + " - malformed URI: " + uriStr,
                                         "SyntaxError");
      }
    };

    rtcConfig.iceServers.forEach(server => {
      if (!server.urls) {
        throw new this._win.DOMException(msg + " - missing urls", "InvalidAccessError");
      }
      server.urls.forEach(urlStr => {
        let url = nicerNewURI(urlStr);
        if (url.scheme in { turn:1, turns:1 }) {
          if (!server.username) {
            throw new this._win.DOMException(msg + " - missing username: " + urlStr,
                                             "InvalidAccessError");
          }
          if (!server.credential) {
            throw new this._win.DOMException(msg + " - missing credential: " + urlStr,
                                             "InvalidAccessError");
          }
        }
        else if (!(url.scheme in { stun:1, stuns:1 })) {
          throw new this._win.DOMException(msg + " - improper scheme: " + url.scheme,
                                           "SyntaxError");
        }
        if (url.scheme in { stuns:1, turns:1 }) {
          this.logWarning(url.scheme.toUpperCase() + " is not yet supported.", null, 0);
        }
      });
    });
  },

  
  
  
  
  _checkClosed: function() {
    if (this._closed) {
      throw new this._win.DOMException("Peer connection is closed",
                                       "InvalidStateError");
    }
  },

  dispatchEvent: function(event) {
    
    
    if (!this._closed) {
      this.__DOM_IMPL__.dispatchEvent(event);
    }
  },

  
  logErrorAndCallOnError: function(e) {
    this.logMsg(e.message, e.fileName, e.lineNumber, Ci.nsIScriptError.exceptionFlag);

    
    try {
      if (typeof this._win.onerror === "function") {
        this._win.onerror(e.message, e.fileName, e.lineNumber);
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

  _addIdentityAssertion: function(p, origin) {
    if (this._localIdp.enabled) {
      return this._localIdp.getIdentityAssertion(this._impl.fingerprint, origin)
        .then(() => p)
        .then(sdp => this._localIdp.addIdentityAttribute(sdp));
    }
    return p;
  },

  createOffer: function(optionsOrOnSuccess, onError, options) {
    let onSuccess;
    if (typeof optionsOrOnSuccess == "function") {
      onSuccess = optionsOrOnSuccess;
    } else {
      options = optionsOrOnSuccess;
    }
    return this._legacyCatch(onSuccess, onError, () => {
      
      
      function convertLegacyOptions(o) {
        
        let lcy = ((o.mandatory && Object.keys(o.mandatory).length) || o.optional) &&
            Object.keys(o).length == (o.mandatory? 1 : 0) + (o.optional? 1 : 0);
        if (!lcy) {
          return false;
        }
        let old = o.mandatory || {};
        if (o.mandatory) {
          delete o.mandatory;
        }
        if (o.optional) {
          o.optional.forEach(one => {
            
            
            let key = Object.keys(one)[0];
            if (key && old[key] === undefined) {
              old[key] = one[key];
            }
          });
          delete o.optional;
        }
        o.offerToReceiveAudio = old.OfferToReceiveAudio;
        o.offerToReceiveVideo = old.OfferToReceiveVideo;
        o.mozDontOfferDataChannel = old.MozDontOfferDataChannel;
        o.mozBundleOnly = old.MozBundleOnly;
        Object.keys(o).forEach(k => {
          if (o[k] === undefined) {
            delete o[k];
          }
        });
        return true;
      }

      if (options && convertLegacyOptions(options)) {
        this.logWarning(
          "Mandatory/optional in createOffer options is deprecated! Use " +
            JSON.stringify(options) + " instead (note the case difference)!",
          null, 0);
      }

      let origin = Cu.getWebIDLCallerPrincipal().origin;
      return this._chain(() => {
        let p = new this._win.Promise((resolve, reject) => {
          this._onCreateOfferSuccess = resolve;
          this._onCreateOfferFailure = reject;
          this._impl.createOffer(options);
        });
        p = this._addIdentityAssertion(p, origin);
        return p.then(
          sdp => new this._win.mozRTCSessionDescription({ type: "offer", sdp: sdp }));
      });
    });
  },

  createAnswer: function(onSuccess, onError) {
    return this._legacyCatch(onSuccess, onError, () => {
      let origin = Cu.getWebIDLCallerPrincipal().origin;
      return this._chain(() => {
        let p = new this._win.Promise((resolve, reject) => {
        
        
        
        if (!this.remoteDescription) {
          throw new this._win.DOMException("setRemoteDescription not called",
                                           "InvalidStateError");
        }
        if (this.remoteDescription.type != "offer") {
          throw new this._win.DOMException("No outstanding offer",
                                           "InvalidStateError");
        }
        this._onCreateAnswerSuccess = resolve;
        this._onCreateAnswerFailure = reject;
        this._impl.createAnswer();
        });
        p = this._addIdentityAssertion(p, origin);
        return p.then(sdp => {
          return new this._win.mozRTCSessionDescription({ type: "answer", sdp: sdp });
        });
      });
    });
  },


  setLocalDescription: function(desc, onSuccess, onError) {
    return this._legacyCatch(onSuccess, onError, () => {
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
          throw new this._win.DOMException("pranswer not yet implemented",
                                           "NotSupportedError");
        default:
          throw new this._win.DOMException(
              "Invalid type " + desc.type + " provided to setLocalDescription",
              "InvalidParameterError");
      }
      return this._chain(() => new this._win.Promise((resolve, reject) => {
        this._onSetLocalDescriptionSuccess = resolve;
        this._onSetLocalDescriptionFailure = reject;
        this._impl.setLocalDescription(type, desc.sdp);
      }));
    });
  },

  _validateIdentity: function(sdp, origin) {
    let expectedIdentity;

    
    
    
    let validation = this._lastIdentityValidation
      .then(() => this._remoteIdp.verifyIdentityFromSDP(sdp, origin))
      .then(msg => {
        expectedIdentity = this._impl.peerIdentity;
        
        if (expectedIdentity && (!msg || msg.identity !== expectedIdentity)) {
          this.close();
          throw new this._win.DOMException(
            "Peer Identity mismatch, expected: " + expectedIdentity,
            "IncompatibleSessionDescriptionError");
        }
        if (msg) {
          
          this._impl.peerIdentity = msg.identity;
          this._resolvePeerIdentity(Cu.cloneInto({
            idp: this._remoteIdp.provider,
            name: msg.identity
          }, this._win));
        }
      })
      .catch(e => {
        this._rejectPeerIdentity(e);
        
        
        
        if (!this._impl.peerIdentity) {
          this._peerIdentity = new this._win.Promise((resolve, reject) => {
            this._resolvePeerIdentity = resolve;
            this._rejectPeerIdentity = reject;
          });
        }
        throw e;
      });
    this._lastIdentityValidation = validation.catch(() => {});

    
    return expectedIdentity ? validation : this._win.Promise.resolve();
  },

  setRemoteDescription: function(desc, onSuccess, onError) {
    return this._legacyCatch(onSuccess, onError, () => {
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
          throw new this._win.DOMException("pranswer not yet implemented",
                                           "NotSupportedError");
        default:
          throw new this._win.DOMException(
              "Invalid type " + desc.type + " provided to setRemoteDescription",
            "InvalidParameterError");
      }

      
      let origin = Cu.getWebIDLCallerPrincipal().origin;

      
      return this._chain(() => this._win.Promise.all([
        new this._win.Promise((resolve, reject) => {
          this._onSetRemoteDescriptionSuccess = resolve;
          this._onSetRemoteDescriptionFailure = reject;
          this._impl.setRemoteDescription(type, desc.sdp);
        }),
        this._validateIdentity(desc.sdp, origin)
      ])).then(() => {}); 
    });
  },

  setIdentityProvider: function(provider, protocol, username) {
    this._checkClosed();
    this._localIdp.setIdentityProvider(provider, protocol, username);
  },

  getIdentityAssertion: function() {
    let origin = Cu.getWebIDLCallerPrincipal().origin;
    return this._chain(() => {
      return this._localIdp.getIdentityAssertion(this._impl.fingerprint, origin);
    });
  },

  updateIce: function(config) {
    throw new this._win.DOMException("updateIce not yet implemented",
                                     "NotSupportedError");
  },

  addIceCandidate: function(c, onSuccess, onError) {
    return this._legacyCatch(onSuccess, onError, () => {
      if (!c.candidate && !c.sdpMLineIndex) {
        throw new this._win.DOMException("Invalid candidate passed to addIceCandidate!",
                                         "InvalidParameterError");
      }
      return this._chain(() => new this._win.Promise((resolve, reject) => {
        this._onAddIceCandidateSuccess = resolve;
        this._onAddIceCandidateError = reject;
        this._impl.addIceCandidate(c.candidate, c.sdpMid || "", c.sdpMLineIndex);
      }));
    });
  },

  addStream: function(stream) {
    stream.getTracks().forEach(track => this.addTrack(track, stream));
  },

  removeStream: function(stream) {
     
     throw new this._win.DOMException("removeStream not yet implemented",
                                      "NotSupportedError");
  },

  getStreamById: function(id) {
    throw new this._win.DOMException("getStreamById not yet implemented",
                                     "NotSupportedError");
  },

  addTrack: function(track, stream) {
    if (stream.currentTime === undefined) {
      throw new this._win.DOMException("invalid stream.", "InvalidParameterError");
    }
    if (stream.getTracks().indexOf(track) < 0) {
      throw new this._win.DOMException("track is not in stream.",
                                       "InvalidParameterError");
    }
    this._checkClosed();
    this._senders.forEach(sender => {
      if (sender.track == track) {
        throw new this._win.DOMException("already added.",
                                         "InvalidParameterError");
      }
    });
    this._impl.addTrack(track, stream);
    let sender = this._win.RTCRtpSender._create(this._win,
                                                new RTCRtpSender(this, track,
                                                                 stream));
    this._senders.push(sender);
    return sender;
  },

  removeTrack: function(sender) {
    this._checkClosed();
    var i = this._senders.indexOf(sender);
    if (i >= 0) {
      this._senders.splice(i, 1);
      this._impl.removeTrack(sender.track); 
    }
  },

  _replaceTrack: function(sender, withTrack) {
    
    
    
    
    
    
    
    
    
    

    return new this._win.Promise((resolve, reject) => {
      this._onReplaceTrackSender = sender;
      this._onReplaceTrackWithTrack = withTrack;
      this._onReplaceTrackSuccess = resolve;
      this._onReplaceTrackFailure = reject;
      this._impl.replaceTrack(sender.track, withTrack);
    });
  },

  close: function() {
    if (this._closed) {
      return;
    }
    this.changeIceConnectionState("closed");
    this._localIdp.close();
    this._remoteIdp.close();
    this._impl.close();
    this._closed = true;
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
    return this._senders;
  },

  getReceivers: function() {
    return this._receivers;
  },

  get localDescription() {
    this._checkClosed();
    let sdp = this._impl.localDescription;
    if (sdp.length == 0) {
      return null;
    }

    sdp = this._localIdp.addIdentityAttribute(sdp);
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
  get idpLoginUrl() { return this._localIdp.idpLoginUrl; },
  get id() { return this._impl.id; },
  set id(s) { this._impl.id = s; },
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
    return this._legacyCatch(onSuccess, onError, () => {
      return this._chain(() => new this._win.Promise((resolve, reject) => {
        this._onGetStatsSuccess = resolve;
        this._onGetStatsFailure = reject;
        this._impl.getStats(selector);
      }));
    });
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

    if (dict.maxRetransmitTime !== null && dict.maxRetransmits !== null) {
      throw new this._win.DOMException(
          "Both maxRetransmitTime and maxRetransmits cannot be provided",
          "InvalidParameterError");
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
  }
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

  newError: function(message, code) {
    
    const reasonName = [
      "",
      "InternalError",
      "InvalidCandidateError",
      "InvalidParameterError",
      "InvalidStateError",
      "InvalidSessionDescriptionError",
      "IncompatibleSessionDescriptionError",
      "InternalError",
      "IncompatibleMediaStreamTrackError",
      "InternalError"
    ];
    let name = reasonName[Math.min(code, reasonName.length - 1)];
    return new this._dompc._win.DOMException(message, name);
  },

  dispatchEvent: function(event) {
    this._dompc.dispatchEvent(event);
  },

  onCreateOfferSuccess: function(sdp) {
    this._dompc._onCreateOfferSuccess(sdp);
  },

  onCreateOfferError: function(code, message) {
    this._dompc._onCreateOfferFailure(this.newError(message, code));
  },

  onCreateAnswerSuccess: function(sdp) {
    this._dompc._onCreateAnswerSuccess(sdp);
  },

  onCreateAnswerError: function(code, message) {
    this._dompc._onCreateAnswerFailure(this.newError(message, code));
  },

  onSetLocalDescriptionSuccess: function() {
    this._dompc._onSetLocalDescriptionSuccess();
  },

  onSetRemoteDescriptionSuccess: function() {
    this._dompc._onSetRemoteDescriptionSuccess();
  },

  onSetLocalDescriptionError: function(code, message) {
    this._localType = null;
    this._dompc._onSetLocalDescriptionFailure(this.newError(message, code));
  },

  onSetRemoteDescriptionError: function(code, message) {
    this._remoteType = null;
    this._dompc._onSetRemoteDescriptionFailure(this.newError(message, code));
  },

  onAddIceCandidateSuccess: function() {
    this._dompc._onAddIceCandidateSuccess();
  },

  onAddIceCandidateError: function(code, message) {
    this._dompc._onAddIceCandidateError(this.newError(message, code));
  },

  onIceCandidate: function(level, mid, candidate) {
    if (candidate == "") {
      this.foundIceCandidate(null);
    } else {
      this.foundIceCandidate(new this._dompc._win.mozRTCIceCandidate(
          {
              candidate: candidate,
              sdpMid: mid,
              sdpMLineIndex: level
          }
      ));
    }
  },

  onNegotiationNeeded: function() {
    this.dispatchEvent(new this._win.Event("negotiationneeded"));
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
        this.dispatchEvent(new this._win.Event("signalingstatechange"));
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
    this._dompc._onGetStatsSuccess(webidlobj);
  },

  onGetStatsError: function(code, message) {
    this._dompc._onGetStatsFailure(this.newError(message, code));
  },

  onAddStream: function(stream) {
    let ev = new this._dompc._win.MediaStreamEvent("addstream",
                                                   { stream: stream });
    this.dispatchEvent(ev);
  },

  onRemoveStream: function(stream, type) {
    this.dispatchEvent(new this._dompc._win.MediaStreamEvent("removestream",
                                                             { stream: stream }));
  },

  onAddTrack: function(track) {
    let ev = new this._dompc._win.MediaStreamTrackEvent("addtrack",
                                                        { track: track });
    this.dispatchEvent(ev);
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
    pc._onReplaceTrackSuccess();
  },

  onReplaceTrackError: function(code, message) {
    var pc = this._dompc;
    pc._onReplaceTrackWithTrack = null;
    pc._onReplaceTrackSender = null;
    pc._onReplaceTrackFailure(this.newError(message, code));
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

  replaceTrack: function(withTrack) {
    return this._pc._chain(() => this._pc._replaceTrack(this, withTrack));
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
   PeerConnectionObserver]
);
