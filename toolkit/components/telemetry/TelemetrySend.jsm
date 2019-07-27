










"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetrySend",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Log.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/PromiseUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/TelemetryUtils.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStorage",
                                  "resource://gre/modules/TelemetryStorage.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");

const Utils = TelemetryUtils;

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetrySend::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_UNIFIED = PREF_BRANCH + "unified";
const PREF_TELEMETRY_ENABLED = PREF_BRANCH + "enabled";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";

const TOPIC_IDLE_DAILY = "idle-daily";
const TOPIC_QUIT_APPLICATION = "quit-application";



const IS_UNIFIED_TELEMETRY = Preferences.get(PREF_UNIFIED, false);

const PING_FORMAT_VERSION = 4;


const MIDNIGHT_FUZZING_INTERVAL_MS = 60 * 60 * 1000;

const MIDNIGHT_FUZZING_DELAY_MS = Math.random() * MIDNIGHT_FUZZING_INTERVAL_MS;


const PING_SUBMIT_TIMEOUT_MS = 2 * 60 * 1000;



const MAX_PING_FILE_AGE = 14 * 24 * 60 * 60 * 1000; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * 60 * 1000; 


const MAX_LRU_PINGS = 50;






let Policy = {
  now: () => new Date(),
  midnightPingFuzzingDelay: () => MIDNIGHT_FUZZING_DELAY_MS,
  setPingSendTimeout: (callback, delayMs) => setTimeout(callback, delayMs),
  clearPingSendTimeout: (id) => clearTimeout(id),
};




function isV4PingFormat(aPing) {
  return ("id" in aPing) && ("application" in aPing) &&
         ("version" in aPing) && (aPing.version >= 2);
}

function tomorrow(date) {
  let d = new Date(date);
  d.setDate(d.getDate() + 1);
  return d;
}




function gzipCompressString(string) {
  let observer = {
    buffer: "",
    onStreamComplete: function(loader, context, status, length, result) {
      this.buffer = String.fromCharCode.apply(this, result);
    }
  };

  let scs = Cc["@mozilla.org/streamConverters;1"]
            .getService(Ci.nsIStreamConverterService);
  let listener = Cc["@mozilla.org/network/stream-loader;1"]
                .createInstance(Ci.nsIStreamLoader);
  listener.init(observer);
  let converter = scs.asyncConvertData("uncompressed", "gzip",
                                       listener, null);
  let stringStream = Cc["@mozilla.org/io/string-input-stream;1"]
                     .createInstance(Ci.nsIStringInputStream);
  stringStream.data = string;
  converter.onStartRequest(null, null);
  converter.onDataAvailable(null, null, stringStream, 0, string.length);
  converter.onStopRequest(null, null, null);
  return observer.buffer;
}

this.TelemetrySend = {
  


  get MAX_PING_FILE_AGE() {
    return MAX_PING_FILE_AGE;
  },

  


  get OVERDUE_PING_FILE_AGE() {
    return OVERDUE_PING_FILE_AGE;
  },

  


  get MAX_LRU_PINGS() {
    return MAX_LRU_PINGS;
  },

  






  setup: function(testing = false) {
    return TelemetrySendImpl.setup(testing);
  },

  





  shutdown: function() {
    return TelemetrySendImpl.shutdown();
  },

  







  submitPing: function(ping) {
    return TelemetrySendImpl.submitPing(ping);
  },

  


  get discardedPingsCount() {
    return TelemetrySendImpl.discardedPingsCount;
  },

  


  get overduePingsCount() {
    return TelemetrySendImpl.overduePingsCount;
  },

  


  reset: function() {
    return TelemetrySendImpl.reset();
  },

  


  setServer: function(server) {
    return TelemetrySendImpl.setServer(server);
  },
};

