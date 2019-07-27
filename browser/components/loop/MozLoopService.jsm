



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;



const INVALID_AUTH_TOKEN = 110;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/FxAccountsOAuthClient.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopService"];

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI",
  "resource:///modules/loop/MozLoopAPI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "convertToRTCStatsReport",
  "resource://gre/modules/media/RTCStatsReport.jsm");

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

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");




let gRegisteredDeferred = null;
let gPushHandler = null;
let gHawkClient = null;
let gRegisteredLoopServer = false;
let gLocalizedStrings =  null;
let gInitializeTimer = null;
let gFxAOAuthClientPromise = null;
let gFxAOAuthClient = null;
let gFxAOAuthTokenData = null;








let MozLoopServiceInternal = {
  
  get loopServerUri() Services.prefs.getCharPref("loop.server"),

  



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
    if (gRegisteredDeferred) {
      return gRegisteredDeferred.promise;
    }

    gRegisteredDeferred = Promise.defer();
    
    
    let result = gRegisteredDeferred.promise;

    gPushHandler = mockPushHandler || MozLoopPushHandler;

    gPushHandler.initialize(this.onPushRegistered.bind(this),
      this.onHandleNotification.bind(this));

    return result;
  },

  












  hawkRequest: function(path, method, payloadObj) {
    if (!gHawkClient) {
      gHawkClient = new HawkClient(this.loopServerUri);
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

    return gHawkClient.request(path, method, credentials, payloadObj).catch(error => {
      console.error("Loop hawkRequest error:", error);
      throw error;
    });
  },

  






  storeSessionToken: function(headers) {
    let sessionToken = headers["hawk-session-token"];
    if (sessionToken) {
      
      if (sessionToken.length === 64) {
        Services.prefs.setCharPref("loop.hawk-session-token", sessionToken);
      } else {
        
        console.warn("Loop server sent an invalid session token");
        gRegisteredDeferred.reject("session-token-wrong-size");
        gRegisteredDeferred = null;
        return false;
      }
    }
    return true;
  },

  





  onPushRegistered: function(err, pushUrl) {
    if (err) {
      gRegisteredDeferred.reject(err);
      gRegisteredDeferred = null;
      return;
    }

    this.registerWithLoopServer(pushUrl);
  },

  





  registerWithLoopServer: function(pushUrl, noRetry) {
    this.hawkRequest("/registration", "POST", { simplePushURL: pushUrl})
      .then((response) => {
        
        
        
        if (!this.storeSessionToken(response.headers))
          return;

        gRegisteredDeferred.resolve();
        
        
      }, (error) => {
        
        
        if (error.code === 401 && error.errno === INVALID_AUTH_TOKEN) {
          if (this.urlExpiryTimeIsInFuture()) {
            
            Cu.reportError("Loop session token is invalid, all previously "
                           + "generated urls will no longer work.");
          }

          
          Services.prefs.clearUserPref("loop.hawk-session-token");
          this.registerWithLoopServer(pushUrl, true);
          return;
        }

        
        Cu.reportError("Failed to register with the loop server. error: " + error);
        gRegisteredDeferred.reject(error.errno);
        gRegisteredDeferred = null;
      }
    );
  },

  





  onHandleNotification: function(version) {
    if (this.doNotDisturb) {
      return;
    }

    this.openChatWindow(null,
                        this.localizedStrings["incoming_call_title"].textContent,
                        "about:loopconversation#incoming/" + version);
  },

  





  get localizedStrings() {
    if (gLocalizedStrings)
      return gLocalizedStrings;

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

    return gLocalizedStrings = map;
  },

  




  stageForTelemetryUpload: function(window, pc) {
    window.WebrtcGlobalInformation.getAllStats(allStats => {
      let internalFormat = allStats.reports[0]; 
      window.WebrtcGlobalInformation.getLogging('', logs => {
        let report = convertToRTCStatsReport(internalFormat);
        let logStr = "";
        logs.forEach(s => { logStr += s + "\n"; });

        

        
        
        

        let ai = Services.appinfo;
        let uuid = uuidgen.generateUUID().toString();
        uuid = uuid.substr(1,uuid.length-2); 

        let directory = OS.Path.join(OS.Constants.Path.profileDir,
                                     "saved-telemetry-pings");
        let job = {
          directory: directory,
          filename: uuid + ".json",
          ping: {
            reason: "loop",
            slug: uuid,
            payload: {
              ver: 1,
              info: {
                appUpdateChannel: ai.defaultUpdateChannel,
                appBuildID: ai.appBuildID,
                appName: ai.name,
                appVersion: ai.version,
                reason: "loop",
                OS: ai.OS,
                version: Services.sysinfo.getProperty("version")
              },
              report: "ice failure",
              connectionstate: pc.iceConnectionState,
              stats: report,
              localSdp: internalFormat.localSdp,
              remoteSdp: internalFormat.remoteSdp,
              log: logStr
            }
          }
        };

        
        

        let worker = new ChromeWorker("MozLoopWorker.js");
        worker.onmessage = function(e) {
          console.log(e.data.ok ?
            "Successfully staged loop report for telemetry upload." :
            ("Failed to stage loop report. Error: " + e.data.fail));
        }
        worker.postMessage(job);
      });
    }, pc.id);
  },

  







  openChatWindow: function(contentWindow, title, url) {
    
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

        let window = chatbox.contentWindow;
        injectLoopAPI(window);

        let ourID = window.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

        let onPCLifecycleChange = (pc, winID, type) => {
          if (winID != ourID) {
            return;
          }
          if (type == "iceconnectionstatechange") {
            switch(pc.iceConnectionState) {
              case "failed":
              case "disconnected":
                if (Services.telemetry.canSend ||
                    Services.prefs.getBoolPref("toolkit.telemetry.test")) {
                  this.stageForTelemetryUpload(window, pc);
                }
                break;
            }
          }
        };

        let pc_static = new window.mozRTCPeerConnectionStatic();
        pc_static.registerPeerConnectionLifecycleCallback(onPCLifecycleChange);
      }.bind(this), true);
    };

    Chat.open(contentWindow, origin, title, url, undefined, undefined, callback);
  },

  




  promiseFxAOAuthParameters: function() {
    return this.hawkRequest("/fxa-oauth/params", "POST").then(response => {
      return JSON.parse(response.body);
    });
  },

  




  promiseFxAOAuthClient: Task.async(function* () {
    
    
    if (gFxAOAuthClientPromise) {
      return gFxAOAuthClientPromise;
    }

    gFxAOAuthClientPromise = this.promiseFxAOAuthParameters().then(
      parameters => {
        try {
          gFxAOAuthClient = new FxAccountsOAuthClient({
            parameters: parameters,
          });
        } catch (ex) {
          gFxAOAuthClientPromise = null;
          throw ex;
        }
        return gFxAOAuthClient;
      },
      error => {
        gFxAOAuthClientPromise = null;
        throw error;
      }
    );

    return gFxAOAuthClientPromise;
  }),

  




  promiseFxAOAuthAuthorization: function() {
    let deferred = Promise.defer();
    this.promiseFxAOAuthClient().then(
      client => {
        client.onComplete = this._fxAOAuthComplete.bind(this, deferred);
        client.launchWebFlow();
      },
      error => {
        console.error(error);
        deferred.reject(error);
      }
    );
    return deferred.promise;
  },

  










  promiseFxAOAuthToken: function(code, state) {
    if (!code || !state) {
      throw new Error("promiseFxAOAuthToken: code and state are required.");
    }

    let payload = {
      code: code,
      state: state,
    };
    return this.hawkRequest("/fxa-oauth/token", "POST", payload).then(response => {
      return JSON.parse(response.body);
    });
  },

  





  _fxAOAuthComplete: function(deferred, result) {
    gFxAOAuthClientPromise = null;

    
    if (result) {
      deferred.resolve(result);
    } else {
      deferred.reject("Invalid token data");
    }
  },
};
Object.freeze(MozLoopServiceInternal);

