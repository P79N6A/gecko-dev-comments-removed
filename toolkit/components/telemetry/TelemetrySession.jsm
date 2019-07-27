




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/DeferredTask.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

const myScope = this;

const IS_CONTENT_PROCESS = (function() {
  
  
  let runtime = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
  return runtime.processType == Ci.nsIXULRuntime.PROCESS_TYPE_CONTENT;
})();


const PAYLOAD_VERSION = 4;
const PING_TYPE_MAIN = "main";
const PING_TYPE_SAVED_SESSION = "saved-session";
const RETENTION_DAYS = 14;

const REASON_ABORTED_SESSION = "aborted-session";
const REASON_DAILY = "daily";
const REASON_SAVED_SESSION = "saved-session";
const REASON_GATHER_PAYLOAD = "gather-payload";
const REASON_TEST_PING = "test-ping";
const REASON_ENVIRONMENT_CHANGE = "environment-change";
const REASON_SHUTDOWN = "shutdown";

const ENVIRONMENT_CHANGE_LISTENER = "TelemetrySession::onEnvironmentChange";

const MS_IN_ONE_HOUR  = 60 * 60 * 1000;
const MIN_SUBSESSION_LENGTH_MS = 10 * 60 * 1000;



#expand const HISTOGRAMS_FILE_VERSION = "__HISTOGRAMS_FILE_VERSION__";

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetrySession::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_PREVIOUS_BUILDID = PREF_BRANCH + "previousBuildID";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_ASYNC_PLUGIN_INIT = "dom.ipc.plugins.asyncInit";

const MESSAGE_TELEMETRY_PAYLOAD = "Telemetry:Payload";
const MESSAGE_TELEMETRY_GET_CHILD_PAYLOAD = "Telemetry:GetChildPayload";

const DATAREPORTING_DIRECTORY = "datareporting";
const ABORTED_SESSION_FILE_NAME = "aborted-session-ping";

const SESSION_STATE_FILE_NAME = "session-state.json";


const MAX_NUM_CONTENT_PAYLOADS = 10;


const TELEMETRY_INTERVAL = 60000;

const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;

const SCHEDULER_TICK_INTERVAL_MS = 5 * 60 * 1000;

const SCHEDULER_TICK_IDLE_INTERVAL_MS = 60 * 60 * 1000;

const SCHEDULER_RETRY_ATTEMPTS = 3;


const SCHEDULER_MIDNIGHT_TOLERANCE_MS = 15 * 60 * 1000;



const SCHEDULER_COALESCE_THRESHOLD_MS = 2 * 60 * 1000;




const IDLE_TIMEOUT_SECONDS = 5 * 60;



const ABORTED_SESSION_UPDATE_INTERVAL_MS = 5 * 60 * 1000;

var gLastMemoryPoll = null;

let gWasDebuggerAttached = false;

function getLocale() {
  return Cc["@mozilla.org/chrome/chrome-registry;1"].
         getService(Ci.nsIXULChromeRegistry).
         getSelectedLocale('global');
}

XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");
XPCOMUtils.defineLazyServiceGetter(this, "idleService",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");
XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");
XPCOMUtils.defineLazyServiceGetter(this, "cpml",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageListenerManager");
XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");
XPCOMUtils.defineLazyServiceGetter(this, "ppml",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManagerPrivate",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryPing",
                                  "resource://gre/modules/TelemetryPing.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStorage",
                                  "resource://gre/modules/TelemetryStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ThirdPartyCookieProbe",
                                  "resource://gre/modules/ThirdPartyCookieProbe.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
                                  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryEnvironment",
                                  "resource://gre/modules/TelemetryEnvironment.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}




let Policy = {
  now: () => new Date(),
  generateSessionUUID: () => generateUUID(),
  generateSubsessionUUID: () => generateUUID(),
  setSchedulerTickTimeout: (callback, delayMs) => setTimeout(callback, delayMs),
  clearSchedulerTickTimeout: id => clearTimeout(id),
};




function truncateToDays(date) {
  return new Date(date.getFullYear(),
                  date.getMonth(),
                  date.getDate(),
                  0, 0, 0, 0);
}









function areTimesClose(t1, t2, tolerance) {
  return Math.abs(t1 - t2) <= tolerance;
}






function getNextMidnight(date) {
  let nextMidnight = new Date(truncateToDays(date));
  nextMidnight.setDate(nextMidnight.getDate() + 1);
  return nextMidnight;
}







function getNearestMidnight(date) {
  let lastMidnight = truncateToDays(date);
  if (areTimesClose(date.getTime(), lastMidnight.getTime(), SCHEDULER_MIDNIGHT_TOLERANCE_MS)) {
    return lastMidnight;
  }

  const nextMidnightDate = getNextMidnight(date);
  if (areTimesClose(date.getTime(), nextMidnightDate.getTime(), SCHEDULER_MIDNIGHT_TOLERANCE_MS)) {
    return nextMidnightDate;
  }
  return null;
}






function getPingType(aPayload) {
  
  
  if (aPayload.info.reason == REASON_SAVED_SESSION) {
    return PING_TYPE_SAVED_SESSION;
  }

  return PING_TYPE_MAIN;
}





function toLocalTimeISOString(date) {
  function padNumber(number, places) {
    number = number.toString();
    while (number.length < places) {
      number = "0" + number;
    }
    return number;
  }

  let sign = (n) => n >= 0 ? "+" : "-";
  
  let tzOffset = - date.getTimezoneOffset();

  
  return    padNumber(date.getFullYear(), 4)
    + "-" + padNumber(date.getMonth() + 1, 2)
    + "-" + padNumber(date.getDate(), 2)
    + "T" + padNumber(date.getHours(), 2)
    + ":" + padNumber(date.getMinutes(), 2)
    + ":" + padNumber(date.getSeconds(), 2)
    + "." + date.getMilliseconds()
    + sign(tzOffset) + padNumber(Math.floor(Math.abs(tzOffset / 60)), 2)
    + ":" + padNumber(Math.abs(tzOffset % 60), 2);
}




let processInfo = {
  _initialized: false,
  _IO_COUNTERS: null,
  _kernel32: null,
  _GetProcessIoCounters: null,
  _GetCurrentProcess: null,
  getCounters: function() {
    let isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
    if (isWindows)
      return this.getCounters_Windows();
    return null;
  },
  getCounters_Windows: function() {
    if (!this._initialized){
      Cu.import("resource://gre/modules/ctypes.jsm");
      this._IO_COUNTERS = new ctypes.StructType("IO_COUNTERS", [
        {'readOps': ctypes.unsigned_long_long},
        {'writeOps': ctypes.unsigned_long_long},
        {'otherOps': ctypes.unsigned_long_long},
        {'readBytes': ctypes.unsigned_long_long},
        {'writeBytes': ctypes.unsigned_long_long},
        {'otherBytes': ctypes.unsigned_long_long} ]);
      try {
        this._kernel32 = ctypes.open("Kernel32.dll");
        this._GetProcessIoCounters = this._kernel32.declare("GetProcessIoCounters",
          ctypes.winapi_abi,
          ctypes.bool, 
          ctypes.voidptr_t, 
          this._IO_COUNTERS.ptr); 
        this._GetCurrentProcess = this._kernel32.declare("GetCurrentProcess",
          ctypes.winapi_abi,
          ctypes.voidptr_t); 
        this._initialized = true;
      } catch (err) {
        return null;
      }
    }
    let io = new this._IO_COUNTERS();
    if(!this._GetProcessIoCounters(this._GetCurrentProcess(), io.address()))
      return null;
    return [parseInt(io.readBytes), parseInt(io.writeBytes)];
  }
};








function SaveSerializer() {
  this._queuedOperations = [];
  this._queuedInProgress = false;
  this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
}

SaveSerializer.prototype = {
  






  enqueueTask: function (aFunction) {
    let promise = new Promise((resolve, reject) =>
      this._queuedOperations.push([aFunction, resolve, reject]));

    if (this._queuedOperations.length == 1) {
      this._popAndPerformQueuedOperation();
    }
    return promise;
  },

  



  flushTasks: function () {
    let dummyTask = () => new Promise(resolve => resolve());
    return this.enqueueTask(dummyTask);
  },

  



  _popAndPerformQueuedOperation: function () {
    if (!this._queuedOperations.length || this._queuedInProgress) {
      return;
    }

    this._log.trace("_popAndPerformQueuedOperation - Performing queued operation.");
    let [func, resolve, reject] = this._queuedOperations.shift();
    let promise;

    try {
      this._queuedInProgress = true;
      promise = func();
    } catch (ex) {
      this._log.warn("_popAndPerformQueuedOperation - Queued operation threw during execution. ",
                     ex);
      this._queuedInProgress = false;
      reject(ex);
      this._popAndPerformQueuedOperation();
      return;
    }

    if (!promise || typeof(promise.then) != "function") {
      let msg = "Queued operation did not return a promise: " + func;
      this._log.warn("_popAndPerformQueuedOperation - " + msg);

      this._queuedInProgress = false;
      reject(new Error(msg));
      this._popAndPerformQueuedOperation();
      return;
    }

    promise.then(result => {
        this._log.trace("_popAndPerformQueuedOperation - Queued operation completed.");
        this._queuedInProgress = false;
        resolve(result);
        this._popAndPerformQueuedOperation();
      },
      error => {
        this._log.warn("_popAndPerformQueuedOperation - Failure when performing queued operation.",
                       error);
        this._queuedInProgress = false;
        reject(error);
        this._popAndPerformQueuedOperation();
      });
  },
};







let TelemetryScheduler = {
  _lastDailyPingTime: 0,
  _lastSessionCheckpointTime: 0,

  
  _lastAdhocPingTime: 0,
  _lastTickTime: 0,

  _log: null,

  
  _dailyPingRetryAttempts: 0,

  
  _schedulerTimer: null,
  
  _schedulerInterval: 0,
  _shuttingDown: true,
  _isUserIdle: false,

  


  init: function() {
    this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, "TelemetryScheduler::");
    this._log.trace("init");
    this._shuttingDown = false;
    this._isUserIdle = false;
    
    
    let now = Policy.now();
    this._lastDailyPingTime = now.getTime();
    this._lastSessionCheckpointTime = now.getTime();
    this._rescheduleTimeout();
    idleService.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
  },

  


  _rescheduleTimeout: function() {
    this._log.trace("_rescheduleTimeout - isUserIdle: " + this._isUserIdle);
    if (this._shuttingDown) {
      this._log.warn("_rescheduleTimeout - already shutdown");
      return;
    }

    if (this._schedulerTimer) {
      Policy.clearSchedulerTickTimeout(this._schedulerTimer);
    }

    const now = Policy.now();
    let timeout = SCHEDULER_TICK_INTERVAL_MS;

    
    if (this._isUserIdle) {
      timeout = SCHEDULER_TICK_IDLE_INTERVAL_MS;
      
      
      const nextMidnight = getNextMidnight(now);
      timeout = Math.min(timeout, nextMidnight.getTime() - now.getTime());
    }

    this._log.trace("_rescheduleTimeout - scheduling next tick for " + new Date(now.getTime() + timeout));
    this._schedulerTimer =
      Policy.setSchedulerTickTimeout(() => this._onSchedulerTick(), timeout);
  },

  _sentDailyPingToday: function(nowDate) {
    
    const todayDate = truncateToDays(nowDate);
    const nearestMidnight = getNearestMidnight(nowDate);
    
    const checkDate = nearestMidnight || todayDate;
    
    return (this._lastDailyPingTime >= (checkDate.getTime() - SCHEDULER_MIDNIGHT_TOLERANCE_MS));
  },

  




  _isDailyPingDue: function(nowDate) {
    const sentPingToday = this._sentDailyPingToday(nowDate);

    
    if (sentPingToday) {
      this._log.trace("_isDailyPingDue - already sent one today");
      return false;
    }

    const nearestMidnight = getNearestMidnight(nowDate);
    if (!sentPingToday && !nearestMidnight) {
      
      this._log.trace("_isDailyPingDue - daily ping is overdue... computer went to sleep?");
      return true;
    }

    
    const timeSinceLastDaily = nowDate.getTime() - this._lastDailyPingTime;
    if (timeSinceLastDaily < MIN_SUBSESSION_LENGTH_MS) {
      this._log.trace("_isDailyPingDue - delaying daily to keep minimum session length");
      return false;
    }

    
    
    if (!this._isUserIdle && (nowDate.getTime() < nearestMidnight.getTime())) {
      this._log.trace("_isDailyPingDue - waiting for user idle period");
      return false;
    }

    this._log.trace("_isDailyPingDue - is due");
    return true;
  },

  







  _saveAbortedPing: function(now, competingPayload=null) {
    this._lastSessionCheckpointTime = now;
    return Impl._saveAbortedSessionPing(competingPayload)
                .catch(e => this._log.error("_saveAbortedPing - Failed", e));
  },

  


  observe: function(aSubject, aTopic, aData) {
    this._log.trace("observe - aTopic: " + aTopic);
    switch(aTopic) {
      case "idle":
        
        this._isUserIdle = true;
        return this._onSchedulerTick();
        break;
      case "active":
        
        this._isUserIdle = false;
        return this._onSchedulerTick();
        break;
    }
  },

  




  _onSchedulerTick: function() {
    if (this._shuttingDown) {
      this._log.warn("_onSchedulerTick - already shutdown.");
      return Promise.reject(new Error("Already shutdown."));
    }

    let promise = Promise.resolve();
    try {
      promise = this._schedulerTickLogic();
    } catch (e) {
      this._log.error("_onSchedulerTick - There was an exception", e);
    } finally {
      this._rescheduleTimeout();
    }

    
    return promise;
  },

  



  _schedulerTickLogic: function() {
    this._log.trace("_schedulerTickLogic");

    let nowDate = Policy.now();
    let now = nowDate.getTime();

    if (now - this._lastTickTime > 1.1 * SCHEDULER_TICK_INTERVAL_MS) {
      this._log.trace("_schedulerTickLogic - First scheduler tick after sleep or startup.");
    }
    this._lastTickTime = now;

    
    let isAbortedPingDue =
      (now - this._lastSessionCheckpointTime) >= ABORTED_SESSION_UPDATE_INTERVAL_MS;
    
    let shouldSendDaily = this._isDailyPingDue(nowDate);
    
    
    
    
    
    let nextSessionCheckpoint =
      this._lastSessionCheckpointTime + ABORTED_SESSION_UPDATE_INTERVAL_MS;
    let combineActions = (shouldSendDaily && isAbortedPingDue) || (shouldSendDaily &&
                          areTimesClose(now, nextSessionCheckpoint, SCHEDULER_COALESCE_THRESHOLD_MS));

    if (combineActions) {
      this._log.trace("_schedulerTickLogic - Combining pings.");
      
      return Impl._sendDailyPing(true).then(() => this._dailyPingSucceeded(now),
                                            () => this._dailyPingFailed(now));
    } else if (shouldSendDaily) {
      this._log.trace("_schedulerTickLogic - Daily ping due.");
      return Impl._sendDailyPing().then(() => this._dailyPingSucceeded(now),
                                        () => this._dailyPingFailed(now));
    } else if (isAbortedPingDue) {
      this._log.trace("_schedulerTickLogic - Aborted session ping due.");
      return this._saveAbortedPing(now);
    }

    
    this._log.trace("_schedulerTickLogic - No ping due.");
    
    
    
    this._dailyPingRetryAttempts = 0;
    return Promise.resolve();
  },

  





  reschedulePings: function(reason, competingPayload = null) {
    if (this._shuttingDown) {
      this._log.error("reschedulePings - already shutdown");
      return;
    }

    this._log.trace("reschedulePings - reason: " + reason);
    let now = Policy.now();
    this._lastAdhocPingTime = now.getTime();
    if (reason == REASON_ENVIRONMENT_CHANGE) {
      
      
      this._saveAbortedPing(now.getTime(), competingPayload);
      
      let nearestMidnight = getNearestMidnight(now);
      if (nearestMidnight) {
        this._lastDailyPingTime = now.getTime();
      }
    }

    this._rescheduleTimeout();
  },

  



  _dailyPingSucceeded: function(now) {
    this._log.trace("_dailyPingSucceeded");
    this._lastDailyPingTime = now;
    this._dailyPingRetryAttempts = 0;
  },

  



  _dailyPingFailed: function(now) {
    this._log.error("_dailyPingFailed");
    this._dailyPingRetryAttempts++;

    
    
    if (this._dailyPingRetryAttempts >= SCHEDULER_RETRY_ATTEMPTS) {
      this._log.error("_pingFailed - The daily ping failed too many times. Skipping it.");
      this._dailyPingRetryAttempts = 0;
      this._lastDailyPingTime = now;
    }
  },

  


  shutdown: function() {
    if (this._shuttingDown) {
      if (this._log) {
        this._log.error("shutdown - Already shut down");
      } else {
        Cu.reportError("TelemetryScheduler.shutdown - Already shut down");
      }
      return;
    }

    this._log.trace("shutdown");
    if (this._schedulerTimer) {
      Policy.clearSchedulerTickTimeout(this._schedulerTimer);
      this._schedulerTimer = null;
    }

    idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);

    this._shuttingDown = true;
  }
};

this.EXPORTED_SYMBOLS = ["TelemetrySession"];

this.TelemetrySession = Object.freeze({
  Constants: Object.freeze({
    PREF_PREVIOUS_BUILDID: PREF_PREVIOUS_BUILDID,
  }),
  


  testPing: function() {
    return Impl.testPing();
  },
  





  getPayload: function(reason, clearSubsession = false) {
    return Impl.getPayload(reason, clearSubsession);
  },
  



  requestChildPayloads: function() {
    return Impl.requestChildPayloads();
  },
  





  testSaveHistograms: function(aFile) {
    return Impl.testSaveHistograms(aFile);
  },
  


  gatherStartup: function() {
    return Impl.gatherStartup();
  },
  




  setAddOns: function(aAddOns) {
    return Impl.setAddOns(aAddOns);
  },
  







  getMetadata: function(reason) {
    return Impl.getMetadata(reason);
  },
  


  reset: function() {
    Impl._sessionId = null;
    Impl._subsessionId = null;
    Impl._previousSubsessionId = null;
    Impl._subsessionCounter = 0;
    Impl._profileSubsessionCounter = 0;
    Impl._subsessionStartActiveTicks = 0;
    this.uninstall();
    return this.setup();
  },
  




  shutdown: function(aForceSavePending = true) {
    return Impl.shutdownChromeProcess(aForceSavePending);
  },
  


  setup: function() {
    return Impl.setupChromeProcess(true);
  },
  


  uninstall: function() {
    try {
      Impl.uninstall();
    } catch (ex) {
      
    }
  },
  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  },
  




   get clientID() {
    return Impl.clientID;
   },
});

