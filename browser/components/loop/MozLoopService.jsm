



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let console = (Cu.import("resource://gre/modules/devtools/Console.jsm", {})).console;

this.EXPORTED_SYMBOLS = ["MozLoopService"];

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI",
  "resource:///modules/loop/MozLoopAPI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Chat", "resource:///modules/Chat.jsm");








let PushHandlerHack = {
  
  pushServerUri: Services.prefs.getCharPref("services.push.serverURL"),
  
  channelID: "8b1081ce-9b35-42b5-b8f5-3ff8cb813a50",
  
  pushUrl: undefined,

  













  initialize: function(registerCallback, notificationCallback) {
    if (Services.io.offline) {
      registerCallback("offline");
      return;
    }

    this._registerCallback = registerCallback;
    this._notificationCallback = notificationCallback;

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
    this.websocket.sendMsg(JSON.stringify({
      messageType: "register",
      channelID: this.channelID
    }));
  }
};








let MozLoopServiceInternal = {
  
  loopServerUri: Services.prefs.getCharPref("loop.server"),

  
  registrationCallbacks: [],

  



  get initialRegistrationDelayMilliseconds() {
    
    let initialDelay = 5000;
    try {
      
      initialDelay = Services.prefs.getIntPref("loop.initialDelay");
    } catch (x) {
      
    }
    return initialDelay;
  },

  




  get expiryTimeSeconds() {
    let expiryTimeSeconds = 0;
    try {
      expiryTimeSeconds = Services.prefs.getIntPref("loop.urlsExpiryTimeSeconds");
    } catch (x) {
      
    }

    return expiryTimeSeconds;
  },

  



  set expiryTimeSeconds(time) {
    if (time > this.expiryTimeSeconds) {
      Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", time);
    }
  },

  











  initialize: function(callback) {
    if (this.registeredPushServer || this.initalizeTimer || this.registrationInProgress) {
      if (callback)
        callback(this.registeredPushServer ? null : false);
      return;
    }

    function secondsToMilli(value) {
      return value * 1000;
    }

    
    if (secondsToMilli(this.expiryTimeSeconds) > Date.now()) {
      
      
      
      this.initializeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.initializeTimer.initWithCallback(function() {
        this.registerWithServers(callback);
        this.initializeTimer = null;
      }.bind(this),
      this.initialRegistrationDelayMilliseconds, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    else if (callback) {
      
      callback(false);
    }
  },

  








  registerWithServers: function(callback) {
    
    
    if (this.registeredLoopServer) {
      callback(null);
      return;
    }

    
    this.registrationCallbacks.push(callback);

    
    if (this.registrationInProgress) {
      return;
    }

    this.registrationInProgress = true;

    PushHandlerHack.initialize(this.onPushRegistered.bind(this),
                               this.onHandleNotification.bind(this));
  },

  




  endRegistration: function(err) {
    
    this.registrationInProgress = false;

    
    this.registrationCallbacks.forEach(function(callback) {
      callback(err);
    });
    this.registrationCallbacks.length = 0;
  },

  





  onPushRegistered: function(err, pushUrl) {
    if (err) {
      this.endRegistration(err);
      return;
    }

    this.loopXhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
      .createInstance(Ci.nsIXMLHttpRequest);

    this.loopXhr.open('POST', MozLoopServiceInternal.loopServerUri + "/registration",
                          true);
    this.loopXhr.setRequestHeader('Content-Type', 'application/json');

    this.loopXhr.channel.loadFlags = Ci.nsIChannel.INHIBIT_CACHING
      | Ci.nsIChannel.LOAD_BYPASS_CACHE
      | Ci.nsIChannel.LOAD_EXPLICIT_CREDENTIALS;

    this.loopXhr.onreadystatechange = this.onLoopRegistered.bind(this);

    this.loopXhr.sendAsBinary(JSON.stringify({
      simple_push_url: pushUrl
    }));
  },

  





  onHandleNotification: function(version) {
    this.openChatWindow(null, "LooP", "about:loopconversation#start/" + version);
  },

  


  onLoopRegistered: function() {
    if (this.loopXhr.readyState != Ci.nsIXMLHttpRequest.DONE)
      return;

    let status = this.loopXhr.status;
    if (status != 200) {
      
      Cu.reportError("Failed to register with the loop server. Code: " +
        status + " Text: " + this.loopXhr.statusText);
      this.endRegistration(status);
      return;
    }

    let sessionToken = this.loopXhr.getResponseHeader("Hawk-Session-Token");
    if (sessionToken !== null) {

      
      if (sessionToken.length === 64) {

        Services.prefs.setCharPref("loop.hawk-session-token", sessionToken);
      } else {
        
        console.warn("Loop server sent an invalid session token");
        this.endRegistration("session-token-wrong-size");
        return;
      }
    }

    
    this.registeredLoopServer = true;
    this.endRegistration(null);
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
  











  initialize: function(callback) {
    MozLoopServiceInternal.initialize(callback);
  },

  








  register: function(callback) {
    MozLoopServiceInternal.registerWithServers(callback);
  },

  











  noteCallUrlExpiry: function(expiryTimeSeconds) {
    MozLoopServiceInternal.expiryTimeSeconds = expiryTimeSeconds;
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
  },

  












  getLoopCharPref: function(prefName) {
    try {
      return Services.prefs.getCharPref("loop." + prefName);
    } catch (ex) {
      console.log("getLoopCharPref had trouble getting " + prefName +
        "; exception: " + ex);
      return null;
    }
  }
};
