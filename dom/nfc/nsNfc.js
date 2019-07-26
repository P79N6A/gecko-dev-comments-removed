





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
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");
const NFC_PEER_EVENT_READY = 0x01;
const NFC_PEER_EVENT_LOST  = 0x02;




function MozNFCTag() {
  debug("In MozNFCTag Constructor");
  this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                             .getService(Ci.nsINfcContentHelper);
  this.session = null;

  
  this._techTypesMap = [];
  this._techTypesMap['NFC_A'] = 0;
  this._techTypesMap['NFC_B'] = 1;
  this._techTypesMap['NFC_ISO_DEP'] = 2;
  this._techTypesMap['NFC_F'] = 3;
  this._techTypesMap['NFC_V'] = 4;
  this._techTypesMap['NDEF'] = 5;
  this._techTypesMap['NDEF_FORMATABLE'] = 6;
  this._techTypesMap['MIFARE_CLASSIC'] = 7;
  this._techTypesMap['MIFARE_ULTRALIGHT'] = 8;
  this._techTypesMap['NFC_BARCODE'] = 9;
  this._techTypesMap['P2P'] = 10;
}
MozNFCTag.prototype = {
  _nfcContentHelper: null,
  _window: null,

  initialize: function(aWindow, aSessionToken) {
    this._window = aWindow;
    this.setSessionToken(aSessionToken);
  },

  
  setSessionToken: function setSessionToken(aSessionToken) {
    debug("Setting session token.");
    this.session = aSessionToken;
    
    this._nfcContentHelper.setSessionToken(aSessionToken);
  },

  _techTypesMap: null,

  
  getDetailsNDEF: function getDetailsNDEF() {
    return this._nfcContentHelper.getDetailsNDEF(this._window, this.session);
  },
  readNDEF: function readNDEF() {
    return this._nfcContentHelper.readNDEF(this._window, this.session);
  },
  writeNDEF: function writeNDEF(records) {
    return this._nfcContentHelper.writeNDEF(this._window, records, this.session);
  },
  makeReadOnlyNDEF: function makeReadOnlyNDEF() {
    return this._nfcContentHelper.makeReadOnlyNDEF(this._window, this.session);
  },
  connect: function connect(enum_tech_type) {
    let int_tech_type = this._techTypesMap[enum_tech_type];
    return this._nfcContentHelper.connect(this._window, int_tech_type, this.session);
  },
  close: function close() {
    return this._nfcContentHelper.close(this._window, this.session);
  },

  classID: Components.ID("{4e1e2e90-3137-11e3-aa6e-0800200c9a66}"),
  contractID: "@mozilla.org/nfc/NFCTag;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
};




function MozNFCPeer() {
  debug("In MozNFCPeer Constructor");
  this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                             .getService(Ci.nsINfcContentHelper);
  this.session = null;
}
MozNFCPeer.prototype = {
  _nfcContentHelper: null,
  _window: null,

  initialize: function(aWindow, aSessionToken) {
    this._window = aWindow;
    this.setSessionToken(aSessionToken);
  },

  
  setSessionToken: function setSessionToken(aSessionToken) {
    debug("Setting session token.");
    this.session = aSessionToken;
    
    return this._nfcContentHelper.setSessionToken(aSessionToken);
  },

  
  sendNDEF: function sendNDEF(records) {
    
    return this._nfcContentHelper.writeNDEF(this._window, records, this.session);
  },

  sendFile: function sendFile(blob) {
    let data = {
      "blob": blob.slice()
    };
    return this._nfcContentHelper.sendFile(this._window,
                                           ObjectWrapper.wrap(data, this._window),
                                           this.session);
  },

  classID: Components.ID("{c1b2bcf0-35eb-11e3-aa6e-0800200c9a66}"),
  contractID: "@mozilla.org/nfc/NFCPeer;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
};




