



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;



const INVALID_AUTH_TOKEN = 110;

const LOOP_SESSION_TYPE = {
  GUEST: 1,
  FXA: 2,
};








const TWO_WAY_MEDIA_CONN_LENGTH = {
  SHORTER_THAN_10S: 0,
  BETWEEN_10S_AND_30S: 1,
  BETWEEN_30S_AND_5M: 2,
  MORE_THAN_5M: 3,
};







const SHARING_STATE_CHANGE = {
  WINDOW_ENABLED: 0,
  WINDOW_DISABLED: 1,
  BROWSER_ENABLED: 2,
  BROWSER_DISABLED: 3
};


const PREF_LOG_LEVEL = "loop.debug.loglevel";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/FxAccountsOAuthClient.jsm");

Cu.importGlobalProperties(["URL"]);

this.EXPORTED_SYMBOLS = ["MozLoopService", "LOOP_SESSION_TYPE",
  "TWO_WAY_MEDIA_CONN_LENGTH", "SHARING_STATE_CHANGE"];

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI",
  "resource:///modules/loop/MozLoopAPI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "convertToRTCStatsReport",
  "resource://gre/modules/media/RTCStatsReport.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "loopUtils",
  "resource:///modules/loop/utils.js", "utils")
XPCOMUtils.defineLazyModuleGetter(this, "loopCrypto",
  "resource:///modules/loop/crypto.js", "LoopCrypto");

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

XPCOMUtils.defineLazyModuleGetter(this, "UITour",
                                  "resource:///modules/UITour.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");

XPCOMUtils.defineLazyServiceGetter(this, "gWM",
                                   "@mozilla.org/appshell/window-mediator;1",
                                   "nsIWindowMediator");


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
let gLocalizedStrings = new Map();
let gFxAEnabled = true;
let gFxAOAuthClientPromise = null;
let gFxAOAuthClient = null;
let gErrors = new Map();
let gLastWindowId = 0;
let gConversationWindowData = new Map();








