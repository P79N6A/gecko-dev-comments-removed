
















"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let NFC = {};
Cu.import("resource://gre/modules/nfc_consts.js", NFC);


let DEBUG = NFC.DEBUG_NFC;

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- Nfc: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

const NFC_CONTRACTID = "@mozilla.org/nfc;1";
const NFC_CID =
  Components.ID("{2ff24790-5e74-11e1-b86c-0800200c9a66}");

const NFC_IPC_MSG_NAMES = [
  "NFC:SetSessionToken",
  "NFC:ReadNDEF",
  "NFC:WriteNDEF",
  "NFC:GetDetailsNDEF",
  "NFC:MakeReadOnlyNDEF",
  "NFC:Connect",
  "NFC:Close"
];

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");
XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");
XPCOMUtils.defineLazyServiceGetter(this, "gSystemWorkerManager",
                                   "@mozilla.org/telephony/system-worker-manager;1",
                                   "nsISystemWorkerManager");
XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");
XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                    "@mozilla.org/uuid-generator;1",
                                    "nsIUUIDGenerator");
XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    nfc: null,

    
    
    targetsBySessionTokens: {},
    sessionTokens: [],

    init: function init(nfc) {
      this.nfc = nfc;

      Services.obs.addObserver(this, "xpcom-shutdown", false);
      this._registerMessageListeners();
    },

    _shutdown: function _shutdown() {
      this.nfc = null;

      Services.obs.removeObserver(this, "xpcom-shutdown");
      this._unregisterMessageListeners();
    },

    _registerMessageListeners: function _registerMessageListeners() {
      ppmm.addMessageListener("child-process-shutdown", this);
      for (let msgname of NFC_IPC_MSG_NAMES) {
        ppmm.addMessageListener(msgname, this);
      }
    },

    _unregisterMessageListeners: function _unregisterMessageListeners() {
      ppmm.removeMessageListener("child-process-shutdown", this);
      for (let msgname of NFC_IPC_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      ppmm = null;
    },

    _registerMessageTarget: function _registerMessageTarget(sessionToken, target) {
      let targets = this.targetsBySessionTokens[sessionToken];
      if (!targets) {
        targets = this.targetsBySessionTokens[sessionToken] = [];
        let list = this.sessionTokens;
        if (list.indexOf(sessionToken) == -1) {
          list.push(sessionToken);
        }
      }

      if (targets.indexOf(target) != -1) {
        debug("Already registered this target!");
        return;
      }

      targets.push(target);
      debug("Registered :" + sessionToken + " target: " + target);
    },

    _unregisterMessageTarget: function _unregisterMessageTarget(sessionToken, target) {
      if (sessionToken == null) {
        
        for (let session of this.sessionTokens) {
          this._unregisterMessageTarget(session, target);
        }
        return;
      }

      
      let targets = this.targetsBySessionTokens[sessionToken];
      if (!targets) {
        return;
      }

      if (target == null) {
        debug("Unregistered all targets for the " + sessionToken + " targets: " + targets);
        targets = [];
        let list = this.sessionTokens;
        if (sessionToken !== null) {
          let index = list.indexOf(sessionToken);
          if (index > -1) {
            list.splice(index, 1);
          }
        }
        return;
      }

      let index = targets.indexOf(target);
      if (index != -1) {
        targets.splice(index, 1);
      }
    },

    _sendTargetMessage: function _sendTargetMessage(sessionToken, message, options) {
      let targets = this.targetsBySessionTokens[sessionToken];
      if (!targets) {
        return;
      }

      for (let target of targets) {
        target.sendAsyncMessage(message, options);
      }
    },

    



    receiveMessage: function receiveMessage(msg) {
      debug("Received '" + msg.name + "' message from content process");
      if (msg.name == "child-process-shutdown") {
        
        
        
        this._unregisterMessageTarget(null, msg.target);
        return null;
      }

      if (NFC_IPC_MSG_NAMES.indexOf(msg.name) != -1) {
        if (!msg.target.assertPermission("nfc-read")) {
          debug("Nfc message " + msg.name +
                " from a content process with no 'nfc-read' privileges.");
          return null;
        }
      } else {
        debug("Ignoring unknown message type: " + msg.name);
        return null;
      }

      switch (msg.name) {
        case "NFC:SetSessionToken":
          this._registerMessageTarget(this.nfc.sessionTokenMap[this.nfc._currentSessionId], msg.target);
          debug("Registering target for this SessionToken : " +
                this.nfc.sessionTokenMap[this.nfc._currentSessionId]);
          return null;
      }

      return null;
    },

    



    observe: function observe(subject, topic, data) {
      switch (topic) {
        case "xpcom-shutdown":
          this._shutdown();
          break;
      }
    },

    sendNfcResponseMessage: function sendNfcResponseMessage(message, data) {
      this._sendTargetMessage(this.nfc.sessionTokenMap[this.nfc._currentSessionId], message, data);
    },
  };
});

