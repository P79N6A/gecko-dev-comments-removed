














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
  "RIL:StkCommand",
  "RIL:StkSessionEnd",
  "RIL:ReadIccContacts",
  "RIL:UpdateIccContact",
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

function RILContentHelper() {
  this.updateDebugFlag();

  this.numClients = gNumRadioInterfaces;
  if (DEBUG) debug("Number of clients: " + this.numClients);

  this.initDOMRequestHelper( null, RIL_IPC_MSG_NAMES);
  this._windowsMap = [];
  this._iccListeners = [];

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
}

RILContentHelper.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccProvider,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  classID:   RILCONTENTHELPER_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILCONTENTHELPER_CID,
                                    classDescription: "RILContentHelper",
                                    interfaces: [Ci.nsIIccProvider]}),

  updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_CONTENT_HELPER ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _windowsMap: null,

  



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
      case "RIL:StkCommand":
        this._deliverEvent(clientId, "_iccListeners", "notifyStkCommand",
                           [JSON.stringify(data)]);
        break;
      case "RIL:StkSessionEnd":
        this._deliverEvent(clientId, "_iccListeners", "notifyStkSessionEnd", null);
        break;
      case "RIL:ReadIccContacts":
        this.handleReadIccContacts(data);
        break;
      case "RIL:UpdateIccContact":
        this.handleUpdateIccContact(data);
        break;
    }
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
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RILContentHelper]);
