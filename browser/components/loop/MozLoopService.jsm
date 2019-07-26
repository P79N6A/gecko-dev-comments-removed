



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopService"];

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI",
  "resource:///modules/loop/MozLoopAPI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Chat", "resource:///modules/Chat.jsm");





let PushHandlerHack = {
  
  pushServerUri: Services.prefs.getCharPref("services.push.serverURL"),
  
  channelID: "8b1081ce-9b35-42b5-b8f5-3ff8cb813a50",
  
  pushUrl: undefined,

  









  initialize: function(registerCallback, notificationCallback) {
    this.registerCallback = registerCallback;
    this.notificationCallback = notificationCallback;

    this.websocket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                       .createInstance(Ci.nsIWebSocketChannel);

    this.websocket.protocol = "push-notification";

    var pushURI = Services.io.newURI(this.pushServerUri, null, null);
    this.websocket.asyncOpen(pushURI, this.pushServerUri, this, null);
  },

  





  onStart: function() {
    var helloMsg = { messageType: "hello", uaid: "", channelIDs: [] };
    this.websocket.sendMsg(JSON.stringify(helloMsg));
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
    var msg = JSON.parse(aMsg);

    switch(msg.messageType) {
      case "hello":
        this._registerChannel();
        break;
      case "register":
        this.pushUrl = msg.pushEndpoint;
        this.registerCallback(this.pushUrl);
        break;
      case "notification":
        msg.updates.forEach(function(update) {
          if (update.channelID === this.channelID) {
            this.notificationCallback(update.version);
          }
        }.bind(this));
        break;
    }
  },

  


  _registerChannel: function() {
    this.websocket.sendMsg(JSON.stringify({
      messageType: "register",
      channelID: this.channelID
    }));
  }
};




let MozLoopServiceInternal = {
  
  loopServerUri: Services.prefs.getCharPref("loop.server"),

  





  pushRegistrationDelay: 100,

  



  initialize: function() {
    if (this.initialized)
      return;

    this.initialized = true;

    
    
    
    this.initializeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.initializeTimer.initWithCallback(this.registerPushHandler.bind(this),
      this.pushRegistrationDelay, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  registerPushHandler: function() {
    PushHandlerHack.initialize(this.onPushRegistered.bind(this),
                               this.onHandleNotification.bind(this));
  },

  





  onPushRegistered: function(pushUrl) {
    this.registerXhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
      .createInstance(Ci.nsIXMLHttpRequest);

    this.registerXhr.open('POST', MozLoopServiceInternal.loopServerUri + "/registration",
                          true);
    this.registerXhr.setRequestHeader('Content-Type', 'application/json');

    this.registerXhr.channel.loadFlags = Ci.nsIChannel.INHIBIT_CACHING
      | Ci.nsIChannel.LOAD_BYPASS_CACHE
      | Ci.nsIChannel.LOAD_EXPLICIT_CREDENTIALS;

    this.registerXhr.onreadystatechange = this.onRegistrationResult.bind(this);

    this.registerXhr.sendAsBinary(JSON.stringify({
      simple_push_url: pushUrl
    }));
  },

  





  onHandleNotification: function(version) {
    this.openChatWindow(null, "LooP", "about:loopconversation#start/" + version);
  },

  


  onRegistrationResult: function() {
    if (this.registerXhr.readyState != Ci.nsIXMLHttpRequest.DONE)
      return;

    if (this.registerXhr.status != 200) {
      
      Cu.reportError("Failed to register with push server. Code: " +
        this.registerXhr.status + " Text: " + this.registerXhr.statusText);
      return;
    }

    
    
    
    this.registeredLoopServer = true;
  },

  





  get localizedStrings() {
    if (this._localizedStrings)
      return this._localizedStrings;

    var stringBundle =
      Services.strings.createBundle('chrome://browser/locale/loop/loop.properties');

    var map = {};
    var enumerator = stringBundle.getSimpleEnumeration();
    while (enumerator.hasMoreElements()) {
      var string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);

      
      var key = string.key, property = 'textContent';
      var i = key.lastIndexOf('.');
      if (i >= 0) {
        property = key.substring(i + 1);
        key = key.substring(0, i);
      }
      if (!(key in map))
        map[key] = {};
      map[key][property] = string.value;
    }

    return this._localizedStrings = map;
  },

  








  openChatWindow: function(contentWindow, title, url, mode) {
    
    let origin = this.loopServerUri;
    url = url.spec || url;

    let callback = chatbox => {
      
      
      
      
      
      if (chatbox.contentWindow.navigator.mozLoop) {
        return;
      }

      chatbox.addEventListener("DOMContentLoaded", function loaded(event) {
        if (event.target != chatbox.contentDocument) {
          return;
        }
        chatbox.removeEventListener("DOMContentLoaded", loaded, true);
        injectLoopAPI(chatbox.contentWindow);
      }, true);
    };

    Chat.open(contentWindow, origin, title, url, undefined, undefined, callback);
  }
};




this.MozLoopService = {
  



  initialize: function() {
    MozLoopServiceInternal.initialize();
  },

  







  getStrings: function(key) {
      var stringData = MozLoopServiceInternal.localizedStrings;
      if (!(key in stringData)) {
        Cu.reportError('No string for key: ' + key + 'found');
        return "";
      }

      return JSON.stringify(stringData[key]);
  },

  




  get locale() {
    try {
      return Services.prefs.getComplexValue("general.useragent.locale",
        Ci.nsISupportsString).data;
    } catch (ex) {
      return "en-US";
    }
  }
};