function Nfc() {
  debug("Starting Worker");
  this.worker = new ChromeWorker("resource://gre/modules/nfc_worker.js");
  this.worker.onerror = this.onerror.bind(this);
  this.worker.onmessage = this.onmessage.bind(this);

  for each (let msgname in NFC_IPC_MSG_NAMES) {
    ppmm.addMessageListener(msgname, this);
  }

  Services.obs.addObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED, false);
  Services.obs.addObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN, false);

  gMessageManager.init(this);
  let lock = gSettingsService.createLock();
  lock.get(NFC.SETTING_NFC_POWER_LEVEL, this);
  lock.get(NFC.SETTING_NFC_ENABLED, this);
  
  this.sessionTokenMap = {};

  gSystemWorkerManager.registerNfcWorker(this.worker);
}

Nfc.prototype = {

  classID:   NFC_CID,
  classInfo: XPCOMUtils.generateCI({classID: NFC_CID,
                                    classDescription: "Nfc",
                                    interfaces: [Ci.nsIWorkerHolder]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWorkerHolder,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  _currentSessionId: null,
  _enabled: false,

  onerror: function onerror(event) {
    debug("Got an error: " + event.filename + ":" +
          event.lineno + ": " + event.message + "\n");
    event.preventDefault();
  },

  







  sendToWorker: function sendToWorker(nfcMessageType, message) {
    message = message || {};
    message.type = nfcMessageType;
    this.worker.postMessage(message);
  },

  





  sendNfcErrorResponse: function sendNfcErrorResponse(message) {
    if (!message.target) {
      return;
    }

    let nfcMsgType = message.name + "Response";
    message.target.sendAsyncMessage(nfcMsgType, {
      sessionId: message.json.sessionToken,
      requestId: message.json.requestId,
      status: NFC.GECKO_NFC_ERROR_GENERIC_FAILURE
    });
  },

  


  onmessage: function onmessage(event) {
    let message = event.data;
    debug("Received message from NFC worker: " + JSON.stringify(message));

    switch (message.type) {
      case "techDiscovered":
        this._currentSessionId = message.sessionId;
        
        
        if (!this.sessionTokenMap[this._currentSessionId]) {
          this.sessionTokenMap[this._currentSessionId] = UUIDGenerator.generateUUID().toString();
        }
        
        message.sessionToken = this.sessionTokenMap[this._currentSessionId];
        
        delete message.sessionId;
        gSystemMessenger.broadcastMessage("nfc-manager-tech-discovered", message);
        break;
      case "techLost":
        gMessageManager._unregisterMessageTarget(this.sessionTokenMap[this._currentSessionId], null);
        
        message.sessionToken = this.sessionTokenMap[this._currentSessionId];
        
        delete message.sessionId;
        gSystemMessenger.broadcastMessage("nfc-manager-tech-lost", message);
        delete this.sessionTokenMap[this._currentSessionId];
        this._currentSessionId = null;
        break;
     case "ConfigResponse":
        gSystemMessenger.broadcastMessage("nfc-powerlevel-change", message);
        break;
      case "ConnectResponse": 
      case "CloseResponse":
      case "GetDetailsNDEFResponse":
      case "ReadNDEFResponse":
      case "MakeReadOnlyNDEFResponse":
      case "WriteNDEFResponse":
        message.sessionToken = this.sessionTokenMap[this._currentSessionId];
        
        delete message.sessionId;
        gMessageManager.sendNfcResponseMessage("NFC:" + message.type, message);
        break;
      default:
        throw new Error("Don't know about this message type: " + message.type);
    }
  },

  
  worker: null,

  powerLevel: NFC.NFC_POWER_LEVEL_DISABLED,

  sessionTokenMap: null,

  


  receiveMessage: function receiveMessage(message) {
    debug("Received '" + JSON.stringify(message) + "' message from content process");

    if (!this._enabled) {
      debug("NFC is not enabled.");
      this.sendNfcErrorResponse(message);
      return null;
    }

    
    switch (message.name) {
      case "NFC:Connect": 
      case "NFC:Close":
      case "NFC:GetDetailsNDEF":
      case "NFC:ReadNDEF":
        if (!message.target.assertPermission("nfc-read")) {
          debug("NFC message " + message.name +
                " from a content process with no 'nfc-read' privileges.");
          this.sendNfcErrorResponse(message);
          return null;
        }
        break;
      case "NFC:WriteNDEF": 
      case "NFC:MakeReadOnlyNDEF":
        if (!message.target.assertPermission("nfc-write")) {
          debug("NFC message " + message.name +
                " from a content process with no 'nfc-write' privileges.");
          this.sendNfcErrorResponse(message);
          return null;
        }
        break;
      case "NFC:SetSessionToken":
        
        return null;
    }

    
    if (message.json.sessionToken !== this.sessionTokenMap[this._currentSessionId]) {
      debug("Invalid Session Token: " + message.json.sessionToken +
            " Expected Session Token: " + this.sessionTokenMap[this._currentSessionId]);
      this.sendNfcErrorResponse(message);
      return null;
    }

    
    message.sessionId = this._currentSessionId;

    switch (message.name) {
      case "NFC:GetDetailsNDEF":
        this.sendToWorker("getDetailsNDEF", message.json);
        break;
      case "NFC:ReadNDEF":
        this.sendToWorker("readNDEF", message.json);
        break;
      case "NFC:WriteNDEF":
        this.sendToWorker("writeNDEF", message.json);
        break;
      case "NFC:MakeReadOnlyNDEF":
        this.sendToWorker("makeReadOnlyNDEF", message.json);
        break;
      case "NFC:Connect":
        this.sendToWorker("connect", message.json);
        break;
      case "NFC:Close":
        this.sendToWorker("close", message.json);
        break;
      default:
        debug("UnSupported : Message Name " + message.name);
        return null;
    }
  },

  



  handle: function handle(aName, aResult) {
    switch(aName) {
      case NFC.SETTING_NFC_ENABLED:
        debug("'nfc.enabled' is now " + aResult);
        this._enabled = aResult;
        
        let powerLevel = this._enabled ? NFC.NFC_POWER_LEVEL_ENABLED :
                                         NFC.NFC_POWER_LEVEL_DISABLED;
        
        if (powerLevel !== this.powerLevel) {
          debug("New Power Level " + powerLevel);
          this.setConfig({powerLevel: powerLevel});
          this.powerLevel = powerLevel;
        }
        break;
    }
  },

  



  observe: function observe(subject, topic, data) {
    switch (topic) {
      case NFC.TOPIC_XPCOM_SHUTDOWN:
        for each (let msgname in NFC_IPC_MSG_NAMES) {
          ppmm.removeMessageListener(msgname, this);
        }
        ppmm = null;
        Services.obs.removeObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN);
        break;
      case NFC.TOPIC_MOZSETTINGS_CHANGED:
        let setting = JSON.parse(data);
        if (setting) {
          let setting = JSON.parse(data);
          this.handle(setting.key, setting.value);
        }
        break;
    }
  },

  setConfig: function setConfig(prop) {
    this.sendToWorker("config", prop);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Nfc]);