let gInitializeTimerFunc = () => {
  
  
  
  gInitializeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  gInitializeTimer.initWithCallback(() => {
    MozLoopService.register();
    gInitializeTimer = null;
  },
  MozLoopServiceInternal.initialRegistrationDelayMilliseconds, Ci.nsITimer.TYPE_ONE_SHOT);
};




this.MozLoopService = {
#ifdef DEBUG
  
  get internal() {
    return MozLoopServiceInternal;
  },

  get gFxAOAuthTokenData() {
    return gFxAOAuthTokenData;
  },

  resetFxA: function() {
    gFxAOAuthClientPromise = null;
    gFxAOAuthClient = null;
    gFxAOAuthTokenData = null;
  },
#endif

  set initializeTimerFunc(value) {
    gInitializeTimerFunc = value;
  },

  



  initialize: function() {
    
    if (!Services.prefs.getBoolPref("loop.enabled")) {
      return;
    }

    
    if (MozLoopServiceInternal.urlExpiryTimeIsInFuture()) {
      gInitializeTimerFunc();
    }
  },

  








  register: function(mockPushHandler) {
    
    if (!Services.prefs.getBoolPref("loop.enabled")) {
      throw new Error("Loop is not enabled");
    }

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

  












  getLoopBoolPref: function(prefName) {
    try {
      return Services.prefs.getBoolPref("loop." + prefName);
    } catch (ex) {
      console.log("getLoopBoolPref had trouble getting " + prefName +
        "; exception: " + ex);
      return null;
    }
  },

  






  logInToFxA: function() {
    return MozLoopServiceInternal.promiseFxAOAuthAuthorization().then(response => {
      return MozLoopServiceInternal.promiseFxAOAuthToken(response.code, response.state);
    }).then(tokenData => {
      gFxAOAuthTokenData = tokenData;
      return tokenData;
    },
    error => {
      gFxAOAuthTokenData = null;
      throw error;
    });
  },

  












  hawkRequest: function(path, method, payloadObj) {
    return MozLoopServiceInternal.hawkRequest(path, method, payloadObj);
  },
};
Object.freeze(this.MozLoopService);
