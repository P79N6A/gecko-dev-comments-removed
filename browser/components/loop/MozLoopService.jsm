



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;



const INVALID_AUTH_TOKEN = 110;




const MAX_SOFT_START_TICKET_NUMBER = 16777214;

const LOOP_SESSION_TYPE = {
  GUEST: 1,
  FXA: 2,
};

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/FxAccountsOAuthClient.jsm");

this.EXPORTED_SYMBOLS = ["MozLoopService", "LOOP_SESSION_TYPE"];

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

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfileClient",
                                  "resource://gre/modules/FxAccountsProfileClient.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HawkClient",
                                  "resource://services-common/hawkclient.js");

XPCOMUtils.defineLazyModuleGetter(this, "deriveHawkCredentials",
                                  "resource://services-common/hawkrequest.js");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");





let gRegisteredDeferred = null;
let gPushHandler = null;
let gHawkClient = null;
let gLocalizedStrings =  null;
let gInitializeTimer = null;
let gFxAOAuthClientPromise = null;
let gFxAOAuthClient = null;
let gFxAOAuthTokenData = null;
let gFxAOAuthProfile = null;
let gErrors = new Map();

 







function CallProgressSocket(progressUrl, callId, token) {
  if (!progressUrl || !callId || !token) {
    throw new Error("missing required arguments");
  }

  this._progressUrl = progressUrl;
  this._callId = callId;
  this._token = token;
}

CallProgressSocket.prototype = {
  







  connect: function(onSuccess, onError) {
    this._onSuccess = onSuccess;
    this._onError = onError ||
      (reason => {console.warn("MozLoopService::callProgessSocket - ", reason);});

    if (!onSuccess) {
      this._onError("missing onSuccess argument");
      return;
    }

    if (Services.io.offline) {
      this._onError("IO offline");
      return;
    }

    let uri = Services.io.newURI(this._progressUrl, null, null);

    
    this._websocket = this._websocket ||
      Cc["@mozilla.org/network/protocol;1?name=" + uri.scheme]
        .createInstance(Ci.nsIWebSocketChannel);

    this._websocket.asyncOpen(uri, this._progressUrl, this, null);
  },

  





  onStart: function() {
    let helloMsg = {
      messageType: "hello",
      callId: this._callId,
      auth: this._token,
    };
    try { 
      this._websocket.sendMsg(JSON.stringify(helloMsg));
    }
    catch (error) {
      this._onError(error);
    }
  },

  





  onStop: function(aContext, aStatusCode) {
    if (!this._handshakeComplete) {
      this._onError("[" + aStatusCode + "]");
    }
  },

  








  onServerClose: function(aContext, aCode, aReason) {
    if (!this._handshakeComplete) {
      this._onError("[" + aCode + "]" + aReason);
    }
  },

  





  onMessageAvailable: function(aContext, aMsg) {
    let msg = {};
    try {
      msg = JSON.parse(aMsg);
    }
    catch (error) {
      console.error("MozLoopService: error parsing progress message - ", error);
      return;
    }

    if (msg.messageType && msg.messageType === 'hello') {
      this._handshakeComplete = true;
      this._onSuccess();
    }
  },


  




  _send: function(aMsg) {
    if (!this._handshakeComplete) {
      console.warn("MozLoopService::_send error - handshake not complete");
      return;
    }

    try {
      this._websocket.sendMsg(JSON.stringify(aMsg));
    }
    catch (error) {
      this._onError(error);
    }
  },

  



  sendBusy: function() {
    this._send({
      messageType: "action",
      event: "terminate",
      reason: "busy"
    });
  },
};








