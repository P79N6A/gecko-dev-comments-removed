




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");

#ifdef MOZ_B2G
XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");
#endif

var threadManager = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);

const kUDP_WAKEUP_WS_STATUS_CODE = 4774;  
                                          
                                          

const kWS_MAX_WENTDOWN = 2;


const kWS_MIN_PING_INTERVAL = 60000;

const prefs = new Preferences("dom.push.");

this.EXPORTED_SYMBOLS = ["PushServiceWebSocket"];


let gDebuggingEnabled = true;

function debug(s) {
  if (gDebuggingEnabled)
    dump("-*- PushServiceWebSocket.jsm: " + s + "\n");
}









this.PushWebSocketListener = function(pushService) {
  this._pushService = pushService;
}

this.PushWebSocketListener.prototype = {
  onStart: function(context) {
    if (!this._pushService)
        return;
    this._pushService._wsOnStart(context);
  },

  onStop: function(context, statusCode) {
    if (!this._pushService)
        return;
    this._pushService._wsOnStop(context, statusCode);
  },

  onAcknowledge: function(context, size) {
    
  },

  onBinaryMessageAvailable: function(context, message) {
    
  },

  onMessageAvailable: function(context, message) {
    if (!this._pushService)
        return;
    this._pushService._wsOnMessageAvailable(context, message);
  },

  onServerClose: function(context, aStatusCode, aReason) {
    if (!this._pushService)
        return;
    this._pushService._wsOnServerClose(context, aStatusCode, aReason);
  }
};



const STATE_SHUT_DOWN = 0;


const STATE_WAITING_FOR_WS_START = 1;

const STATE_WAITING_FOR_HELLO = 2;

const STATE_READY = 3;

