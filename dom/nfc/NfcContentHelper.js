
















"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

Cu.import("resource://gre/modules/systemlibs.js");
const NFC_ENABLED = libcutils.property_get("ro.moz.nfc.enabled", "false") === "true";


let DEBUG = NFC.DEBUG_CONTENT_HELPER;

let debug;
function updateDebug() {
  if (DEBUG) {
    debug = function (s) {
      dump("-*- NfcContentHelper: " + s + "\n");
    };
  } else {
    debug = function (s) {};
  }
};
updateDebug();

const NFCCONTENTHELPER_CID =
  Components.ID("{4d72c120-da5f-11e1-9b23-0800200c9a66}");

const NFC_IPC_MSG_NAMES = [
  "NFC:ReadNDEFResponse",
  "NFC:WriteNDEFResponse",
  "NFC:MakeReadOnlyResponse",
  "NFC:FormatResponse",
  "NFC:ConnectResponse",
  "NFC:CloseResponse",
  "NFC:CheckP2PRegistrationResponse",
  "NFC:DOMEvent",
  "NFC:NotifySendFileStatusResponse",
  "NFC:ChangeRFStateResponse"
];

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function NfcContentHelper() {
  Services.obs.addObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED, false);
  Services.obs.addObserver(this, "xpcom-shutdown", false);

  this._requestMap = [];
}

