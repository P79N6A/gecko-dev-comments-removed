














"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";

let DEBUG;
function debug(s) {
  dump("-*- RILContentHelper: " + s + "\n");
}

const RILCONTENTHELPER_CID =
  Components.ID("{472816e1-1fd6-4405-996c-806f9ea68174}");

const RIL_IPC_MSG_NAMES = [
  "RIL:CardStateChanged",
  "RIL:IccInfoChanged",
  "RIL:GetCardLockResult",
  "RIL:SetUnlockCardLockResult",
  "RIL:CardLockRetryCount",
  "RIL:StkCommand",
  "RIL:StkSessionEnd",
  "RIL:IccOpenChannel",
  "RIL:IccCloseChannel",
  "RIL:IccExchangeAPDU",
  "RIL:ReadIccContacts",
  "RIL:UpdateIccContact",
  "RIL:MatchMvno",
  "RIL:GetServiceState"
];


XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");


XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                  "@mozilla.org/uuid-generator;1",
                  "nsIUUIDGenerator");


XPCOMUtils.defineLazyGetter(this, "gNumRadioInterfaces", function() {
  let appInfo = Cc["@mozilla.org/xre/app-info;1"];
  let isParentProcess = !appInfo || appInfo.getService(Ci.nsIXULRuntime)
                          .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

  if (isParentProcess) {
    let ril = { numRadioInterfaces: 0 };
    try {
      ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
    } catch(e) {}
    return ril.numRadioInterfaces;
  }

  return Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);
});

function IccInfo() {}
IccInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccInfo]),

  

  iccType: null,
  iccid: null,
  mcc: null,
  mnc: null,
  spn: null,
  isDisplayNetworkNameRequired: false,
  isDisplaySpnRequired: false
};

function GsmIccInfo() {}
GsmIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGsmIccInfo,
                                         Ci.nsIIccInfo]),

  

  msisdn: null
};

function CdmaIccInfo() {}
CdmaIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICdmaIccInfo,
                                         Ci.nsIIccInfo]),

  

  mdn: null,
  prlVersion: 0
};

function RILContentHelper() {
  this.updateDebugFlag();

  this.numClients = gNumRadioInterfaces;
  if (DEBUG) debug("Number of clients: " + this.numClients);

  this._iccs = [];
  this.rilContexts = [];
  for (let clientId = 0; clientId < this.numClients; clientId++) {
    this._iccs.push(new Icc(this, clientId));
    this.rilContexts[clientId] = {
      cardState: Ci.nsIIcc.CARD_STATE_UNKNOWN,
      iccInfo: null
    };
  }

  this.initDOMRequestHelper( null, RIL_IPC_MSG_NAMES);
  this._windowsMap = [];
  this._requestMap = [];
  this._iccListeners = [];
  this._iccChannelCallback = [];

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
}

