





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");


const kSysMsgListenerReadyObserverTopic = "system-message-listener-ready";

let DEBUG;
function debug(s) {
  dump("RilMessageManager: " + s + "\n");
}

this.RilMessageManager = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                         Ci.nsIObserver]),

  ril: null,

  
  
  targetsByTopic: {},
  topics: [],

  targetMessageQueue: [],
  ready: false,

  init: function init(ril) {
    this.ril = ril;

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
    this._registerMessageListeners();
  },

  _shutdown: function _shutdown() {
    this.ril = null;

    Services.obs.removeObserver(this, "xpcom-shutdown");
    this._unregisterMessageListeners();
  },

  _registerMessageListeners: function _registerMessageListeners() {
    ppmm.addMessageListener("child-process-shutdown", this);
    for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }
    for (let msgName of RIL_IPC_ICCMANAGER_MSG_NAMES) {
      ppmm.addMessageListener(msgName, this);
    }
    for (let msgname of RIL_IPC_VOICEMAIL_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }
    for (let msgname of RIL_IPC_CELLBROADCAST_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }
  },

  _unregisterMessageListeners: function _unregisterMessageListeners() {
    ppmm.removeMessageListener("child-process-shutdown", this);
    for (let msgname of RIL_IPC_MOBILECONNECTION_MSG_NAMES) {
      ppmm.removeMessageListener(msgname, this);
    }
    for (let msgName of RIL_IPC_ICCMANAGER_MSG_NAMES) {
      ppmm.removeMessageListener(msgName, this);
    }
    for (let msgname of RIL_IPC_VOICEMAIL_MSG_NAMES) {
      ppmm.removeMessageListener(msgname, this);
    }
    for (let msgname of RIL_IPC_CELLBROADCAST_MSG_NAMES) {
      ppmm.removeMessageListener(msgname, this);
    }
    ppmm = null;
  },

  _registerMessageTarget: function _registerMessageTarget(topic, target) {
    let targets = this.targetsByTopic[topic];
    if (!targets) {
      targets = this.targetsByTopic[topic] = [];
      let list = this.topics;
      if (list.indexOf(topic) == -1) {
        list.push(topic);
      }
    }

    if (targets.indexOf(target) != -1) {
      if (DEBUG) debug("Already registered this target!");
      return;
    }

    targets.push(target);
    if (DEBUG) debug("Registered " + topic + " target: " + target);
  },

  _unregisterMessageTarget: function _unregisterMessageTarget(topic, target) {
    if (topic == null) {
      
      for (let type of this.topics) {
        this._unregisterMessageTarget(type, target);
      }
      return;
    }

    
    let targets = this.targetsByTopic[topic];
    if (!targets) {
      return;
    }

    let index = targets.indexOf(target);
    if (index != -1) {
      targets.splice(index, 1);
      if (DEBUG) debug("Unregistered " + topic + " target: " + target);
    }
  },

  _enqueueTargetMessage: function _enqueueTargetMessage(topic, name, options) {
    let msg = { topic : topic,
                name : name,
                options : options };
    
    
    let messageQueue = this.targetMessageQueue;
    for (let i = 0; i < messageQueue.length; i++) {
      if (messageQueue[i].name === name) {
        messageQueue.splice(i, 1);
        break;
      }
    }

    messageQueue.push(msg);
  },

  _sendTargetMessage: function _sendTargetMessage(topic, name, options) {
    if (!this.ready) {
      this._enqueueTargetMessage(topic, name, options);
      return;
    }

    let targets = this.targetsByTopic[topic];
    if (!targets) {
      return;
    }

    for (let target of targets) {
      target.sendAsyncMessage(name, options);
    }
  },

  _resendQueuedTargetMessage: function _resendQueuedTargetMessage() {
    this.ready = true;

    
    
    
    

    
    for (let msg of this.targetMessageQueue) {
      this._sendTargetMessage(msg.topic, msg.name, msg.options);
    }
    this.targetMessageQueue = null;
  },

  



  receiveMessage: function receiveMessage(msg) {
    if (DEBUG) debug("Received '" + msg.name + "' message from content process");
    if (msg.name == "child-process-shutdown") {
      
      
      
      this._unregisterMessageTarget(null, msg.target);
      return;
    }

    if (RIL_IPC_MOBILECONNECTION_MSG_NAMES.indexOf(msg.name) != -1) {
      if (!msg.target.assertPermission("mobileconnection")) {
        if (DEBUG) {
          debug("MobileConnection message " + msg.name +
                " from a content process with no 'mobileconnection' privileges.");
        }
        return null;
      }
    } else if (RIL_IPC_ICCMANAGER_MSG_NAMES.indexOf(msg.name) != -1) {
      if (!msg.target.assertPermission("mobileconnection")) {
        if (DEBUG) {
          debug("IccManager message " + msg.name +
                " from a content process with no 'mobileconnection' privileges.");
        }
        return null;
      }
    } else if (RIL_IPC_VOICEMAIL_MSG_NAMES.indexOf(msg.name) != -1) {
      if (!msg.target.assertPermission("voicemail")) {
        if (DEBUG) {
          debug("Voicemail message " + msg.name +
                " from a content process with no 'voicemail' privileges.");
        }
        return null;
      }
    } else if (RIL_IPC_CELLBROADCAST_MSG_NAMES.indexOf(msg.name) != -1) {
      if (!msg.target.assertPermission("cellbroadcast")) {
        if (DEBUG) {
          debug("Cell Broadcast message " + msg.name +
                " from a content process with no 'cellbroadcast' privileges.");
        }
        return null;
      }
    } else {
      if (DEBUG) debug("Ignoring unknown message type: " + msg.name);
      return null;
    }

    switch (msg.name) {
      case "RIL:RegisterMobileConnectionMsg":
        this._registerMessageTarget("mobileconnection", msg.target);
        return;
      case "RIL:RegisterIccMsg":
        this._registerMessageTarget("icc", msg.target);
        return;
      case "RIL:RegisterVoicemailMsg":
        this._registerMessageTarget("voicemail", msg.target);
        return;
      case "RIL:RegisterCellBroadcastMsg":
        this._registerMessageTarget("cellbroadcast", msg.target);
        return;
    }

    let clientId = msg.json.clientId || 0;
    let radioInterface = this.ril.getRadioInterface(clientId);
    if (!radioInterface) {
      if (DEBUG) debug("No such radio interface: " + clientId);
      return null;
    }

    return radioInterface.receiveMessage(msg);
  },

  



  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kSysMsgListenerReadyObserverTopic:
        Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);
        this._resendQueuedTargetMessage();
        break;
      case "xpcom-shutdown":
        this._shutdown();
        break;
    }
  },

  



  sendMobileConnectionMessage: function sendMobileConnectionMessage(name, clientId, data) {
    this._sendTargetMessage("mobileconnection", name, {
      clientId: clientId,
      data: data
    });
  },

  sendVoicemailMessage: function sendVoicemailMessage(name, clientId, data) {
    this._sendTargetMessage("voicemail", name, {
      clientId: clientId,
      data: data
    });
  },

  sendCellBroadcastMessage: function sendCellBroadcastMessage(name, clientId, data) {
    this._sendTargetMessage("cellbroadcast", name, {
      clientId: clientId,
      data: data
    });
  },

  sendIccMessage: function sendIccMessage(name, clientId, data) {
    this._sendTargetMessage("icc", name, {
      clientId: clientId,
      data: data
    });
  }
};

this.EXPORTED_SYMBOLS = ["RilMessageManager"];