NfcContentHelper.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINfcContentHelper,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver]),
  classID:   NFCCONTENTHELPER_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          NFCCONTENTHELPER_CID,
    classDescription: "NfcContentHelper",
    interfaces:       [Ci.nsINfcContentHelper]
  }),

  _window: null,
  _requestMap: null,
  eventListener: null,

  init: function init(aWindow) {
    if (aWindow == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }
    this._window = aWindow;
    this.initDOMRequestHelper(this._window, NFC_IPC_MSG_NAMES);

    if (this._window.navigator.mozSettings) {
      let lock = this._window.navigator.mozSettings.createLock();
      var nfcDebug = lock.get(NFC.SETTING_NFC_DEBUG);
      nfcDebug.onsuccess = function _nfcDebug() {
        DEBUG = nfcDebug.result[NFC.SETTING_NFC_DEBUG];
        updateDebug();
      };
    }
  },

  encodeNDEFRecords: function encodeNDEFRecords(records) {
    let encodedRecords = [];
    for (let i = 0; i < records.length; i++) {
      let record = records[i];
      encodedRecords.push({
        tnf: record.tnf,
        type: record.type || undefined,
        id: record.id || undefined,
        payload: record.payload || undefined,
      });
    }
    return encodedRecords;
  },

  
  checkSessionToken: function checkSessionToken(sessionToken, isP2P) {
    if (sessionToken == null) {
      throw Components.Exception("No session token!",
                                  Cr.NS_ERROR_UNEXPECTED);
      return false;
    }
    
    let val = cpmm.sendSyncMessage("NFC:CheckSessionToken", {
      sessionToken: sessionToken,
      isP2P: isP2P
    });
    return (val[0] === NFC.NFC_GECKO_SUCCESS);
  },

  
  readNDEF: function readNDEF(sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:ReadNDEF", {
      requestId: requestId,
      sessionToken: sessionToken
    });
    return request;
  },

  writeNDEF: function writeNDEF(records, sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    let encodedRecords = this.encodeNDEFRecords(records);
    cpmm.sendAsyncMessage("NFC:WriteNDEF", {
      requestId: requestId,
      sessionToken: sessionToken,
      records: encodedRecords
    });
    return request;
  },

  makeReadOnlyNDEF: function makeReadOnlyNDEF(sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:MakeReadOnly", {
      requestId: requestId,
      sessionToken: sessionToken
    });
    return request;
  },

  format: function format(sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:Format", {
      requestId: requestId,
      sessionToken: sessionToken
    });
    return request;
  },

  connect: function connect(techType, sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:Connect", {
      requestId: requestId,
      sessionToken: sessionToken,
      techType: techType
    });
    return request;
  },

  close: function close(sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:Close", {
      requestId: requestId,
      sessionToken: sessionToken
    });
    return request;
  },

  sendFile: function sendFile(data, sessionToken) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:SendFile", {
      requestId: requestId,
      sessionToken: sessionToken,
      blob: data.blob
    });
    return request;
  },

  notifySendFileStatus: function notifySendFileStatus(status, requestId) {
    cpmm.sendAsyncMessage("NFC:NotifySendFileStatus", {
      status: status,
      requestId: requestId
    });
  },

  addEventListener: function addEventListener(listener) {
    this.eventListener = listener;
    cpmm.sendAsyncMessage("NFC:AddEventListener");
  },

  registerTargetForPeerReady: function registerTargetForPeerReady(appId) {
    cpmm.sendAsyncMessage("NFC:RegisterPeerReadyTarget", { appId: appId });
  },

  unregisterTargetForPeerReady: function unregisterTargetForPeerReady(appId) {
    cpmm.sendAsyncMessage("NFC:UnregisterPeerReadyTarget", { appId: appId });
  },

  checkP2PRegistration: function checkP2PRegistration(appId) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:CheckP2PRegistration", {
      appId: appId,
      requestId: requestId
    });
    return request;
  },

  notifyUserAcceptedP2P: function notifyUserAcceptedP2P(appId) {
    cpmm.sendAsyncMessage("NFC:NotifyUserAcceptedP2P", {
      appId: appId
    });
  },

  changeRFState: function changeRFState(rfState) {
    let request = Services.DOMRequest.createRequest(this._window);
    let requestId = btoa(this.getRequestId(request));
    this._requestMap[requestId] = this._window;

    cpmm.sendAsyncMessage("NFC:ChangeRFState",
                          {requestId: requestId,
                           rfState: rfState});
    return request;

  },

  
  observe: function observe(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      this.destroyDOMRequestHelper();
      Services.obs.removeObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED);
      Services.obs.removeObserver(this, "xpcom-shutdown");
      cpmm = null;
    } else if (topic == NFC.TOPIC_MOZSETTINGS_CHANGED) {
      if ("wrappedJSObject" in subject) {
        subject = subject.wrappedJSObject;
      }
      if (subject) {
        this.handle(subject.key, subject.value);
      }
    }
  },

  

  fireRequestSuccess: function fireRequestSuccess(requestId, result) {
    let request = this.takeRequest(requestId);
    if (!request) {
      debug("not firing success for id: " + requestId);
      return;
    }

    debug("fire request success, id: " + requestId);
    Services.DOMRequest.fireSuccess(request, result);
  },

  fireRequestError: function fireRequestError(requestId, errorMsg) {
    let request = this.takeRequest(requestId);
    if (!request) {
      debug("not firing error for id: " + requestId +
            ", errormsg: " + errorMsg);
      return;
    }

    debug("fire request error, id: " + requestId +
          ", errormsg: " + errorMsg);
    Services.DOMRequest.fireError(request, errorMsg);
  },

  receiveMessage: function receiveMessage(message) {
    DEBUG && debug("Message received: " + JSON.stringify(message));
    let result = message.json;

    switch (message.name) {
      case "NFC:ReadNDEFResponse":
        this.handleReadNDEFResponse(result);
        break;
      case "NFC:CheckP2PRegistrationResponse":
        this.handleCheckP2PRegistrationResponse(result);
        break;
      case "NFC:ConnectResponse": 
      case "NFC:CloseResponse":
      case "NFC:WriteNDEFResponse":
      case "NFC:MakeReadOnlyResponse":
      case "NFC:FormatResponse":
      case "NFC:NotifySendFileStatusResponse":
      case "NFC:ChangeRFStateResponse":
        if (result.errorMsg) {
          this.fireRequestError(atob(result.requestId), result.errorMsg);
        } else {
          this.fireRequestSuccess(atob(result.requestId), result);
        }
        break;
      case "NFC:DOMEvent":
        switch (result.event) {
          case NFC.PEER_EVENT_READY:
            this.eventListener.notifyPeerFound(result.sessionToken,  true);
            break;
          case NFC.PEER_EVENT_FOUND:
            this.eventListener.notifyPeerFound(result.sessionToken);
            break;
          case NFC.PEER_EVENT_LOST:
            this.eventListener.notifyPeerLost(result.sessionToken);
            break;
          case NFC.TAG_EVENT_FOUND:
            let event = new NfcTagEvent(result.techList,
                                        result.tagType,
                                        result.maxNDEFSize,
                                        result.isReadOnly,
                                        result.isFormatable);

            this.eventListener.notifyTagFound(result.sessionToken, event, result.records);
            break;
          case NFC.TAG_EVENT_LOST:
            this.eventListener.notifyTagLost(result.sessionToken);
            break;
        }
        break;
    }
  },

  handle: function handle(name, result) {
    switch (name) {
      case NFC.SETTING_NFC_DEBUG:
        DEBUG = result;
        updateDebug();
        break;
    }
  },

  handleReadNDEFResponse: function handleReadNDEFResponse(result) {
    let requester = this._requestMap[result.requestId];
    if (!requester) {
      debug("Response Invalid requestId=" + result.requestId);
      return;
    }
    delete this._requestMap[result.requestId];

    if (result.errorMsg) {
      this.fireRequestError(atob(result.requestId), result.errorMsg);
      return;
    }

    let requestId = atob(result.requestId);
    let ndefMsg = [];
    let records = result.records;
    for (let i = 0; i < records.length; i++) {
      let record = records[i];
      ndefMsg.push(new requester.MozNDEFRecord({tnf: record.tnf,
                                                type: record.type,
                                                id: record.id,
                                                payload: record.payload}));
    }
    this.fireRequestSuccess(requestId, ndefMsg);
  },

  handleCheckP2PRegistrationResponse: function handleCheckP2PRegistrationResponse(result) {
    
    
    let requestId = atob(result.requestId);
    this.fireRequestSuccess(requestId, !result.errorMsg);
  },
};

function NfcTagEvent(techList, tagType, maxNDEFSize, isReadOnly, isFormatable) {
  this.techList = techList;
  this.tagType = tagType;
  this.maxNDEFSize = maxNDEFSize;
  this.isReadOnly = isReadOnly;
  this.isFormatable = isFormatable;
}
NfcTagEvent.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINfcTagEvent]),

  techList: null,
  tagType: null,
  maxNDEFSize: 0,
  isReadOnly: false,
  isFormatable: false
};

if (NFC_ENABLED) {
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NfcContentHelper]);
}