function mozNfc() {
  debug("In mozNfc Constructor");
  try {
    this._nfcContentHelper = Cc["@mozilla.org/nfc/content-helper;1"]
                               .getService(Ci.nsINfcContentHelper);
  } catch(e) {
    debug("No NFC support.")
  }
}
mozNfc.prototype = {
  _nfcContentHelper: null,
  _window: null,
  _wrap: function _wrap(obj) {
    return ObjectWrapper.wrap(obj, this._window);
  },

  init: function init(aWindow) {
    debug("mozNfc init called");
    this._window = aWindow;
    let origin = this._window.document.nodePrincipal.origin;
    
    if (origin !== 'app://system.gaiamobile.org') {
      return;
    }
    let self = this;
    this._window.addEventListener("nfc-p2p-user-accept", function (event) {
      let appID = appsService.getAppLocalIdByManifestURL(event.detail.manifestUrl);
      
      self._nfcContentHelper.notifyUserAcceptedP2P(self._window, appID);
    });
  },

  checkP2PRegistration: function checkP2PRegistration(manifestUrl) {
    
    let appID = appsService.getAppLocalIdByManifestURL(manifestUrl);
    return this._nfcContentHelper.checkP2PRegistration(this._window, appID);
  },

  getNFCTag: function getNFCTag(sessionToken) {
    let obj = new MozNFCTag();
    let nfcTag = this._window.MozNFCTag._create(this._window, obj);
    if (nfcTag) {
      obj.initialize(this._window, sessionToken);
      return nfcTag;
    } else {
      debug("Error: Unable to create NFCTag");
      return null;
    }
  },

  getNFCPeer: function getNFCPeer(sessionToken) {
    let obj = new MozNFCPeer();
    let nfcPeer = this._window.MozNFCPeer._create(this._window, obj);
    if (nfcPeer) {
      obj.initialize(this._window, sessionToken);
      return nfcPeer;
    } else {
      debug("Error: Unable to create NFCPeer");
      return null;
    }
  },

  
  get onpeerready() {
    return this.__DOM_IMPL__.getEventHandler("onpeerready");
  },

  set onpeerready(handler) {
    this.__DOM_IMPL__.setEventHandler("onpeerready", handler);
  },

  
  get onpeerlost() {
    return this.__DOM_IMPL__.getEventHandler("onpeerlost");
  },

  set onpeerlost(handler) {
    this.__DOM_IMPL__.setEventHandler("onpeerlost", handler);
  },

  eventListenerWasAdded: function(evt) {
    let eventType = this.getEventType(evt);
    if (eventType == -1)
      return;
    this.registerTarget(eventType);
  },

  eventListenerWasRemoved: function(evt) {
    let eventType = this.getEventType(evt);
    if (eventType == -1)
      return;
    this.unregisterTarget(eventType);
  },

  registerTarget: function registerTarget(event) {
    let self = this;
    let appId = this._window.document.nodePrincipal.appId;
    this._nfcContentHelper.registerTargetForPeerEvent(this._window, appId,
      event, function(evt, sessionToken) {
        self.session = sessionToken;
        self.firePeerEvent(evt, sessionToken);
    });
  },

  unregisterTarget: function unregisterTarget(event) {
    let appId = this._window.document.nodePrincipal.appId;
    this._nfcContentHelper.unregisterTargetForPeerEvent(this._window,
                                                        appId, event);
  },

  getEventType: function getEventType(evt) {
    let eventType = -1;
    switch (evt) {
      case 'peerready':
        eventType = NFC_PEER_EVENT_READY;
        break;
      case 'peerlost':
        eventType = NFC_PEER_EVENT_LOST;
        break;
      default:
        break;
    }
    return eventType;
  },

  firePeerEvent: function firePeerEvent(evt, sessionToken) {
    let peerEvent = (NFC_PEER_EVENT_READY === evt) ? "peerready" : "peerlost";
    let detail = {
      "detail":sessionToken
    };
    let event = new this._window.CustomEvent(peerEvent,
      ObjectWrapper.wrap(detail, this._window));
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  classID: Components.ID("{6ff2b290-2573-11e3-8224-0800200c9a66}"),
  contractID: "@mozilla.org/navigatorNfc;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MozNFCTag, MozNFCPeer, mozNfc]);
