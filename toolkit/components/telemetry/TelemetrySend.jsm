










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

const MS_IN_A_MINUTE = 60 * 1000;

const PING_TYPE_DELETION = "deletion";


const MIDNIGHT_FUZZING_INTERVAL_MS = 60 * MS_IN_A_MINUTE;

const MIDNIGHT_FUZZING_DELAY_MS = Math.random() * MIDNIGHT_FUZZING_INTERVAL_MS;


const PING_SUBMIT_TIMEOUT_MS = 1.5 * MS_IN_A_MINUTE;



const MAX_PING_SENDS_PER_MINUTE = 10;



const SEND_TICK_DELAY = 1 * MS_IN_A_MINUTE;




const SEND_MAXIMUM_BACKOFF_DELAY_MS = 120 * MS_IN_A_MINUTE;



const MAX_PING_FILE_AGE = 14 * 24 * 60 * MS_IN_A_MINUTE; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * MS_IN_A_MINUTE; 


const MAX_LRU_PINGS = 50;






let Policy = {
  now: () => new Date(),
  midnightPingFuzzingDelay: () => MIDNIGHT_FUZZING_DELAY_MS,
  setSchedulerTickTimeout: (callback, delayMs) => setTimeout(callback, delayMs),
  clearSchedulerTickTimeout: (id) => clearTimeout(id),
};




function isV4PingFormat(aPing) {
  return ("id" in aPing) && ("application" in aPing) &&
         ("version" in aPing) && (aPing.version >= 2);
}






function isDeletionPing(aPing) {
  return isV4PingFormat(aPing) && (aPing.type == PING_TYPE_DELETION);
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

  get pendingPingCount() {
    return TelemetrySendImpl.pendingPingCount;
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

  


  testWaitOnOutgoingPings: function() {
    return TelemetrySendImpl.promisePendingPingActivity();
  },
};

let CancellableTimeout = {
  _deferred: null,
  _timer: null,

  






  promiseWaitOnTimeout: function(timeoutMs) {
    if (!this._deferred) {
      this._deferred = PromiseUtils.defer();
      this._timer = Policy.setSchedulerTickTimeout(() => this._onTimeout(), timeoutMs);
    }

    return this._deferred.promise;
  },

  _onTimeout: function() {
    if (this._deferred) {
      this._deferred.resolve(false);
      this._timer = null;
      this._deferred = null;
    }
  },

  cancelTimeout: function() {
    if (this._deferred) {
      Policy.clearSchedulerTickTimeout(this._timer);
      this._deferred.resolve(true);
      this._timer = null;
      this._deferred = null;
    }
  },
};




let SendScheduler = {
  
  
  _sendsFailed: false,
  
  
  _backoffDelay: SEND_TICK_DELAY,
  _shutdown: false,
  _sendTask: null,

  _logger: null,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX + "Scheduler::");
    }

    return this._logger;
  },

  shutdown: function() {
    this._shutdown = true;
    CancellableTimeout.cancelTimeout();
    return this._sendTask;
  },

  


  reset: function() {
    CancellableTimeout.cancelTimeout();
    this._sendsFailed = false;
    this._backoffDelay = SEND_TICK_DELAY;
    this._shutdown = false;

    return this._sendTask;
  },

  



  notifySendsFailed: function() {
    this._log.trace("notifySendsFailed");
    if (this._sendsFailed) {
      return;
    }

    this._sendsFailed = true;
    this._log.trace("notifySendsFailed - had send failures");
  },

  


  isThrottled: function() {
    const now = Policy.now();
    const nextPingSendTime = this._getNextPingSendTime(now);
    return (nextPingSendTime > now.getTime());
  },

  triggerSendingPings: function(immediately) {
    this._log.trace("triggerSendingPings - active send task: " + !!this._sendTask + ", immediately: " + immediately);

    if (!this._sendTask) {
      this._sendTask = this._doSendTask();
      let clear = () => this._sendTask = null;
      this._sendTask.then(clear, clear);
    } else if (immediately) {
      CancellableTimeout.cancelTimeout();
    }

    return this._sendTask;
  },

  _doSendTask: Task.async(function*() {
    this._backoffDelay = SEND_TICK_DELAY;
    this._sendsFailed = false;

    const resetBackoffTimer = () => {
      this._backoffDelay = SEND_TICK_DELAY;
    };

    for (;;) {
      this._log.trace("_doSendTask iteration");

      if (this._shutdown) {
        this._log.trace("_doSendTask - shutting down, bailing out");
        return;
      }

      
      
      
      let pending = TelemetryStorage.getPendingPingList();
      let current = TelemetrySendImpl.getUnpersistedPings();
      this._log.trace("_doSendTask - pending: " + pending.length + ", current: " + current.length);
      pending = pending.filter(p => TelemetrySendImpl.canSend(p));
      current = current.filter(p => TelemetrySendImpl.canSend(p));
      this._log.trace("_doSendTask - can send - pending: " + pending.length + ", current: " + current.length);

      
      if ((pending.length == 0) && (current.length == 0)) {
        this._log.trace("_doSendTask - no pending pings, bailing out");
        return;
      }

      
      const now = Policy.now();
      if (this.isThrottled()) {
        const nextPingSendTime = this._getNextPingSendTime(now);
        this._log.trace("_doSendTask - throttled, delaying ping send to " + new Date(nextPingSendTime));
        const delay = nextPingSendTime - now.getTime();
        const cancelled = yield CancellableTimeout.promiseWaitOnTimeout(delay);
        if (cancelled) {
          this._log.trace("_doSendTask - throttling wait was cancelled, resetting backoff timer");
          resetBackoffTimer();
        }

        continue;
      }

      let sending = pending.slice(0, MAX_PING_SENDS_PER_MINUTE);
      pending = pending.slice(MAX_PING_SENDS_PER_MINUTE);
      this._log.trace("_doSendTask - triggering sending of " + sending.length + " pings now" +
                      ", " + pending.length + " pings waiting");

      this._sendsFailed = false;
      const sendStartTime = Policy.now();
      yield TelemetrySendImpl.sendPings(current, [for (p of sending) p.id]);
      if (this._shutdown || (TelemetrySend.pendingPingCount == 0)) {
        this._log.trace("_doSendTask - bailing out after sending, shutdown: " + this._shutdown +
                        ", pendingPingCount: " + TelemetrySend.pendingPingCount);
        return;
      }

      
      
      
      
      const timeSinceLastSend = Policy.now() - sendStartTime;
      let nextSendDelay = Math.max(0, SEND_TICK_DELAY - timeSinceLastSend);

      if (!this._sendsFailed) {
        this._log.trace("_doSendTask - had no send failures, resetting backoff timer");
        resetBackoffTimer();
      } else {
        const newDelay = Math.min(SEND_MAXIMUM_BACKOFF_DELAY_MS,
                                  this._backoffDelay * 2);
        this._log.trace("_doSendTask - had send failures, backing off -" +
                        " old timeout: " + this._backoffDelay +
                        ", new timeout: " + newDelay);
        this._backoffDelay = newDelay;
        nextSendDelay = this._backoffDelay;
      }

      this._log.trace("_doSendTask - waiting for next send opportunity, timeout is " + nextSendDelay)
      const cancelled = yield CancellableTimeout.promiseWaitOnTimeout(nextSendDelay);
      if (cancelled) {
        this._log.trace("_doSendTask - batch send wait was cancelled, resetting backoff timer");
        resetBackoffTimer();
      }
    }
  }),

  







  _getNextPingSendTime: function(now) {
    
    
    
    
    

    const midnight = Utils.truncateToDays(now);
    
    if ((now.getTime() - midnight.getTime()) > MIDNIGHT_FUZZING_INTERVAL_MS) {
      return now.getTime();
    }

    
    
    return midnight.getTime() + Policy.midnightPingFuzzingDelay();
  },
 };

