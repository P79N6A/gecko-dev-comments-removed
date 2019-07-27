



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

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "CryptoUtils",
                                  "resource://services-crypto/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "HawkClient",
                                  "resource://services-common/hawkclient.js");

XPCOMUtils.defineLazyModuleGetter(this, "deriveHawkCredentials",
                                  "resource://services-common/hawkrequest.js");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");








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

  


  urlExpiryTimeIsInFuture: function() {
    return this.expiryTimeSeconds * 1000 > Date.now();
  },

  




  get doNotDisturb() {
    return Services.prefs.getBoolPref("loop.do_not_disturb");
  },

  




  set doNotDisturb(aFlag) {
    Services.prefs.setBoolPref("loop.do_not_disturb", Boolean(aFlag));
  },

  








  promiseRegisteredWithServers: function(mockPushHandler) {
    if (this._registeredDeferred) {
      return this._registeredDeferred.promise;
    }

    this._registeredDeferred = Promise.defer();
    
    
    let result = this._registeredDeferred.promise;

    this._pushHandler = mockPushHandler || MozLoopPushHandler;

    this._pushHandler.initialize(this.onPushRegistered.bind(this),
      this.onHandleNotification.bind(this));

    return result;
  },

  












  hawkRequest: function(path, method, payloadObj) {
    if (!this._hawkClient) {
      this._hawkClient = new HawkClient(this.loopServerUri);
    }

    let sessionToken;
    try {
      sessionToken = Services.prefs.getCharPref("loop.hawk-session-token");
    } catch (x) {
      
    }

    let credentials;
    if (sessionToken) {
      
      credentials = deriveHawkCredentials(sessionToken, "sessionToken",
                                          2 * 32, true);
    }

    return this._hawkClient.request(path, method, credentials, payloadObj);
  },

  






  storeSessionToken: function(headers) {
    let sessionToken = headers["hawk-session-token"];
    if (sessionToken) {
      
      if (sessionToken.length === 64) {
        Services.prefs.setCharPref("loop.hawk-session-token", sessionToken);
      } else {
        
        console.warn("Loop server sent an invalid session token");
        this._registeredDeferred.reject("session-token-wrong-size");
        this._registeredDeferred = null;
        return false;
      }
    }
    return true;
  },

  





  onPushRegistered: function(err, pushUrl) {
    if (err) {
      this._registeredDeferred.reject(err);
      this._registeredDeferred = null;
      return;
    }

    this.registerWithLoopServer(pushUrl);
  },

  





  registerWithLoopServer: function(pushUrl, noRetry) {
    this.hawkRequest("/registration", "POST", { simple_push_url: pushUrl})
      .then((response) => {
        
        
        
        if (!this.storeSessionToken(response.headers))
          return;

        this.registeredLoopServer = true;
        this._registeredDeferred.resolve();
        
        
      }, (error) => {
        if (error.errno == 401) {
          if (this.urlExpiryTimeIsInFuture()) {
            
            Cu.reportError("Loop session token is invalid, all previously "
                           + "generated urls will no longer work.");
          }

          
          Services.prefs.clearUserPref("loop.hawk-session-token");
          this.registerWithLoopServer(pushUrl, true);
          return;
        }

        
        Cu.reportError("Failed to register with the loop server. error: " + error);
        this._registeredDeferred.reject(error.errno);
        this._registeredDeferred = null;
      }
    );
  },

  





  onHandleNotification: function(version) {
    if (this.doNotDisturb) {
      return;
    }

    this.openChatWindow(null, "LooP", "about:loopconversation#incoming/" + version);
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
    
    if (MozLoopServiceInternal.urlExpiryTimeIsInFuture()) {
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

  








  register: function(mockPushHandler) {
    return MozLoopServiceInternal.promiseRegisteredWithServers(mockPushHandler);
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

  







  setLoopCharPref: function(prefName, value) {
    try {
      Services.prefs.setCharPref("loop." + prefName, value);
    } catch (ex) {
      console.log("setLoopCharPref had trouble setting " + prefName +
        "; exception: " + ex);
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
  },

  












  hawkRequest: function(path, method, payloadObj) {
    return MozLoopServiceInternal.hawkRequest(path, method, payloadObj);
  },
};
