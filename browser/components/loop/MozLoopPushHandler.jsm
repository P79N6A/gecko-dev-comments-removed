



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopPushHandler"];

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");





let MozLoopPushHandler = {
  
  pushServerUri: undefined,
  
  channelID: "8b1081ce-9b35-42b5-b8f5-3ff8cb813a50",
  
  uaID: undefined,
  
  pushUrl: undefined,
  
  registered: false,

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

   




















  initialize: function(registerCallback, notificationCallback, mockPushHandler) {
    if (mockPushHandler) {
      this._mockPushHandler = mockPushHandler;
    }

    this._registerCallback = registerCallback;
    this._notificationCallback = notificationCallback;
    this._openSocket();
  },

  





  onStart: function() {
    this._retryEnd();
    
    
    
    let helloMsg = { messageType: "hello",
		     uaid: this.uaID,
		     channelIDs: this.registered ? [this.channelID] :[] };
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
	if (this.uaID !== msg.uaid) {
	  this.uaID = msg.uaid;
          this._registerChannel();
	}
        break;

      case "register":
        this._onRegister(msg);
        break;

      case "notification":
        msg.updates.forEach((update) => {
          if (update.channelID === this.channelID) {
            this._notificationCallback(update.version);
          }
        });
        break;
    }
  },

  




  _onRegister: function(msg) {
    switch (msg.status) {
      case 200:
        this._retryEnd(); 
	this.registered = true;
        if (this.pushUrl !== msg.pushEndpoint) {
          this.pushUrl = msg.pushEndpoint;
          this._registerCallback(null, this.pushUrl);
        }
        break;

      case 500:
        
        this._retryOperation(() => this._registerChannel());
        break;

      case 409:
        this._registerCallback("error: PushServer ChannelID already in use");
	break;

      default:
        this._registerCallback("error: PushServer registration failure, status = " + msg.status);
	break;
    }
  },

  







  _openSocket: function() {
    if (this._mockPushHandler) {
      
      this._websocket = this._mockPushHandler;
    } else if (!Services.io.offline) {
      this._websocket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                        .createInstance(Ci.nsIWebSocketChannel);
    } else {
      this._registerCallback("offline");
      console.warn("MozLoopPushHandler - IO offline");
      return;
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

  


  _registerChannel: function() {
    this.registered = false;
    try { 
      this._websocket.sendMsg(JSON.stringify({messageType: "register",
                                              channelID: this.channelID}));
    }
    catch (e) {console.warn("MozLoopPushHandler::_registerChannel websocket.sendMsg() failure");}
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