let TelemetrySendImpl = {
  _sendingEnabled: false,
  _logger: null,
  
  _pingSendTimer: null,
  
  _pendingPingRequests: new Map(),
  
  _pendingPingActivity: new Set(),
  
  _testMode: false,
  
  _currentPings: new Map(),

  
  _discardedPingsCount: 0,
  
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

  get pendingPingRequests() {
    return this._pendingPingRequests;
  },

  get pendingPingCount() {
    return TelemetryStorage.getPendingPingList().length + this._currentPings.size;
  },

  setup: Task.async(function*(testing) {
    this._log.trace("setup");

    this._testMode = testing;
    this._sendingEnabled = true;

    this._discardedPingsCount = 0;

    Services.obs.addObserver(this, TOPIC_IDLE_DAILY, false);

    this._server = Preferences.get(PREF_SERVER, undefined);

    
    try {
      yield this._checkPendingPings();
    } catch (ex) {
      this._log.error("setup - _checkPendingPings rejected", ex);
    }

    
    SendScheduler.triggerSendingPings(true);
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
        ++evictedCount;
      } catch(ex) {
        this._log.error("_checkPendingPings - failed to evict ping", ex);
      }
    }

    Services.telemetry.getHistogramById('TELEMETRY_FILES_EVICTED')
                      .add(evictedCount);

    
    const overduePings = infos.filter((info) =>
      (now.getTime() - info.lastModificationDate) > OVERDUE_PING_FILE_AGE);
    this._overduePingCount = overduePings.length;
   }),

  shutdown: Task.async(function*() {
    for (let topic of this.OBSERVER_TOPICS) {
      try {
        Services.obs.removeObserver(this, topic);
      } catch (ex) {
        this._log.error("shutdown - failed to remove observer for " + topic, ex);
      }
    }

    
    this._sendingEnabled = false;

    
    yield this._cancelOutgoingRequests();

    yield SendScheduler.shutdown();
    yield this._persistCurrentPings();

    
    yield this.promisePendingPingActivity();
  }),

  reset: function() {
    this._log.trace("reset");

    this._overduePingCount = 0;
    this._discardedPingsCount = 0;

    const histograms = [
      "TELEMETRY_SUCCESS",
      "TELEMETRY_FILES_EVICTED",
      "TELEMETRY_SEND",
      "TELEMETRY_PING",
    ];

    histograms.forEach(h => Telemetry.getHistogramById(h).clear());

    return SendScheduler.reset();
  },

  observe: function(subject, topic, data) {
    switch(topic) {
    case TOPIC_IDLE_DAILY:
      SendScheduler.triggerSendingPings(true);
      break;
    }
  },

  submitPing: function(ping) {
    if (!this.canSend(ping)) {
      this._log.trace("submitPing - Telemetry is not allowed to send pings.");
      return Promise.resolve();
    }

    if (!this._sendingEnabled) {
      
      this._log.trace("submitPing - can't send ping now, persisting to disk - " +
                      "sendingEnabled: " + this._sendingEnabled);
      return TelemetryStorage.savePendingPing(ping);
    }

    
    
    this._log.trace("submitPing - can send pings, trying to send now");
    this._currentPings.set(ping.id, ping);
    SendScheduler.triggerSendingPings(true);
    return this.promisePendingPingActivity();
  },

  


  setServer: function (server) {
    this._log.trace("setServer", server);
    this._server = server;
  },

  _cancelOutgoingRequests: function() {
    
    for (let [id, request] of this._pendingPingRequests) {
      this._log.trace("_cancelOutgoingRequests - aborting ping request for id " + id);
      try {
        request.abort();
      } catch (e) {
        this._log.error("_cancelOutgoingRequests - failed to abort request for id " + id, e);
      }
    }
    this._pendingPingRequests.clear();
  },

  sendPings: function(currentPings, persistedPingIds) {
    let pingSends = [];

    for (let current of currentPings) {
      let ping = current;
      let p = this._doPing(ping, ping.id, false)
        .then(() => this._currentPings.delete(ping.id))
        .catch((ex) => {
          this._log.info("sendPings - ping " + ping.id + " not sent, saving to disk", ex);
          this._currentPings.delete(ping.id)
          return TelemetryStorage.savePendingPing(ping);
        });
      this._trackPendingPingTask(p);
      pingSends.push(p);
    }

    if (persistedPingIds.length > 0) {
      pingSends.push(this._sendPersistedPings(persistedPingIds).catch((ex) => {
        this._log.info("sendPings - persisted pings not sent", ex);
      }));
    }

    return Promise.all(pingSends);
  },

  






  _sendPersistedPings: Task.async(function*(pingIds) {
    this._log.trace("sendPersistedPings");

    if (TelemetryStorage.pendingPingCount < 1) {
      this._log.trace("_sendPersistedPings - no pings to send");
      return;
    }

    if (pingIds.length < 1) {
      this._log.trace("sendPersistedPings - no pings to send");
      return;
    }

    
    
    this._log.trace("sendPersistedPings - sending " + pingIds.length + " pings");
    let pingSendPromises = [];
    for (let pingId of pingIds) {
      const id = pingId;
      pingSendPromises.push(
        TelemetryStorage.loadPendingPing(id)
          .then((data) => this._doPing(data, id, true))
          .catch(e => this._log.error("sendPersistedPings - failed to send ping " + id, e)));
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

    if (!success) {
      
      SendScheduler.notifySendsFailed();
    }

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
    if (!this.canSend(ping)) {
      
      this._log.trace("_doPing - Can't send ping " + ping.id);
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

    this._pendingPingRequests.set(id, request);

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

      this._pendingPingRequests.delete(id);
      this._onPingRequestFinished(success, startTime, id, isPersisted)
        .then(() => onCompletion(),
              (error) => {
                this._log.error("_doPing - request success: " + success + ", error: " + error);
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

  








  canSend: function(ping = null) {
    
    if (!Telemetry.isOfficialTelemetry && !this._testMode) {
      return false;
    }

    
    
    if (IS_UNIFIED_TELEMETRY) {
      
      if (ping && isDeletionPing(ping)) {
        return true;
      }
      return Preferences.get(PREF_FHR_UPLOAD_ENABLED, false);
    }

    
    return Preferences.get(PREF_TELEMETRY_ENABLED, false);
  },

  



  _trackPendingPingTask: function (promise) {
    let clear = () => this._pendingPingActivity.delete(promise);
    promise.then(clear, clear);
    this._pendingPingActivity.add(promise);
  },

  




  promisePendingPingActivity: function () {
    this._log.trace("promisePendingPingActivity - Waiting for ping task");
    return Promise.all([for (p of this._pendingPingActivity) p.catch(ex => {
      this._log.error("promisePendingPingActivity - ping activity had an error", ex);
    })]);
  },

  _persistCurrentPings: Task.async(function*() {
    for (let [id, ping] of this._currentPings) {
      try {
        yield TelemetryStorage.savePendingPing(ping);
        this._log.trace("_persistCurrentPings - saved ping " + id);
      } catch (ex) {
        this._log.error("_persistCurrentPings - failed to save ping " + id, ex);
      }
    }
  }),

  


  getUnpersistedPings: function() {
    let current = [...this._currentPings.values()];
    current.reverse();
    return current;
  },
};
