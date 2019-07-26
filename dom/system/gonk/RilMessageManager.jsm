





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");


const kPrefenceChangedObserverTopic     = "nsPref:changed";
const kSysMsgListenerReadyObserverTopic = "system-message-listener-ready";
const kXpcomShutdownObserverTopic       = "xpcom-shutdown";


const kPrefKeyRilDebuggingEnabled = "ril.debugging.enabled";


const kMsgNameChildProcessShutdown = "child-process-shutdown";

let DEBUG;
function debug(s) {
  dump("RilMessageManager: " + s + "\n");
}

this.RilMessageManager = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                         Ci.nsIObserver]),

  topicRegistrationNames: {
    cellbroadcast:    ["RIL:RegisterCellBroadcastMsg"],
    mobileconnection: ["RIL:RegisterMobileConnectionMsg", "RIL:RegisterIccMsg"],
    voicemail:        ["RIL:RegisterVoicemailMsg"],
  },

  





  callbacksByName: {},

  
  
  targetsByTopic: {},
  topics: [],

  targetMessageQueue: [],
  ready: false,

  _init: function _init() {
    this._updateDebugFlag();

    Services.obs.addObserver(this, kPrefenceChangedObserverTopic, false);
    Services.obs.addObserver(this, kSysMsgListenerReadyObserverTopic, false);
    Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);

    ppmm.addMessageListener(kMsgNameChildProcessShutdown, this);

    let callback = this._registerMessageTarget.bind(this);
    for (let topic in this.topicRegistrationNames) {
      let names = this.topicRegistrationNames[topic];
      this.registerMessageListeners(topic, names, callback);
    }
  },

  _shutdown: function _shutdown() {
    if (!this.ready) {
      Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);
    }
    Services.obs.removeObserver(this, kPrefenceChangedObserverTopic);
    Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);

    for (let name in this.callbacksByName) {
      ppmm.removeMessageListener(name, this);
    }
    this.callbacksByName = null;

    ppmm.removeMessageListener(kMsgNameChildProcessShutdown, this);
    ppmm = null;

    this.targetsByTopic = null;
    this.targetMessageQueue = null;
  },

  _registerMessageTarget: function _registerMessageTarget(topic, msg) {
    let targets = this.targetsByTopic[topic];
    if (!targets) {
      targets = this.targetsByTopic[topic] = [];
      let list = this.topics;
      if (list.indexOf(topic) == -1) {
        list.push(topic);
      }
    }

    let target = msg.target;
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
    
    
    

    
    for (let msg of this.targetMessageQueue) {
      this._sendTargetMessage(msg.topic, msg.name, msg.options);
    }
    this.targetMessageQueue = null;
  },

  _updateDebugFlag: function _updateDebugFlag() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefKeyRilDebuggingEnabled);
    } catch(e) {}
  },

  



  receiveMessage: function receiveMessage(msg) {
    if (DEBUG) {
      debug("Received '" + msg.name + "' message from content process");
    }

    if (msg.name == kMsgNameChildProcessShutdown) {
      
      
      
      this._unregisterMessageTarget(null, msg.target);
      return;
    }

    let entry = this.callbacksByName[msg.name];
    if (!entry) {
      if (DEBUG) debug("Ignoring unknown message type: " + msg.name);
      return null;
    }

    if (entry.topic && !msg.target.assertPermission(entry.topic)) {
      if (DEBUG) {
        debug("Message " + msg.name + " from a content process with no '" +
              entry.topic + "' privileges.");
      }
      return null;
    }

    return entry.callback(entry.topic, msg);
  },

  



  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kSysMsgListenerReadyObserverTopic:
        this.ready = true;
        Services.obs.removeObserver(this, kSysMsgListenerReadyObserverTopic);

        this._resendQueuedTargetMessage();
        break;

      case kPrefenceChangedObserverTopic:
        if (data === kPrefKeyRilDebuggingEnabled) {
          this._updateDebugFlag();
        }
        break;

      case kXpcomShutdownObserverTopic:
        this._shutdown();
        break;
    }
  },

  



  








  registerMessageListeners: function registerMessageListeners(topic, names,
                                                              callback) {
    for (let name of names) {
      if (this.callbacksByName[name]) {
        if (DEBUG) {
          debug("Message name '" + name + "' was already registered. Ignored.");
        }
        continue;
      }

      this.callbacksByName[name] = { topic: topic, callback: callback };
      ppmm.addMessageListener(name, this);
    }
  },

  





  unregisterMessageListeners: function unregisterMessageListeners(callback) {
    let remains = {};
    for (let name in this.callbacksByName) {
      let entry = this.callbacksByName[name];
      if (entry.callback != callback) {
        remains[name] = entry;
      } else {
        ppmm.removeMessageListener(name, this);
      }
    }
    this.callbacksByName = remains;
  },

  sendMobileConnectionMessage: function sendMobileConnectionMessage(name,
                                                                    clientId,
                                                                    data) {
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

  sendCellBroadcastMessage: function sendCellBroadcastMessage(name, clientId,
                                                              data) {
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

RilMessageManager._init();

this.EXPORTED_SYMBOLS = ["RilMessageManager"];