this.PushServiceWebSocket = {
  _mainPushService: null,

  observeNetworkChange: function(aSubject, aTopic, aData) {
    
    
    
    if (this._udpServer) {
      this._udpServer.close();
      
      this._udpServer = null;
    }

    this._shutdownWS();

    
    
    if (aTopic === "network-active-changed" || aData === "online") {
      this.startListeningIfChannelsPresent();
    }
  },

  observePushConnectionPref: function(enabled) {
    if (enabled) {
      this.startListeningIfChannelsPresent();
    } else {
      this._shutdownWS();
    }
  },

  observeDebug: function(enabled) {
    gDebuggingEnabled = enabled;
  },
  shutdownService: function() {
    this._shutdownWS();
  },

  observeTimer: function(aSubject, aTopic, aData) {
    if (aSubject == this._requestTimeoutTimer) {
      if (Object.keys(this._pendingRequests).length == 0) {
        this._requestTimeoutTimer.cancel();
      }

      
      let requestTimedOut = false;
      for (let channelID in this._pendingRequests) {
        let duration = Date.now() - this._pendingRequests[channelID].ctime;
        
        
        if (requestTimedOut || duration > this._requestTimeout) {
          debug("Request timeout: Removing " + channelID);
          requestTimedOut = true;
          this._pendingRequests[channelID]
            .deferred.reject({status: 0, error: "TimeoutError"});

          delete this._pendingRequests[channelID];
          for (let i = this._requestQueue.length - 1; i >= 0; --i) {
            let [, data] = this._requestQueue[i];
            if (data && data.channelID == channelID) {
              this._requestQueue.splice(i, 1);
            }
          }
        }
      }

      
      
      if (requestTimedOut) {
        this._shutdownWS();
        this._reconnectAfterBackoff();
      }
    }
  },

  get _UAID() {
    return prefs.get("userAgentID");
  },

  set _UAID(newID) {
    if (typeof(newID) !== "string") {
      debug("Got invalid, non-string UAID " + newID +
            ". Not updating userAgentID");
      return;
    }
    debug("New _UAID: " + newID);
    prefs.set("userAgentID", newID);
  },

  
  _requestQueue: [],
  _ws: null,
  _pendingRequests: {},
  _currentState: STATE_SHUT_DOWN,
  _requestTimeout: 0,
  _requestTimeoutTimer: null,
  _retryFailCount: 0,

  









  _willBeWokenUpByUDP: false,

  





  _adaptiveEnabled: false,

  






  _recalculatePing: true,

  





  _pingIntervalRetryTimes: {},

  


  _lastGoodPingInterval: 0,

  


  _upperLimit: 0,

  



  _wsWentDownCounter: 0,

  



  _wsSendMessage: function(msg) {
    if (!this._ws) {
      debug("No WebSocket initialized. Cannot send a message.");
      return;
    }
    msg = JSON.stringify(msg);
    debug("Sending message: " + msg);
    this._ws.sendMsg(msg);
  },

  init: function(options, mainPushService) {
    debug("init()");

    this._mainPushService = mainPushService;

    
    
    
    if (options.makeWebSocket) {
      this._makeWebSocket = options.makeWebSocket;
    }

    
    
    
    if (options.makeUDPSocket) {
      this._makeUDPSocket = options.makeUDPSocket;
    }

    this._networkInfo = options.networkInfo;
    if (!this._networkInfo) {
      this._networkInfo = PushNetworkInfo;
    }

    this._requestTimeout = prefs.get("requestTimeout");
    this._adaptiveEnabled = prefs.get('adaptive.enabled');
    this._upperLimit = prefs.get('adaptive.upperLimit');
  },

  _shutdownWS: function() {
    debug("shutdownWS()");
    this._currentState = STATE_SHUT_DOWN;
    this._willBeWokenUpByUDP = false;

    if (this._wsListener)
      this._wsListener._pushService = null;
    try {
        this._ws.close(0, null);
    } catch (e) {}
    this._ws = null;

    this._waitingForPong = false;
    this._mainPushService.stopAlarm();
    this._cancelPendingRequests();
  },

  uninit: function() {

    if (this._udpServer) {
      this._udpServer.close();
      this._udpServer = null;
    }

    
    
    
    this._shutdownWS();

    if (this._requestTimeoutTimer) {
      this._requestTimeoutTimer.cancel();
    }

    this._mainPushService = null;
  },

  
















  _reconnectAfterBackoff: function() {
    debug("reconnectAfterBackoff()");
    
    this._calculateAdaptivePing(true );

    
    let retryTimeout = prefs.get("retryBaseInterval") *
                       Math.pow(2, this._retryFailCount);
    retryTimeout = Math.min(retryTimeout, prefs.get("pingInterval"));

    this._retryFailCount++;

    debug("Retry in " + retryTimeout + " Try number " + this._retryFailCount);
    this._mainPushService.setAlarm(retryTimeout);
  },

  























  _calculateAdaptivePing: function(wsWentDown) {
    debug('_calculateAdaptivePing()');
    if (!this._adaptiveEnabled) {
      debug('Adaptive ping is disabled');
      return;
    }

    if (this._retryFailCount > 0) {
      debug('Push has failed to connect to the Push Server ' +
        this._retryFailCount + ' times. ' +
        'Do not calculate a new pingInterval now');
      return;
    }

    if (!wsWentDown) {
      debug('Setting websocket down counter to 0');
      this._wsWentDownCounter = 0;
    }

    if (!this._recalculatePing && !wsWentDown) {
      debug('We do not need to recalculate the ping now, based on previous data');
      return;
    }

    
    let ns = this._networkInfo.getNetworkInformation();

    if (ns.ip) {
      
      debug('mobile');
      let oldNetwork = prefs.get('adaptive.mobile');
      let newNetwork = 'mobile-' + ns.mcc + '-' + ns.mnc;

      
      if (oldNetwork !== newNetwork) {
        
        debug('Mobile networks differ. Old network is ' + oldNetwork +
              ' and new is ' + newNetwork);
        prefs.set('adaptive.mobile', newNetwork);
        
        this._recalculatePing = true;
        this._pingIntervalRetryTimes = {};

        
        let defaultPing = prefs.get('pingInterval.default');
        prefs.set('pingInterval', defaultPing);
        this._lastGoodPingInterval = defaultPing;

      } else {
        
        prefs.set('pingInterval', prefs.get('pingInterval.mobile'));
        this._lastGoodPingInterval = prefs.get('adaptive.lastGoodPingInterval.mobile');
      }

    } else {
      
      debug('wifi');
      prefs.set('pingInterval', prefs.get('pingInterval.wifi'));
      this._lastGoodPingInterval = prefs.get('adaptive.lastGoodPingInterval.wifi');
    }

    let nextPingInterval;
    let lastTriedPingInterval = prefs.get('pingInterval');

    if (!this._recalculatePing && wsWentDown) {
      debug('Websocket disconnected without ping adaptative algorithm running');
      this._wsWentDownCounter++;
      if (this._wsWentDownCounter > kWS_MAX_WENTDOWN) {
        debug('Too many disconnects. Reenabling ping adaptative algoritm');
        this._wsWentDownCounter = 0;
        this._recalculatePing = true;
        this._lastGoodPingInterval = Math.floor(lastTriedPingInterval / 2);
        if (this._lastGoodPingInterval < kWS_MIN_PING_INTERVAL) {
          nextPingInterval = kWS_MIN_PING_INTERVAL;
        } else {
          nextPingInterval = this._lastGoodPingInterval;
        }
        prefs.set('pingInterval', nextPingInterval);
        this._save(ns, nextPingInterval);
        return;
      }

      debug('We do not need to recalculate the ping, based on previous data');
    }

    if (wsWentDown) {
      debug('The WebSocket was disconnected, calculating next ping');

      
      this._pingIntervalRetryTimes[lastTriedPingInterval] =
           (this._pingIntervalRetryTimes[lastTriedPingInterval] || 0) + 1;

       
       
       if (this._pingIntervalRetryTimes[lastTriedPingInterval] < 2) {
         debug('pingInterval= ' + lastTriedPingInterval + ' tried only ' +
           this._pingIntervalRetryTimes[lastTriedPingInterval] + ' times');
         return;
       }

       
       nextPingInterval = Math.floor(lastTriedPingInterval / 2);

      
      
      if (nextPingInterval - this._lastGoodPingInterval < prefs.get('adaptive.gap')) {
        debug('We have reached the gap, we have finished the calculation');
        debug('nextPingInterval=' + nextPingInterval);
        debug('lastGoodPing=' + this._lastGoodPingInterval);
        nextPingInterval = this._lastGoodPingInterval;
        this._recalculatePing = false;
      } else {
        debug('We need to calculate next time');
        this._recalculatePing = true;
      }

    } else {
      debug('The WebSocket is still up');
      this._lastGoodPingInterval = lastTriedPingInterval;
      nextPingInterval = Math.floor(lastTriedPingInterval * 1.5);
    }

    
    if (this._upperLimit < nextPingInterval) {
      debug('Next ping will be bigger than the configured upper limit, capping interval');
      this._recalculatePing = false;
      this._lastGoodPingInterval = lastTriedPingInterval;
      nextPingInterval = lastTriedPingInterval;
    }

    debug('Setting the pingInterval to ' + nextPingInterval);
    prefs.set('pingInterval', nextPingInterval);

    this._save(ns, nextPingInterval);
  },

  _save: function(ns, nextPingInterval){
    
    if (ns.ip) {
      prefs.set('pingInterval.mobile', nextPingInterval);
      prefs.set('adaptive.lastGoodPingInterval.mobile', this._lastGoodPingInterval);
    } else {
      prefs.set('pingInterval.wifi', nextPingInterval);
      prefs.set('adaptive.lastGoodPingInterval.wifi', this._lastGoodPingInterval);
    }
  },

  _makeWebSocket: function(uri) {
    if (!prefs.get("connection.enabled")) {
      debug("_makeWebSocket: connection.enabled is not set to true. Aborting.");
      return null;
    }
    if (Services.io.offline) {
      debug("Network is offline.");
      return null;
    }
    let socket;
    if (uri.scheme === "wss") {
      socket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                 .createInstance(Ci.nsIWebSocketChannel);
    }
    else if (uri.scheme === "ws") {
      if (uri.host != "localhost") {
        debug("Push over an insecure connection (ws://) is not allowed!");
        return null;
      }
      socket = Cc["@mozilla.org/network/protocol;1?name=ws"]
                 .createInstance(Ci.nsIWebSocketChannel);
    }
    else {
      debug("Unsupported websocket scheme " + uri.scheme);
      return null;
    }
    socket.initLoadInfo(null, 
                        Services.scriptSecurityManager.getSystemPrincipal(),
                        null, 
                        Ci.nsILoadInfo.SEC_NORMAL,
                        Ci.nsIContentPolicy.TYPE_WEBSOCKET);

    return socket;
  },

  _beginWSSetup: function() {
    debug("beginWSSetup()");
    if (this._currentState != STATE_SHUT_DOWN) {
      debug("_beginWSSetup: Not in shutdown state! Current state " +
            this._currentState);
      return;
    }

    
    this._mainPushService.stopAlarm();

    let uri = this._mainPushService.getServerURI();
    if (!uri) {
      return;
    }
    let socket = this._makeWebSocket(uri);
    if (!socket) {
      return;
    }
    this._ws = socket.QueryInterface(Ci.nsIWebSocketChannel);

    debug("serverURL: " + uri.spec);
    this._wsListener = new PushWebSocketListener(this);
    this._ws.protocol = "push-notification";

    try {
      
      
      this._ws.asyncOpen(uri, uri.spec, this._wsListener, null);
      this._acquireWakeLock();
      this._currentState = STATE_WAITING_FOR_WS_START;
    } catch(e) {
      debug("Error opening websocket. asyncOpen failed!");
      this._shutdownWS();
      this._reconnectAfterBackoff();
    }
  },

  startListeningIfChannelsPresent: function() {
    
    if (this._requestQueue.length > 0) {
      this._beginWSSetup();
      return;
    }
    this._mainPushService.getAllChannelIDs()
      .then(channelIDs => {
        if (channelIDs.length > 0) {
          this._beginWSSetup();
        }
      });
  },

  

























  onAlarmFired: function() {
    
    
    if (this._waitingForPong) {
      debug("Did not receive pong in time. Reconnecting WebSocket.");
      this._shutdownWS();
      this._reconnectAfterBackoff();
    }
    else if (this._currentState == STATE_READY) {
      
      
      
      
      
      
      try {
        this._wsSendMessage({});
      } catch (e) {
      }

      this._waitingForPong = true;
      this._mainPushService.setAlarm(prefs.get("requestTimeout"));
    }
    else if (this._alarmID !== null) {
      debug("reconnect alarm fired.");
      
      
      
      
      
      
      
      
      
      
      

      
      this._beginWSSetup();
    }
  },

  _acquireWakeLock: function() {
#ifdef MOZ_B2G
    
    if (!this._socketWakeLock) {
      debug("Acquiring Socket Wakelock");
      this._socketWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._socketWakeLockTimer) {
      debug("Creating Socket WakeLock Timer");
      this._socketWakeLockTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }

    debug("Setting Socket WakeLock Timer");
    this._socketWakeLockTimer
      .initWithCallback(this._releaseWakeLock.bind(this),
                        
                        
                        
                        
                        this._requestTimeout + 1000,
                        Ci.nsITimer.TYPE_ONE_SHOT);
#endif
  },

  _releaseWakeLock: function() {
#ifdef MOZ_B2G
    debug("Releasing Socket WakeLock");
    if (this._socketWakeLockTimer) {
      this._socketWakeLockTimer.cancel();
    }
    if (this._socketWakeLock) {
      this._socketWakeLock.unlock();
      this._socketWakeLock = null;
    }
#endif
  },

  


  _handleHelloReply: function(reply) {
    debug("handleHelloReply()");
    if (this._currentState != STATE_WAITING_FOR_HELLO) {
      debug("Unexpected state " + this._currentState +
            "(expected STATE_WAITING_FOR_HELLO)");
      this._shutdownWS();
      return;
    }

    if (typeof reply.uaid !== "string") {
      debug("No UAID received or non string UAID received");
      this._shutdownWS();
      return;
    }

    if (reply.uaid === "") {
      debug("Empty UAID received!");
      this._shutdownWS();
      return;
    }

    
    if (reply.uaid.length > 128) {
      debug("UAID received from server was too long: " +
            reply.uaid);
      this._shutdownWS();
      return;
    }

    function finishHandshake() {
      this._UAID = reply.uaid;
      this._currentState = STATE_READY;
      this._processNextRequestInQueue();
    }

    
    
    
    
    
    if (this._UAID && this._UAID != reply.uaid) {
      debug("got new UAID: all re-register");

      this._mainPushService.notifyAllAppsRegister()
          .then(_ => this._mainPushService.dropRegistrations())
          .then(finishHandshake.bind(this));

      return;
    }

    
    finishHandshake.bind(this)();
  },

  


  _handleRegisterReply: function(reply) {
    debug("handleRegisterReply()");
    if (typeof reply.channelID !== "string" ||
        typeof this._pendingRequests[reply.channelID] !== "object")
      return;

    let tmp = this._pendingRequests[reply.channelID];
    delete this._pendingRequests[reply.channelID];
    if (Object.keys(this._pendingRequests).length == 0 &&
        this._requestTimeoutTimer)
      this._requestTimeoutTimer.cancel();

    if (reply.status == 200) {
      tmp.deferred.resolve(reply);
    } else {
      tmp.deferred.reject(reply);
    }
  },

  


  _handleNotificationReply: function(reply) {
    debug("handleNotificationReply()");
    if (typeof reply.updates !== 'object') {
      debug("No 'updates' field in response. Type = " + typeof reply.updates);
      return;
    }

    debug("Reply updates: " + reply.updates.length);
    for (let i = 0; i < reply.updates.length; i++) {
      let update = reply.updates[i];
      debug("Update: " + update.channelID + ": " + update.version);
      if (typeof update.channelID !== "string") {
        debug("Invalid update literal at index " + i);
        continue;
      }

      if (update.version === undefined) {
        debug("update.version does not exist");
        continue;
      }

      let version = update.version;

      if (typeof version === "string") {
        version = parseInt(version, 10);
      }

      if (typeof version === "number" && version >= 0) {
        
        
        this._receivedUpdate(update.channelID, version);
        this._sendAck(update.channelID, version);
      }
    }
  },

  
  _sendAck: function(channelID, version) {
    debug("sendAck()");
    this._send('ack', {
      updates: [{channelID: channelID, version: version}]
    });
  },

  


  _sendRequest: function(action, data) {
    debug("sendRequest() " + action);
    if (typeof data.channelID !== "string") {
      debug("Received non-string channelID");
      return Promise.reject({error: "Received non-string channelID"});
    }

    if (Object.keys(this._pendingRequests).length == 0) {
      
      if (!this._requestTimeoutTimer)
        this._requestTimeoutTimer = Cc["@mozilla.org/timer;1"]
                                      .createInstance(Ci.nsITimer);
      this._requestTimeoutTimer.init(this,
                                     this._requestTimeout,
                                     Ci.nsITimer.TYPE_REPEATING_SLACK);
    }

    let deferred;
    let request = this._pendingRequests[data.channelID];
    if (request) {
      
      
      
      deferred = request.deferred;
    } else {
      deferred = Promise.defer();
      request = this._pendingRequests[data.channelID] = {deferred};
    }
    request.ctime = Date.now();
    this._send(action, data);
    return deferred.promise;
  },

  _send: function(action, data) {
    debug("send()");
    this._requestQueue.push([action, data]);
    debug("Queued " + action);
    this._processNextRequestInQueue();
  },

  _processNextRequestInQueue: function() {
    debug("_processNextRequestInQueue()");

    if (this._requestQueue.length == 0) {
      debug("Request queue empty");
      return;
    }

    if (this._currentState != STATE_READY) {
      if (!this._ws) {
        
        this._beginWSSetup();
      }
      else {
        
        
      }
      return;
    }

    let [action, data] = this._requestQueue.shift();
    data.messageType = action;
    if (!this._ws) {
      
      
      
      
      debug("This should never happen!");
      this._shutdownWS();
    }

    this._wsSendMessage(data);
    
    setTimeout(this._processNextRequestInQueue.bind(this), 0);
  },

  _receivedUpdate: function(aChannelID, aLatestVersion) {
    debug("Updating: " + aChannelID + " -> " + aLatestVersion);

    let compareRecordVersionAndNotify = function(aPushRecord) {
      debug("compareRecordVersionAndNotify()");
      if (!aPushRecord) {
        debug("No record for channel ID " + aChannelID);
        return;
      }

      if (aPushRecord.version == null ||
          aPushRecord.version < aLatestVersion) {
        debug("Version changed, notifying app and updating DB");
        aPushRecord.version = aLatestVersion;
        aPushRecord.pushCount = aPushRecord.pushCount + 1;
        aPushRecord.lastPush = new Date().getTime();
        this._mainPushService.notifyApp(aPushRecord);
        this._mainPushService.updatePushRecord(aPushRecord)
          .then(
            null,
            function(e) {
              debug("Error updating push record");
            }
          );
      }
      else {
        debug("No significant version change: " + aLatestVersion);
      }
    }

    let recoverNoSuchChannelID = function(aChannelIDFromServer) {
      debug("Could not get channelID " + aChannelIDFromServer + " from DB");
    }

    this._mainPushService.getByChannelID(aChannelID)
      .then(compareRecordVersionAndNotify.bind(this),
            err => recoverNoSuchChannelID(err));
  },

  register: function(channelID) {
    return this._sendRequest("register", {channelID: channelID})
  },

  unregister: function(aRecord) {
    
    
    if (this._ws) {
      debug("Had a connection, so telling the server");
      this._send("unregister", {channelID: aRecord.channelID});
    }
  },

  
  _wsOnStart: function(context) {
    debug("wsOnStart()");
    this._releaseWakeLock();

    if (this._currentState != STATE_WAITING_FOR_WS_START) {
      debug("NOT in STATE_WAITING_FOR_WS_START. Current state " +
            this._currentState + ". Skipping");
      return;
    }

    
    this._retryFailCount = 0;

    let data = {
      messageType: "hello",
    }

    if (this._UAID)
      data["uaid"] = this._UAID;

    function sendHelloMessage(ids) {
      
      data["channelIDs"] = ids.map ?
                           ids.map(function(el) { return el.channelID; }) : [];
      this._wsSendMessage(data);
      this._currentState = STATE_WAITING_FOR_HELLO;
    }

    this._networkInfo.getNetworkState((networkState) => {
      if (networkState.ip) {
        
        this._listenForUDPWakeup();

        
        data["wakeup_hostport"] = {
          ip: networkState.ip,
          port: this._udpServer && this._udpServer.port
        };

        data["mobilenetwork"] = {
          mcc: networkState.mcc,
          mnc: networkState.mnc,
          netid: networkState.netid
        };
      }

      this._mainPushService.getAllChannelIDs()
        .then(sendHelloMessage.bind(this),
              sendHelloMessage.bind(this));
    });
  },

  






  _wsOnStop: function(context, statusCode) {
    debug("wsOnStop()");
    this._releaseWakeLock();

    if (statusCode != Cr.NS_OK &&
        !(statusCode == Cr.NS_BASE_STREAM_CLOSED && this._willBeWokenUpByUDP)) {
      debug("Socket error " + statusCode);
      this._reconnectAfterBackoff();
    }

    
    
    
    
    this._shutdownWS();
  },

  _wsOnMessageAvailable: function(context, message) {
    debug("wsOnMessageAvailable() " + message);

    this._waitingForPong = false;

    let reply = undefined;
    try {
      reply = JSON.parse(message);
    } catch(e) {
      debug("Parsing JSON failed. text : " + message);
      return;
    }

    
    if (this._currentState != STATE_WAITING_FOR_HELLO) {
      debug('Reseting _retryFailCount and _pingIntervalRetryTimes');
      this._retryFailCount = 0;
      this._pingIntervalRetryTimes = {};
    }

    let doNotHandle = false;
    if ((message === '{}') ||
        (reply.messageType === undefined) ||
        (reply.messageType === "ping") ||
        (typeof reply.messageType != "string")) {
      debug('Pong received');
      this._calculateAdaptivePing(false);
      doNotHandle = true;
    }

    
    
    this._mainPushService.setAlarm(prefs.get("pingInterval"));

    
    if (doNotHandle) {
      return;
    }

    
    
    let handlers = ["Hello", "Register", "Notification"];

    
    
    let handlerName = reply.messageType[0].toUpperCase() +
                      reply.messageType.slice(1).toLowerCase();

    if (handlers.indexOf(handlerName) == -1) {
      debug("No whitelisted handler " + handlerName + ". messageType: " +
            reply.messageType);
      return;
    }

    let handler = "_handle" + handlerName + "Reply";

    if (typeof this[handler] !== "function") {
      debug("Handler whitelisted but not implemented! " + handler);
      return;
    }

    this[handler](reply);
  },

  








  _wsOnServerClose: function(context, aStatusCode, aReason) {
    debug("wsOnServerClose() " + aStatusCode + " " + aReason);

    
    if (aStatusCode == kUDP_WAKEUP_WS_STATUS_CODE) {
      debug("Server closed with promise to wake up");
      this._willBeWokenUpByUDP = true;
      
    }
  },

  


  _cancelPendingRequests: function() {
    for (let channelID in this._pendingRequests) {
      let request = this._pendingRequests[channelID];
      delete this._pendingRequests[channelID];
      request.deferred.reject({status: 0, error: "AbortError"});
    }
  },

  _makeUDPSocket: function() {
    return Cc["@mozilla.org/network/udp-socket;1"]
             .createInstance(Ci.nsIUDPSocket);
  },

  


  _listenForUDPWakeup: function() {
    debug("listenForUDPWakeup()");

    if (this._udpServer) {
      debug("UDP Server already running");
      return;
    }

    if (!prefs.get("udp.wakeupEnabled")) {
      debug("UDP support disabled");
      return;
    }

    let socket = this._makeUDPSocket();
    if (!socket) {
      return;
    }
    this._udpServer = socket.QueryInterface(Ci.nsIUDPSocket);
    this._udpServer.init(-1, false, Services.scriptSecurityManager.getSystemPrincipal());
    this._udpServer.asyncListen(this);
    debug("listenForUDPWakeup listening on " + this._udpServer.port);

    return this._udpServer.port;
  },

  



  onPacketReceived: function(aServ, aMessage) {
    debug("Recv UDP datagram on port: " + this._udpServer.port);
    this._beginWSSetup();
  },

  





  onStopListening: function(aServ, aStatus) {
    debug("UDP Server socket was shutdown. Status: " + aStatus);
    this._udpServer = undefined;
    this._beginWSSetup();
  }
};

