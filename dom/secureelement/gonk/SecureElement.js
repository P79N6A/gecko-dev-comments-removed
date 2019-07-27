
















"use strict";




const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyGetter(this, "SE", () => {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});


let DEBUG = SE.DEBUG_SE;
function debug(s) {
  if (DEBUG) {
    dump("-*- SecureElement: " + s + "\n");
  }
}

const SE_IPC_SECUREELEMENT_MSG_NAMES = [
  "SE:GetSEReaders",
  "SE:OpenChannel",
  "SE:CloseChannel",
  "SE:TransmitAPDU"
];

const SECUREELEMENTMANAGER_CONTRACTID =
  "@mozilla.org/secureelement/parent-manager;1";
const SECUREELEMENTMANAGER_CID =
  Components.ID("{48f4e650-28d2-11e4-8c21-0800200c9a66}");
const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "UiccConnector", () => {
  let uiccClass = Cc["@mozilla.org/secureelement/connector/uicc;1"];
  return uiccClass ? uiccClass.getService(Ci.nsISecureElementConnector) : null;
});

function getConnector(type) {
  switch (type) {
    case SE.TYPE_UICC:
      return UiccConnector;
    case SE.TYPE_ESE:
    default:
      debug("Unsupported SEConnector : " + type);
      return null;
  }
}






XPCOMUtils.defineLazyGetter(this, "gMap", function() {
  return {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    appInfoMap: {},

    registerSecureElementTarget: function(appId, target) {
      if (this.isAppIdRegistered(appId)) {
        debug("AppId: " + appId + "already registered");
        return;
      }

      this.appInfoMap[appId] = {
        target: target,
        channels: {}
      };

      debug("Registered a new SE target " + appId);
    },

    unregisterSecureElementTarget: function(target) {
      let appId = Object.keys(this.appInfoMap).find((id) => {
        return this.appInfoMap[id].target === target;
      });

      if (!appId) {
        return;
      }

      debug("Unregistered SE Target for AppId: " + appId);
      delete this.appInfoMap[appId];
    },

    isAppIdRegistered: function(appId) {
      return this.appInfoMap[appId] !== undefined;
    },

    getChannelCountByAppIdType: function(appId, type) {
      return Object.keys(this.appInfoMap[appId].channels)
                   .reduce((cnt, ch) => ch.type === type ? ++cnt : cnt, 0);
    },

    
    
    addChannel: function(appId, type, aid, channelNumber) {
      let token = UUIDGenerator.generateUUID().toString();
      this.appInfoMap[appId].channels[token] = {
        seType: type,
        aid: aid,
        channelNumber: channelNumber
      };
      return token;
    },

    removeChannel: function(appId, channelToken) {
      if (this.appInfoMap[appId].channels[channelToken]) {
        debug("Deleting channel with token : " + channelToken);
        delete this.appInfoMap[appId].channels[channelToken];
      }
    },

    getChannel: function(appId, channelToken) {
      if (!this.appInfoMap[appId].channels[channelToken]) {
        return null;
      }

      return this.appInfoMap[appId].channels[channelToken];
    },

    getChannelsByTarget: function(target) {
      let appId = Object.keys(this.appInfoMap).find((id) => {
        return this.appInfoMap[id].target === target;
      });

      if (!appId) {
        return [];
      }

      return Object.keys(this.appInfoMap[appId].channels)
                   .map(token => this.appInfoMap[appId].channels[token]);
    },

    getTargets: function() {
      return Object.keys(this.appInfoMap)
                   .map(appId => this.appInfoMap[appId].target);
    },
  };
});









function SecureElementManager() {
  this._registerMessageListeners();
  this._registerSEListeners();
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}

