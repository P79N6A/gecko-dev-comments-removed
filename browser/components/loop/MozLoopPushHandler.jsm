



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

const {MozLoopService} = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
const consoleLog = MozLoopService.log;

this.EXPORTED_SYMBOLS = ["MozLoopPushHandler"];

const CONNECTION_STATE_CLOSED = 0;
const CONNECTION_STATE_CONNECTING = 1;
const CONNECTION_STATE_OPEN = 2;

const SERVICE_STATE_OFFLINE = 0;
const SERVICE_STATE_PENDING = 1;
const SERVICE_STATE_ACTIVE = 2;

function PushSocket(webSocket = null) {
  this._websocket = webSocket;
}

PushSocket.prototype = {

  















  connect: function(pushUri, onMsg, onStart, onClose) {
    if (!pushUri || !onMsg || !onStart || !onClose) {
      throw new Error("PushSocket: missing required parameter(s):"
                      (pushUri ? "" : " pushUri") +
                      (onMsg ? "" : " onMsg") +
                      (onStart ? "" : " onStart") +
                      (onClose ? "" : " onClose"));
    }

    this._onMsg = onMsg;
    this._onStart = onStart;
    this._onClose = onClose;

    if (!this._websocket) {
      this._websocket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                          .createInstance(Ci.nsIWebSocketChannel);
      this._websocket.initLoadInfo(null, 
                                   Services.scriptSecurityManager.getSystemPrincipal(),
                                   null, 
                                   Ci.nsILoadInfo.SEC_NORMAL,
                                   Ci.nsIContentPolicy.TYPE_WEBSOCKET);
    }

    let uri = Services.io.newURI(pushUri, null, null);
    this._websocket.protocol = "push-notification";
    this._websocket.asyncOpen(uri, pushUri, this, null);
  },

  




  onStart: function() {
    this._socketOpen = true;
    this._onStart();
  },

  





  onStop: function(aContext, aStatusCode) {
    this._socketOpen = false;
    this._onClose(aStatusCode, "websocket onStop");
  },

  







  onServerClose: function(aContext, aCode, aReason) {
    this._socketOpen = false;
    this._onClose(aCode, aReason);
  },

  






  onMessageAvailable: function(aContext, aMsg) {
    consoleLog.log("PushSocket: Message received: ", aMsg);
    if (!this._socketOpen) {
      consoleLog.error("Message received in Winsocket closed state");
      return;
    }

    try {
      this._onMsg(JSON.parse(aMsg));
    }
    catch (error) {
      consoleLog.error("PushSocket: error parsing message payload - ", error);
    }
  },

  







  onBinaryMessageAvailable: function(aContext, aMsg) {
    consoleLog.log("PushSocket: Binary message received: ", aMsg);
    if (!this._socketOpen) {
      consoleLog.error("PushSocket: message receive in Winsocket closed state");
      return;
    }

    try {
      this._onMsg(JSON.parse(aMsg));
    }
    catch (error) {
      consoleLog.error("PushSocket: error parsing message payload - ", error);
    }
  },

  






  send: function(aMsg) {
    if (!this._socketOpen) {
      consoleLog.error("PushSocket: attempt to send before websocket is open");
      return false;
    }

    let msg;
    try {
      msg = JSON.stringify(aMsg);
    }
    catch (error) {
      consoleLog.error("PushSocket: JSON generation error - ", error);
      return false;
    }

    try {
      this._websocket.sendMsg(msg);
      consoleLog.log("PushSocket: Message sent: ", msg);
    }
    
    catch (e) {
      consoleLog.warn("PushSocket: websocket send error", e);
      return false;
    }

    return true;
  },

  


  close: function() {
    if (!this._socketOpen) {
      return;
    }

    this._socketOpen = false;
    consoleLog.info("PushSocket: websocket closing");

    
    this._onStart = function() {};
    this._onMsg = this._onStart;
    this._onClose = this._onStart;

    try {
      this._websocket.close(this._websocket.CLOSE_NORMAL);
    }
    catch (e) {}
  },
};











function RetryManager (startDelay, maxDelay) {
  if (!startDelay || !maxDelay) {
    throw new Error("RetryManager: missing required parameters(s)" +
                     (startDelay ? "" : " startDelay") +
                     (maxDelay ? "" : " maxDelay"));
  }

  this._startDelay = startDelay;
  
  this._maxDelay = maxDelay > startDelay ? maxDelay : startDelay;
}

