



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

const kSystemMessageInternalReady = "system-message-internal-ready";

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");


let kMaxPendingMessages;
try {
  kMaxPendingMessages = Services.prefs.getIntPref("dom.messages.maxPendingMessages");
} catch(e) {
  
  kMaxPendingMessages = 5;
}

function debug(aMsg) {
  
}



function SystemMessageManager() {
  
  
  this._handlers = {};

  
  this._pendings = {};

  
  this._registerManifestReady = false;

  
  let appInfo = Cc["@mozilla.org/xre/app-info;1"];
  this._isParentProcess =
    !appInfo || appInfo.getService(Ci.nsIXULRuntime)
                  .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

  
  if (this._isParentProcess) {
    Services.obs.addObserver(this, kSystemMessageInternalReady, false);
  }
}

SystemMessageManager.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  _dispatchMessage: function sysMessMgr_dispatchMessage(aType, aHandler, aMessage) {
    
    
    
    
    
    debug("Dispatching " + JSON.stringify(aMessage) + "\n");
    let contractID = "@mozilla.org/dom/system-messages/wrapper/" + aType + ";1";
    let wrapped = false;

    if (contractID in Cc) {
      debug(contractID + " is registered, creating an instance");
      let wrapper = Cc[contractID].createInstance(Ci.nsISystemMessagesWrapper);
      if (wrapper) {
        aMessage = wrapper.wrapMessage(aMessage, this._window);
        wrapped = true;
        debug("wrapped = " + aMessage);
      }
    }

    aHandler.handleMessage(wrapped ? aMessage
                                   : ObjectWrapper.wrap(aMessage, this._window));
  },

  mozSetMessageHandler: function sysMessMgr_setMessageHandler(aType, aHandler) {
    debug("setMessage handler for [" + aType + "] " + aHandler);
    if (!aType) {
      
      return;
    }

    let handlers = this._handlers;
    if (!aHandler) {
      
      
      delete handlers[aType];
      return;
    }

    
    handlers[aType] = aHandler;

    
    if (this._getPendingMessages(aType, true)) {
      let thread = Services.tm.mainThread;
      let pending = this._pendings[aType];
      this._pendings[aType] = [];
      let self = this;
      pending.forEach(function dispatch_pending(aPending) {
        thread.dispatch({
          run: function run() {
            self._dispatchMessage(aType, aHandler, aPending);
          }
        }, Ci.nsIEventTarget.DISPATCH_NORMAL);
      });
    }
  },

  _getPendingMessages: function sysMessMgr_getPendingMessages(aType, aForceUpdate) {
    debug("hasPendingMessage " + aType);
    let pendings = this._pendings;

    
    
    
    if (aType in this._handlers && !aForceUpdate) {
      return false;
    }

    
    
    let messages = cpmm.sendSyncMessage("SystemMessageManager:GetPendingMessages",
                                        { type: aType,
                                          uri: this._uri,
                                          manifest: this._manifest })[0];
    if (!messages) {
      
      return pendings[aType] && pendings[aType].length != 0;
    }

    if (!pendings[aType]) {
      pendings[aType] = [];
    }

    
    messages.forEach(function hpm_addPendings(aMessage) {
      pendings[aType].push(aMessage);
      if (pendings[aType].length > kMaxPendingMessages) {
        pendings[aType].splice(0, 1);
      }
    });

    return pendings[aType].length != 0;
  },

  mozHasPendingMessage: function sysMessMgr_hasPendingMessage(aType) {
    return this._getPendingMessages(aType, false);
  },

  uninit: function sysMessMgr_uninit()  {
    this._handlers = null;
    this._pendings = null;

    if (this._isParentProcess) {
      Services.obs.removeObserver(this, kSystemMessageInternalReady);
    }
  },

  receiveMessage: function sysMessMgr_receiveMessage(aMessage) {
    debug("receiveMessage " + aMessage.name + " - " +
          aMessage.json.type + " for " + aMessage.json.manifest +
          " (" + this._manifest + ")");

    let msg = aMessage.json;
    if (msg.manifest != this._manifest)
      return;

    
    
    cpmm.sendAsyncMessage(
      "SystemMessageManager:Message:Return:OK",
      { type: msg.type,
        manifest: msg.manifest,
        uri: msg.uri,
        msgID: msg.msgID });

    
    if (!(msg.type in this._handlers)) {
      debug("No handler for this type");
      return;
    }

    this._dispatchMessage(msg.type, this._handlers[msg.type], msg.msg);
  },

  
  init: function sysMessMgr_init(aWindow) {
    debug("init");
    this.initHelper(aWindow, ["SystemMessageManager:Message"]);

    let principal = aWindow.document.nodePrincipal;
    this._uri = principal.URI.spec;

    let appsService = Cc["@mozilla.org/AppsService;1"]
                        .getService(Ci.nsIAppsService);
    this._manifest = appsService.getManifestURLByLocalId(principal.appId);
    this._window = aWindow;

    
    
    
    
    let readyToRegister = true;
    if (this._isParentProcess) {
      let ready = cpmm.sendSyncMessage(
        "SystemMessageManager:AskReadyToRegister", null);
      if (ready.length == 0 || !ready[0]) {
        readyToRegister = false;
      }
    }
    if (readyToRegister) {
      this._registerManifest();
    }

    debug("done");
  },

  observe: function sysMessMgr_observe(aSubject, aTopic, aData) {
    if (aTopic === kSystemMessageInternalReady) {
      this._registerManifest();
    }
  },

  _registerManifest: function sysMessMgr_registerManifest() {
    if (!this._registerManifestReady) {
      cpmm.sendAsyncMessage("SystemMessageManager:Register",
                            { manifest: this._manifest });
      this._registerManifestReady = true;
    }
  },

  classID: Components.ID("{bc076ea0-609b-4d8f-83d7-5af7cbdc3bb2}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMNavigatorSystemMessages,
                                         Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsIObserver]),

  classInfo: XPCOMUtils.generateCI({classID: Components.ID("{bc076ea0-609b-4d8f-83d7-5af7cbdc3bb2}"),
                                    contractID: "@mozilla.org/system-message-manager;1",
                                    interfaces: [Ci.nsIDOMNavigatorSystemMessages],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "System Messages"})
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageManager]);
