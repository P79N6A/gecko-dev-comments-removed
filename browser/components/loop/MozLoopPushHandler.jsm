



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopPushHandler"];

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");





let MozLoopPushHandler = {
  
  pushServerUri: undefined,
  
  
  channels: {},
  
  uaID: undefined,
  
  registeredChannels: {},

  _channelsToRegister: {},

  _minRetryDelay_ms: (() => {
    try {
      return Services.prefs.getIntPref("loop.retry_delay.start")
    }
    catch (e) {
      return 60000 
    }
  })(),

  _maxRetryDelay_ms: (() => {
    try {
      return Services.prefs.getIntPref("loop.retry_delay.limit")
    }
    catch (e) {
      return 300000 
    }
  })(),

   








  initialize: function(options = {}) {
    if (Services.io.offline) {
      console.warn("MozLoopPushHandler - IO offline");
      return false;
    }

    if (this._initDone) {
      return true;
    }

    this._initDone = true;

    if ("mockWebSocket" in options) {
      this._mockWebSocket = options.mockWebSocket;
    }

    this._openSocket();
    return true;
  },

   





















  register: function(channelID, onRegistered, onNotification) {
    if (!channelID || !onRegistered || !onNotification) {
      throw new Error("missing required parameter(s):"
                      + (channelID ? "" : " channelID")
                      + (onRegistered ? "" : " onRegistered")
                      + (onNotification ? "" : " onNotification"));
    }

    
    
    if (channelID in this.channels) {
      onRegistered("error: channel already registered: " + channelID);
      return;
    }

    this.channels[channelID] = {
      onRegistered: onRegistered,
      onNotification: onNotification
    };

    
    
    if (this._registrationID) {
      this._channelsToRegister.push(channelID);
    } else {
      this._registerChannels();
    }
  },

  





  onStart: function() {
    this._retryEnd();
    
    
    
    let helloMsg = {
          messageType: "hello",
          uaid: this.uaID || "",
          channelIDs: Object.keys(this.registeredChannels)};

    this._retryOperation(() => this.onStart(), this._maxRetryDelay_ms);
    try { 
      this._websocket.sendMsg(JSON.stringify(helloMsg));
    }
    catch (e) {console.warn("MozLoopPushHandler::onStart websocket.sendMsg() failure");}
  },

  





  onStop: function(aContext, aStatusCode) {
    Cu.reportError("Loop Push server web socket closed! Code: " + aStatusCode);
    this._retryOperation(() => this._openSocket());
  },

  








  onServerClose: function(aContext, aCode) {
    Cu.reportError("Loop Push server web socket closed (server)! Code: " + aCode);
    this._retryOperation(() => this._openSocket());
  },

  





  onMessageAvailable: function(aContext, aMsg) {
    let msg = JSON.parse(aMsg);

    switch(msg.messageType) {
      case "hello":
        this._retryEnd();
        this._isConnected = true;
        if (this.uaID !== msg.uaid) {
          this.uaID = msg.uaid;
          this.registeredChannels = {};
          this._registerChannels();
        }
        break;

      case "register":
        this._onRegister(msg);
        break;

      case "notification":
        msg.updates.forEach((update) => {
          if (update.channelID in this.registeredChannels) {
            this.channels[update.channelID].onNotification(update.version, update.channelID);
          }
        });
        break;
    }
  },

  




  _onRegister: function(msg) {
    let registerNext = () => {
      this._registrationID = this._channelsToRegister.shift();
      this._sendRegistration(this._registrationID);
    }

    switch (msg.status) {
      case 200:
        if (msg.channelID == this._registrationID) {
          this._retryEnd(); 
          this.registeredChannels[msg.channelID] = msg.pushEndpoint;
          this.channels[msg.channelID].onRegistered(null, msg.pushEndpoint, msg.channelID);
          registerNext();
        }
        break;

      case 500:
        
        this._retryOperation(() => this._sendRegistration(msg.channelID));
        break;

      case 409:
        this.channels[this._registrationID].onRegistered(
          "error: PushServer ChannelID already in use: " + msg.channelID);
        registerNext();
        break;

      default:
        let id = this._channelsToRegister.shift();
        this.channels[this._registrationID].onRegistered(
          "error: PushServer registration failure, status = " + msg.status);
        registerNext();
        break;
    }
  },

  







  _openSocket: function() {
    this._isConnected = false;

    if (this._mockWebSocket) {
      
      this._websocket = this._mockWebSocket;
    } else {
      this._websocket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                        .createInstance(Ci.nsIWebSocketChannel);
    }

    this._websocket.protocol = "push-notification";

    let performOpen = () => {
      let uri = Services.io.newURI(this.pushServerUri, null, null);
      this._websocket.asyncOpen(uri, this.pushServerUri, this, null);
    }

    let pushServerURLFetchError = () => {
      console.warn("MozLoopPushHandler - Could not retrieve push server URL from Loop server; using default");
      this.pushServerUri = Services.prefs.getCharPref("services.push.serverURL");
      performOpen();
    }

    if (!this.pushServerUri) {
      
      let pushUrlEndpoint = Services.prefs.getCharPref("loop.server") + "/push-server-config";
      let req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                createInstance(Ci.nsIXMLHttpRequest);
      req.open("GET", pushUrlEndpoint);
      req.onload = () => {
        if (req.status >= 200 && req.status < 300) {
          let pushServerConfig;
          try {
            pushServerConfig = JSON.parse(req.responseText);
          } catch (e) {
            console.warn("MozLoopPushHandler - Error parsing JSON response for push server URL");
            pushServerURLFetchError();
          }
          if (pushServerConfig.pushServerURI) {
            this.pushServerUri = pushServerConfig.pushServerURI;
            performOpen();
          } else {
            console.warn("MozLoopPushHandler - push server URL config lacks pushServerURI parameter");
            pushServerURLFetchError();
          }
        } else {
          console.warn("MozLoopPushHandler - push server URL retrieve error: " + req.status);
          pushServerURLFetchError();
        }
      };
      req.onerror = pushServerURLFetchError;
      req.send();
    } else {
      
      performOpen();
    }
  },

  


  _registerChannels: function() {
    
    if (!this._isConnected) {
      return;
    }

    
    
    if (!this._registrationID) {
      
      this._channelsToRegister = Object.keys(this.channels).filter((id) => {
        return !(id in this.registeredChannels);
      });
      this._registrationID = this._channelsToRegister.shift();
      this._sendRegistration(this._registrationID);
    }
  },

  




  _sendRegistration: function(channelID) {
    if (channelID) {
      try { 
        this._websocket.sendMsg(JSON.stringify({messageType: "register",
                                                channelID: channelID}));
      }
      catch (e) {console.warn("MozLoopPushHandler::_registerChannel websocket.sendMsg() failure");}
    }
  },

  







  _retryOperation: function(delayedOp, retryDelay) {
    if (!this._retryCount) {
      this._retryDelay = retryDelay || this._minRetryDelay_ms;
      this._retryCount = 1;
    } else {
      let nextDelay = this._retryDelay * 2;
      this._retryDelay = nextDelay > this._maxRetryDelay_ms ? this._maxRetryDelay_ms : nextDelay;
      this._retryCount += 1;
    }
    this._timeoutID = setTimeout(delayedOp, this._retryDelay);
  },

  



  _retryEnd: function() {
    if (this._retryCount) {
      clearTimeout(this._timeoutID);
      this._retryCount = 0;
    }
  }
};