RILContentHelper.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccProvider,
                                         Ci.nsIIccService,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  classID:   RILCONTENTHELPER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILCONTENTHELPER_CID,
                                    classDescription: "RILContentHelper",
                                    interfaces: [Ci.nsIIccProvider,
                                                 Ci.nsIIccService]}),

  updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_CONTENT_HELPER ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  
  updateInfo: function(srcInfo, destInfo) {
    for (let key in srcInfo) {
      destInfo[key] = srcInfo[key];
    }
  },

  




  updateIccInfo: function(clientId, newInfo) {
    let rilContext = this.rilContexts[clientId];

    
    if (!newInfo || !newInfo.iccid) {
      if (rilContext.iccInfo) {
        rilContext.iccInfo = null;
      }
      return;
    }

    
    if (!rilContext.iccInfo) {
      if (newInfo.iccType === "ruim" || newInfo.iccType === "csim") {
        rilContext.iccInfo = new CdmaIccInfo();
      } else if (newInfo.iccType === "sim" || newInfo.iccType === "usim") {
        rilContext.iccInfo = new GsmIccInfo();
      } else {
        rilContext.iccInfo = new IccInfo();
      }
    }

    this.updateInfo(newInfo, rilContext.iccInfo);
  },

  _windowsMap: null,

  _requestMap: null,

  rilContexts: null,

  getRilContext: function(clientId) {
    
    
    
    this.getRilContext = function getRilContext(clientId) {
      return this.rilContexts[clientId];
    };

    for (let cId = 0; cId < this.numClients; cId++) {
      let rilContext =
        cpmm.sendSyncMessage("RIL:GetRilContext", {clientId: cId})[0];
      if (!rilContext) {
        if (DEBUG) debug("Received null rilContext from chrome process.");
        continue;
      }
      this.rilContexts[cId].cardState = rilContext.cardState;
      this.updateIccInfo(cId, rilContext.iccInfo);
    }

    return this.rilContexts[clientId];
  },

  



  sendStkResponse: function(clientId, window, command, response) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }
    response.command = command;
    cpmm.sendAsyncMessage("RIL:SendStkResponse", {
      clientId: clientId,
      data: response
    });
  },

  sendStkMenuSelection: function(clientId, window, itemIdentifier,
                                 helpRequested) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }
    cpmm.sendAsyncMessage("RIL:SendStkMenuSelection", {
      clientId: clientId,
      data: {
        itemIdentifier: itemIdentifier,
        helpRequested: helpRequested
      }
    });
  },

  sendStkTimerExpiration: function(clientId, window, timer) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }
    cpmm.sendAsyncMessage("RIL:SendStkTimerExpiration", {
      clientId: clientId,
      data: {
        timer: timer
      }
    });
  },

  sendStkEventDownload: function(clientId, window, event) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }
    cpmm.sendAsyncMessage("RIL:SendStkEventDownload", {
      clientId: clientId,
      data: {
        event: event
      }
    });
  },

  iccOpenChannel: function(clientId, aid, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._addIccChannelCallback(requestId, callback);

    cpmm.sendAsyncMessage("RIL:IccOpenChannel", {
      clientId: clientId,
      data: {
        requestId: requestId,
        aid: aid
      }
    });
  },

  iccExchangeAPDU: function(clientId, channel, cla, ins, p1, p2, p3, data, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._addIccChannelCallback(requestId, callback);

    if (!data) {
      if (DEBUG) debug('data is not set , p3 : ' + p3);
    }

    let apdu = {
      cla: cla,
      command: ins,
      p1: p1,
      p2: p2,
      p3: p3,
      data: data
    };

    
    cpmm.sendAsyncMessage("RIL:IccExchangeAPDU", {
      clientId: clientId,
      data: {
        requestId: requestId,
        channel: channel,
        apdu: apdu
      }
    });
  },

  iccCloseChannel: function(clientId, channel, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._addIccChannelCallback(requestId, callback);

    cpmm.sendAsyncMessage("RIL:IccCloseChannel", {
      clientId: clientId,
      data: {
        requestId: requestId,
        channel: channel
      }
    });
  },

  readContacts: function(clientId, window, contactType) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }

    let request = Services.DOMRequest.createRequest(window);
    let requestId = this.getRequestId(request);
    this._windowsMap[requestId] = window;

    cpmm.sendAsyncMessage("RIL:ReadIccContacts", {
      clientId: clientId,
      data: {
        requestId: requestId,
        contactType: contactType
      }
    });
    return request;
  },

  updateContact: function(clientId, window, contactType, contact, pin2) {
    if (window == null) {
      throw Components.Exception("Can't get window object",
                                  Cr.NS_ERROR_UNEXPECTED);
    }

    let request = Services.DOMRequest.createRequest(window);
    let requestId = this.getRequestId(request);
    this._windowsMap[requestId] = window;

    
    let iccContact = {};

    if (Array.isArray(contact.name) && contact.name[0]) {
      iccContact.alphaId = contact.name[0];
    }

    if (Array.isArray(contact.tel)) {
      iccContact.number = contact.tel[0] && contact.tel[0].value;
      let telArray = contact.tel.slice(1);
      let length = telArray.length;
      if (length > 0) {
        iccContact.anr = [];
      }
      for (let i = 0; i < telArray.length; i++) {
        iccContact.anr.push(telArray[i].value);
      }
    }

    if (Array.isArray(contact.email) && contact.email[0]) {
      iccContact.email = contact.email[0].value;
    }

    iccContact.contactId = contact.id;

    cpmm.sendAsyncMessage("RIL:UpdateIccContact", {
      clientId: clientId,
      data: {
        requestId: requestId,
        contactType: contactType,
        contact: iccContact,
        pin2: pin2
      }
    });

    return request;
  },

  _iccListeners: null,

  registerListener: function(listenerType, clientId, listener) {
    if (!this[listenerType]) {
      return;
    }
    let listeners = this[listenerType][clientId];
    if (!listeners) {
      listeners = this[listenerType][clientId] = [];
    }

    if (listeners.indexOf(listener) != -1) {
      throw new Error("Already registered this listener!");
    }

    listeners.push(listener);
    if (DEBUG) debug("Registered " + listenerType + " listener: " + listener);
  },

  unregisterListener: function(listenerType, clientId, listener) {
    if (!this[listenerType]) {
      return;
    }
    let listeners = this[listenerType][clientId];
    if (!listeners) {
      return;
    }

    let index = listeners.indexOf(listener);
    if (index != -1) {
      listeners.splice(index, 1);
      if (DEBUG) debug("Unregistered listener: " + listener);
    }
  },

  _iccChannelCallback: null,

  _addIccChannelCallback: function(requestId, channelCb) {
    let cbInterfaces = this._iccChannelCallback;
    if (!cbInterfaces[requestId] && channelCb) {
      cbInterfaces[requestId] = channelCb;
      return;
    }

    if (DEBUG) debug("Unable to add channelCbInterface for requestId : " + requestId);
  },

  _getIccChannelCallback: function(requestId) {
    let cb = this._iccChannelCallback[requestId];
    delete this._iccChannelCallback[requestId];
    return cb;
  },

  registerIccMsg: function(clientId, listener) {
    if (DEBUG) debug("Registering for ICC related messages");
    this.registerListener("_iccListeners", clientId, listener);
    cpmm.sendAsyncMessage("RIL:RegisterIccMsg");
  },

  unregisterIccMsg: function(clientId, listener) {
    this.unregisterListener("_iccListeners", clientId, listener);
  },

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (data == kPrefRilDebuggingEnabled) {
          this.updateDebugFlag();
        }
        break;

      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        this.destroyDOMRequestHelper();
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;
    }
  },

  

  fireRequestSuccess: function(requestId, result) {
    let request = this.takeRequest(requestId);
    if (!request) {
      if (DEBUG) {
        debug("not firing success for id: " + requestId +
              ", result: " + JSON.stringify(result));
      }
      return;
    }

    if (DEBUG) {
      debug("fire request success, id: " + requestId +
            ", result: " + JSON.stringify(result));
    }
    Services.DOMRequest.fireSuccess(request, result);
  },

  dispatchFireRequestSuccess: function(requestId, result) {
    let currentThread = Services.tm.currentThread;

    currentThread.dispatch(this.fireRequestSuccess.bind(this, requestId, result),
                           Ci.nsIThread.DISPATCH_NORMAL);
  },

  fireRequestError: function(requestId, error) {
    let request = this.takeRequest(requestId);
    if (!request) {
      if (DEBUG) {
        debug("not firing error for id: " + requestId +
              ", error: " + JSON.stringify(error));
      }
      return;
    }

    if (DEBUG) {
      debug("fire request error, id: " + requestId +
            ", result: " + JSON.stringify(error));
    }
    Services.DOMRequest.fireError(request, error);
  },

  dispatchFireRequestError: function(requestId, error) {
    let currentThread = Services.tm.currentThread;

    currentThread.dispatch(this.fireRequestError.bind(this, requestId, error),
                           Ci.nsIThread.DISPATCH_NORMAL);
  },

  fireRequestDetailedError: function(requestId, detailedError) {
    let request = this.takeRequest(requestId);
    if (!request) {
      if (DEBUG) {
        debug("not firing detailed error for id: " + requestId +
              ", detailedError: " + JSON.stringify(detailedError));
      }
      return;
    }

    Services.DOMRequest.fireDetailedError(request, detailedError);
  },

  receiveMessage: function(msg) {
    let request;
    if (DEBUG) {
      debug("Received message '" + msg.name + "': " + JSON.stringify(msg.json));
    }

    let data = msg.json.data;
    let clientId = msg.json.clientId;
    switch (msg.name) {
      case "RIL:CardStateChanged":
        if (this.rilContexts[clientId].cardState != data.cardState) {
          this.rilContexts[clientId].cardState = data.cardState;
          this._deliverIccEvent(clientId,
                                "notifyCardStateChanged",
                                null);
        }
        break;
      case "RIL:IccInfoChanged":
        this.updateIccInfo(clientId, data);
        this._deliverIccEvent(clientId,
                              "notifyIccInfoChanged",
                              null);
        break;
      case "RIL:GetCardLockResult": {
        let requestId = data.requestId;
        let callback = this._requestMap[requestId];
        delete this._requestMap[requestId];

        if (data.errorMsg) {
          callback.notifyError(data.errorMsg);
          break;
        }

        callback.notifySuccessWithBoolean(data.enabled);
        break;
      }
      case "RIL:SetUnlockCardLockResult": {
        let requestId = data.requestId;
        let callback = this._requestMap[requestId];
        delete this._requestMap[requestId];

        if (data.errorMsg) {
          let retryCount =
            (data.retryCount !== undefined) ? data.retryCount : -1;
          callback.notifyCardLockError(data.errorMsg, retryCount);
          break;
        }

        callback.notifySuccess();
        break;
      }
      case "RIL:CardLockRetryCount": {
        let requestId = data.requestId;
        let callback = this._requestMap[requestId];
        delete this._requestMap[requestId];

        if (data.errorMsg) {
          callback.notifyError(data.errorMsg);
          break;
        }

        callback.notifyGetCardLockRetryCount(data.retryCount);
        break;
      }
      case "RIL:StkCommand":
        this._deliverEvent(clientId, "_iccListeners", "notifyStkCommand",
                           [JSON.stringify(data)]);
        break;
      case "RIL:StkSessionEnd":
        this._deliverEvent(clientId, "_iccListeners", "notifyStkSessionEnd", null);
        break;
      case "RIL:IccOpenChannel":
        this.handleIccOpenChannel(data);
        break;
      case "RIL:IccCloseChannel":
        this.handleIccCloseChannel(data);
        break;
      case "RIL:IccExchangeAPDU":
        this.handleIccExchangeAPDU(data);
        break;
      case "RIL:ReadIccContacts":
        this.handleReadIccContacts(data);
        break;
      case "RIL:UpdateIccContact":
        this.handleUpdateIccContact(data);
        break;
      case "RIL:MatchMvno": {
        let requestId = data.requestId;
        let callback = this._requestMap[requestId];
        delete this._requestMap[requestId];

        if (data.errorMsg) {
          callback.notifyError(data.errorMsg);
          break;
        }
        callback.notifySuccessWithBoolean(data.result);
        break;
      }
      case "RIL:GetServiceState": {
        let requestId = data.requestId;
        let callback = this._requestMap[requestId];
        delete this._requestMap[requestId];

        if (data.errorMsg) {
          callback.notifyError(data.errorMsg);
          break;
        }
        callback.notifySuccessWithBoolean(data.result);
        break;
      }
    }
  },

  handleSimpleRequest: function(requestId, errorMsg, result) {
    if (errorMsg) {
      this.fireRequestError(requestId, errorMsg);
    } else {
      this.fireRequestSuccess(requestId, result);
    }
  },

  handleIccOpenChannel: function(message) {
    let requestId = message.requestId;
    let callback = this._getIccChannelCallback(requestId);
    if (!callback) {
      return;
    }

    return !message.errorMsg ? callback.notifyOpenChannelSuccess(message.channel) :
                               callback.notifyError(message.errorMsg);
  },

  handleIccCloseChannel: function(message) {
    let requestId = message.requestId;
    let callback = this._getIccChannelCallback(requestId);
    if (!callback) {
      return;
    }

    return !message.errorMsg ? callback.notifyCloseChannelSuccess() :
                               callback.notifyError(message.errorMsg);
  },

  handleIccExchangeAPDU: function(message) {
    let requestId = message.requestId;
    let callback = this._getIccChannelCallback(requestId);
    if (!callback) {
      return;
    }

    return !message.errorMsg ?
           callback.notifyExchangeAPDUResponse(message.sw1, message.sw2, message.simResponse) :
           callback.notifyError(message.errorMsg);
  },

  handleReadIccContacts: function(message) {
    if (message.errorMsg) {
      this.fireRequestError(message.requestId, message.errorMsg);
      return;
    }

    let window = this._windowsMap[message.requestId];
    delete this._windowsMap[message.requestId];
    let contacts = message.contacts;
    let result = new window.Array();
    contacts.forEach(function(c) {
      let prop = {name: [c.alphaId], tel: [{value: c.number}]};

      if (c.email) {
        prop.email = [{value: c.email}];
      }

      
      let anrLen = c.anr ? c.anr.length : 0;
      for (let i = 0; i < anrLen; i++) {
        prop.tel.push({value: c.anr[i]});
      }

      let contact = new window.mozContact(prop);
      contact.id = c.contactId;
      result.push(contact);
    });

    this.fireRequestSuccess(message.requestId, result);
  },

  handleUpdateIccContact: function(message) {
    if (message.errorMsg) {
      this.fireRequestError(message.requestId, message.errorMsg);
      return;
    }

    let window = this._windowsMap[message.requestId];
    delete this._windowsMap[message.requestId];
    let iccContact = message.contact;
    let prop = {name: [iccContact.alphaId], tel: [{value: iccContact.number}]};
    if (iccContact.email) {
      prop.email = [{value: iccContact.email}];
    }

    
    let anrLen = iccContact.anr ? iccContact.anr.length : 0;
    for (let i = 0; i < anrLen; i++) {
      prop.tel.push({value: iccContact.anr[i]});
    }

    let contact = new window.mozContact(prop);
    contact.id = iccContact.contactId;

    this.fireRequestSuccess(message.requestId, contact);
  },

  _deliverEvent: function(clientId, listenerType, name, args) {
    if (!this[listenerType]) {
      return;
    }
    let thisListeners = this[listenerType][clientId];
    if (!thisListeners) {
      return;
    }

    let listeners = thisListeners.slice();
    for (let listener of listeners) {
      if (thisListeners.indexOf(listener) == -1) {
        continue;
      }
      let handler = listener[name];
      if (typeof handler != "function") {
        throw new Error("No handler for " + name);
      }
      try {
        handler.apply(listener, args);
      } catch (e) {
        if (DEBUG) debug("listener for " + name + " threw an exception: " + e);
      }
    }
  },

  



  _iccs: null, 

  getIccByServiceId: function(serviceId) {
    let icc = this._iccs[serviceId];
    if (!icc) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return icc;
  },

  



  _deliverIccEvent: function(clientId, name, args) {
    let icc = this._iccs[clientId];
    if (!icc) {
      if (DEBUG) debug("_deliverIccEvent: Invalid clientId: " + clientId);
      return;
    }

    icc.deliverListenerEvent(name, args);
  },

  getIccInfo: function(clientId) {
    let context = this.getRilContext(clientId);
    return context && context.iccInfo;
  },

  getCardState: function(clientId) {
    let context = this.getRilContext(clientId);
    return context && context.cardState;
  },

  matchMvno: function(clientId, mvnoType, mvnoData, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:MatchMvno", {
      clientId: clientId,
      data: {
        requestId: requestId,
        mvnoType: mvnoType,
        mvnoData: mvnoData
      }
    });
  },

  getCardLockEnabled: function(clientId, lockType, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:GetCardLockEnabled", {
      clientId: clientId,
      data: {
        lockType: lockType,
        requestId: requestId
      }
    });
  },

  unlockCardLock: function(clientId, lockType, password, newPin, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:UnlockCardLock", {
      clientId: clientId,
      data: {
        lockType: lockType,
        password: password,
        newPin: newPin,
        requestId: requestId
      }
    });
  },

  setCardLockEnabled: function(clientId, lockType, password, enabled, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:SetCardLockEnabled", {
      clientId: clientId,
      data: {
        lockType: lockType,
        password: password,
        enabled: enabled,
        requestId: requestId
      }
    });
  },

  changeCardLockPassword: function(clientId, lockType, password, newPassword,
                                   callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:ChangeCardLockPassword", {
      clientId: clientId,
      data: {
        lockType: lockType,
        password: password,
        newPassword: newPassword,
        requestId: requestId
      }
    });
  },

  getCardLockRetryCount: function(clientId, lockType, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:GetCardLockRetryCount", {
      clientId: clientId,
      data: {
        lockType: lockType,
        requestId: requestId
      }
    });
  },

  getServiceStateEnabled: function(clientId, service, callback) {
    let requestId = UUIDGenerator.generateUUID().toString();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("RIL:GetServiceState", {
      clientId: clientId,
      data: {
        requestId: requestId,
        service: service
      }
    });
  }
};

