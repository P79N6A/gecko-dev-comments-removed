



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;



const INVALID_AUTH_TOKEN = 110;

const LOOP_SESSION_TYPE = {
  GUEST: 1,
  FXA: 2,
};


const PREF_LOG_LEVEL = "loop.debug.loglevel";

const EMAIL_OR_PHONE_RE = /^(:?\S+@\S+|\+\d+)$/;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/FxAccountsOAuthClient.jsm");

Cu.importGlobalProperties(["URL"]);

this.EXPORTED_SYMBOLS = ["MozLoopService", "LOOP_SESSION_TYPE"];

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI",
  "resource:///modules/loop/MozLoopAPI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "convertToRTCStatsReport",
  "resource://gre/modules/media/RTCStatsReport.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Chat", "resource:///modules/Chat.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "CryptoUtils",
                                  "resource://services-crypto/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfileClient",
                                  "resource://gre/modules/FxAccountsProfileClient.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HawkClient",
                                  "resource://services-common/hawkclient.js");

XPCOMUtils.defineLazyModuleGetter(this, "deriveHawkCredentials",
                                  "resource://services-common/hawkrequest.js");

XPCOMUtils.defineLazyModuleGetter(this, "LoopContacts",
                                  "resource:///modules/loop/LoopContacts.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoopStorage",
                                  "resource:///modules/loop/LoopStorage.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoopCalls",
                                  "resource:///modules/loop/LoopCalls.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoopRooms",
                                  "resource:///modules/loop/LoopRooms.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "roomsPushNotification",
                                  "resource:///modules/loop/LoopRooms.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");


XPCOMUtils.defineLazyGetter(this, "log", () => {
  let ConsoleAPI = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).ConsoleAPI;
  let consoleOptions = {
    maxLogLevel: Services.prefs.getCharPref(PREF_LOG_LEVEL).toLowerCase(),
    prefix: "Loop",
  };
  return new ConsoleAPI(consoleOptions);
});

function setJSONPref(aName, aValue) {
  let value = !!aValue ? JSON.stringify(aValue) : "";
  Services.prefs.setCharPref(aName, value);
}

function getJSONPref(aName) {
  let value = Services.prefs.getCharPref(aName);
  return !!value ? JSON.parse(value) : null;
}

let gHawkClient = null;
let gLocalizedStrings = null;
let gFxAEnabled = true;
let gFxAOAuthClientPromise = null;
let gFxAOAuthClient = null;
let gErrors = new Map();
let gLastWindowId = 0;
let gConversationWindowData = new Map();








