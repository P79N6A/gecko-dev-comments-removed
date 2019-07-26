



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
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

  
  
  
  _registeredDeferred: null,

  



  get initialRegistrationDelayMilliseconds() {
    try {
      
      return Services.prefs.getIntPref("loop.initialDelay");
    } catch (x) {
      
      return 5000;
    }
    return initialDelay;
  },

  




  get expiryTimeSeconds() {
    try {
      return Services.prefs.getIntPref("loop.urlsExpiryTimeSeconds");
    } catch (x) {
      
      return 0;
    }
  },

  



  set expiryTimeSeconds(time) {
    if (time > this.expiryTimeSeconds) {
      Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", time);
    }
  },

  




  get doNotDisturb() {
    return Services.prefs.getBoolPref("loop.do_not_disturb");
  },

  




  set doNotDisturb(aFlag) {
    Services.prefs.setBoolPref("loop.do_not_disturb", Boolean(aFlag));
  },

  






  promiseRegisteredWithServers: function() {
    if (this._registeredDeferred) {
      return this._registeredDeferred.promise;
    }

    this._registeredDeferred = Promise.defer();
    
    
    let result = this._registeredDeferred.promise;

    PushHandlerHack.initialize(this.onPushRegistered.bind(this),
                               this.onHandleNotification.bind(this));

    return result;
  },

  





  onPushRegistered: function(err, pushUrl) {
    if (err) {
      this._registeredDeferred.reject(err);
      this._registeredDeferred = null;
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
    if (this.doNotDisturb) {
      return;
    }

    this.openChatWindow(null, "LooP", "about:loopconversation#start/" + version);
  },

  


  onLoopRegistered: function() {
    if (this.loopXhr.readyState != Ci.nsIXMLHttpRequest.DONE)
      return;

    let status = this.loopXhr.status;
    if (status != 200) {
      
      Cu.reportError("Failed to register with the loop server. Code: " +
        status + " Text: " + this.loopXhr.statusText);
      this._registeredDeferred.reject(status);
      this._registeredDeferred = null;
      return;
    }

    let sessionToken = this.loopXhr.getResponseHeader("Hawk-Session-Token");
    if (sessionToken !== null) {

      
      if (sessionToken.length === 64) {

        Services.prefs.setCharPref("loop.hawk-session-token", sessionToken);
      } else {
        
        console.warn("Loop server sent an invalid session token");
        this._registeredDeferred.reject("session-token-wrong-size");
        this._registeredDeferred = null;
        return;
      }
    }

    
    this.registeredLoopServer = true;
    this._registeredDeferred.resolve();
    
    
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
    
    if ((MozLoopServiceInternal.expiryTimeSeconds * 1000) > Date.now()) {
      this._startInitializeTimer();
    }
  },

  



  _startInitializeTimer: function() {
    
    
    
    this._initializeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._initializeTimer.initWithCallback(function() {
      this.register();
      this._initializeTimer = null;
    }.bind(this),
    MozLoopServiceInternal.initialRegistrationDelayMilliseconds, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  






  register: function() {
    return MozLoopServiceInternal.promiseRegisteredWithServers();
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

  




  get doNotDisturb() {
    return MozLoopServiceInternal.doNotDisturb;
  },

  




  set doNotDisturb(aFlag) {
    MozLoopServiceInternal.doNotDisturb = aFlag;
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