function Icc(aIccProvider, aClientId) {
  this._iccProvider = aIccProvider;
  this._clientId = aClientId;
  this._listeners = [];
}
Icc.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIcc]),

  _iccProvider: null,
  _clientId: -1,
  _listeners: null,

  deliverListenerEvent: function(aName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) === -1) {
        continue;
      }
      let handler = listener[aName];
      if (typeof handler != "function") {
        throw new Error("No handler for " + aName);
      }
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        if (DEBUG) {
          debug("listener for " + aName + " threw an exception: " + e);
        }
      }
    }
  },

  


  registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.push(aListener);
    cpmm.sendAsyncMessage("RIL:RegisterIccMsg");
  },

  unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index >= 0) {
      this._listeners.splice(index, 1);
    }
  },

  get iccInfo() {
    return this._iccProvider.getIccInfo(this._clientId);
  },

  get cardState() {
    return this._iccProvider.getCardState(this._clientId);
  },

  getCardLockEnabled: function(aLockType, aCallback) {
    this._iccProvider.getCardLockEnabled(this._clientId, aLockType, aCallback);
  },

  unlockCardLock: function(aLockType, aPassword, aNewPin, aCallback) {
    this._iccProvider.unlockCardLock(this._clientId, aLockType,
                                     aPassword, aNewPin, aCallback);
  },

  setCardLockEnabled: function(aLockType, aPassword, aEnabled, aCallback) {
    this._iccProvider.setCardLockEnabled(this._clientId, aLockType,
                                         aPassword, aEnabled, aCallback);
  },

  changeCardLockPassword: function(aLockType, aPassword, aNewPassword, aCallback) {
    this._iccProvider.changeCardLockPassword(this._clientId, aLockType,
                                             aPassword, aNewPassword, aCallback);
  },

  getCardLockRetryCount: function(aLockType, aCallback) {
    this._iccProvider.getCardLockRetryCount(this._clientId, aLockType, aCallback);
  },

  matchMvno: function(aMvnoType, aMvnoData, aCallback) {
    this._iccProvider.matchMvno(this._clientId, aMvnoType, aMvnoData, aCallback);
  },

  getServiceStateEnabled: function(aService, aCallback) {
    this._iccProvider.getServiceStateEnabled(this._clientId, aService, aCallback);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RILContentHelper]);