RetryManager.prototype = {
  




  retry: function(delayedOp) {
    if (!this._timeoutID) {
      this._retryDelay = this._startDelay;
    } else {
      clearTimeout(this._timeoutID);
      let nextDelay = this._retryDelay * 2;
      this._retryDelay = nextDelay > this._maxDelay ? this._maxDelay : nextDelay;
    }

    this._timeoutID = setTimeout(delayedOp, this._retryDelay);
    consoleLog.log("PushHandler: retry delay set for ", this._retryDelay);
  },

  



  reset: function() {
    if (this._timeoutID) {
      clearTimeout(this._timeoutID);
      this._timeoutID = null;
    }
  },
};

















function PingMonitor(pingFunc, onTimeout, interval, timeout) {
  if (!pingFunc || !onTimeout || !interval || !timeout) {
    throw new Error("PingMonitor: missing required parameters");
  }
  this._onTimeout = onTimeout;
  this._pingFunc = pingFunc;
  this._pingInterval = interval;
  this._pingTimeout = timeout;
}

PingMonitor.prototype = {
  


  restart: function () {
    consoleLog.info("PushHandler: ping timeout restart");
    this.stop();
    this._pingTimerID = setTimeout(() => {this._pingSend()}, this._pingInterval);
  },

  


  stop: function() {
    if (this._pingTimerID){
      clearTimeout(this._pingTimerID);
      this._pingTimerID = undefined;
    }
  },

  _pingSend: function () {
    consoleLog.info("PushHandler: ping sent");
    this._pingTimerID = setTimeout(this._onTimeout, this._pingTimeout);
    this._pingFunc();
  },
};