let Impl = {
  _histograms: {},
  _initialized: false,
  _log: null,
  _prevValues: {},
  
  
  _startupHistogramRegex: /SQLITE|HTTP|SPDY|CACHE|DNS/,
  _slowSQLStartup: {},
  _hasWindowRestoredObserver: false,
  _hasXulWindowVisibleObserver: false,
  _startupIO : {},
  
  
  _previousBuildId: null,
  
  
  
  
  _childTelemetry: [],
  
  
  _sessionId: null,
  
  _subsessionId: null,
  
  
  _previousSubsessionId: null,
  
  _subsessionCounter: 0,
  
  _profileSubsessionCounter: 0,
  
  _subsessionStartDate: null,
  
  _subsessionStartActiveTicks: 0,
  
  _delayedInitTask: null,
  
  _delayedInitTaskDeferred: null,
  
  _stateSaveSerializer: new SaveSerializer(),
  
  _abortedSessionSerializer: new SaveSerializer(),

  







  getSimpleMeasurements: function getSimpleMeasurements(forSavedSession, isSubsession, clearSubsession) {
    this._log.trace("getSimpleMeasurements");

    let si = Services.startup.getStartupInfo();

    
    let elapsedTime = Date.now() - si.process;
    var ret = {
      totalTime: Math.round(elapsedTime / 1000), 
      uptime: Math.round(elapsedTime / 60000) 
    }

    
    var appTimestamps = {};
    try {
      let o = {};
      Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", o);
      appTimestamps = o.TelemetryTimestamps.get();
    } catch (ex) {}

    
    if (!IS_CONTENT_PROCESS && Telemetry.canRecordExtended) {
      try {
        ret.addonManager = AddonManagerPrivate.getSimpleMeasures();
        ret.UITelemetry = UITelemetry.getSimpleMeasures();
      } catch (ex) {}
    }

    if (si.process) {
      for each (let field in Object.keys(si)) {
        if (field == "process")
          continue;
        ret[field] = si[field] - si.process
      }

      for (let p in appTimestamps) {
        if (!(p in ret) && appTimestamps[p])
          ret[p] = appTimestamps[p] - si.process;
      }
    }

    ret.startupInterrupted = Number(Services.startup.interrupted);

    ret.js = Cu.getJSEngineTelemetryValue();

    let maximalNumberOfConcurrentThreads = Telemetry.maximalNumberOfConcurrentThreads;
    if (maximalNumberOfConcurrentThreads) {
      ret.maximalNumberOfConcurrentThreads = maximalNumberOfConcurrentThreads;
    }

    if (IS_CONTENT_PROCESS) {
      return ret;
    }

    

    
    let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
    let isDebuggerAttached = debugService.isDebuggerAttached;
    gWasDebuggerAttached = gWasDebuggerAttached || isDebuggerAttached;
    ret.debuggerAttached = Number(gWasDebuggerAttached);

    let shutdownDuration = Telemetry.lastShutdownDuration;
    if (shutdownDuration)
      ret.shutdownDuration = shutdownDuration;

    let failedProfileLockCount = Telemetry.failedProfileLockCount;
    if (failedProfileLockCount)
      ret.failedProfileLockCount = failedProfileLockCount;

    for (let ioCounter in this._startupIO)
      ret[ioCounter] = this._startupIO[ioCounter];

    let hasPingBeenSent = false;
    try {
      hasPingBeenSent = Telemetry.getHistogramById("TELEMETRY_SUCCESS").snapshot().sum > 0;
    } catch(e) {
    }
    if (!forSavedSession || hasPingBeenSent) {
      ret.savedPings = TelemetryStorage.pingsLoaded;
    }

    ret.activeTicks = -1;
    let sr = TelemetryPing.getSessionRecorder();
    if (sr) {
      let activeTicks = sr.activeTicks;
      if (isSubsession) {
        activeTicks = sr.activeTicks - this._subsessionStartActiveTicks;
      }

      if (clearSubsession) {
        this._subsessionStartActiveTicks = activeTicks;
      }

      ret.activeTicks = activeTicks;
    }

    ret.pingsOverdue = TelemetryStorage.pingsOverdue;
    ret.pingsDiscarded = TelemetryStorage.pingsDiscarded;

    return ret;
  },

  




















  packHistogram: function packHistogram(hgram) {
    let r = hgram.ranges;;
    let c = hgram.counts;
    let retgram = {
      range: [r[1], r[r.length - 1]],
      bucket_count: r.length,
      histogram_type: hgram.histogram_type,
      values: {},
      sum: hgram.sum
    };

    if (hgram.histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
      retgram.log_sum = hgram.log_sum;
      retgram.log_sum_squares = hgram.log_sum_squares;
    } else {
      retgram.sum_squares_lo = hgram.sum_squares_lo;
      retgram.sum_squares_hi = hgram.sum_squares_hi;
    }

    let first = true;
    let last = 0;

    for (let i = 0; i < c.length; i++) {
      let value = c[i];
      if (!value)
        continue;

      
      if (i && first) {
        retgram.values[r[i - 1]] = 0;
      }
      first = false;
      last = i + 1;
      retgram.values[r[i]] = value;
    }

    
    if (last && last < c.length)
      retgram.values[r[last]] = 0;
    return retgram;
  },

  



  getDatasetType: function() {
    return Telemetry.canRecordExtended ? Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN
                                       : Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTOUT;
  },

  getHistograms: function getHistograms(subsession, clearSubsession) {
    this._log.trace("getHistograms - subsession: " + subsession + ", clearSubsession: " + clearSubsession);

    let registered =
      Telemetry.registeredHistograms(this.getDatasetType(), []);
    let hls = subsession ? Telemetry.snapshotSubsessionHistograms(clearSubsession)
                         : Telemetry.histogramSnapshots;
    let ret = {};

    for (let name of registered) {
      for (let n of [name, "STARTUP_" + name]) {
        if (n in hls) {
          ret[n] = this.packHistogram(hls[n]);
        }
      }
    }

    return ret;
  },

  getAddonHistograms: function getAddonHistograms() {
    this._log.trace("getAddonHistograms");

    let ahs = Telemetry.addonHistogramSnapshots;
    let ret = {};

    for (let addonName in ahs) {
      let addonHistograms = ahs[addonName];
      let packedHistograms = {};
      for (let name in addonHistograms) {
        packedHistograms[name] = this.packHistogram(addonHistograms[name]);
      }
      if (Object.keys(packedHistograms).length != 0)
        ret[addonName] = packedHistograms;
    }

    return ret;
  },

  getKeyedHistograms: function(subsession, clearSubsession) {
    this._log.trace("getKeyedHistograms - subsession: " + subsession + ", clearSubsession: " + clearSubsession);

    let registered =
      Telemetry.registeredKeyedHistograms(this.getDatasetType(), []);
    let ret = {};

    for (let id of registered) {
      ret[id] = {};
      let keyed = Telemetry.getKeyedHistogramById(id);
      let snapshot = null;
      if (subsession) {
        snapshot = clearSubsession ? keyed.snapshotSubsessionAndClear()
                                   : keyed.subsessionSnapshot();
      } else {
        snapshot = keyed.snapshot();
      }
      for (let key of Object.keys(snapshot)) {
        ret[id][key] = this.packHistogram(snapshot[key]);
      }
    }

    return ret;
  },

  getThreadHangStats: function getThreadHangStats(stats) {
    this._log.trace("getThreadHangStats");

    stats.forEach((thread) => {
      thread.activity = this.packHistogram(thread.activity);
      thread.hangs.forEach((hang) => {
        hang.histogram = this.packHistogram(hang.histogram);
      });
    });
    return stats;
  },

  







  getMetadata: function getMetadata(reason) {
    this._log.trace("getMetadata - Reason " + reason);

    let sessionStartDate = toLocalTimeISOString(truncateToDays(this._sessionStartDate));
    let subsessionStartDate = toLocalTimeISOString(truncateToDays(this._subsessionStartDate));
    
    let subsessionLength =
      Math.floor((Policy.now() - this._subsessionStartDate.getTime()) / 1000);

    let ret = {
      reason: reason,
      revision: HISTOGRAMS_FILE_VERSION,
      asyncPluginInit: Preferences.get(PREF_ASYNC_PLUGIN_INIT, false),

      
      
      timezoneOffset: -this._subsessionStartDate.getTimezoneOffset(),
      previousBuildId: this._previousBuildId,

      sessionId: this._sessionId,
      subsessionId: this._subsessionId,
      previousSubsessionId: this._previousSubsessionId,

      subsessionCounter: this._subsessionCounter,
      profileSubsessionCounter: this._profileSubsessionCounter,

      sessionStartDate: sessionStartDate,
      subsessionStartDate: subsessionStartDate,
      subsessionLength: subsessionLength,
    };

    
    if (this._addons)
      ret.addons = this._addons;

    
    let flashVersion = this.getFlashVersion();
    if (flashVersion)
      ret.flashVersion = flashVersion;

    return ret;
  },

  


  gatherMemory: function gatherMemory() {
    if (!Telemetry.canRecordExtended) {
      this._log.trace("gatherMemory - Extended data recording disabled, skipping.");
      return;
    }

    this._log.trace("gatherMemory");

    let mgr;
    try {
      mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
            getService(Ci.nsIMemoryReporterManager);
    } catch (e) {
      
      return;
    }

    let histogram = Telemetry.getHistogramById("TELEMETRY_MEMORY_REPORTER_MS");
    let startTime = new Date();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let boundHandleMemoryReport = this.handleMemoryReport.bind(this);
    function h(id, units, amountName) {
      try {
        
        
        
        
        let amount = mgr[amountName];
        NS_ASSERT(amount !== undefined,
                  "telemetry accessed an unknown distinguished amount");
        boundHandleMemoryReport(id, units, amount);
      } catch (e) {
      };
    }
    let b = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_BYTES, n);
    let c = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_COUNT, n);
    let cc= (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE, n);
    let p = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_PERCENTAGE, n);

    b("MEMORY_VSIZE", "vsize");
    b("MEMORY_VSIZE_MAX_CONTIGUOUS", "vsizeMaxContiguous");
    b("MEMORY_RESIDENT", "residentFast");
    b("MEMORY_HEAP_ALLOCATED", "heapAllocated");
    p("MEMORY_HEAP_COMMITTED_UNUSED_RATIO", "heapOverheadRatio");
    b("MEMORY_JS_GC_HEAP", "JSMainRuntimeGCHeap");
    b("MEMORY_JS_MAIN_RUNTIME_TEMPORARY_PEAK", "JSMainRuntimeTemporaryPeak");
    c("MEMORY_JS_COMPARTMENTS_SYSTEM", "JSMainRuntimeCompartmentsSystem");
    c("MEMORY_JS_COMPARTMENTS_USER", "JSMainRuntimeCompartmentsUser");
    b("MEMORY_IMAGES_CONTENT_USED_UNCOMPRESSED", "imagesContentUsedUncompressed");
    b("MEMORY_STORAGE_SQLITE", "storageSQLite");
    cc("MEMORY_EVENTS_VIRTUAL", "lowMemoryEventsVirtual");
    cc("MEMORY_EVENTS_PHYSICAL", "lowMemoryEventsPhysical");
    c("GHOST_WINDOWS", "ghostWindows");
    cc("PAGE_FAULTS_HARD", "pageFaultsHard");

    histogram.add(new Date() - startTime);
  },

  handleMemoryReport: function(id, units, amount) {
    let val;
    if (units == Ci.nsIMemoryReporter.UNITS_BYTES) {
      val = Math.floor(amount / 1024);
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_PERCENTAGE) {
      
      val = Math.floor(amount / 100);
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_COUNT) {
      val = amount;
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE) {
      
      

      if (!(id in this._prevValues)) {
        
        
        
        this._prevValues[id] = amount;
        return;
      }

      val = amount - this._prevValues[id];
      this._prevValues[id] = amount;
    }
    else {
      NS_ASSERT(false, "Can't handle memory reporter with units " + units);
      return;
    }

    let h = this._histograms[id];
    if (!h) {
      h = Telemetry.getHistogramById(id);
      this._histograms[id] = h;
    }
    h.add(val);
  },

  



  isInterestingStartupHistogram: function isInterestingStartupHistogram(name) {
    return this._startupHistogramRegex.test(name);
  },

  getChildPayloads: function getChildPayloads() {
    return [for (child of this._childTelemetry) child.payload];
  },

  


  gatherStartupHistograms: function gatherStartupHistograms() {
    this._log.trace("gatherStartupHistograms");

    let info =
      Telemetry.registeredHistograms(this.getDatasetType(), []);
    let snapshots = Telemetry.histogramSnapshots;
    for (let name of info) {
      
      if (this.isInterestingStartupHistogram(name) && name in snapshots) {
        Telemetry.histogramFrom("STARTUP_" + name, name);
      }
    }
  },

  





  assemblePayloadWithMeasurements: function(simpleMeasurements, info, reason, clearSubsession) {
#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)
    const isSubsession = !this._isClassicReason(reason);
#else
    const isSubsession = false;
    clearSubsession = false;
#endif
    this._log.trace("assemblePayloadWithMeasurements - reason: " + reason +
                    ", submitting subsession data: " + isSubsession);

    
    let payloadObj = {
      ver: PAYLOAD_VERSION,
      simpleMeasurements: simpleMeasurements,
      histograms: this.getHistograms(isSubsession, clearSubsession),
      keyedHistograms: this.getKeyedHistograms(isSubsession, clearSubsession),
    };

    
    if (Telemetry.canRecordExtended) {
      payloadObj.chromeHangs = Telemetry.chromeHangs;
      payloadObj.threadHangStats = this.getThreadHangStats(Telemetry.threadHangStats);
      payloadObj.log = TelemetryLog.entries();
    }

    if (IS_CONTENT_PROCESS) {
      return payloadObj;
    }

    
    payloadObj.info = info;

    
    if (Telemetry.canRecordExtended) {
      payloadObj.slowSQL = Telemetry.slowSQL;
      payloadObj.fileIOReports = Telemetry.fileIOReports;
      payloadObj.lateWrites = Telemetry.lateWrites;
      payloadObj.addonHistograms = this.getAddonHistograms();
      payloadObj.addonDetails = AddonManagerPrivate.getTelemetryDetails();
      payloadObj.UIMeasurements = UITelemetry.getUIMeasurements();

      if (Object.keys(this._slowSQLStartup).length != 0 &&
          (Object.keys(this._slowSQLStartup.mainThread).length ||
           Object.keys(this._slowSQLStartup.otherThreads).length)) {
        payloadObj.slowSQLStartup = this._slowSQLStartup;
      }
    }

    if (this._childTelemetry.length) {
      payloadObj.childPayloads = this.getChildPayloads();
    }

    return payloadObj;
  },

  


  startNewSubsession: function () {
    this._subsessionStartDate = Policy.now();
    this._previousSubsessionId = this._subsessionId;
    this._subsessionId = Policy.generateSubsessionUUID();
    this._subsessionCounter++;
    this._profileSubsessionCounter++;
  },

  getSessionPayload: function getSessionPayload(reason, clearSubsession) {
    this._log.trace("getSessionPayload - reason: " + reason + ", clearSubsession: " + clearSubsession);
#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
    clearSubsession = false;
    const isSubsession = false;
#else
    const isSubsession = !this._isClassicReason(reason);
#endif

    let measurements =
      this.getSimpleMeasurements(reason == REASON_SAVED_SESSION, isSubsession, clearSubsession);
    let info = !IS_CONTENT_PROCESS ? this.getMetadata(reason) : null;
    let payload = this.assemblePayloadWithMeasurements(measurements, info, reason, clearSubsession);

    if (!IS_CONTENT_PROCESS && clearSubsession) {
      this.startNewSubsession();
      
      let sessionData = this._getSessionDataObject();
      this._stateSaveSerializer.enqueueTask(() => this._saveSessionData(sessionData));
    }

    return payload;
  },

  


  send: function send(reason) {
    this._log.trace("send - Reason " + reason);
    
    this.gatherMemory();

    const isSubsession = !this._isClassicReason(reason);
    let payload = this.getSessionPayload(reason, isSubsession);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    return TelemetryPing.send(getPingType(payload), payload, options);
  },

  attachObservers: function attachObservers() {
    if (!this._initialized)
      return;
    Services.obs.addObserver(this, "cycle-collector-begin", false);
    Services.obs.addObserver(this, "idle-daily", false);
  },

  detachObservers: function detachObservers() {
    if (!this._initialized)
      return;
    Services.obs.removeObserver(this, "idle-daily");
    Services.obs.removeObserver(this, "cycle-collector-begin");
  },

  


  setupChromeProcess: function setupChromeProcess(testing) {
    this._initStarted = true;
    if (testing && !this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("setupChromeProcess");

    if (this._delayedInitTask) {
      this._log.error("setupChromeProcess - init task already running");
      return this._delayedInitTaskDeferred.promise;
    }

    if (this._initialized && !testing) {
      this._log.error("setupChromeProcess - already initialized");
      return Promise.resolve();
    }

    
    
    this._sessionId = Policy.generateSessionUUID();
    this.startNewSubsession();
    
    
    this._sessionStartDate = this._subsessionStartDate;

    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    
    
    let previousBuildId = Preferences.get(PREF_PREVIOUS_BUILDID, null);
    let thisBuildID = Services.appinfo.appBuildID;
    
    if (previousBuildId != thisBuildID) {
      this._previousBuildId = previousBuildId;
      Preferences.set(PREF_PREVIOUS_BUILDID, thisBuildID);
    }

    if (!Telemetry.canRecordBase && !testing) {
      this._log.config("setupChromeProcess - Telemetry recording is disabled, skipping Chrome process setup.");
      return Promise.resolve();
    }

    TelemetryPing.shutdown.addBlocker("TelemetrySession: shutting down",
                                      () => this.shutdownChromeProcess(),
                                      () => this._getState());

    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.addObserver(this, "application-background", false);
#endif
    Services.obs.addObserver(this, "xul-window-visible", false);
    this._hasWindowRestoredObserver = true;
    this._hasXulWindowVisibleObserver = true;

    ppml.addMessageListener(MESSAGE_TELEMETRY_PAYLOAD, this);

    
    
    
    this._delayedInitTaskDeferred = Promise.defer();
    this._delayedInitTask = new DeferredTask(function* () {
      try {
        this._initialized = true;

        let hasLoaded = yield this._loadSessionData();
        if (!hasLoaded) {
          
          yield this._saveSessionData(this._getSessionDataObject()).catch(() =>
            this._log.error("setupChromeProcess - Could not write session data to disk."));
        }
        this.attachObservers();
        this.gatherMemory();

        Telemetry.asyncFetchTelemetryData(function () {});

#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)
        
        yield this._checkAbortedSessionPing();

        TelemetryEnvironment.registerChangeListener(ENVIRONMENT_CHANGE_LISTENER,
                                                    (reason, data) => this._onEnvironmentChange(reason, data));
        
        
        
        if (!testing) {
          yield this._saveAbortedSessionPing();
        }

        
        TelemetryScheduler.init();
#endif

        this._delayedInitTaskDeferred.resolve();
      } catch (e) {
        this._delayedInitTaskDeferred.reject(e);
      } finally {
        this._delayedInitTask = null;
        this._delayedInitTaskDeferred = null;
      }
    }.bind(this), testing ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY);

    this._delayedInitTask.arm();
    return this._delayedInitTaskDeferred.promise;
  },

  


  setupContentProcess: function setupContentProcess() {
    this._log.trace("setupContentProcess");

    if (!Telemetry.canRecordBase) {
      return;
    }

    Services.obs.addObserver(this, "content-child-shutdown", false);
    cpml.addMessageListener(MESSAGE_TELEMETRY_GET_CHILD_PAYLOAD, this);

    this.gatherStartupHistograms();

    let delayedTask = new DeferredTask(function* () {
      this._initialized = true;

      this.attachObservers();
      this.gatherMemory();
    }.bind(this), TELEMETRY_DELAY);

    delayedTask.arm();
  },

  getFlashVersion: function getFlashVersion() {
    this._log.trace("getFlashVersion");
    let host = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let tags = host.getPluginTags();

    for (let i = 0; i < tags.length; i++) {
      if (tags[i].name == "Shockwave Flash")
        return tags[i].version;
    }

    return null;
  },

  receiveMessage: function receiveMessage(message) {
    this._log.trace("receiveMessage - Message name " + message.name);
    switch (message.name) {
    case MESSAGE_TELEMETRY_PAYLOAD:
    {
      let source = message.data.childUUID;
      delete message.data.childUUID;

      for (let child of this._childTelemetry) {
        if (child.source === source) {
          
          child.payload = message.data;
          return;
        }
      }
      
      this._childTelemetry.push({
        source: source,
        payload: message.data,
      });

      if (this._childTelemetry.length == MAX_NUM_CONTENT_PAYLOADS + 1) {
        this._childTelemetry.shift();
        Telemetry.getHistogramById("TELEMETRY_DISCARDED_CONTENT_PINGS_COUNT").add();
      }

      break;
    }
    case MESSAGE_TELEMETRY_GET_CHILD_PAYLOAD:
    {
      this.sendContentProcessPing("saved-session");
      break;
    }
    default:
      throw new Error("Telemetry.receiveMessage: bad message name");
    }
  },

  _processUUID: generateUUID(),

  sendContentProcessPing: function sendContentProcessPing(reason) {
    this._log.trace("sendContentProcessPing - Reason " + reason);
    const isSubsession = !this._isClassicReason(reason);
    let payload = this.getSessionPayload(reason, isSubsession);
    payload.childUUID = this._processUUID;
    cpmm.sendAsyncMessage(MESSAGE_TELEMETRY_PAYLOAD, payload);
  },

  


  savePendingPings: function savePendingPings() {
    this._log.trace("savePendingPings");

#ifndef MOZ_WIDGET_ANDROID
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
      overwrite: true,
    };

    let shutdownPayload = this.getSessionPayload(REASON_SHUTDOWN, false);
    
    
    return TelemetryPing.addPendingPing(getPingType(shutdownPayload), shutdownPayload, options)
                        .then(() => this.savePendingPingsClassic(),
                              () => this.savePendingPingsClassic());
#else
    return this.savePendingPingsClassic();
#endif
  },

  


  savePendingPingsClassic: function savePendingPingsClassic() {
    this._log.trace("savePendingPingsClassic");
    let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    return TelemetryPing.savePendingPings(getPingType(payload), payload, options);
  },

  testSaveHistograms: function testSaveHistograms(file) {
    this._log.trace("testSaveHistograms - Path: " + file.path);
    let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
      overwrite: true,
    };
    return TelemetryPing.savePing(getPingType(payload), payload, file.path, options);
  },

  


  uninstall: function uninstall() {
    this.detachObservers();
    if (this._hasWindowRestoredObserver) {
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._hasWindowRestoredObserver = false;
    }
    if (this._hasXulWindowVisibleObserver) {
      Services.obs.removeObserver(this, "xul-window-visible");
      this._hasXulWindowVisibleObserver = false;
    }
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.removeObserver(this, "application-background", false);
#endif
  },

  getPayload: function getPayload(reason, clearSubsession) {
    this._log.trace("getPayload - clearSubsession: " + clearSubsession);
    reason = reason || REASON_GATHER_PAYLOAD;
    
    
    if (Object.keys(this._slowSQLStartup).length == 0) {
      this.gatherStartupHistograms();
      this._slowSQLStartup = Telemetry.slowSQL;
    }
    this.gatherMemory();
    return this.getSessionPayload(reason, clearSubsession);
  },

  requestChildPayloads: function() {
    this._log.trace("requestChildPayloads");
    ppmm.broadcastAsyncMessage(MESSAGE_TELEMETRY_GET_CHILD_PAYLOAD, {});
  },

  gatherStartup: function gatherStartup() {
    this._log.trace("gatherStartup");
    let counters = processInfo.getCounters();
    if (counters) {
      [this._startupIO.startupSessionRestoreReadBytes,
        this._startupIO.startupSessionRestoreWriteBytes] = counters;
    }
    this.gatherStartupHistograms();
    this._slowSQLStartup = Telemetry.slowSQL;
  },

  setAddOns: function setAddOns(aAddOns) {
    this._addons = aAddOns;
  },

  testPing: function testPing() {
    return this.send(REASON_TEST_PING);
  },

  


  observe: function (aSubject, aTopic, aData) {
    if (!this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("observe - " + aTopic + " notified.");

    switch (aTopic) {
    case "profile-after-change":
      
      return this.setupChromeProcess();
    case "app-startup":
      
      return this.setupContentProcess();
    case "content-child-shutdown":
      
      Services.obs.removeObserver(this, "content-child-shutdown");
      this.uninstall();

      if (Telemetry.isOfficialTelemetry) {
        this.sendContentProcessPing(REASON_SAVED_SESSION);
      }
      break;
    case "cycle-collector-begin":
      let now = new Date();
      if (!gLastMemoryPoll
          || (TELEMETRY_INTERVAL <= now - gLastMemoryPoll)) {
        gLastMemoryPoll = now;
        this.gatherMemory();
      }
      break;
    case "xul-window-visible":
      Services.obs.removeObserver(this, "xul-window-visible");
      this._hasXulWindowVisibleObserver = false;
      var counters = processInfo.getCounters();
      if (counters) {
        [this._startupIO.startupWindowVisibleReadBytes,
          this._startupIO.startupWindowVisibleWriteBytes] = counters;
      }
      break;
    case "sessionstore-windows-restored":
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._hasWindowRestoredObserver = false;
      
      let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
      gWasDebuggerAttached = debugService.isDebuggerAttached;
      this.gatherStartup();
      break;
    case "idle-daily":
      
      
      Services.tm.mainThread.dispatch((function() {
        
        
        
        Services.obs.notifyObservers(null, "gather-telemetry", null);
      }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      
      
      TelemetryPing.sendPersistedPings();
      break;

#ifdef MOZ_WIDGET_ANDROID
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    case "application-background":
      if (Telemetry.isOfficialTelemetry) {
        let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
        let options = {
          retentionDays: RETENTION_DAYS,
          addClientId: true,
          addEnvironment: true,
          overwrite: true,
        };
        TelemetryPing.addPendingPing(getPingType(payload), payload, options);
      }
      break;
#endif
    }
  },

  




  shutdownChromeProcess: function(testing = false) {
    this._log.trace("shutdownChromeProcess - testing: " + testing);

    let cleanup = () => {
#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)
      TelemetryEnvironment.unregisterChangeListener(ENVIRONMENT_CHANGE_LISTENER);
      TelemetryScheduler.shutdown();
#endif
      this.uninstall();

      let reset = () => {
        this._initStarted = false;
        this._initialized = false;
      };

      if (Telemetry.isOfficialTelemetry || testing) {
        return this.savePendingPings()
                .then(() => this._stateSaveSerializer.flushTasks())
#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)
                .then(() => this._abortedSessionSerializer
                                .enqueueTask(() => this._removeAbortedSessionPing()))
#endif
                .then(reset);
      }

      reset();
      return Promise.resolve();
    };

    
    
    
    
    
    

    
    if (!this._initStarted) {
      return Promise.resolve();
     }

    
    if (!this._delayedInitTask) {
      
      return cleanup();
     }

    
    return this._delayedInitTask.finalize().then(cleanup);
   },

  





  _sendDailyPing: function(saveAsAborted = false) {
    this._log.trace("_sendDailyPing");
    let payload = this.getSessionPayload(REASON_DAILY, true);

    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };

    let promise = TelemetryPing.send(getPingType(payload), payload, options);
#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)
    
    if (saveAsAborted) {
      let abortedPromise = this._saveAbortedSessionPing(payload);
      promise = promise.then(() => abortedPromise);
    }
#endif
    return promise;
  },

  




  _loadSessionData: Task.async(function* () {
    let dataFile = OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY,
                                SESSION_STATE_FILE_NAME);

    
    try {
      let data = yield CommonUtils.readJSON(dataFile);
      if (data &&
          "profileSubsessionCounter" in data &&
          typeof(data.profileSubsessionCounter) == "number" &&
          "previousSubsessionId" in data) {
        this._previousSubsessionId = data.previousSubsessionId;
        
        
        
        this._profileSubsessionCounter = data.profileSubsessionCounter +
                                         this._subsessionCounter;
        return true;
      }
    } catch (e) {
      this._log.info("_loadSessionData - Cannot load session data file " + dataFile, e);
    }
    return false;
  }),

  


  _getSessionDataObject: function() {
    return {
      previousSubsessionId: this._previousSubsessionId,
      profileSubsessionCounter: this._profileSubsessionCounter,
    };
  },

  


  _saveSessionData: Task.async(function* (sessionData) {
    let dataDir = OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY);
    yield OS.File.makeDir(dataDir);

    let filePath = OS.Path.join(dataDir, SESSION_STATE_FILE_NAME);
    try {
      yield CommonUtils.writeJSON(sessionData, filePath);
    } catch(e) {
      this._log.error("_saveSessionData - Failed to write session data to " + filePath, e);
    }
  }),

  _onEnvironmentChange: function(reason, oldEnvironment) {
    this._log.trace("_onEnvironmentChange", reason);
    let payload = this.getSessionPayload(REASON_ENVIRONMENT_CHANGE, true);

    let clonedPayload = Cu.cloneInto(payload, myScope);
    TelemetryScheduler.reschedulePings(REASON_ENVIRONMENT_CHANGE, clonedPayload);

    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
      overrideEnvironment: oldEnvironment,
    };
    TelemetryPing.send(getPingType(payload), payload, options);
  },

  _isClassicReason: function(reason) {
    const classicReasons = [
      REASON_SAVED_SESSION,
      REASON_GATHER_PAYLOAD,
      REASON_TEST_PING,
    ];
    return classicReasons.indexOf(reason) != -1;
  },

  


  _getState: function() {
    return {
      initialized: this._initialized,
      initStarted: this._initStarted,
      haveDelayedInitTask: !!this._delayedInitTask,
    };
  },

  




  _removeAbortedSessionPing: function() {
    const FILE_PATH = OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY,
                                   ABORTED_SESSION_FILE_NAME);
    try {
      this._log.trace("_removeAbortedSessionPing - success");
      return OS.File.remove(FILE_PATH);
    } catch (ex if ex.becauseNoSuchFile) {
      this._log.trace("_removeAbortedSessionPing - no such file");
    } catch (ex) {
      this._log.error("_removeAbortedSessionPing - error removing ping", ex)
    }
    return Promise.resolve();
  },

  



  _checkAbortedSessionPing: Task.async(function* () {
    
    
    
    const ABORTED_SESSIONS_DIR =
      OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY);
    yield OS.File.makeDir(ABORTED_SESSIONS_DIR, { ignoreExisting: true });

    const FILE_PATH = OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY,
                                   ABORTED_SESSION_FILE_NAME);
    let abortedExists = yield OS.File.exists(FILE_PATH);
    if (abortedExists) {
      this._log.trace("_checkAbortedSessionPing - aborted session found: " + FILE_PATH);
      yield this._abortedSessionSerializer.enqueueTask(
        () => TelemetryPing.addPendingPingFromFile(FILE_PATH, true));
    }
  }),

  





  _saveAbortedSessionPing: function(aProvidedPayload = null) {
    const FILE_PATH = OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIRECTORY,
                                   ABORTED_SESSION_FILE_NAME);
    this._log.trace("_saveAbortedSessionPing - ping path: " + FILE_PATH);

    let payload = null;
    if (aProvidedPayload) {
      payload = aProvidedPayload;
      
      payload.info.reason = REASON_ABORTED_SESSION;
    } else {
      payload = this.getSessionPayload(REASON_ABORTED_SESSION, false);
    }

    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
      overwrite: true,
    };
    return this._abortedSessionSerializer.enqueueTask(() =>
      TelemetryPing.savePing(getPingType(payload), payload, FILE_PATH, options));
  },
};
