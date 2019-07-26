



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopPushHandler"];

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");








let MozLoopPushHandler = {
  
  pushServerUri: Services.prefs.getCharPref("services.push.serverURL"),
  
  channelID: "8b1081ce-9b35-42b5-b8f5-3ff8cb813a50",
  
  pushUrl: undefined,

   




















  initialize: function(registerCallback, notificationCallback, mockPushHandler) {
    if (Services.io.offline) {
      registerCallback("offline");
      return;
    }

    this._registerCallback = registerCallback;
    this._notificationCallback = notificationCallback;

    if (mockPushHandler) {
      
      this._websocket = mockPushHandler;
    } else {
      this._websocket = Cc["@mozilla.org/network/protocol;1?name=wss"]
        .createInstance(Ci.nsIWebSocketChannel);
    }
    this._websocket.protocol = "push-notification";

    let pushURI = Services.io.newURI(this.pushServerUri, null, null);
    this._websocket.asyncOpen(pushURI, this.pushServerUri, this, null);
  },

  





  onStart: function() {
    let helloMsg = { messageType: "hello", uaid: "", channelIDs: [] };
    this._websocket.sendMsg(JSON.stringify(helloMsg));
  },

  





  onStop: function(aContext, aStatusCode) {
    
    
    
    Cu.reportError("Loop Push server web socket closed! Code: " + aStatusCode);
    this.pushUrl = undefined;
  },

  








  onServerClose: function(aContext, aCode) {
    
    
    
    Cu.reportError("Loop Push server web socket closed (server)! Code: " + aCode);
    this.pushUrl = undefined;
  },

  





  onMessageAvailable: function(aContext, aMsg) {
    let msg = JSON.parse(aMsg);

    switch(msg.messageType) {
      case "hello":
        this._registerChannel();
        break;
      case "register":
        this.pushUrl = msg.pushEndpoint;
        this._registerCallback(null, this.pushUrl);
        break;
      case "notification":
        msg.updates.forEach(function(update) {
          if (update.channelID === this.channelID) {
            this._notificationCallback(update.version);
          }
        }.bind(this));
        break;
    }
  },

  


  _registerChannel: function() {
    this._websocket.sendMsg(JSON.stringify({
      messageType: "register",
      channelID: this.channelID
    }));
  }
};