let MozLoopPushHandler = {
  
  pushServerUri: undefined,
  
  
  channels: new Map(),
  
  uaID: undefined,
  
  registeredChannels: {},
  
  serviceState: SERVICE_STATE_OFFLINE,
  
  connectionState: CONNECTION_STATE_CLOSED,
  
  _channelsToRegister: [],

  get _startRetryDelay_ms() {
    try {
      return Services.prefs.getIntPref("loop.retry_delay.start");
    }
    catch (e) {
      return 60000; 
    }
  },

  get _maxRetryDelay_ms() {
    try {
      return Services.prefs.getIntPref("loop.retry_delay.limit");
    }
    catch (e) {
      return 300000; 
    }
  },

  get _pingInterval_ms() {
    try {
      return Services.prefs.getIntPref("loop.ping.interval");
    }
    catch (e) {
      return 18000000; 
    }
  },

  get _pingTimeout_ms() {
    try {
      return Services.prefs.getIntPref("loop.ping.timeout");
    }
    catch (e) {
      return 10000; 
    }
  },

   








  initialize: function(options = {}) {
    consoleLog.info("PushHandler: initialize options = ", options);
    if (Services.io.offline) {
      consoleLog.warn("PushHandler: IO offline");
      return false;
    }

    if (this._initDone) {
      return true;
    }

    this._initDone = true;
    this._retryManager = new RetryManager(this._startRetryDelay_ms,
                                          this._maxRetryDelay_ms);
    
    
    this._pingMonitor = new PingMonitor(() => this._pushSocket.send({}),
                                        () => this._restartConnection(),
                                        this._pingInterval_ms,
                                        this._pingTimeout_ms);

    if ("mockWebSocket" in options) {
      this._mockWebSocket = options.mockWebSocket;
    }

    this._openSocket();
    return true;
  },

  



  shutdown: function() {
    consoleLog.info("PushHandler: shutdown");
    if (!this._initDone) {
      return;
    }

    this._initDone = false;
    this._retryManager.reset();
    this._pingMonitor.stop();

    
    if (this.connectionState === CONNECTION_STATE_OPEN) {
      Object.keys(this.registeredChannels).forEach((id) => {
        let unRegMsg = {messageType: "unregister",
                        channelID: id};
        this._pushSocket.send(unRegMsg);
      });
      this.registeredChannels = {};
    }

    this.connectionState = CONNECTION_STATE_CLOSED;
    this.serviceState = SERVICE_STATE_OFFLINE;
    this._pushSocket.close();
    this._pushSocket = undefined;
    
    
    this.channels.clear();
    this.uaID = undefined;
    this.pushUrl = undefined;
    this.pushServerUri = undefined;
  },

   




























  register: function(channelID, onRegistered, onNotification) {
    if (!channelID || !onRegistered || !onNotification) {
      throw new Error("missing required parameter(s):" +
                      (channelID ? "" : " channelID") +
                      (onRegistered ? "" : " onRegistered") +
                      (onNotification ? "" : " onNotification"));
    }

    consoleLog.info("PushHandler: channel registration: ", channelID);
    if (this.channels.has(channelID)) {
      
      
      if (this.registeredChannels[channelID]) {
        onRegistered(null, this.registeredChannels[channelID], channelID);
      }
      
      this.channels.set(channelID, {onRegistered: onRegistered,
                        onNotification: onNotification});
      return;
    }

    this.channels.set(channelID, {onRegistered: onRegistered,
                                  onNotification: onNotification});
    this._channelsToRegister.push(channelID);
    this._registerChannels();
  },
  
  




  unregister: function(channelID) {
    consoleLog.info("MozLoopPushHandler: un-register channel ", channelID);
    if (!this.channels.has(channelID)) {
      return;
    }

    this.channels.delete(channelID);

    if (this.registeredChannels[channelID]) {
      delete this.registeredChannels[channelID];
      if (this.connectionState === CONNECTION_STATE_OPEN) {
        this._pushSocket.send({messageType: "unregister",
                               channelID: channelID});
      }
    }
  },

  




  _onStart: function() {
    consoleLog.info("PushHandler: websocket open, sending 'hello' to PushServer");
    this.connectionState = CONNECTION_STATE_OPEN;
    
    
    
    
    this.serviceState = SERVICE_STATE_PENDING;
    let helloMsg = {
      messageType: "hello",
      uaid: this.uaID || "",
      channelIDs: this.uaID ? Object.keys(this.registeredChannels) : []
    };
    
    
    
    
    this._retryManager.reset();
    this._retryManager.retry(() => this._restartConnection());
    this._pushSocket.send(helloMsg);
  },

  





  _onClose: function(aCode, aReason) {
    this._pingMonitor.stop();

    switch (this.connectionState) {
    case CONNECTION_STATE_OPEN:
        this.connectionState = CONNECTION_STATE_CLOSED;
        consoleLog.info("PushHandler: websocket closed: begin reconnect - ", aCode);
        
        this._retryManager.reset();
        this._openSocket();
        break;

      case CONNECTION_STATE_CONNECTING:
        
        consoleLog.info("PushHandler: websocket closed: delay and retry - ", aCode);
        this._retryManager.retry(() => this._openSocket());
        break;
     }
   },

  




  _onMsg: function(aMsg) {
    
    
    if (aMsg.error) {
      consoleLog.error("PushHandler: received error response msg: ", aMsg.error);
      return;
    }

    
    
    if (!aMsg.messageType && this.serviceState === SERVICE_STATE_ACTIVE) {
      
      this._pingMonitor.restart();
      return;
    }

    switch(aMsg.messageType) {
      case "hello":
        this._onHello(aMsg);
        break;

      case "register":
        this._onRegister(aMsg);
        break;

      case "notification":
        this._onNotification(aMsg);
        break;

      default:
        consoleLog.warn("PushHandler: unknown message type = ", aMsg.messageType);
        if (this.serviceState === SERVICE_STATE_ACTIVE) {
          
          this._pingMonitor.restart();
        }
        break;
     }
   },

  







  _onHello: function(aMsg) {
    if (this.serviceState !== SERVICE_STATE_PENDING) {
      consoleLog.error("PushHandler: extra 'hello' response received from PushServer");
      return;
    }

    
    this._retryManager.reset();
    this.serviceState = SERVICE_STATE_ACTIVE;
    consoleLog.info("PushHandler: 'hello' handshake complete");
    
    this._pingMonitor.restart();
    
    
    if (this.uaID !== aMsg.uaid) {
      consoleLog.log("PushHandler: registering all channels");
      this.uaID = aMsg.uaid;
      
      this._channelsToRegister = [...this.channels.keys()];
      this.registeredChannels = {};
    }
    
    this._registerChannels();
  },

  









  _onNotification: function(aMsg) {
    if (this.serviceState !== SERVICE_STATE_ACTIVE ||
       this.registeredChannels.length === 0) {
      
      
      consoleLog.error("PushHandler: protocol error - notification received in wrong state");
      this._restartConnection();
      return;
    }

    this._pingMonitor.restart();
    if (Array.isArray(aMsg.updates) && aMsg.updates.length > 0) {
      let ackChannels = [];
      aMsg.updates.forEach(update => {
        if (update.channelID in this.registeredChannels) {
          consoleLog.log("PushHandler: notification: version = ", update.version,
                         ", channelID = ", update.channelID);
          this.channels.get(update.channelID)
            .onNotification(update.version, update.channelID);
          ackChannels.push(update);
        } else {
          consoleLog.error("PushHandler: notification received for unknown channelID: ",
                           update.channelID);
        }
      });

      consoleLog.log("PushHandler: PusherServer 'ack': ", ackChannels);
      this._pushSocket.send({messageType: "ack",
                             updates: ackChannels});
     }
   },

  




  _onRegister: function(msg) {
    if (this.serviceState !== SERVICE_STATE_ACTIVE ||
        msg.channelID != this._pendingChannelID) {
      
      
      
      consoleLog.error("PushHandler: registration protocol error");
      this._restartConnection();
      return;
    }

    this._retryManager.reset();
    this._pingMonitor.restart();

    switch (msg.status) {
      case 200:
        consoleLog.info("PushHandler: channel registered: ", msg.channelID);
        this.registeredChannels[msg.channelID] = msg.pushEndpoint;
        this.channels.get(msg.channelID)
          .onRegistered(null, msg.pushEndpoint, msg.channelID);
        this._registerNext();
        break;

      case 500:
        consoleLog.info("PushHandler: eeceived a 500 retry response from the PushServer: ",
                        msg.channelID);
        
        this._retryManager.retry(() => this._sendRegistration(msg.channelID));
        break;

      case 409:
        consoleLog.error("PushHandler: received a 409 response from the PushServer: ",
                         msg.channelID);
        this.channels.get(this._pendingChannelID).onRegistered("409");
        
        this.channels.delete(this._pendingChannelID);
        this._registerNext();
        break;

      default:
        consoleLog.error("PushHandler: received error ", msg.status,
                         " from the PushServer: ", msg.channelID);
        this.channels.get(this._pendingChannelID).onRegistered(msg.status);
        this.channels.delete(this._pendingChannelID);
        this._registerNext();
        break;
    }
  },

  







  _openSocket: function() {
    this.connectionState = CONNECTION_STATE_CONNECTING;
    
    this._pushSocket = new PushSocket(this._mockWebSocket);

    let performOpen = () => {
      consoleLog.info("PushHandler: attempt to open websocket to PushServer: ", this.pushServerUri);
      this._pushSocket.connect(this.pushServerUri,
                               (aMsg) => this._onMsg(aMsg),
                               () => this._onStart(),
                               (aCode, aReason) => this._onClose(aCode, aReason));
    }

    let pushServerURLFetchError = () => {
      consoleLog.warn("PushHandler: Could not retrieve push server URL from Loop server, will retry");
      this._pushSocket = undefined;
      this._retryManager.retry(() => this._openSocket());
      return;
    }

    try {
      this.pushServerUri = Services.prefs.getCharPref("loop.debug.pushserver");
    }
    catch (e) {}

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
            consoleLog.warn("PushHandler: Error parsing JSON response for push server URL");
            pushServerURLFetchError();
          }
          if (pushServerConfig.pushServerURI) {
            this._retryManager.reset();
            this.pushServerUri = pushServerConfig.pushServerURI;
            performOpen();
          } else {
            consoleLog.warn("PushHandler: push server URL config lacks pushServerURI parameter");
            pushServerURLFetchError();
          }
        } else {
          consoleLog.warn("PushHandler: push server URL retrieve error: " + req.status);
          pushServerURLFetchError();
        }
      };
      req.onerror = pushServerURLFetchError;
      req.send();
    } else {
      
      performOpen();
    }
  },

  


  _restartConnection: function() {
    this._retryManager.reset();
    this._pingMonitor.stop();
    this.serviceState = SERVICE_STATE_OFFLINE;
    this._pendingChannelID = null;

    if (this.connectionState === CONNECTION_STATE_OPEN) {
      
      this.connectionState = CONNECTION_STATE_CLOSED;
      this._pushSocket.close();
      consoleLog.warn("PushHandler: connection error: re-establishing connection to PushServer");
      this._openSocket();
    }
  },

  


  _registerChannels: function() {
    
    
    if (this.serviceState !== SERVICE_STATE_ACTIVE ||
       this._pendingChannelID) {
      return;
    }
    this._registerNext();
  },

  


  _registerNext: function() {
    this._pendingChannelID = this._channelsToRegister.pop();
    this._sendRegistration(this._pendingChannelID);
  },

  




  _sendRegistration: function(channelID) {
    if (channelID) {
      this._pushSocket.send({messageType: "register",
                             channelID: channelID});
    }
  },
}