let PushNetworkInfo = {
  


  getNetworkInformation: function() {
    debug("getNetworkInformation()");

    try {
      if (!prefs.get("udp.wakeupEnabled")) {
        debug("UDP support disabled, we do not send any carrier info");
        throw new Error("UDP disabled");
      }

      let nm = Cc["@mozilla.org/network/manager;1"].getService(Ci.nsINetworkManager);
      if (nm.active && nm.active.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
        let iccService = Cc["@mozilla.org/icc/iccservice;1"].getService(Ci.nsIIccService);
        
        
        
        
        
        let clientId = 0;
        let icc = iccService.getIccByServiceId(clientId);
        let iccInfo = icc && icc.iccInfo;
        if (iccInfo) {
          debug("Running on mobile data");

          let ips = {};
          let prefixLengths = {};
          nm.active.getAddresses(ips, prefixLengths);

          return {
            mcc: iccInfo.mcc,
            mnc: iccInfo.mnc,
            ip:  ips.value[0]
          }
        }
      }
    } catch (e) {
      debug("Error recovering mobile network information: " + e);
    }

    debug("Running on wifi");
    return {
      mcc: 0,
      mnc: 0,
      ip: undefined
    };
  },

  




  getNetworkState: function(callback) {
    debug("getNetworkState()");

    if (typeof callback !== 'function') {
      throw new Error("No callback method. Aborting push agent !");
    }

    var networkInfo = this.getNetworkInformation();

    if (networkInfo.ip) {
      this._getMobileNetworkId(networkInfo, function(netid) {
        debug("Recovered netID = " + netid);
        callback({
          mcc: networkInfo.mcc,
          mnc: networkInfo.mnc,
          ip:  networkInfo.ip,
          netid: netid
        });
      });
    } else {
      callback(networkInfo);
    }
  },

  







  _getMobileNetworkId: function(networkInfo, callback) {
    if (typeof callback !== 'function') {
      return;
    }

    function queryDNSForDomain(domain) {
      debug("[_getMobileNetworkId:queryDNSForDomain] Querying DNS for " +
        domain);
      let netIDDNSListener = {
        onLookupComplete: function(aRequest, aRecord, aStatus) {
          if (aRecord) {
            let netid = aRecord.getNextAddrAsString();
            debug("[_getMobileNetworkId:queryDNSForDomain] NetID found: " +
              netid);
            callback(netid);
          } else {
            debug("[_getMobileNetworkId:queryDNSForDomain] NetID not found");
            callback(null);
          }
        }
      };
      gDNSService.asyncResolve(domain, 0, netIDDNSListener,
        threadManager.currentThread);
      return [];
    }

    debug("[_getMobileNetworkId:queryDNSForDomain] Getting mobile network ID");

    let netidAddress = "wakeup.mnc" + ("00" + networkInfo.mnc).slice(-3) +
      ".mcc" + ("00" + networkInfo.mcc).slice(-3) + ".3gppnetwork.org";
    queryDNSForDomain(netidAddress, callback);
  }
};