let TelemetrySendImpl = {
  _sendingEnabled: false,
  _logger: null,
  
  _pingSendTimer: null,
  
  _pendingPingRequests: new Map(),
  
  _connectionsBarrier: new AsyncShutdown.Barrier("TelemetrySend: Waiting for pending ping activity"),
  
  _testMode: false,

  
  _discardedPingsCount: 0,
  
  _evictedPingsCount: 0,
  
  _overduePingCount: 0,

  OBSERVER_TOPICS: [
    TOPIC_IDLE_DAILY,
  ],

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
  },

  get discardedPingsCount() {
    return this._discardedPingsCount;
  },

  get overduePingsCount() {
    return this._overduePingCount;
  },

  setup: Task.async(function*(testing) {
    this._log.trace("setup");

    this._testMode = testing;
    this._sendingEnabled = true;

    this._discardedPingsCount = 0;
    this._evictedPingsCount = 0;

    Services.obs.addObserver(this, TOPIC_IDLE_DAILY, false);

    this._server = Preferences.get(PREF_SERVER, undefined);

    
    
    this._sendPersistedPings();

    
    let haveOverduePings = yield this._checkPendingPings();
    if (haveOverduePings) {
      
      this._sendPersistedPings();
    }
  }),

  



  _checkPendingPings: Task.async(function*() {
    
    let infos = yield TelemetryStorage.loadPendingPingList();
    this._log.info("_checkPendingPings - pending ping count: " + infos.length);
    if (infos.length == 0) {
      this._log.trace("_checkPendingPings - no pending pings");
      return;
    }

    
    const now = new Date();
    const tooOld = (info) => (now.getTime() - info.lastModificationDate) > MAX_PING_FILE_AGE;

    const oldPings = infos.filter((info) => tooOld(info));
    infos = infos.filter((info) => !tooOld(info));
    this._log.info("_checkPendingPings - clearing out " + oldPings.length + " old pings");

    for (let info of oldPings) {
      try {
        yield TelemetryStorage.removePendingPing(info.id);
        ++this._discardedPingsCount;
      } catch(ex) {
        this._log.error("_checkPendingPings - failed to remove old ping", ex);
      }
    }

    
    const shouldEvict = infos.splice(MAX_LRU_PINGS, infos.length);
    let evictedCount = 0;
    this._log.info("_checkPendingPings - evicting " + shouldEvict.length + " pings to " +
                   "avoid overgrowing the backlog");

    for (let info of shouldEvict) {
      try {
        yield TelemetryStorage.removePendingPing(info.id);
        ++this._evictedPingsCount;
      } catch(ex) {
        this._log.error("_checkPendingPings - failed to evict ping", ex);
      }
    }

    Services.telemetry.getHistogramById('TELEMETRY_FILES_EVICTED')
                      .add(evictedCount);

    
    const overduePings = infos.filter((info) =>
      (now.getTime() - info.lastModificationDate) > OVERDUE_PING_FILE_AGE);
    this._overduePingCount = overduePings.length;

    if (overduePings.length > 0) {
      this._log.trace("_checkForOverduePings - Have " + overduePings.length +
                       " overdue pending pings, ready to send " + infos.length +
                       " pings now.");
      return true;
    }

    return false;
   }),

  shutdown: Task.async(function*() {
    for (let topic of this.OBSERVER_TOPICS) {
      Services.obs.removeObserver(this, topic);
    }

    
    this._sendingEnabled = false;

    
    this._clearPingSendTimer();
    
    yield this._cancelOutgoingRequests();
    
    yield this._connectionsBarrier.wait();
  }),

  reset: function() {
    this._log.trace("reset");

    this._overduePingCount = 0;
    this._discardedPingsCount = 0;
    this._evictedPingsCount = 0;

    const histograms = [
      "TELEMETRY_SUCCESS",
      "TELEMETRY_FILES_EVICTED",
      "TELEMETRY_SEND",
      "TELEMETRY_PING",
    ];

    histograms.forEach(h => Telemetry.getHistogramById(h).clear());
  },

  observe: function(subject, topic, data) {
    switch(topic) {
    case TOPIC_IDLE_DAILY:
      this._sendPersistedPings();
      break;
    }
  },

  submitPing: function(ping) {
    if (!this._canSend()) {
      this._log.trace("submitPing - Telemetry is not allowed to send pings.");
      return Promise.resolve();
    }

    
    const now = Policy.now();
    const nextPingSendTime = this._getNextPingSendTime(now);
    const throttled = (nextPingSendTime > now.getTime());

    
    if (throttled) {
      this._log.trace("submitPing - throttled, delaying ping send to " + new Date(nextPingSendTime));
      this._reschedulePingSendTimer(nextPingSendTime);
    }

    if (!this._sendingEnabled || throttled) {
      
      this._log.trace("submitPing - ping is pending, sendingEnabled: " + this._sendingEnabled +
                      ", throttled: " + throttled);
      return TelemetryStorage.savePendingPing(ping);
    }

    
    this._log.trace("submitPing - already initialized, ping will be sent");
    let ps = [];
    ps.push(this._doPing(ping, ping.id, false)
                .catch((ex) => {
                  this._log.info("submitPing - ping not sent, saving to disk", ex);
                  TelemetryStorage.savePendingPing(ping);
                }));
    ps.push(this._sendPersistedPings());

    return Promise.all([for (p of ps) p.catch((ex) => {
      this._log.error("submitPing - ping activity had an error", ex);
    })]);
  },

  


  setServer: function (server) {
    this._log.trace("setServer", server);
    this._server = server;
  },

  _cancelOutgoingRequests: function() {
    
    for (let [url, request] of this._pendingPingRequests) {
      this._log.trace("_cancelOutgoingRequests - aborting ping request for " + url);
      try {
        request.abort();
      } catch (e) {
        this._log.error("_cancelOutgoingRequests - failed to abort request to " + url, e);
      }
    }
    this._pendingPingRequests.clear();
  },

  







  _getNextPingSendTime: function(now) {
    
    
    
    
    

    const midnight = Utils.truncateToDays(now);
    
    if ((now.getTime() - midnight.getTime()) > MIDNIGHT_FUZZING_INTERVAL_MS) {
      return now.getTime();
    }

    
    
    return midnight.getTime() + Policy.midnightPingFuzzingDelay();
  },

  




  _sendPersistedPings: Task.async(function*() {
    this._log.trace("_sendPersistedPings - Can send: " + this._canSend());

    if (TelemetryStorage.pendingPingCount < 1) {
      this._log.trace("_sendPersistedPings - no pings to send");
      return Promise.resolve();
    }

    if (!this._canSend()) {
      this._log.trace("_sendPersistedPings - Telemetry is not allowed to send pings.");
      return Promise.resolve();
    }

    
    const now = Policy.now();
    const nextPingSendTime = this._getNextPingSendTime(now);
    if (nextPingSendTime > now.getTime()) {
      this._log.trace("_sendPersistedPings - delaying ping send to " + new Date(nextPingSendTime));
      this._reschedulePingSendTimer(nextPingSendTime);
      return Promise.resolve();
    }

    
    const pendingPings = TelemetryStorage.getPendingPingList();
    this._log.trace("_sendPersistedPings - sending " + pendingPings.length + " pings");
    let pingSendPromises = [];
    for (let ping of pendingPings) {
      let p = ping;
      pingSendPromises.push(
        TelemetryStorage.loadPendingPing(p.id)
          .then((data) => this._doPing(data, p.id, true)
          .catch(e => this._log.error("_sendPersistedPings - _doPing rejected", e))));
    }

    let promise = Promise.all(pingSendPromises);
    this._trackPendingPingTask(promise);
    yield promise;
  }),

  _onPingRequestFinished: function(success, startTime, id, isPersisted) {
    this._log.trace("_onPingRequestFinished - success: " + success + ", persisted: " + isPersisted);

    Telemetry.getHistogramById("TELEMETRY_SEND").add(new Date() - startTime);
    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success && isPersisted) {
      return TelemetryStorage.removePendingPing(id);
    } else {
      return Promise.resolve();
    }
  },

  _getSubmissionPath: function(ping) {
    
    let pathComponents;
    if (isV4PingFormat(ping)) {
      
      
      let app = ping.application;
      pathComponents = [
        ping.id, ping.type, app.name, app.version, app.channel, app.buildId
      ];
    } else {
      
      if (!("slug" in ping)) {
        
        ping.slug = Utils.generateUUID();
      }

      
      let payload = ("payload" in ping) ? ping.payload : null;
      if (payload && ("info" in payload)) {
        let info = ping.payload.info;
        pathComponents = [ ping.slug, info.reason, info.appName, info.appVersion,
                           info.appUpdateChannel, info.appBuildID ];
      } else {
        
        pathComponents = [ ping.slug ];
      }
    }

    let slug = pathComponents.join("/");
    return "/submit/telemetry/" + slug;
  },

  _doPing: function(ping, id, isPersisted) {
    if (!this._canSend()) {
      
      this._log.trace("_doPing - Sending is disabled.");
      return Promise.resolve();
    }

    this._log.trace("_doPing - server: " + this._server + ", persisted: " + isPersisted +
                    ", id: " + id);
    const isNewPing = isV4PingFormat(ping);
    const version = isNewPing ? PING_FORMAT_VERSION : 1;
    const url = this._server + this._getSubmissionPath(ping) + "?v=" + version;

    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.timeout = PING_SUBMIT_TIMEOUT_MS;

    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json; charset=UTF-8");

    this._pendingPingRequests.set(url, request);

    let startTime = new Date();
    let deferred = PromiseUtils.defer();

    let onRequestFinished = (success, event) => {
      let onCompletion = () => {
        if (success) {
          deferred.resolve();
        } else {
          deferred.reject(event);
        }
      };

      this._pendingPingRequests.delete(url);
      this._onPingRequestFinished(success, startTime, id, isPersisted)
        .then(() => onCompletion(),
              (error) => {
                this._log.error("_doPing - request success: " + success + ", error" + error);
                onCompletion();
              });
    };

    let errorhandler = (event) => {
      this._log.error("_doPing - error making request to " + url + ": " + event.type);
      onRequestFinished(false, event);
    };
    request.onerror = errorhandler;
    request.ontimeout = errorhandler;
    request.onabort = errorhandler;

    request.onload = (event) => {
      let status = request.status;
      let statusClass = status - (status % 100);
      let success = false;

      if (statusClass === 200) {
        
        this._log.info("_doPing - successfully loaded, status: " + status);
        success = true;
      } else if (statusClass === 400) {
        
        this._log.error("_doPing - error submitting to " + url + ", status: " + status
                        + " - ping request broken?");
        
        
        success = true;
      } else if (statusClass === 500) {
        
        this._log.error("_doPing - error submitting to " + url + ", status: " + status
                        + " - server error, should retry later");
      } else {
        
        this._log.error("_doPing - error submitting to " + url + ", status: " + status
                        + ", type: " + event.type);
      }

      onRequestFinished(success, event);
    };

    
    let networkPayload = isNewPing ? ping : ping.payload;
    request.setRequestHeader("Content-Encoding", "gzip");
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    startTime = new Date();
    let utf8Payload = converter.ConvertFromUnicode(JSON.stringify(networkPayload));
    utf8Payload += converter.Finish();
    Telemetry.getHistogramById("TELEMETRY_STRINGIFY").add(new Date() - startTime);
    let payloadStream = Cc["@mozilla.org/io/string-input-stream;1"]
                        .createInstance(Ci.nsIStringInputStream);
    startTime = new Date();
    payloadStream.data = gzipCompressString(utf8Payload);
    Telemetry.getHistogramById("TELEMETRY_COMPRESS").add(new Date() - startTime);
    startTime = new Date();
    request.send(payloadStream);

    return deferred.promise;
  },

  






  _canSend: function() {
    
    if (!Telemetry.isOfficialTelemetry && !this._testMode) {
      return false;
    }

    
    
    if (IS_UNIFIED_TELEMETRY) {
      return Preferences.get(PREF_FHR_UPLOAD_ENABLED, false);
    }

    
    return Preferences.get(PREF_TELEMETRY_ENABLED, false);
  },

  _reschedulePingSendTimer: function(timestamp) {
    this._clearPingSendTimer();
    const interval = timestamp - Policy.now();
    this._pingSendTimer = Policy.setPingSendTimeout(() => this._sendPersistedPings(), interval);
  },

  _clearPingSendTimer: function() {
    if (this._pingSendTimer) {
      Policy.clearPingSendTimeout(this._pingSendTimer);
      this._pingSendTimer = null;
    }
  },

  



  _trackPendingPingTask: function (promise) {
    this._connectionsBarrier.client.addBlocker("Waiting for ping task", promise);
  },
};