SecureElementManager.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIMessageListener,
    Ci.nsISEListener,
    Ci.nsIObserver]),
  classID: SECUREELEMENTMANAGER_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          SECUREELEMENTMANAGER_CID,
    classDescription: "SecureElementManager",
    interfaces:       [Ci.nsIMessageListener,
                       Ci.nsISEListener,
                       Ci.nsIObserver]
  }),

  
  
  _sePresence: {},

  _shutdown: function() {
    this.secureelement = null;
    Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    this._unregisterMessageListeners();
    this._unregisterSEListeners();
  },

  _registerMessageListeners: function() {
    ppmm.addMessageListener("child-process-shutdown", this);
    for (let msgname of SE_IPC_SECUREELEMENT_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }
  },

  _unregisterMessageListeners: function() {
    ppmm.removeMessageListener("child-process-shutdown", this);
    for (let msgname of SE_IPC_SECUREELEMENT_MSG_NAMES) {
      ppmm.removeMessageListener(msgname, this);
    }
    ppmm = null;
  },

  _registerSEListeners: function() {
    let connector = getConnector(SE.TYPE_UICC);
    if (!connector) {
      return;
    }

    this._sePresence[SE.TYPE_UICC] = false;
    connector.registerListener(this);
  },

  _unregisterSEListeners: function() {
    Object.keys(this._sePresence).forEach((type) => {
      let connector = getConnector(type);
      if (connector) {
        connector.unregisterListener(this);
      }
    });

    this._sePresence = {};
  },

  notifySEPresenceChanged: function(type, isPresent) {
    
    
    debug("notifying DOM about SE state change");
    this._sePresence[type] = isPresent;
    gMap.getTargets().forEach(target => {
      let result = { type: type, isPresent: isPresent };
      target.sendAsyncMessage("SE:ReaderPresenceChanged", { result: result });
    });
  },

  _canOpenChannel: function(appId, type) {
    let opened = gMap.getChannelCountByAppIdType(appId, type);
    let limit = SE.MAX_CHANNELS_ALLOWED_PER_SESSION;
    
    limit = type === SE.TYPE_UICC ? limit - 1 : limit;
    return opened < limit;
  },

  _handleOpenChannel: function(msg, callback) {
    if (!this._canOpenChannel(msg.appId, msg.type)) {
      debug("Max channels per session exceed");
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    
    let connector = getConnector(msg.type);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.openChannel(SEUtils.byteArrayToHexString(msg.aid), {
      notifyOpenChannelSuccess: (channelNumber, openResponse) => {
        
        let channelToken =
          gMap.addChannel(msg.appId, msg.type, msg.aid, channelNumber);
        if (channelToken) {
          callback({
            error: SE.ERROR_NONE,
            channelToken: channelToken,
            isBasicChannel: (channelNumber === SE.BASIC_CHANNEL),
            openResponse: SEUtils.hexStringToByteArray(openResponse)
          });
        } else {
          callback({ error: SE.ERROR_GENERIC });
        }
      },

      notifyError: (reason) => {
        debug("Failed to open the channel to AID : " +
               SEUtils.byteArrayToHexString(msg.aid) +
               ", Rejected with Reason : " + reason);
        callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
      }
    });
  },

  _handleTransmit: function(msg, callback) {
    let channel = gMap.getChannel(msg.appId, msg.channelToken);
    if (!channel) {
      debug("Invalid token:" + msg.channelToken + ", appId: " + msg.appId);
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(channel.seType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.exchangeAPDU(channel.channelNumber, msg.apdu.cla, msg.apdu.ins,
                           msg.apdu.p1, msg.apdu.p2,
                           SEUtils.byteArrayToHexString(msg.apdu.data),
                           msg.apdu.le, {
      notifyExchangeAPDUResponse: (sw1, sw2, response) => {
        callback({
          error: SE.ERROR_NONE,
          sw1: sw1,
          sw2: sw2,
          response: SEUtils.hexStringToByteArray(response)
        });
      },

      notifyError: (reason) => {
        debug("Transmit failed, rejected with Reason : " + reason);
        callback({ error: SE.ERROR_INVALIDAPPLICATION, reason: reason });
      }
    });
  },

  _handleCloseChannel: function(msg, callback) {
    let channel = gMap.getChannel(msg.appId, msg.channelToken);
    if (!channel) {
      debug("Invalid token:" + msg.channelToken + ", appId:" + msg.appId);
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(channel.seType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.closeChannel(channel.channelNumber, {
      notifyCloseChannelSuccess: () => {
        gMap.removeChannel(msg.appId, msg.channelToken);
        callback({ error: SE.ERROR_NONE });
      },

      notifyError: (reason) => {
        debug("Failed to close channel with token: " + msg.channelToken +
              ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleGetSEReadersRequest: function(msg, target, callback) {
    gMap.registerSecureElementTarget(msg.appId, target);
    let readers = Object.keys(this._sePresence).map(type => {
      return { type: type, isPresent: this._sePresence[type] };
    });
    callback({ readers: readers, error: SE.ERROR_NONE });
  },

  _handleChildProcessShutdown: function(target) {
    let channels = gMap.getChannelsByTarget(target);

    let createCb = (seType, channelNumber) => {
      return {
        notifyCloseChannelSuccess: () => {
          debug("closed " + seType + ", channel " + channelNumber);
        },

        notifyError: (reason) => {
          debug("Failed to close  " + seType + " channel " +
                channelNumber + ", reason: " + reason);
        }
      };
    };

    channels.forEach((channel) => {
      let connector = getConnector(channel.seType);
      if (!connector) {
        return;
      }

      connector.closeChannel(channel.channelNumber,
                             createCb(channel.seType, channel.channelNumber));
    });

    gMap.unregisterSecureElementTarget(target);
  },

  _sendSEResponse: function(msg, result) {
    let promiseStatus = (result.error === SE.ERROR_NONE) ? "Resolved" : "Rejected";
    result.resolverId = msg.data.resolverId;
    msg.target.sendAsyncMessage(msg.name + promiseStatus, {result: result});
  },

  _isValidMessage: function(msg) {
    let appIdValid = gMap.isAppIdRegistered(msg.data.appId);
    return msg.name === "SE:GetSEReaders" ? true : appIdValid;
  },

  



  receiveMessage: function(msg) {
    DEBUG && debug("Received '" + msg.name + "' message from content process" +
                   ": " + JSON.stringify(msg.data));

    if (msg.name === "child-process-shutdown") {
      this._handleChildProcessShutdown(msg.target);
      return null;
    }

    if (SE_IPC_SECUREELEMENT_MSG_NAMES.indexOf(msg.name) !== -1) {
      if (!msg.target.assertPermission("secureelement-manage")) {
        debug("SecureElement message " + msg.name + " from a content process " +
              "with no 'secureelement-manage' privileges.");
        return null;
      }
    } else {
      debug("Ignoring unknown message type: " + msg.name);
      return null;
    }

    let callback = (result) => this._sendSEResponse(msg, result);
    if (!this._isValidMessage(msg)) {
      debug("Message not valid");
      callback({ error: SE.ERROR_GENERIC });
      return null;
    }

    switch (msg.name) {
      case "SE:GetSEReaders":
        this._handleGetSEReadersRequest(msg.data, msg.target, callback);
        break;
      case "SE:OpenChannel":
        this._handleOpenChannel(msg.data, callback);
        break;
      case "SE:CloseChannel":
        this._handleCloseChannel(msg.data, callback);
        break;
      case "SE:TransmitAPDU":
        this._handleTransmit(msg.data, callback);
        break;
    }
    return null;
  },

  



  observe: function(subject, topic, data) {
    if (topic === NS_XPCOM_SHUTDOWN_OBSERVER_ID) {
      this._shutdown();
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SecureElementManager]);
