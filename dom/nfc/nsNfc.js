





"use strict";

const DEBUG = false;
function debug(s) {
  if (DEBUG) dump("-*- Nfc DOM: " + s + "\n");
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

function NfcCallback(aWindow) {
  this.initDOMRequestHelper(aWindow, null);
  this._createPromise();
}
NfcCallback.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  promise: null,
  _requestId: null,

  _createPromise: function _createPromise() {
    this.promise = this.createPromise((aResolve, aReject) => {
      this._requestId = btoa(this.getPromiseResolverId({
        resolve: aResolve,
        reject: aReject
      }));
    });
  },

  getCallbackId: function getCallbackId() {
    return this._requestId;
  },

  notifySuccess: function notifySuccess() {
    let resolver = this.takePromiseResolver(atob(this._requestId));
    if (!resolver) {
      debug("can not find promise resolver for id: " + this._requestId);
      return;
    }
    resolver.resolve();
  },

  notifySuccessWithBoolean: function notifySuccessWithBoolean(aResult) {
    let resolver = this.takePromiseResolver(atob(this._requestId));
    if (!resolver) {
      debug("can not find promise resolver for id: " + this._requestId);
      return;
    }
    resolver.resolve(aResult);
  },

  notifySuccessWithNDEFRecords: function notifySuccessWithNDEFRecords(aRecords) {
    let resolver = this.takePromiseResolver(atob(this._requestId));
    if (!resolver) {
      debug("can not find promise resolver for id: " + this._requestId);
      return;
    }
    resolver.resolve(aRecords);
  },

  notifyError: function notifyError(aErrorMsg) {
    let resolver = this.takePromiseResolver(atob(this._requestId));
    if (!resolver) {
      debug("can not find promise resolver for id: " + this._requestId +
           ", errormsg: " + aErrorMsg);
      return;
    }
    resolver.reject(aErrorMsg);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver,
                                         Ci.nsINfcRequestCallback]),
};








function MozNFCTagImpl(window, sessionToken, event) {
  debug("In MozNFCTagImpl Constructor");
  this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                             .getService(Ci.nsINfcContentHelper);
  this._window = window;
  this.session = sessionToken;
  this.techList = event.techList;
  this.type = event.tagType || null;
  this.maxNDEFSize = event.maxNDEFSize || null;
  this.isReadOnly = event.isReadOnly || null;
  this.isFormatable = event.isFormatable || null;
  this.canBeMadeReadOnly = this.type ?
                             (this.type == "type1" || this.type == "type2" ||
                              this.type == "mifare_classic") :
                             null;
}
MozNFCTagImpl.prototype = {
  _nfcContentHelper: null,
  _window: null,
  session: null,
  techList: null,
  type: null,
  maxNDEFSize: 0,
  isReadOnly: false,
  isFormatable: false,
  canBeMadeReadOnly: false,
  isLost: false,

  
  readNDEF: function readNDEF() {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCTag object is invalid");
    }

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.readNDEF(this.session, callback);
    return callback.promise;
  },

  writeNDEF: function writeNDEF(records) {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCTag object is invalid");
    }

    if (this.isReadOnly) {
      throw new this._window.DOMError("InvalidAccessError", "NFCTag object is read-only");
    }

    let ndefLen = 0;
    for (let record of records) {
      ndefLen += record.size;
    }

    if (ndefLen > this.maxNDEFSize) {
      throw new this._window.DOMError("NotSupportedError", "Exceed max NDEF size");
    }

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.writeNDEF(records, this.session, callback);
    return callback.promise;
  },

  makeReadOnly: function makeReadOnly() {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCTag object is invalid");
    }

    if (!this.canBeMadeReadOnly) {
      throw new this._window.DOMError("InvalidAccessError",
                                      "NFCTag object cannot be made read-only");
    }

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.makeReadOnly(this.session, callback);
    return callback.promise;
  },

  format: function format() {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCTag object is invalid");
    }

    if (!this.isFormatable) {
      throw new this._window.DOMError("InvalidAccessError",
                                      "NFCTag object is not formatable");
    }

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.format(this.session, callback);
    return callback.promise;
  },

  classID: Components.ID("{4e1e2e90-3137-11e3-aa6e-0800200c9a66}"),
  contractID: "@mozilla.org/nfc/NFCTag;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
};