let MozLoopServiceInternal = {
  conversationContexts: new Map(),
  pushURLs: new Map(),

  mocks: {
    pushHandler: undefined,
  },

  




  deferredRegistrations: new Map(),

  get pushHandler() {
    return this.mocks.pushHandler || MozLoopPushHandler
  },

  
  get loopServerUri() {
    return Services.prefs.getCharPref("loop.server")
  },

  



  get initialRegistrationDelayMilliseconds() {
    try {
      
      return Services.prefs.getIntPref("loop.initialDelay");
    } catch (x) {
      
      return 5000;
    }
    return initialDelay;
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
    LoopStorage.switchDatabase(profile && profile.uid);
    LoopRooms.maybeRefresh(profile && profile.uid);
    Services.obs.notifyObservers(null, "loop-status-changed", aReason);
  },

  









  setError: function(errorType, error, actionCallback = null) {
    log.debug("setError", errorType, error);
    log.trace();
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

    error.friendlyMessage = this.localizedStrings.get(messageString);

    
    
    error.friendlyDetailsButtonLabel = detailsButtonLabelString ?
                                         this.localizedStrings.get(detailsButtonLabelString) :
                                         this.localizedStrings.get("retry_button");

    error.friendlyDetailsButtonCallback = actionCallback || detailsButtonCallback || null;

    if (detailsString) {
      error.friendlyDetails = this.localizedStrings.get(detailsString);
    } else if (error.friendlyDetailsButtonCallback) {
      
      error.friendlyDetails = this.localizedStrings.get("generic_failure_no_reason2");
    } else {
      error.friendlyDetails = null;
    }

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

  














  createNotificationChannel: function(channelID, sessionType, serviceType, onNotification) {
    log.debug("createNotificationChannel", channelID, sessionType, serviceType);
    
    return new Promise((resolve, reject) => {
      let onRegistered = (error, pushURL, channelID) => {
        log.debug("createNotificationChannel onRegistered:", error, pushURL, channelID);
        if (error) {
          reject(Error(error));
        } else {
          resolve(this.registerWithLoopServer(sessionType, serviceType, pushURL));
        }
      }

      this.pushHandler.register(channelID, onRegistered, onNotification);
    });
  },

  









  promiseRegisteredWithServers: function(sessionType = LOOP_SESSION_TYPE.GUEST) {
    if (sessionType !== LOOP_SESSION_TYPE.GUEST && sessionType !== LOOP_SESSION_TYPE.FXA) {
      return Promise.reject(new Error("promiseRegisteredWithServers: Invalid sessionType"));
    }

    if (this.deferredRegistrations.has(sessionType)) {
      log.debug("promiseRegisteredWithServers: registration already completed or in progress:",
                sessionType);
      return this.deferredRegistrations.get(sessionType);
    }

    let options = this.mocks.webSocket ? { mockWebSocket: this.mocks.webSocket } : {};
    this.pushHandler.initialize(options); 

    let regPromise;
    if (sessionType == LOOP_SESSION_TYPE.GUEST) {
      regPromise = this.createNotificationChannel(
        MozLoopService.channelIDs.roomsGuest, sessionType, "rooms",
        roomsPushNotification);
    } else {
      regPromise = this.createNotificationChannel(
        MozLoopService.channelIDs.callsFxA, sessionType, "calls",
        LoopCalls.onNotification).then(() => {
          return this.createNotificationChannel(
            MozLoopService.channelIDs.roomsFxA, sessionType, "rooms",
            roomsPushNotification);
        });
    }

    log.debug("assigning to deferredRegistrations for sessionType:", sessionType);
    this.deferredRegistrations.set(sessionType, regPromise);

    
    
    regPromise.catch((error) => {
      log.error("Failed to register with Loop server with sessionType ", sessionType, error);
      this.deferredRegistrations.delete(sessionType);
      log.debug("Cleared deferredRegistration for sessionType:", sessionType);
    });

    return regPromise;
  },

  








  registerWithLoopServer: function(sessionType, serviceType, pushURL, retry = true) {
    log.debug("registerWithLoopServer with sessionType:", sessionType, serviceType, retry);
    if (!pushURL || !sessionType || !serviceType) {
      return Promise.reject(new Error("Invalid or missing parameters for registerWithLoopServer"));
    }

    let pushURLs = this.pushURLs.get(sessionType);

    
    if (!pushURLs) {
      pushURLs = {calls: undefined, rooms: undefined};
      this.pushURLs.set(sessionType, pushURLs);
    }

    if (pushURLs[serviceType] == pushURL) {
      return Promise.resolve(pushURL);
    }

    let newURLs = {calls: pushURLs.calls,
                   rooms: pushURLs.rooms};
    newURLs[serviceType] = pushURL;

    return this.hawkRequestInternal(sessionType, "/registration", "POST",
                                    {simplePushURLs: newURLs}).then(
      (response) => {
        
        if (!this.storeSessionToken(sessionType, response.headers)) {
          throw new Error("session-token-wrong-size");
        }

        
        pushURLs[serviceType] = pushURL;
        log.debug("Successfully registered with server for sessionType", sessionType);
        this.clearError("registration");
        return pushURL;
      }, (error) => {
        
        
        if (error.code === 401) {
          
          
          
          if (retry) {
            return this.registerWithLoopServer(sessionType, serviceType, pushURL, false);
          }
        }

        log.error("Failed to register with the loop server. Error: ", error);
        throw error;
      }
    );
  },

  












  unregisterFromLoopServer: function(sessionType) {
    let prefType = Services.prefs.getPrefType(this.getSessionTokenPrefName(sessionType));
    if (prefType == Services.prefs.PREF_INVALID) {
      log.debug("already unregistered from LoopServer", sessionType);
      return Promise.resolve("already unregistered");
    }

    let error,
        pushURLs = this.pushURLs.get(sessionType),
        callsPushURL = pushURLs ? pushURLs.calls : null,
        roomsPushURL = pushURLs ? pushURLs.rooms : null;
    this.pushURLs.delete(sessionType);

    let unregister = (sessionType, pushURL) => {
      if (!pushURL) {
        return Promise.resolve("no pushURL of this type to unregister");
      }

      let unregisterURL = "/registration?simplePushURL=" + encodeURIComponent(pushURL);
      return this.hawkRequestInternal(sessionType, unregisterURL, "DELETE").then(
        () => {
          log.debug("Successfully unregistered from server for sessionType = ", sessionType);
          return "unregistered sessionType " + sessionType;
        },
        error => {
          if (error.code === 401) {
            
            log.debug("already unregistered - invalid token", sessionType);
            return "already unregistered, sessionType = " + sessionType;
          }

          log.error("Failed to unregister with the loop server. Error: ", error);
          throw error;
        });
    }

    return Promise.all([unregister(sessionType, callsPushURL), unregister(sessionType, roomsPushURL)]);
  },

  
















  hawkRequestInternal: function(sessionType, path, method, payloadObj, retryOn401 = true) {
    log.debug("hawkRequestInternal: ", sessionType, path, method);
    if (!gHawkClient) {
      gHawkClient = new HawkClient(this.loopServerUri);
    }

    let sessionToken, credentials;
    try {
      sessionToken = Services.prefs.getCharPref(this.getSessionTokenPrefName(sessionType));
    } catch (x) {
      
    }

    if (sessionToken) {
      
      credentials = deriveHawkCredentials(sessionToken, "sessionToken",
                                          2 * 32, true);
    }

    if (payloadObj) {
      
      
      let newPayloadObj = {};
      for (let property of Object.getOwnPropertyNames(payloadObj)) {
        if (typeof payloadObj[property] == "string") {
          newPayloadObj[property] = CommonUtils.encodeUTF8(payloadObj[property]);
        } else {
          newPayloadObj[property] = payloadObj[property];
        }
      };
      payloadObj = newPayloadObj;
    }

    let handle401Error = (error) => {
      if (sessionType === LOOP_SESSION_TYPE.FXA) {
        return MozLoopService.logOutFromFxA().then(() => {
          
          this.setError("login", error);
          throw error;
        });
      }
      this.setError("registration", error);
      throw error;
    };

    return gHawkClient.request(path, method, credentials, payloadObj).then(
      (result) => {
        this.clearError("network");
        return result;
      },
      (error) => {
      if (error.code && error.code == 401) {
        this.clearSessionToken(sessionType);
        if (retryOn401 && sessionType === LOOP_SESSION_TYPE.GUEST) {
          log.info("401 and INVALID_AUTH_TOKEN - retry registration");
          return this.registerWithLoopServer(sessionType, false).then(
            () => {
              return this.hawkRequestInternal(sessionType, path, method, payloadObj, false);
            },
            () => {
              return handle401Error(error); 
            }
          );
        }
        return handle401Error(error);
      }
      throw error;
    });
  },

  














  hawkRequest: function(sessionType, path, method, payloadObj) {
    log.debug("hawkRequest: " + path, sessionType);
    return new Promise((resolve, reject) => {
      MozLoopService.promiseRegisteredWithServers(sessionType).then(() => {
        this.hawkRequestInternal(sessionType, path, method, payloadObj).then(resolve, reject);
      }, err => {
        reject(err);
      }).catch(reject);
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

  





  get localizedStrings() {
    if (gLocalizedStrings.size)
      return gLocalizedStrings;

    let stringBundle =
      Services.strings.createBundle("chrome://browser/locale/loop/loop.properties");

    let enumerator = stringBundle.getSimpleEnumeration();
    while (enumerator.hasMoreElements()) {
      let string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
      gLocalizedStrings.set(string.key, string.value);
    }
    
    let brandBundle =
      Services.strings.createBundle("chrome://branding/locale/brand.properties");
    
    gLocalizedStrings.set("brandShortname", brandBundle.GetStringFromName("brandShortName"));

    return gLocalizedStrings;
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

  getChatWindowID: function(conversationWindowData) {
    
    
    
    
    let windowId = ("contact" in conversationWindowData) ?
                   conversationWindowData.contact._guid || gLastWindowId++ :
                   conversationWindowData.roomToken || conversationWindowData.callId ||
                   gLastWindowId++;
    return windowId.toString();
  },

  getChatURL: function(chatWindowId) {
    return "about:loopconversation#" + chatWindowId;
  },

  







  openChatWindow: function(conversationWindowData) {
    
    let origin = this.loopServerUri;
    let windowId = this.getChatWindowID(conversationWindowData);

    gConversationWindowData.set(windowId, conversationWindowData);

    let url = this.getChatURL(windowId);

    let callback = chatbox => {
      
      
      
      
      
      if (chatbox.contentWindow.navigator.mozLoop) {
        return;
      }

      chatbox.setAttribute("dark", true);
      chatbox.setAttribute("large", true);

      chatbox.addEventListener("DOMContentLoaded", function loaded(event) {
        if (event.target != chatbox.contentDocument) {
          return;
        }
        chatbox.removeEventListener("DOMContentLoaded", loaded, true);

        let window = chatbox.contentWindow;

        function socialFrameChanged(eventName) {
          UITour.availableTargetsCache.clear();
          UITour.notify(eventName);
        }

        window.addEventListener("socialFrameHide", socialFrameChanged.bind(null, "Loop:ChatWindowHidden"));
        window.addEventListener("socialFrameShow", socialFrameChanged.bind(null, "Loop:ChatWindowShown"));
        window.addEventListener("socialFrameDetached", socialFrameChanged.bind(null, "Loop:ChatWindowDetached"));
        window.addEventListener("unload", socialFrameChanged.bind(null, "Loop:ChatWindowClosed"));

        injectLoopAPI(window);

        let ourID = window.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

        let onPCLifecycleChange = (pc, winID, type) => {
          if (winID != ourID) {
            return;
          }

          
          let windowId = window.location.hash.slice(1);
          var context = this.conversationContexts.get(windowId);
          var exists = pc.id.match(/session=(\S+)/);
          if (context && !exists) {
            
            
            
            var pair = pc.id.split("(");  
            if (pair.length == 2) {
              pc.id = pair[0] + "(session=" + context.sessionId +
                  (context.callId? " call=" + context.callId : "") + " " + pair[1]; 
            }
          }

          if (type == "iceconnectionstatechange") {
            switch(pc.iceConnectionState) {
              case "failed":
              case "disconnected":
                if (Services.telemetry.canRecordExtended) {
                  this.stageForTelemetryUpload(window, pc);
                }
                break;
            }
          }
        };

        let pc_static = new window.mozRTCPeerConnectionStatic();
        pc_static.registerPeerConnectionLifecycleCallback(onPCLifecycleChange);

        UITour.notify("Loop:ChatWindowOpened");
      }.bind(this), true);
    };

    if (!Chat.open(null, origin, "", url, undefined, undefined, callback)) {
      return null;
    }
    return windowId;
  },

  




  promiseFxAOAuthParameters: function() {
    const SESSION_TYPE = LOOP_SESSION_TYPE.FXA;
    return this.hawkRequestInternal(SESSION_TYPE, "/fxa-oauth/params", "POST").then(response => {
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
        
        parameters.keys = true;

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
        client.onError = this._fxAOAuthError.bind(this, deferred);
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
    return this.hawkRequestInternal(LOOP_SESSION_TYPE.FXA, "/fxa-oauth/token", "POST", payload).then(response => {
      return JSON.parse(response.body);
    },
    error => {this._hawkRequestError(error);});
  },

  





  _fxAOAuthComplete: function(deferred, result, keys) {
    if (keys.kBr) {
      Services.prefs.setCharPref("loop.key.fxa", keys.kBr.k);
    }
    gFxAOAuthClientPromise = null;
    
    deferred.resolve(result);
  },

  





  _fxAOAuthError: function(deferred, err) {
    gFxAOAuthClientPromise = null;
    deferred.reject(err);
  },
};
Object.freeze(MozLoopServiceInternal);


let gInitializeTimerFunc = (deferredInitialization) => {
  
  
  

  setTimeout(MozLoopService.delayedInitialize.bind(MozLoopService, deferredInitialization),
             MozLoopServiceInternal.initialRegistrationDelayMilliseconds);
};





this.MozLoopService = {
  _DNSService: gDNSService,
  _activeScreenShares: [],

  get channelIDs() {
    
    return {
      callsFxA: "25389583-921f-4169-a426-a4673658944b",
      roomsFxA: "6add272a-d316-477c-8335-f00f73dfde71",
      roomsGuest: "19d3f799-a8f3-4328-9822-b7cd02765832",
    };
  },

  set initializeTimerFunc(value) {
    gInitializeTimerFunc = value;
  },

  get roomsParticipantsCount() {
    return LoopRooms.participantsCount;
  },

  





  initialize: Task.async(function*() {
    
    
    Object.freeze(this);

    
    if (!Services.prefs.getBoolPref("loop.enabled")) {
      return Promise.reject(new Error("loop is not enabled"));
    }

    if (Services.prefs.getPrefType("loop.fxa.enabled") == Services.prefs.PREF_BOOL) {
      gFxAEnabled = Services.prefs.getBoolPref("loop.fxa.enabled");
      if (!gFxAEnabled) {
        yield this.logOutFromFxA();
      }
    }

    
    
    const onRoomsChange = (e) => {
      
      MozLoopServiceInternal.notifyStatusChanged("room-" + e);
    };
    LoopRooms.on("add", onRoomsChange);
    LoopRooms.on("update", onRoomsChange);
    LoopRooms.on("delete", onRoomsChange);
    LoopRooms.on("joined", (e, room, participant) => {
      
      
      if (MozLoopServiceInternal.doNotDisturb || participant.owner) {
        return;
      }

      let window = gWM.getMostRecentWindow("navigator:browser");
      if (window) {
        window.LoopUI.showNotification({
          sound: "room-joined",
          
          title: room.roomName || MozLoopServiceInternal.localizedStrings.get("clientShortname2"),
          message: MozLoopServiceInternal.localizedStrings.get("rooms_room_joined_label"),
          selectTab: "rooms"
        });
      }
    });

    LoopRooms.on("joined", this.maybeResumeTourOnRoomJoined.bind(this));

    
    
    if (!LoopRooms.getGuestCreatedRoom() &&
        !MozLoopServiceInternal.fxAOAuthTokenData) {
      return Promise.resolve("registration not needed");
    }

    let deferredInitialization = Promise.defer();
    gInitializeTimerFunc(deferredInitialization);

    return deferredInitialization.promise;
  }),

  



  maybeResumeTourOnRoomJoined: function(e, room, participant) {
    let isOwnerInRoom = false;
    let isOtherInRoom = false;

    if (!this.getLoopPref("gettingStarted.resumeOnFirstJoin")) {
      return;
    }

    if (!room.participants) {
      return;
    }

    
    
    for (let participant of room.participants.concat(participant)) {
      if (participant.owner) {
        isOwnerInRoom = true;
      } else {
        isOtherInRoom = true;
      }
    }

    if (!isOwnerInRoom || !isOtherInRoom) {
      return;
    }

    
    let chatboxesForRoom = [...Chat.chatboxes].filter(chatbox => {
      return chatbox.src == MozLoopServiceInternal.getChatURL(room.roomToken);
    });

    if (!chatboxesForRoom.length) {
      log.warn("Tried to resume the tour from a join when the chatbox was closed", room);
      return;
    }

    this.resumeTour("open");
  },

  






  delayedInitialize: Task.async(function*(deferredInitialization) {
    log.debug("delayedInitialize");
    
    
    let completedPromise = deferredInitialization.promise.then(result => {
      MozLoopServiceInternal.clearError("initialization");
      return result;
    },
    error => {
      
      if (typeof error == "object") {
        MozLoopServiceInternal.setError("initialization", error, () => MozLoopService.delayedInitialize(Promise.defer()));
      }
    });

    try {
      if (LoopRooms.getGuestCreatedRoom()) {
        yield this.promiseRegisteredWithServers(LOOP_SESSION_TYPE.GUEST);
      } else {
        log.debug("delayedInitialize: Guest Room hasn't been created so not registering as a guest");
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
      let retryFunc = () => MozLoopServiceInternal.promiseRegisteredWithServers(LOOP_SESSION_TYPE.FXA);
      MozLoopServiceInternal.setError("login", error, retryFunc);
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

  






  getStrings: function(key) {
    var stringData = MozLoopServiceInternal.localizedStrings;
    if (!stringData.has(key)) {
      log.error("No string found for key: ", key);
      return "";
    }

    return JSON.stringify({ textContent: stringData.get(key) });
  },

  


  generateUUID: function() {
    return uuidgen.generateUUID().toString();
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

  


  promiseProfileEncryptionKey: function() {
    return new Promise((resolve, reject) => {
      if (this.userProfile) {
        
        if (Services.prefs.prefHasUserValue("loop.key.fxa")) {
          resolve(MozLoopService.getLoopPref("key.fxa"));
          return;
        }

        
        
        reject(new Error("FxA re-register not implemented"));
        return;
      }

      
      
      if (!Services.prefs.prefHasUserValue("loop.key")) {
        
        loopCrypto.generateKey().then(key => {
          Services.prefs.setCharPref("loop.key", key);
          resolve(key);
        }).catch(function(error) {
          MozLoopService.log.error(error);
          reject(error);
        });
        return;
      }

      resolve(MozLoopService.getLoopPref("key"));
    });
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

  








  setLoopPref: function(prefSuffix, value, prefType) {
    let prefName = "loop." + prefSuffix;
    try {
      if (!prefType) {
        prefType = Services.prefs.getPrefType(prefName);
      }
      switch (prefType) {
        case Ci.nsIPrefBranch.PREF_STRING:
          Services.prefs.setCharPref(prefName, value);
          break;
        case Ci.nsIPrefBranch.PREF_INT:
          Services.prefs.setIntPref(prefName, value);
          break;
        case Ci.nsIPrefBranch.PREF_BOOL:
          Services.prefs.setBoolPref(prefName, value);
          break;
        default:
          log.error("invalid preference type setting " + prefName);
          break;
      }
    } catch (ex) {
      log.error("setLoopPref had trouble setting " + prefName +
        "; exception: " + ex);
    }
  },

  












  getLoopPref: function(prefSuffix, prefType) {
    let prefName = "loop." + prefSuffix;
    try {
      if (!prefType) {
        prefType = Services.prefs.getPrefType(prefName);
      } else if (prefType != Services.prefs.getPrefType(prefName)) {
        log.error("invalid type specified for preference");
        return null;
      }
      switch (prefType) {
        case Ci.nsIPrefBranch.PREF_STRING:
          return Services.prefs.getCharPref(prefName);
        case Ci.nsIPrefBranch.PREF_INT:
          return Services.prefs.getIntPref(prefName);
        case Ci.nsIPrefBranch.PREF_BOOL:
          return Services.prefs.getBoolPref(prefName);
        default:
          log.error("invalid preference type getting " + prefName);
          return null;
      }
    } catch (ex) {
      log.error("getLoopPref had trouble getting " + prefName +
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
    }).then(Task.async(function* fetchProfile(tokenData) {
      yield MozLoopService.fetchFxAProfile(tokenData);
      return tokenData;
    })).catch(error => {
      MozLoopServiceInternal.fxAOAuthTokenData = null;
      MozLoopServiceInternal.fxAOAuthProfile = null;
      MozLoopServiceInternal.deferredRegistrations.delete(LOOP_SESSION_TYPE.FXA);
      throw error;
    }).catch((error) => {
      MozLoopServiceInternal.setError("login", error,
                                      () => MozLoopService.logInToFxA());
      
      throw error;
    });
  },

  






  logOutFromFxA: Task.async(function*() {
    log.debug("logOutFromFxA");
    try {
      yield MozLoopServiceInternal.unregisterFromLoopServer(LOOP_SESSION_TYPE.FXA);
    }
    catch (err) {
      throw err;
    }
    finally {
      MozLoopServiceInternal.clearSessionToken(LOOP_SESSION_TYPE.FXA);
      MozLoopServiceInternal.fxAOAuthTokenData = null;
      MozLoopServiceInternal.fxAOAuthProfile = null;
      MozLoopServiceInternal.deferredRegistrations.delete(LOOP_SESSION_TYPE.FXA);
      
      
      MozLoopServiceInternal.pushHandler.unregister(MozLoopService.channelIDs.callsFxA);
      MozLoopServiceInternal.pushHandler.unregister(MozLoopService.channelIDs.roomsFxA);

      
      
      gFxAOAuthClient = null;
      gFxAOAuthClientPromise = null;

      
      
      MozLoopServiceInternal.clearError("registration");
      MozLoopServiceInternal.clearError("login");
      MozLoopServiceInternal.clearError("profile");
    }
  }),

  






  fetchFxAProfile: function() {
    log.debug("fetchFxAProfile");
    let client = new FxAccountsProfileClient({
      serverURL: gFxAOAuthClient.parameters.profile_uri,
      token: MozLoopServiceInternal.fxAOAuthTokenData.access_token
    });
    return client.fetchProfile().then(result => {
      MozLoopServiceInternal.fxAOAuthProfile = result;
      MozLoopServiceInternal.clearError("profile");
    }, error => {
      log.error("Failed to retrieve profile", error, this.fetchFxAProfile.bind(this));
      MozLoopServiceInternal.setError("profile", error);
      MozLoopServiceInternal.fxAOAuthProfile = null;
      MozLoopServiceInternal.notifyStatusChanged();
    });
  },

  openFxASettings: Task.async(function* () {
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

  





  getTourURL: function(aSrc = null, aAdditionalParams = {}) {
    let urlStr = this.getLoopPref("gettingStarted.url");
    let url = new URL(Services.urlFormatter.formatURL(urlStr));
    for (let paramName in aAdditionalParams) {
      url.searchParams.set(paramName, aAdditionalParams[paramName]);
    }
    if (aSrc) {
      url.searchParams.set("utm_source", "firefox-browser");
      url.searchParams.set("utm_medium", "firefox-browser");
      url.searchParams.set("utm_campaign", aSrc);
    }

    
    let mostRecentLoopPageID = {id: null, lastSeen: null};
    for (let pageID of UITour.pageIDsForSession) {
      if (pageID[0] && pageID[0].startsWith("hello-tour_OpenPanel_") &&
          pageID[1] && pageID[1].lastSeen > mostRecentLoopPageID.lastSeen) {
        mostRecentLoopPageID.id = pageID[0];
        mostRecentLoopPageID.lastSeen = pageID[1].lastSeen;
      }
    }

    const PAGE_ID_EXPIRATION_MS = 60 * 60 * 1000;
    if (mostRecentLoopPageID.id &&
        mostRecentLoopPageID.lastSeen > Date.now() - PAGE_ID_EXPIRATION_MS) {
      url.searchParams.set("utm_content", mostRecentLoopPageID.id);
    }
    return url;
  },

  resumeTour: function(aIncomingConversationState) {
    if (!this.getLoopPref("gettingStarted.resumeOnFirstJoin")) {
      return;
    }

    let url = this.getTourURL("resume-with-conversation", {
      incomingConversation: aIncomingConversationState,
    });

    let win = Services.wm.getMostRecentWindow("navigator:browser");

    this.setLoopPref("gettingStarted.resumeOnFirstJoin", false);

    
    
    let hadExistingTab = win.switchToTabHavingURI(url, true, {
      ignoreFragment: true,
      ignoreQueryString: true,
    });

    
    
    if (hadExistingTab) {
      UITour.notify("Loop:IncomingConversation", {
        conversationOpen: aIncomingConversationState === "open",
      });
    }
  },

  




  openGettingStartedTour: Task.async(function(aSrc = null) {
    try {
      let url = this.getTourURL(aSrc);
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      win.switchToTabHavingURI(url, true, {
        ignoreFragment: true,
        replaceQueryString: true,
      });
    } catch (ex) {
      log.error("Error opening Getting Started tour", ex);
    }
  }),

  




  openURL: function(url) {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    win.openUILinkIn(url, "tab");
  },

  














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
  },

  getConversationContext: function(winId) {
    return MozLoopServiceInternal.conversationContexts.get(winId);
  },

  addConversationContext: function(windowId, context) {
    MozLoopServiceInternal.conversationContexts.set(windowId, context);
  },

  







  setScreenShareState: function(windowId, active) {
    if (active) {
      this._activeScreenShares.push(windowId);
    } else {
      var index = this._activeScreenShares.indexOf(windowId);
      if (index != -1) {
        this._activeScreenShares.splice(index, 1);
      }
    }

    MozLoopServiceInternal.notifyStatusChanged();
  },

  


  get screenShareActive() {
    return this._activeScreenShares.length > 0;
  }
};