let MozLoopServiceInternal = {
  callsData: {inUse: false},
  _mocks: {webSocket: undefined},

  
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
    this.notifyStatusChanged();
  },

  notifyStatusChanged: function(aReason = null) {
    Services.obs.notifyObservers(null, "loop-status-changed", aReason);
  },

  




  setError: function(errorType, error) {
    gErrors.set(errorType, error);
    this.notifyStatusChanged();
  },

  clearError: function(errorType) {
    gErrors.delete(errorType);
    this.notifyStatusChanged();
  },

  get errors() {
    return gErrors;
  },

  








  promiseRegisteredWithServers: function(mockPushHandler, mockWebSocket) {
    this._mocks.webSocket = mockWebSocket;

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

  














  hawkRequest: function(sessionType, path, method, payloadObj) {
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

    return gHawkClient.request(path, method, credentials, payloadObj);
  },

  






  _hawkRequestError: function(error) {
    console.error("Loop hawkRequest error:", error);
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
      } else {
        
        console.warn("Loop server sent an invalid session token");
        gRegisteredDeferred.reject("session-token-wrong-size");
        gRegisteredDeferred = null;
        return false;
      }
    }
    return true;
  },


  








  clearSessionToken: function(sessionType) {
    Services.prefs.clearUserPref(this.getSessionTokenPrefName(sessionType));
  },

  





  onPushRegistered: function(err, pushUrl) {
    if (err) {
      gRegisteredDeferred.reject(err);
      gRegisteredDeferred = null;
      return;
    }

    this.registerWithLoopServer(LOOP_SESSION_TYPE.GUEST, pushUrl).then(() => {
      
      if (!gRegisteredDeferred) {
        return;
      }
      gRegisteredDeferred.resolve();
      
      
    }, (error) => {
      console.error("Failed to register with Loop server: ", error);
      gRegisteredDeferred.reject(error.errno);
      gRegisteredDeferred = null;
    });
  },

  







  registerWithLoopServer: function(sessionType, pushUrl, retry = true) {
    return this.hawkRequest(sessionType, "/registration", "POST", { simplePushURL: pushUrl})
      .then((response) => {
        
        
        
        if (!this.storeSessionToken(sessionType, response.headers))
          return;

        this.clearError("registration");
      }, (error) => {
        
        
        if (error.code === 401 && error.errno === INVALID_AUTH_TOKEN) {
          if (this.urlExpiryTimeIsInFuture()) {
            
            Cu.reportError("Loop session token is invalid, all previously "
                           + "generated urls will no longer work.");
          }

          
          this.clearSessionToken(sessionType);
          if (retry) {
            return this.registerWithLoopServer(sessionType, pushUrl, false);
          }
        }

        
        console.error("Failed to register with the loop server. Error: ", error);
        this.setError("registration", error);
        throw error;
      }
    );
  },

  










  unregisterFromLoopServer: function(sessionType, pushURL) {
    let unregisterURL = "/registration?simplePushURL=" + encodeURIComponent(pushURL);
    return this.hawkRequest(sessionType, unregisterURL, "DELETE")
      .then(() => {
        MozLoopServiceInternal.clearSessionToken(sessionType);
      },
      error => {
        
        MozLoopServiceInternal.clearSessionToken(sessionType);
        if (error.code === 401 && error.errno === INVALID_AUTH_TOKEN) {
          
          return;
        }

        console.error("Failed to unregister with the loop server. Error: ", error);
        throw error;
      });
  },

  





  onHandleNotification: function(version) {
    if (this.doNotDisturb) {
      return;
    }

    
    
    
    Services.prefs.setCharPref("loop.seenToS", "seen");

    
    
    
    

    this._getCalls(LOOP_SESSION_TYPE.FXA, version).catch(() => {});
    this._getCalls(LOOP_SESSION_TYPE.GUEST, version).catch(
      error => {this._hawkRequestError(error);});
  },

  










  _getCalls: function(sessionType, version) {
    return this.hawkRequest(sessionType, "/calls?version=" + version, "GET").then(
      response => {this._processCalls(response, sessionType);}
    );
  },

  










  _processCalls: function(response, sessionType) {
    try {
      let respData = JSON.parse(response.body);
      if (respData.calls && Array.isArray(respData.calls)) {
        respData.calls.forEach((callData) => {
          if (!this.callsData.inUse) {
            this.callsData.inUse = true;
            callData.sessionType = sessionType;
            this.callsData.data = callData;
            this.openChatWindow(
              null,
              this.localizedStrings["incoming_call_title2"].textContent,
              "about:loopconversation#incoming/" + callData.callId);
          } else {
            this._returnBusy(callData);
          }
        });
      } else {
        console.warn("Error: missing calls[] in response");
      }
    } catch (err) {
      console.warn("Error parsing calls info", err);
    }
  },

   






  _returnBusy: function(callData) {
    let callProgress = new CallProgressSocket(
      callData.progressURL,
      callData.callId,
      callData.websocketToken);
    callProgress._websocket = this._mocks.webSocket;
    
    
    callProgress.connect(() => {callProgress.sendBusy();});
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

    Chat.open(contentWindow, origin, title, url, undefined, undefined, callback);
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

let gInitializeTimerFunc = () => {
  
  
  
  gInitializeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  gInitializeTimer.initWithCallback(() => {
    MozLoopService.register();
    gInitializeTimer = null;
  },
  MozLoopServiceInternal.initialRegistrationDelayMilliseconds, Ci.nsITimer.TYPE_ONE_SHOT);
};




this.MozLoopService = {
  _DNSService: gDNSService,

  set initializeTimerFunc(value) {
    gInitializeTimerFunc = value;
  },

  



  initialize: function() {

    
    
    Object.freeze(this);

    
    if (!Services.prefs.getBoolPref("loop.enabled") ||
        Services.prefs.getBoolPref("loop.throttled")) {
      return;
    }

    
    if (MozLoopServiceInternal.urlExpiryTimeIsInFuture()) {
      gInitializeTimerFunc();
    }
  },

  










  checkSoftStart(buttonNode, doneCb) {
    if (!Services.prefs.getBoolPref("loop.throttled")) {
      if (typeof(doneCb) == "function") {
        doneCb(new Error("Throttling is not active"));
      }
      return;
    }

    if (Services.io.offline) {
      if (typeof(doneCb) == "function") {
        doneCb(new Error("Cannot check soft-start value: browser is offline"));
      }
      return;
    }

    let ticket = Services.prefs.getIntPref("loop.soft_start_ticket_number");
    if (!ticket || ticket > MAX_SOFT_START_TICKET_NUMBER || ticket < 0) {
      
      
      
      ticket = Math.floor(Math.random() * MAX_SOFT_START_TICKET_NUMBER) + 1;
      
      
      if (ticket > MAX_SOFT_START_TICKET_NUMBER) {
        ticket = MAX_SOFT_START_TICKET_NUMBER;
      }
      Services.prefs.setIntPref("loop.soft_start_ticket_number", ticket);
    }

    let onLookupComplete = (request, record, status) => {
      
      
      
      if (!Components.isSuccessCode(status)) {
        if (typeof(doneCb) == "function") {
          doneCb(new Error("Error in DNS Lookup: " + status));
        }
        return;
      }

      let address = record.getNextAddrAsString().split(".");
      if (address.length != 4) {
        if (typeof(doneCb) == "function") {
          doneCb(new Error("Invalid IP address"));
        }
        return;
      }

      if (address[0] != 127) {
        if (typeof(doneCb) == "function") {
          doneCb(new Error("Throttling IP address is not on localhost subnet"));
        }
        return
      }

      
      
      let now_serving = ((parseInt(address[1]) * 0x10000) +
                         (parseInt(address[2]) * 0x100) +
                         parseInt(address[3]));

      if (now_serving > ticket) {
        
        console.log("MozLoopService: Activating Loop via soft-start");
        Services.prefs.setBoolPref("loop.throttled", false);
        buttonNode.hidden = false;
        this.initialize();
      }
      if (typeof(doneCb) == "function") {
        doneCb(null);
      }
    };

    
    
    
    
    
    
    
    let host = Services.prefs.getCharPref("loop.soft_start_hostname");
    let task = this._DNSService.asyncResolve(host,
                                             this._DNSService.RESOLVE_DISABLE_IPV6,
                                             onLookupComplete,
                                             Services.tm.mainThread);
  },


  








  register: function(mockPushHandler, mockWebSocket) {
    
    if (!Services.prefs.getBoolPref("loop.enabled")) {
      throw new Error("Loop is not enabled");
    }

    if (Services.prefs.getBoolPref("loop.throttled")) {
      throw new Error("Loop is disabled by the soft-start mechanism");
    }

    return MozLoopServiceInternal.promiseRegisteredWithServers(mockPushHandler, mockWebSocket);
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

  get userProfile() {
    return gFxAOAuthProfile;
  },

  get errors() {
    return MozLoopServiceInternal.errors;
  },

  




  get locale() {
    try {
      return Services.prefs.getComplexValue("general.useragent.locale",
        Ci.nsISupportsString).data;
    } catch (ex) {
      return "en-US";
    }
  },

  








  getCallData: function(loopCallId) {
    if (MozLoopServiceInternal.callsData.data &&
        MozLoopServiceInternal.callsData.data.callId == loopCallId) {
      return MozLoopServiceInternal.callsData.data;
    } else {
      return undefined;
    }
  },

  






  releaseCallData: function(loopCallId) {
    if (MozLoopServiceInternal.callsData.data &&
        MozLoopServiceInternal.callsData.data.callId == loopCallId) {
      MozLoopServiceInternal.callsData.data = undefined;
      MozLoopServiceInternal.callsData.inUse = false;
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
    if (gFxAOAuthTokenData) {
      return Promise.resolve(gFxAOAuthTokenData);
    }

    return MozLoopServiceInternal.promiseFxAOAuthAuthorization().then(response => {
      return MozLoopServiceInternal.promiseFxAOAuthToken(response.code, response.state);
    }).then(tokenData => {
      gFxAOAuthTokenData = tokenData;
      return tokenData;
    }).then(tokenData => {
      return gRegisteredDeferred.promise.then(Task.async(function*() {
        if (gPushHandler.pushUrl) {
          yield MozLoopServiceInternal.registerWithLoopServer(LOOP_SESSION_TYPE.FXA, gPushHandler.pushUrl);
        } else {
          throw new Error("No pushUrl for FxA registration");
        }
        return gFxAOAuthTokenData;
      }));
    }).then(tokenData => {
      let client = new FxAccountsProfileClient({
        serverURL: gFxAOAuthClient.parameters.profile_uri,
        token: tokenData.access_token
      });
      client.fetchProfile().then(result => {
        gFxAOAuthProfile = result;
        MozLoopServiceInternal.notifyStatusChanged("login");
      }, error => {
        console.error("Failed to retrieve profile", error);
        gFxAOAuthProfile = null;
        MozLoopServiceInternal.notifyStatusChanged();
      });
      return tokenData;
    }).catch(error => {
      gFxAOAuthTokenData = null;
      gFxAOAuthProfile = null;
      throw error;
    });
  },

  






  logOutFromFxA: Task.async(function*() {
    yield MozLoopServiceInternal.unregisterFromLoopServer(LOOP_SESSION_TYPE.FXA,
                                                          gPushHandler.pushUrl);

    gFxAOAuthTokenData = null;
    gFxAOAuthProfile = null;

    
    
    gFxAOAuthClient = null;
    gFxAOAuthClientPromise = null;

    
    
    MozLoopServiceInternal.clearError("registration");
  }),

  














  hawkRequest: function(sessionType, path, method, payloadObj) {
    return MozLoopServiceInternal.hawkRequest(sessionType, path, method, payloadObj).catch(
      error => {this._hawkRequestError(error);});
  },
};