let MozLoopServiceInternal = {
  mocks: {
    pushHandler: undefined,
    webSocket: undefined,
  },

  




  deferredRegistrations: new Map(),

  get pushHandler() this.mocks.pushHandler || MozLoopPushHandler,

  
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

  




  get fxAOAuthTokenData() {
    return getJSONPref("loop.fxa_oauth.tokendata");
  },

  






  set fxAOAuthTokenData(aTokenData) {
    setJSONPref("loop.fxa_oauth.tokendata", aTokenData);
    if (!aTokenData) {
      this.fxAOAuthProfile = null;
    }
  },

  




  set fxAOAuthProfile(aProfileData) {
    setJSONPref("loop.fxa_oauth.profile", aProfileData);
    this.notifyStatusChanged(aProfileData ? "login" : undefined);
  },

  




  get doNotDisturb() {
    return Services.prefs.getBoolPref("loop.do_not_disturb");
  },

  




  set doNotDisturb(aFlag) {
    Services.prefs.setBoolPref("loop.do_not_disturb", Boolean(aFlag));
    this.notifyStatusChanged();
  },

  notifyStatusChanged: function(aReason = null) {
    log.debug("notifyStatusChanged with reason:", aReason);
    let profile = MozLoopService.userProfile;
    LoopStorage.switchDatabase(profile ? profile.uid : null);
    Services.obs.notifyObservers(null, "loop-status-changed", aReason);
  },

  









  setError: function(errorType, error, actionCallback = null) {
    log.debug("setError", errorType, error);
    let messageString, detailsString, detailsButtonLabelString, detailsButtonCallback;
    const NETWORK_ERRORS = [
      Cr.NS_ERROR_CONNECTION_REFUSED,
      Cr.NS_ERROR_NET_INTERRUPT,
      Cr.NS_ERROR_NET_RESET,
      Cr.NS_ERROR_NET_TIMEOUT,
      Cr.NS_ERROR_OFFLINE,
      Cr.NS_ERROR_PROXY_CONNECTION_REFUSED,
      Cr.NS_ERROR_UNKNOWN_HOST,
      Cr.NS_ERROR_UNKNOWN_PROXY_HOST,
    ];

    if (error.code === null && error.errno === null &&
        error.error instanceof Ci.nsIException &&
        NETWORK_ERRORS.indexOf(error.error.result) != -1) {
      
      errorType = "network";
      messageString = "could_not_connect";
      detailsString = "check_internet_connection";
      detailsButtonLabelString = "retry_button";
    } else if (errorType == "profile" && error.code >= 500 && error.code < 600) {
      messageString = "problem_accessing_account";
    } else if (error.code == 401) {
      if (errorType == "login") {
        messageString = "could_not_authenticate"; 
        detailsString = "password_changed_question";
        detailsButtonLabelString = "retry_button";
        detailsButtonCallback = () => MozLoopService.logInToFxA();
      } else {
        messageString = "session_expired_error_description";
      }
    } else if (error.code >= 500 && error.code < 600) {
      messageString = "service_not_available";
      detailsString = "try_again_later";
      detailsButtonLabelString = "retry_button";
    } else {
      messageString = "generic_failure_title";
    }

    error.friendlyMessage = this.localizedStrings[messageString].textContent;
    error.friendlyDetails = detailsString ?
                              this.localizedStrings[detailsString].textContent :
                              null;
    error.friendlyDetailsButtonLabel = detailsButtonLabelString ?
                                         this.localizedStrings[detailsButtonLabelString].textContent :
                                         null;

    error.friendlyDetailsButtonCallback = actionCallback || detailsButtonCallback || null;

    gErrors.set(errorType, error);
    this.notifyStatusChanged();
  },

  clearError: function(errorType) {
    if (gErrors.has(errorType)) {
      gErrors.delete(errorType);
      this.notifyStatusChanged();
    }
  },

  get errors() {
    return gErrors;
  },

  






  promiseRegisteredWithPushServer: function() {
    
    function registerForNotification(channelID, onNotification) {
      log.debug("registerForNotification", channelID);
      return new Promise((resolve, reject) => {
        function onRegistered(error, pushUrl) {
          log.debug("registerForNotification onRegistered:", error, pushUrl);
          if (error) {
            reject(Error(error));
          } else {
            resolve(pushUrl);
          }
        }

        
        let pushURL = MozLoopServiceInternal.pushHandler.registeredChannels[channelID];
        if (pushURL) {
          log.debug("Using the existing push endpoint for channelID:", channelID);
          resolve(pushURL);
          return;
        }

        MozLoopServiceInternal.pushHandler.register(channelID, onRegistered, onNotification);
      });
    }

    let options = this.mocks.webSocket ? { mockWebSocket: this.mocks.webSocket } : {};
    this.pushHandler.initialize(options);

    let callsRegGuest = registerForNotification(MozLoopService.channelIDs.callsGuest,
                                                LoopCalls.onNotification);

    let roomsRegGuest = registerForNotification(MozLoopService.channelIDs.roomsGuest,
                                                roomsPushNotification);

    let callsRegFxA = registerForNotification(MozLoopService.channelIDs.callsFxA,
                                              LoopCalls.onNotification);

    let roomsRegFxA = registerForNotification(MozLoopService.channelIDs.roomsFxA,
                                              roomsPushNotification);

    return Promise.all([callsRegGuest, roomsRegGuest, callsRegFxA, roomsRegFxA]);
  },

  







  promiseRegisteredWithServers: function(sessionType = LOOP_SESSION_TYPE.GUEST) {
    if (this.deferredRegistrations.has(sessionType)) {
      log.debug("promiseRegisteredWithServers: registration already completed or in progress:", sessionType);
      return this.deferredRegistrations.get(sessionType).promise;
    }

    let result = null;
    let deferred = Promise.defer();
    log.debug("assigning to deferredRegistrations for sessionType:", sessionType);
    this.deferredRegistrations.set(sessionType, deferred);

    
    result = deferred.promise;

    this.promiseRegisteredWithPushServer().then(() => {
      return this.registerWithLoopServer(sessionType);
    }).then(() => {
      deferred.resolve("registered to status:" + sessionType);
      
      
    }, error => {
      log.error("Failed to register with Loop server with sessionType " + sessionType, error);
      deferred.reject(error);
      this.deferredRegistrations.delete(sessionType);
      log.debug("Cleared deferredRegistration for sessionType:", sessionType);
    });

    return result;
  },

  














  hawkRequest: function(sessionType, path, method, payloadObj) {
    log.debug("hawkRequest: " + path, sessionType);
    if (!gHawkClient) {
      gHawkClient = new HawkClient(this.loopServerUri);
    }

    let sessionToken;
    try {
      sessionToken = Services.prefs.getCharPref(this.getSessionTokenPrefName(sessionType));
    } catch (x) {
      
    }

    let credentials;
    if (sessionToken) {
      
      credentials = deriveHawkCredentials(sessionToken, "sessionToken",
                                          2 * 32, true);
    }

    return gHawkClient.request(path, method, credentials, payloadObj).then((result) => {
      this.clearError("network");
      return result;
    }, (error) => {
      if (error.code == 401) {
        this.clearSessionToken(sessionType);

        if (sessionType == LOOP_SESSION_TYPE.FXA) {
          MozLoopService.logOutFromFxA().then(() => {
            
            this.setError("login", error);
          });
        } else {
          if (!this.urlExpiryTimeIsInFuture()) {
            
            
            throw error;
          }

          this.setError("registration", error);
        }
      }
      throw error;
    });
  },

  






  _hawkRequestError: function(error) {
    log.error("Loop hawkRequest error:", error);
    throw error;
  },

  getSessionTokenPrefName: function(sessionType) {
    let suffix;
    switch (sessionType) {
      case LOOP_SESSION_TYPE.GUEST:
        suffix = "";
        break;
      case LOOP_SESSION_TYPE.FXA:
        suffix = ".fxa";
        break;
      default:
        throw new Error("Unknown LOOP_SESSION_TYPE");
        break;
    }
    return "loop.hawk-session-token" + suffix;
  },

  








  storeSessionToken: function(sessionType, headers) {
    let sessionToken = headers["hawk-session-token"];
    if (sessionToken) {
      
      if (sessionToken.length === 64) {
        Services.prefs.setCharPref(this.getSessionTokenPrefName(sessionType), sessionToken);
        log.debug("Stored a hawk session token for sessionType", sessionType);
      } else {
        
        log.warn("Loop server sent an invalid session token");
        return false;
      }
    }
    return true;
  },

  








  clearSessionToken: function(sessionType) {
    Services.prefs.clearUserPref(this.getSessionTokenPrefName(sessionType));
    log.debug("Cleared hawk session token for sessionType", sessionType);
  },

  









  registerWithLoopServer: function(sessionType, retry = true) {
    log.debug("registerWithLoopServer with sessionType:", sessionType);

    let callsPushURL, roomsPushURL;
    if (sessionType == LOOP_SESSION_TYPE.FXA) {
      callsPushURL = this.pushHandler.registeredChannels[MozLoopService.channelIDs.callsFxA];
      roomsPushURL = this.pushHandler.registeredChannels[MozLoopService.channelIDs.roomsFxA];
    } else if (sessionType == LOOP_SESSION_TYPE.GUEST) {
      callsPushURL = this.pushHandler.registeredChannels[MozLoopService.channelIDs.callsGuest];
      roomsPushURL = this.pushHandler.registeredChannels[MozLoopService.channelIDs.roomsGuest];
    }

    if (!callsPushURL || !roomsPushURL) {
      return Promise.reject("Invalid sessionType or missing push URLs for registerWithLoopServer: " + sessionType);
    }

    
    
    let msg = {
        simplePushURL: callsPushURL,
        simplePushURLs: {
          calls: callsPushURL,
          rooms: roomsPushURL,
        },
    };
    return this.hawkRequest(sessionType, "/registration", "POST", msg)
      .then((response) => {
        
        if (!this.storeSessionToken(sessionType, response.headers)) {
          return Promise.reject("session-token-wrong-size");
        }

        log.debug("Successfully registered with server for sessionType", sessionType);
        this.clearError("registration");
        return undefined;
      }, (error) => {
        
        
        if (error.code === 401) {
          
          if (retry) {
            return this.registerWithLoopServer(sessionType, false);
          }
        }

        log.error("Failed to register with the loop server. Error: ", error);
        let deferred = Promise.defer();
        deferred.promise.then(() => {
          log.debug("registration retry succeeded");
        },
        error => {
          log.debug("registration retry failed");
        });
        this.setError("registration", error, () => MozLoopService.delayedInitialize(deferred));
        throw error;
      }
    );
  },

  














  unregisterFromLoopServer: function(sessionType, pushURL) {
    let prefType = Services.prefs.getPrefType(this.getSessionTokenPrefName(sessionType));
    if (prefType == Services.prefs.PREF_INVALID) {
      return Promise.resolve("already unregistered");
    }

    let unregisterURL = "/registration?simplePushURL=" + encodeURIComponent(pushURL);
    return this.hawkRequest(sessionType, unregisterURL, "DELETE")
      .then(() => {
        log.debug("Successfully unregistered from server for sessionType", sessionType);
      },
      error => {
        if (error.code === 401) {
          
          return;
        }

        log.error("Failed to unregister with the loop server. Error: ", error);
        throw error;
      });
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
          log.info(e.data.ok ?
            "Successfully staged loop report for telemetry upload." :
            ("Failed to stage loop report. Error: " + e.data.fail));
        }
        worker.postMessage(job);
      });
    }, pc.id);
  },

  






  openChatWindow: function(conversationWindowData) {
    
    let origin = this.loopServerUri;
    let windowId = gLastWindowId++;
    
    windowId = windowId.toString();

    gConversationWindowData.set(windowId, conversationWindowData);

    let url = "about:loopconversation#" + windowId;

    let callback = chatbox => {
      
      
      
      
      
      if (chatbox.contentWindow.navigator.mozLoop) {
        return;
      }

      chatbox.setAttribute("dark", true);

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

    Chat.open(null, origin, "", url, undefined, undefined, callback);
    return windowId;
  },

  




  promiseFxAOAuthParameters: function() {
    const SESSION_TYPE = LOOP_SESSION_TYPE.FXA;
    return this.hawkRequest(SESSION_TYPE, "/fxa-oauth/params", "POST").then(response => {
      if (!this.storeSessionToken(SESSION_TYPE, response.headers)) {
        throw new Error("Invalid FxA hawk token returned");
      }
      let prefType = Services.prefs.getPrefType(this.getSessionTokenPrefName(SESSION_TYPE));
      if (prefType == Services.prefs.PREF_INVALID) {
        throw new Error("No FxA hawk token returned and we don't have one saved");
      }

      return JSON.parse(response.body);
    },
    error => {this._hawkRequestError(error);});
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
        log.error(error);
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
    return this.hawkRequest(LOOP_SESSION_TYPE.FXA, "/fxa-oauth/token", "POST", payload).then(response => {
      return JSON.parse(response.body);
    },
    error => {this._hawkRequestError(error);});
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


let gInitializeTimerFunc = (deferredInitialization) => {
  
  
  

  setTimeout(MozLoopService.delayedInitialize.bind(MozLoopService, deferredInitialization),
             MozLoopServiceInternal.initialRegistrationDelayMilliseconds);
};





this.MozLoopService = {
  _DNSService: gDNSService,

  get channelIDs() {
    
    return {
      callsFxA: "25389583-921f-4169-a426-a4673658944b",
      callsGuest: "801f754b-686b-43ec-bd83-1419bbf58388",
      roomsFxA: "6add272a-d316-477c-8335-f00f73dfde71",
      roomsGuest: "19d3f799-a8f3-4328-9822-b7cd02765832",
    };
  },

  set initializeTimerFunc(value) {
    gInitializeTimerFunc = value;
  },

  





  initialize: Task.async(function*() {
    
    
    Object.freeze(this);

    
    
    Services.prefs.clearUserPref("loop.throttled");
    Services.prefs.clearUserPref("loop.throttled2");
    Services.prefs.clearUserPref("loop.soft_start_ticket_number");
    Services.prefs.clearUserPref("loop.soft_start_hostname");

    
    if (!Services.prefs.getBoolPref("loop.enabled")) {
      return Promise.reject("loop is not enabled");
    }

    if (Services.prefs.getPrefType("loop.fxa.enabled") == Services.prefs.PREF_BOOL) {
      gFxAEnabled = Services.prefs.getBoolPref("loop.fxa.enabled");
      if (!gFxAEnabled) {
        yield this.logOutFromFxA();
      }
    }

    
    
    if (!MozLoopServiceInternal.urlExpiryTimeIsInFuture() &&
        !MozLoopServiceInternal.fxAOAuthTokenData) {
      return Promise.resolve("registration not needed");
    }

    let deferredInitialization = Promise.defer();
    gInitializeTimerFunc(deferredInitialization);

    return deferredInitialization.promise;
  }),

  






  delayedInitialize: Task.async(function*(deferredInitialization) {
    log.debug("delayedInitialize");
    
    
    let completedPromise = deferredInitialization.promise.then(result => {
      MozLoopServiceInternal.clearError("initialization");
      return result;
    },
    error => {
      
      if (typeof(error) == "object") {
        MozLoopServiceInternal.setError("initialization", error, () => MozLoopService.delayedInitialize(Promise.defer()));
      }
    });

    try {
      if (MozLoopServiceInternal.urlExpiryTimeIsInFuture()) {
        yield this.promiseRegisteredWithServers(LOOP_SESSION_TYPE.GUEST);
      } else {
        log.debug("delayedInitialize: URL expiry time isn't in the future so not registering as a guest");
      }
    } catch (ex) {
      log.debug("MozLoopService: Failure of guest registration", ex);
      deferredInitialization.reject(ex);
      yield completedPromise;
      return;
    }

    if (!MozLoopServiceInternal.fxAOAuthTokenData) {
      log.debug("delayedInitialize: Initialized without an already logged-in account");
      deferredInitialization.resolve("initialized without FxA status");
      yield completedPromise;
      return;
    }

    log.debug("MozLoopService: Initializing with already logged-in account");
    MozLoopServiceInternal.promiseRegisteredWithServers(LOOP_SESSION_TYPE.FXA).then(() => {
      deferredInitialization.resolve("initialized to logged-in status");
    }, error => {
      log.debug("MozLoopService: error logging in using cached auth token");
      MozLoopServiceInternal.setError("login", error);
      deferredInitialization.reject("error logging in using cached auth token");
    });
    yield completedPromise;
  }),

  






  openChatWindow: function(conversationWindowData) {
    return MozLoopServiceInternal.openChatWindow(conversationWindowData);
  },

  


  promiseRegisteredWithServers: function(sessionType = LOOP_SESSION_TYPE.GUEST) {
    return MozLoopServiceInternal.promiseRegisteredWithServers(sessionType);
  },

  











  noteCallUrlExpiry: function(expiryTimeSeconds) {
    MozLoopServiceInternal.expiryTimeSeconds = expiryTimeSeconds;
  },

  







  getStrings: function(key) {
      var stringData = MozLoopServiceInternal.localizedStrings;
      if (!(key in stringData)) {
        log.error("No string found for key: ", key);
        return "";
      }

      return JSON.stringify(stringData[key]);
  },

  


  generateUUID: function() {
    return uuidgen.generateUUID().toString();
  },

  






  generateLocalID: function(notUnique = ((id) => {return false})) {
    do {
      var id = Date.now().toString(36) + Math.floor((Math.random() * 4096)).toString(16);
    }
    while (notUnique(id));
    return id;
  },

  




  get doNotDisturb() {
    return MozLoopServiceInternal.doNotDisturb;
  },

  




  set doNotDisturb(aFlag) {
    MozLoopServiceInternal.doNotDisturb = aFlag;
  },

  get fxAEnabled() {
    return gFxAEnabled;
  },

  






  get userProfile() {
    return getJSONPref("loop.fxa_oauth.tokendata") &&
           getJSONPref("loop.fxa_oauth.profile");
  },

  get errors() {
    return MozLoopServiceInternal.errors;
  },

  get log() {
    return log;
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
      log.error("setLoopCharPref had trouble setting " + prefName +
        "; exception: " + ex);
    }
  },

  












  getLoopCharPref: function(prefName) {
    try {
      return Services.prefs.getCharPref("loop." + prefName);
    } catch (ex) {
      log.error("getLoopCharPref had trouble getting " + prefName +
        "; exception: " + ex);
      return null;
    }
  },

  












  getLoopBoolPref: function(prefName) {
    try {
      return Services.prefs.getBoolPref("loop." + prefName);
    } catch (ex) {
      log.error("getLoopBoolPref had trouble getting " + prefName +
        "; exception: " + ex);
      return null;
    }
  },

  






  logInToFxA: function() {
    log.debug("logInToFxA with fxAOAuthTokenData:", !!MozLoopServiceInternal.fxAOAuthTokenData);
    if (MozLoopServiceInternal.fxAOAuthTokenData) {
      return Promise.resolve(MozLoopServiceInternal.fxAOAuthTokenData);
    }
    return MozLoopServiceInternal.promiseFxAOAuthAuthorization().then(response => {
      return MozLoopServiceInternal.promiseFxAOAuthToken(response.code, response.state);
    }).then(tokenData => {
      MozLoopServiceInternal.fxAOAuthTokenData = tokenData;
      return tokenData;
    }).then(tokenData => {
      return MozLoopServiceInternal.promiseRegisteredWithServers(LOOP_SESSION_TYPE.FXA).then(() => {
        MozLoopServiceInternal.clearError("login");
        MozLoopServiceInternal.clearError("profile");
        return MozLoopServiceInternal.fxAOAuthTokenData;
      });
    }).then(tokenData => {
      let client = new FxAccountsProfileClient({
        serverURL: gFxAOAuthClient.parameters.profile_uri,
        token: tokenData.access_token
      });
      client.fetchProfile().then(result => {
        MozLoopServiceInternal.fxAOAuthProfile = result;
      }, error => {
        log.error("Failed to retrieve profile", error);
        this.setError("profile", error);
        MozLoopServiceInternal.fxAOAuthProfile = null;
        MozLoopServiceInternal.notifyStatusChanged();
      });
      return tokenData;
    }).catch(error => {
      MozLoopServiceInternal.fxAOAuthTokenData = null;
      MozLoopServiceInternal.fxAOAuthProfile = null;
      MozLoopServiceInternal.deferredRegistrations.delete(LOOP_SESSION_TYPE.FXA);
      throw error;
    }).catch((error) => {
      MozLoopServiceInternal.setError("login", error);
      
      throw error;
    });
  },

  






  logOutFromFxA: Task.async(function*() {
    log.debug("logOutFromFxA");
    let pushHandler = MozLoopServiceInternal.pushHandler;
    let callsPushUrl = pushHandler.registeredChannels[MozLoopService.channelIDs.callsFxA];
    let roomsPushUrl = pushHandler.registeredChannels[MozLoopService.channelIDs.roomsFxA];
    try {
      if (callsPushUrl) {
        yield MozLoopServiceInternal.unregisterFromLoopServer(LOOP_SESSION_TYPE.FXA, callsPushUrl);
      }
      if (roomsPushUrl) {
        yield MozLoopServiceInternal.unregisterFromLoopServer(LOOP_SESSION_TYPE.FXA, roomsPushUrl);
      }
    } catch (error) {
      throw error;
    } finally {
      MozLoopServiceInternal.clearSessionToken(LOOP_SESSION_TYPE.FXA);

      MozLoopServiceInternal.fxAOAuthTokenData = null;
      MozLoopServiceInternal.fxAOAuthProfile = null;
      MozLoopServiceInternal.deferredRegistrations.delete(LOOP_SESSION_TYPE.FXA);

      
      
      gFxAOAuthClient = null;
      gFxAOAuthClientPromise = null;

      
      
      MozLoopServiceInternal.clearError("registration");
      MozLoopServiceInternal.clearError("login");
      MozLoopServiceInternal.clearError("profile");
    }
  }),

  openFxASettings: Task.async(function() {
    try {
      let fxAOAuthClient = yield MozLoopServiceInternal.promiseFxAOAuthClient();
      if (!fxAOAuthClient) {
        log.error("Could not get the OAuth client");
        return;
      }
      let url = new URL("/settings", fxAOAuthClient.parameters.content_uri);
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      win.switchToTabHavingURI(url.toString(), true);
    } catch (ex) {
      log.error("Error opening FxA settings", ex);
    }
  }),

  














  hawkRequest: function(sessionType, path, method, payloadObj) {
    return MozLoopServiceInternal.hawkRequest(sessionType, path, method, payloadObj).catch(
      error => {MozLoopServiceInternal._hawkRequestError(error);});
  },

  








  getConversationWindowData: function(conversationWindowId) {
    if (gConversationWindowData.has(conversationWindowId)) {
      var conversationData = gConversationWindowData.get(conversationWindowId);
      gConversationWindowData.delete(conversationWindowId);
      return conversationData;
    }

    log.error("Window data was already fetched before. Possible race condition!");
    return null;
  }
};
