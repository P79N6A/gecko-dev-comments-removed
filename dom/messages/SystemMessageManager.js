



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

const kSystemMessageInternalReady = "system-message-internal-ready";

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function debug(aMsg) {
   
}



function SystemMessageManager() {
  
  
  
  
  
  
  
  
  this._dispatchers = {};

  
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

  _dispatchMessage: function sysMessMgr_dispatchMessage(aType, aDispatcher, aMessage) {
    if (aDispatcher.isHandling) {
      
      
      
      
      
      
      
      aDispatcher.messages.push(aMessage);
      return;
    }

    aDispatcher.isHandling = true;

    
    
    
    
    
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

    aDispatcher.handler
      .handleMessage(wrapped ? aMessage
                             : ObjectWrapper.wrap(aMessage, this._window));

    
    
    cpmm.sendAsyncMessage("SystemMessageManager:HandleMessagesDone",
                          { type: aType,
                            manifest: this._manifest,
                            uri: this._uri,
                            handledCount: 1 });

    aDispatcher.isHandling = false;
    if (aDispatcher.messages.length > 0) {
      this._dispatchMessage(aType, aDispatcher, aDispatcher.messages.shift());
    }
  },

  mozSetMessageHandler: function sysMessMgr_setMessageHandler(aType, aHandler) {
    debug("set message handler for [" + aType + "] " + aHandler);

    if (this._isInBrowserElement) {
      debug("the app loaded in the browser cannot set message handler");
      
      return;
    }

    if (!aType) {
      
      return;
    }

    let dispatchers = this._dispatchers;
    if (!aHandler) {
      
      
      delete dispatchers[aType];
      return;
    }

    
    dispatchers[aType] = { handler: aHandler, messages: [], isHandling: false };

    
    this.addMessageListeners("SystemMessageManager:GetPendingMessages:Return");
    cpmm.sendAsyncMessage("SystemMessageManager:GetPendingMessages",
                          { type: aType,
                            uri: this._uri,
                            manifest: this._manifest });
  },

  mozHasPendingMessage: function sysMessMgr_hasPendingMessage(aType) {
    debug("asking pending message for [" + aType + "]");

    if (this._isInBrowserElement) {
      debug("the app loaded in the browser cannot ask pending message");
      
      return false;
    }

    
    if (aType in this._dispatchers) {
      return false;
    }

    return cpmm.sendSyncMessage("SystemMessageManager:HasPendingMessages",
                                { type: aType,
                                  uri: this._uri,
                                  manifest: this._manifest })[0];
  },

  uninit: function sysMessMgr_uninit()  {
    this._dispatchers = null;
    this._pendings = null;

    if (this._isParentProcess) {
      Services.obs.removeObserver(this, kSystemMessageInternalReady);
    }

    if (this._isInBrowserElement) {
      debug("the app loaded in the browser doesn't need to unregister " +
            "the manifest for listening to the system messages");
      return;
    }

    cpmm.sendAsyncMessage("SystemMessageManager:Unregister",
                          { manifest: this._manifest,
                            uri: this._uri,
                            innerWindowID: this.innerWindowID });
  },

  
  
  
  
  
  
  
  
  
  
  receiveMessage: function sysMessMgr_receiveMessage(aMessage) {
    let msg = aMessage.data;
    debug("receiveMessage " + aMessage.name + " for [" + msg.type + "] " +
          "with manifest = " + msg.manifest + " and uri = " + msg.uri);

    
    
    
    if (msg.manifest !== this._manifest || msg.uri !== this._uri) {
      debug("This page shouldn't handle the messages because its " +
            "manifest = " + this._manifest + " and uri = " + this._uri);
      return;
    }

    if (aMessage.name == "SystemMessageManager:Message") {
      
      
      cpmm.sendAsyncMessage("SystemMessageManager:Message:Return:OK",
                            { type: msg.type,
                              manifest: this._manifest,
                              uri: this._uri,
                              msgID: msg.msgID });
    } else if (aMessage.name == "SystemMessageManager:GetPendingMessages:Return") {
      this.removeMessageListeners(aMessage.name);
    }

    let messages = (aMessage.name == "SystemMessageManager:Message")
                   ? [msg.msg]
                   : msg.msgQueue;

    
    let dispatcher = this._dispatchers[msg.type];
    if (dispatcher) {
      messages.forEach(function(aMsg) {
        this._dispatchMessage(msg.type, dispatcher, aMsg);
      }, this);
    } else {
      
      
      
      cpmm.sendAsyncMessage("SystemMessageManager:HandleMessagesDone",
                            { type: msg.type,
                              manifest: this._manifest,
                              uri: this._uri,
                              handledCount: messages.length });
    }

    if (!dispatcher || !dispatcher.isHandling) {
      
      Services.obs.notifyObservers( null,
                                   "handle-system-messages-done",
                                    null);
    }
  },

  
  init: function sysMessMgr_init(aWindow) {
    debug("init");
    this.initDOMRequestHelper(aWindow, ["SystemMessageManager:Message"]);

    let principal = aWindow.document.nodePrincipal;
    this._isInBrowserElement = principal.isInBrowserElement;
    this._uri = principal.URI.spec;

    let appsService = Cc["@mozilla.org/AppsService;1"]
                        .getService(Ci.nsIAppsService);
    this._manifest = appsService.getManifestURLByLocalId(principal.appId);

    
    
    
    
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

    
    this.__proto__.__proto__.observe.call(this, aSubject, aTopic, aData);
  },

  _registerManifest: function sysMessMgr_registerManifest() {
    if (this._isInBrowserElement) {
      debug("the app loaded in the browser doesn't need to register " +
            "the manifest for listening to the system messages");
      return;
    }

    if (!this._registerManifestReady) {
      cpmm.sendAsyncMessage("SystemMessageManager:Register",
                            { manifest: this._manifest,
                              uri: this._uri,
                              innerWindowID: this.innerWindowID });

      this._registerManifestReady = true;
    }
  },

  classID: Components.ID("{bc076ea0-609b-4d8f-83d7-5af7cbdc3bb2}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMNavigatorSystemMessages,
                                         Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  classInfo: XPCOMUtils.generateCI({
    classID: Components.ID("{bc076ea0-609b-4d8f-83d7-5af7cbdc3bb2}"),
    contractID: "@mozilla.org/system-message-manager;1",
    interfaces: [Ci.nsIDOMNavigatorSystemMessages],
    flags: Ci.nsIClassInfo.DOM_OBJECT,
    classDescription: "System Messages"})
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageManager]);