function MozNFCPeerImpl(aWindow, aSessionToken) {
  debug("In MozNFCPeerImpl Constructor");
  this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                             .getService(Ci.nsINfcContentHelper);

  this._window = aWindow;
  this.session = aSessionToken;
}
MozNFCPeerImpl.prototype = {
  _nfcContentHelper: null,
  _window: null,
  isLost: false,

  
  sendNDEF: function sendNDEF(records) {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCPeer object is invalid");
    }

    
    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.writeNDEF(records, this.session, callback);
    return callback.promise;
  },

  sendFile: function sendFile(blob) {
    if (this.isLost) {
      throw new this._window.DOMError("InvalidStateError", "NFCPeer object is invalid");
    }

    let data = {
      "blob": blob
    };

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.sendFile(Cu.cloneInto(data, this._window),
                                    this.session, callback);
    return callback.promise;
  },

  classID: Components.ID("{c1b2bcf0-35eb-11e3-aa6e-0800200c9a66}"),
  contractID: "@mozilla.org/nfc/NFCPeer;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
};




function MozNFCImpl() {
  debug("In MozNFCImpl Constructor");
  try {
    this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                               .getService(Ci.nsINfcContentHelper);
  } catch(e) {
    debug("No NFC support.")
  }

  this.eventService = Cc["@mozilla.org/eventlistenerservice;1"]
                        .getService(Ci.nsIEventListenerService);
  this._nfcContentHelper.addEventListener(this);
}
MozNFCImpl.prototype = {
  _nfcContentHelper: null,
  _window: null,
  _rfState: null,
  nfcPeer: null,
  nfcTag: null,
  eventService: null,

  
  rfState: {
    IDLE: "idle",
    LISTEN: "listen",
    DISCOVERY: "discovery"
  },

  init: function init(aWindow) {
    debug("MozNFCImpl init called");
    this._window = aWindow;
    this.defineEventHandlerGetterSetter("ontagfound");
    this.defineEventHandlerGetterSetter("ontaglost");
    this.defineEventHandlerGetterSetter("onpeerready");
    this.defineEventHandlerGetterSetter("onpeerfound");
    this.defineEventHandlerGetterSetter("onpeerlost");

    if (this._nfcContentHelper) {
      this._nfcContentHelper.init(aWindow);
      this._rfState = this._nfcContentHelper.queryRFState();
    }
  },

  
  
  
  checkP2PRegistration: function checkP2PRegistration(manifestUrl) {
    
    let appID = appsService.getAppLocalIdByManifestURL(manifestUrl);

    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.checkP2PRegistration(appID, callback);
    return callback.promise;
  },

  notifyUserAcceptedP2P: function notifyUserAcceptedP2P(manifestUrl) {
    let appID = appsService.getAppLocalIdByManifestURL(manifestUrl);
    
    this._nfcContentHelper.notifyUserAcceptedP2P(appID);
  },

  notifySendFileStatus: function notifySendFileStatus(status, requestId) {
    this._nfcContentHelper.notifySendFileStatus(status, requestId);
  },

  startPoll: function startPoll() {
    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.changeRFState(this.rfState.DISCOVERY, callback);
    return callback.promise;
  },

  stopPoll: function stopPoll() {
    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.changeRFState(this.rfState.LISTEN, callback);
    return callback.promise;
  },

  powerOff: function powerOff() {
    let callback = new NfcCallback(this._window);
    this._nfcContentHelper.changeRFState(this.rfState.IDLE, callback);
    return callback.promise;
  },

  _createNFCPeer: function _createNFCPeer(sessionToken) {
    let peer = new MozNFCPeerImpl(this._window, sessionToken);
    return this._window.MozNFCPeer._create(this._window, peer);
  },

  getNFCPeer: function getNFCPeer(sessionToken) {
    if (!sessionToken || !this._nfcContentHelper.checkSessionToken(sessionToken, true)) {
      return null;
    }

    if (!this.nfcPeer || this.nfcPeer.session != sessionToken) {
      this.nfcPeer = this._createNFCPeer(sessionToken);
    }

    return this.nfcPeer;
  },

  defineEventHandlerGetterSetter: function defineEventHandlerGetterSetter(name) {
    Object.defineProperty(this, name, {
      get: function get() {
        return this.__DOM_IMPL__.getEventHandler(name);
      },
      set: function set(handler) {
        this.__DOM_IMPL__.setEventHandler(name, handler);
      }
    });
  },

  eventListenerWasAdded: function(eventType) {
    if (eventType !== "peerready") {
      return;
    }

    let appId = this._window.document.nodePrincipal.appId;
    this._nfcContentHelper.registerTargetForPeerReady(appId);
  },

  eventListenerWasRemoved: function(eventType) {
    if (eventType !== "peerready") {
      return;
    }

    let appId = this._window.document.nodePrincipal.appId;
    this._nfcContentHelper.unregisterTargetForPeerReady(appId);
  },

  notifyTagFound: function notifyTagFound(sessionToken, event, records) {
    if (this.hasDeadWrapper()) {
      dump("this._window or this.__DOM_IMPL__ is a dead wrapper.");
      return;
    }

    if (!this.eventService.hasListenersFor(this.__DOM_IMPL__, "tagfound")) {
      debug("ontagfound is not registered.");
      return;
    }

    if (!this.checkPermissions(["nfc-read", "nfc-write"])) {
      return;
    }

    this.eventService.addSystemEventListener(this._window, "visibilitychange",
      this, false);

    let tagImpl = new MozNFCTagImpl(this._window, sessionToken, event);
    let tag = this._window.MozNFCTag._create(this._window, tagImpl);
    this.nfcTag = tag;

    let length = records ? records.length : 0;
    let ndefRecords = records ? [] : null;
    for (let i = 0; i < length; i++) {
      let record = records[i];
      ndefRecords.push(new this._window.MozNDEFRecord({tnf: record.tnf,
                                                       type: record.type,
                                                       id: record.id,
                                                       payload: record.payload}));
    }

    let eventData = {
      "tag": tag,
      "ndefRecords": ndefRecords
    };

    debug("fire ontagfound " + sessionToken);
    let tagEvent = new this._window.MozNFCTagEvent("tagfound", eventData);
    this.__DOM_IMPL__.dispatchEvent(tagEvent);
  },

  notifyTagLost: function notifyTagLost(sessionToken) {
    if (this.hasDeadWrapper()) {
      dump("this._window or this.__DOM_IMPL__ is a dead wrapper.");
      return;
    }

    if (!this.checkPermissions(["nfc-read", "nfc-write"])) {
      return;
    }

    if (!this.nfcTag) {
      debug("No NFCTag object existing.");
      return;
    }

    
    if (!this.nfcPeer) {
      this.eventService.removeSystemEventListener(this._window, "visibilitychange",
        this, false);
    }

    this.nfcTag.isLost = true;
    this.nfcTag = null;

    debug("fire ontaglost " + sessionToken);
    let event = new this._window.Event("taglost");
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  notifyPeerFound: function notifyPeerFound(sessionToken, isPeerReady) {
    if (this.hasDeadWrapper()) {
      dump("this._window or this.__DOM_IMPL__ is a dead wrapper.");
      return;
    }

    if (!isPeerReady &&
        !this.eventService.hasListenersFor(this.__DOM_IMPL__, "peerfound")) {
      debug("onpeerfound is not registered.");
      return;
    }

    if (!this.checkPermissions(["nfc-write"])) {
      return;
    }

    this.eventService.addSystemEventListener(this._window, "visibilitychange",
      this, false);

    this.nfcPeer = this._createNFCPeer(sessionToken);
    let eventData = { "peer": this.nfcPeer };
    let type = (isPeerReady) ? "peerready" : "peerfound";

    debug("fire on" + type + " " + sessionToken);
    let event = new this._window.MozNFCPeerEvent(type, eventData);
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  notifyPeerLost: function notifyPeerLost(sessionToken) {
    if (this.hasDeadWrapper()) {
      dump("this._window or this.__DOM_IMPL__ is a dead wrapper.");
      return;
    }

    if (!this.checkPermissions(["nfc-write"])) {
      return;
    }

    if (!this.nfcPeer) {
      debug("No NFCPeer object existing.");
      return;
    }

    
    if (!this.nfcTag) {
      this.eventService.removeSystemEventListener(this._window, "visibilitychange",
        this, false);
    }

    this.nfcPeer.isLost = true;
    this.nfcPeer = null;

    debug("fire onpeerlost");
    let event = new this._window.Event("peerlost");
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  handleEvent: function handleEvent (event) {
    if (!this._window.document.hidden) {
      return;
    }

    if (this.nfcTag) {
      debug("handleEvent notifyTagLost");
      this.notifyTagLost(this.nfcTag.session);
    }

    if (this.nfcPeer) {
      debug("handleEvent notifyPeerLost");
      this.notifyPeerLost(this.nfcPeer.session);
    }
  },

  notifyRFStateChange: function notifyRFStateChange(rfState) {
    this._rfState = rfState;
  },

  checkPermissions: function checkPermissions(perms) {
    let principal = this._window.document.nodePrincipal;
    for (let perm of perms) {
      let permValue =
        Services.perms.testExactPermissionFromPrincipal(principal, perm);
      if (permValue == Ci.nsIPermissionManager.ALLOW_ACTION) {
        return true;
      } else {
        debug("doesn't have " + perm + " permission.");
      }
    }

    return false;
  },

  hasDeadWrapper: function hasDeadWrapper() {
    return Cu.isDeadWrapper(this._window) || Cu.isDeadWrapper(this.__DOM_IMPL__);
  },

  classID: Components.ID("{6ff2b290-2573-11e3-8224-0800200c9a66}"),
  contractID: "@mozilla.org/navigatorNfc;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsINfcEventListener,
                                         Ci.nsIDOMEventListener]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MozNFCTagImpl,
  MozNFCPeerImpl, MozNFCImpl]);
