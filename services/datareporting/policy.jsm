














"use strict";

#ifndef MERGED_COMPARTMENT

this.EXPORTED_SYMBOLS = [
  "DataSubmissionRequest", 
  "DataReportingPolicy",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

#endif

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/utils.js");

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;



const OLDEST_ALLOWED_YEAR = 2012;





































function NotifyPolicyRequest(policy, promise) {
  this.policy = policy;
  this.promise = promise;
}
NotifyPolicyRequest.prototype = {
  





  onUserNotifyComplete: function onUserNotified() {
    this.promise.resolve();
  },

  





  onUserNotifyFailed: function onUserNotifyFailed(error) {
    this.promise.reject(error);
  },

  





  onUserAccept: function onUserAccept(reason) {
    this.policy.recordUserAcceptance(reason);
  },

  





  onUserReject: function onUserReject(reason) {
    this.policy.recordUserRejection(reason);
  },
};

Object.freeze(NotifyPolicyRequest.prototype);














function DataSubmissionRequest(promise, expiresDate, isDelete) {
  this.promise = promise;
  this.expiresDate = expiresDate;
  this.isDelete = isDelete;

  this.state = null;
  this.reason = null;
}

DataSubmissionRequest.prototype = {
  NO_DATA_AVAILABLE: "no-data-available",
  SUBMISSION_SUCCESS: "success",
  SUBMISSION_FAILURE_SOFT: "failure-soft",
  SUBMISSION_FAILURE_HARD: "failure-hard",

  






  onNoDataAvailable: function onNoDataAvailable() {
    this.state = this.NO_DATA_AVAILABLE;
    this.promise.resolve(this);
  },

  








  onSubmissionSuccess: function onSubmissionSuccess(date) {
    this.state = this.SUBMISSION_SUCCESS;
    this.submissionDate = date;
    this.promise.resolve(this);
  },

  








  onSubmissionFailureSoft: function onSubmissionFailureSoft(reason=null) {
    this.state = this.SUBMISSION_FAILURE_SOFT;
    this.reason = reason;
    this.promise.resolve(this);
  },

  









  onSubmissionFailureHard: function onSubmissionFailureHard(reason=null) {
    this.state = this.SUBMISSION_FAILURE_HARD;
    this.reason = reason;
    this.promise.resolve(this);
  },
};

Object.freeze(DataSubmissionRequest.prototype);



















































this.DataReportingPolicy = function (prefs, healthReportPrefs, listener) {
  this._log = Log4Moz.repository.getLogger("Services.DataReporting.Policy");
  this._log.level = Log4Moz.Level["Debug"];

  for (let handler of this.REQUIRED_LISTENERS) {
    if (!listener[handler]) {
      throw new Error("Passed listener does not contain required handler: " +
                      handler);
    }
  }

  this._prefs = prefs;
  this._healthReportPrefs = healthReportPrefs;
  this._listener = listener;

  
  if (!this.firstRunDate.getTime()) {
    this.firstRunDate = this.now();
  }

  
  
  
  
  
  
  this.uploadEnabledObserver = function onUploadEnabledChanged() {
    if (this.pendingDeleteRemoteData || this.healthReportUploadEnabled) {
      
      
      return;
    }
    this._log.info("uploadEnabled pref changed. Scheduling deletion.");
    this.deleteRemoteData();
  }.bind(this);

  healthReportPrefs.observe("uploadEnabled", this.uploadEnabledObserver);

  
  if (!this.nextDataSubmissionDate.getTime()) {
    this.nextDataSubmissionDate = this._futureDate(MILLISECONDS_PER_DAY);
  }

  
  
  
  this._dataSubmissionPolicyNotifiedDate = null;

  
  
  this._inProgressSubmissionRequest = null;
}

DataReportingPolicy.prototype = Object.freeze({
  


  SUBMISSION_NOTIFY_INTERVAL_MSEC: 12 * 60 * 60 * 1000,

  





  IMPLICIT_ACCEPTANCE_INTERVAL_MSEC: 8 * 60 * 60 * 1000,

  









  POLL_INTERVAL_MSEC: (60 * 1000) + Math.floor(2.5 * 1000 * Math.random()),

  








  SUBMISSION_REQUEST_EXPIRE_INTERVAL_MSEC: 10 * 60 * 1000,

  











  FAILURE_BACKOFF_INTERVALS: [
    15 * 60 * 1000,
    60 * 60 * 1000,
  ],

  


  STATE_NOTIFY_UNNOTIFIED: "not-notified",
  STATE_NOTIFY_WAIT: "waiting",
  STATE_NOTIFY_COMPLETE: "ok",

  REQUIRED_LISTENERS: [
    "onRequestDataUpload",
    "onRequestRemoteDelete",
    "onNotifyDataPolicy",
  ],

  




  get firstRunDate() {
    return CommonUtils.getDatePref(this._prefs, "firstRunTime", 0, this._log,
                                   OLDEST_ALLOWED_YEAR);
  },

  set firstRunDate(value) {
    this._log.debug("Setting first-run date: " + value);
    CommonUtils.setDatePref(this._prefs, "firstRunTime", value,
                            OLDEST_ALLOWED_YEAR);
  },

  





  get dataSubmissionPolicyBypassAcceptance() {
    return this._prefs.get("dataSubmissionPolicyBypassAcceptance", false);
  },

  





  get dataSubmissionPolicyNotifiedDate() {
    return CommonUtils.getDatePref(this._prefs,
                                   "dataSubmissionPolicyNotifiedTime", 0,
                                   this._log, OLDEST_ALLOWED_YEAR);
  },

  set dataSubmissionPolicyNotifiedDate(value) {
    this._log.debug("Setting user notified date: " + value);
    CommonUtils.setDatePref(this._prefs, "dataSubmissionPolicyNotifiedTime",
                            value, OLDEST_ALLOWED_YEAR);
  },

  




  get dataSubmissionPolicyResponseDate() {
    return CommonUtils.getDatePref(this._prefs,
                                   "dataSubmissionPolicyResponseTime",
                                   0, this._log, OLDEST_ALLOWED_YEAR);
  },

  set dataSubmissionPolicyResponseDate(value) {
    this._log.debug("Setting user notified reaction date: " + value);
    CommonUtils.setDatePref(this._prefs,
                            "dataSubmissionPolicyResponseTime",
                            value, OLDEST_ALLOWED_YEAR);
  },

  










  get dataSubmissionPolicyResponseType() {
    return this._prefs.get("dataSubmissionPolicyResponseType",
                           "none-recorded");
  },

  set dataSubmissionPolicyResponseType(value) {
    if (typeof(value) != "string") {
      throw new Error("Value must be a string. Got " + typeof(value));
    }

    this._prefs.set("dataSubmissionPolicyResponseType", value);
  },

  





  get dataSubmissionEnabled() {
    
    return this._prefs.get("dataSubmissionEnabled", true);
  },

  set dataSubmissionEnabled(value) {
    this._prefs.set("dataSubmissionEnabled", !!value);
  },

  




  get dataSubmissionPolicyAccepted() {
    
    return this._prefs.get("dataSubmissionPolicyAccepted", false);
  },

  set dataSubmissionPolicyAccepted(value) {
    this._prefs.set("dataSubmissionPolicyAccepted", !!value);
  },

  set dataSubmissionPolicyAcceptedVersion(value) {
    this._prefs.set("dataSubmissionPolicyAcceptedVersion", value);
  },

  







  get notifyState() {
    if (this.dataSubmissionPolicyResponseDate.getTime()) {
      return this.STATE_NOTIFY_COMPLETE;
    }

    
    
    
    
    if (!this._dataSubmissionPolicyNotifiedDate) {
      return this.STATE_NOTIFY_UNNOTIFIED;
    }

    return this.STATE_NOTIFY_WAIT;
  },

  





  get lastDataSubmissionRequestedDate() {
    return CommonUtils.getDatePref(this._healthReportPrefs,
                                   "lastDataSubmissionRequestedTime", 0,
                                   this._log, OLDEST_ALLOWED_YEAR);
  },

  set lastDataSubmissionRequestedDate(value) {
    CommonUtils.setDatePref(this._healthReportPrefs,
                            "lastDataSubmissionRequestedTime",
                            value, OLDEST_ALLOWED_YEAR);
  },

  





  get lastDataSubmissionSuccessfulDate() {
    return CommonUtils.getDatePref(this._healthReportPrefs,
                                   "lastDataSubmissionSuccessfulTime", 0,
                                   this._log, OLDEST_ALLOWED_YEAR);
  },

  set lastDataSubmissionSuccessfulDate(value) {
    CommonUtils.setDatePref(this._healthReportPrefs,
                            "lastDataSubmissionSuccessfulTime",
                            value, OLDEST_ALLOWED_YEAR);
  },

  





  get lastDataSubmissionFailureDate() {
    return CommonUtils.getDatePref(this._healthReportPrefs,
                                   "lastDataSubmissionFailureTime",
                                   0, this._log, OLDEST_ALLOWED_YEAR);
  },

  set lastDataSubmissionFailureDate(value) {
    CommonUtils.setDatePref(this._healthReportPrefs,
                            "lastDataSubmissionFailureTime",
                            value, OLDEST_ALLOWED_YEAR);
  },

  





  get nextDataSubmissionDate() {
    return CommonUtils.getDatePref(this._healthReportPrefs,
                                   "nextDataSubmissionTime", 0,
                                   this._log, OLDEST_ALLOWED_YEAR);
  },

  set nextDataSubmissionDate(value) {
    CommonUtils.setDatePref(this._healthReportPrefs,
                            "nextDataSubmissionTime", value,
                            OLDEST_ALLOWED_YEAR);
  },

  




  get currentDaySubmissionFailureCount() {
    let v = this._healthReportPrefs.get("currentDaySubmissionFailureCount", 0);

    if (!Number.isInteger(v)) {
      v = 0;
    }

    return v;
  },

  set currentDaySubmissionFailureCount(value) {
    if (!Number.isInteger(value)) {
      throw new Error("Value must be integer: " + value);
    }

    this._healthReportPrefs.set("currentDaySubmissionFailureCount", value);
  },

  






  get pendingDeleteRemoteData() {
    return !!this._healthReportPrefs.get("pendingDeleteRemoteData", false);
  },

  set pendingDeleteRemoteData(value) {
    this._healthReportPrefs.set("pendingDeleteRemoteData", !!value);
  },

  


  get healthReportUploadEnabled() {
    return !!this._healthReportPrefs.get("uploadEnabled", true);
  },

  
  
  set healthReportUploadEnabled(value) {
    this._healthReportPrefs.set("uploadEnabled", !!value);
  },

  












  recordUserAcceptance: function recordUserAcceptance(reason="no-reason") {
    this._log.info("User accepted data submission policy: " + reason);
    this.dataSubmissionPolicyResponseDate = this.now();
    this.dataSubmissionPolicyResponseType = "accepted-" + reason;
    this.dataSubmissionPolicyAccepted = true;
    this.dataSubmissionPolicyAcceptedVersion = 1;
  },

  









  recordUserRejection: function recordUserRejection(reason="no-reason") {
    this._log.info("User rejected data submission policy: " + reason);
    this.dataSubmissionPolicyResponseDate = this.now();
    this.dataSubmissionPolicyResponseType = "rejected-" + reason;
    this.dataSubmissionPolicyAccepted = false;
  },

  






















  recordHealthReportUploadEnabled: function (flag, reason="no-reason") {
    let result = null;
    if (!flag) {
      result = this.deleteRemoteData(reason);
    }

    this.healthReportUploadEnabled = flag;
    return result;
  },

  







  deleteRemoteData: function deleteRemoteData(reason="no-reason") {
    this._log.info("Remote data deletion requested: " + reason);

    this.pendingDeleteRemoteData = true;

    
    
    this.nextDataSubmissionDate = this.now();
    return this.checkStateAndTrigger();
  },

  







  startPolling: function startPolling() {
    this.stopPolling();

    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback({
      notify: function notify() {
        this.checkStateAndTrigger();
      }.bind(this)
    }, this.POLL_INTERVAL_MSEC, this._timer.TYPE_REPEATING_SLACK);
  },

  




  stopPolling: function stopPolling() {
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
  },

  





  now: function now() {
    return new Date();
  },

  









  checkStateAndTrigger: function checkStateAndTrigger() {
    
    
    
    if (!this.dataSubmissionEnabled) {
      this._log.debug("Data submission is disabled. Doing nothing.");
      return;
    }

    let now = this.now();
    let nowT = now.getTime();
    let nextSubmissionDate = this.nextDataSubmissionDate;

    
    
    
    
    if (nextSubmissionDate.getTime() >= nowT + 3 * MILLISECONDS_PER_DAY) {
      this._log.warn("Next data submission time is far away. Was the system " +
                     "clock recently readjusted? " + nextSubmissionDate);

      
      
      this._moveScheduleForward24h();

      
    }

    
    if (this._processInProgressSubmission()) {
      return;
    }

    
    if (this.pendingDeleteRemoteData) {
      if (nowT < nextSubmissionDate.getTime()) {
        this._log.debug("Deletion request is scheduled for the future: " +
                        nextSubmissionDate);
        return;
      }

      return this._dispatchSubmissionRequest("onRequestRemoteDelete", true);
    }

    if (!this.healthReportUploadEnabled) {
      this._log.debug("Data upload is disabled. Doing nothing.");
      return;
    }

    
    if (!this.ensureNotifyResponse(now)) {
      return;
    }

    
    if (!this.dataSubmissionPolicyAccepted && !this.dataSubmissionPolicyBypassAcceptance) {
      this._log.debug("Data submission has been disabled per user request.");
      return;
    }

    
    

    if (nowT < nextSubmissionDate.getTime()) {
      this._log.debug("Next data submission is scheduled in the future: " +
                     nextSubmissionDate);
      return;
    }

    return this._dispatchSubmissionRequest("onRequestDataUpload", false);
  },

  







  ensureNotifyResponse: function ensureNotifyResponse(now) {
    if (this.dataSubmissionPolicyBypassAcceptance) {
      return true;
    }

    let notifyState = this.notifyState;

    if (notifyState == this.STATE_NOTIFY_UNNOTIFIED) {
      let notifyAt = new Date(this.firstRunDate.getTime() +
                              this.SUBMISSION_NOTIFY_INTERVAL_MSEC);

      if (now.getTime() < notifyAt.getTime()) {
        this._log.debug("Don't have to notify about data submission yet.");
        return false;
      }

      let onComplete = function onComplete() {
        this._log.info("Data submission notification presented.");
        let now = this.now();

        this._dataSubmissionPolicyNotifiedDate = now;
        this.dataSubmissionPolicyNotifiedDate = now;
      }.bind(this);

      let deferred = Promise.defer();

      deferred.promise.then(onComplete, function onError(error) {
        this._log.warn("Data policy notification presentation failed: " +
                       CommonUtils.exceptionStr(error));
      });

      this._log.info("Requesting display of data policy.");
      let request = new NotifyPolicyRequest(this, deferred);

      try {
        this._listener.onNotifyDataPolicy(request);
      } catch (ex) {
        this._log.warn("Exception when calling onNotifyDataPolicy: " +
                       CommonUtils.exceptionStr(ex));
      }
      return false;
    }

    
    if (notifyState == this.STATE_NOTIFY_WAIT) {
      
      let implicitAcceptanceDate =
        new Date(this._dataSubmissionPolicyNotifiedDate.getTime() +
                 this.IMPLICIT_ACCEPTANCE_INTERVAL_MSEC);

      if (now.getTime() < implicitAcceptanceDate.getTime()) {
        this._log.debug("Still waiting for reaction or implicit acceptance.");
        return false;
      }

      this.recordUserAcceptance("implicit-time-elapsed");
      return true;
    }

    
    if (notifyState != this.STATE_NOTIFY_COMPLETE) {
      throw new Error("Unknown notification state: " + notifyState);
    }

    return true;
  },

  _processInProgressSubmission: function _processInProgressSubmission() {
    if (!this._inProgressSubmissionRequest) {
      return false;
    }

    let now = this.now().getTime();
    if (this._inProgressSubmissionRequest.expiresDate.getTime() > now) {
      this._log.info("Waiting on in-progress submission request to finish.");
      return true;
    }

    this._log.warn("Old submission request has expired from no activity.");
    this._inProgressSubmissionRequest.promise.reject(new Error("Request has expired."));
    this._inProgressSubmissionRequest = null;
    this._handleSubmissionFailure();

    return false;
  },

  _dispatchSubmissionRequest: function _dispatchSubmissionRequest(handler, isDelete) {
    let now = this.now();

    
    this.lastDataSubmissionRequestedDate = now;
    let deferred = Promise.defer();
    let requestExpiresDate =
      this._futureDate(this.SUBMISSION_REQUEST_EXPIRE_INTERVAL_MSEC);
    this._inProgressSubmissionRequest = new DataSubmissionRequest(deferred,
                                                                  requestExpiresDate,
                                                                  isDelete);

    let onSuccess = function onSuccess(result) {
      this._inProgressSubmissionRequest = null;
      this._handleSubmissionResult(result);
    }.bind(this);

    let onError = function onError(error) {
      this._log.error("Error when handling data submission result: " +
                      CommonUtils.exceptionStr(result));
      this._inProgressSubmissionRequest = null;
      this._handleSubmissionFailure();
    }.bind(this);

    let chained = deferred.promise.then(onSuccess, onError);

    this._log.info("Requesting data submission. Will expire at " +
                   requestExpiresDate);
    try {
      this._listener[handler](this._inProgressSubmissionRequest);
    } catch (ex) {
      this._log.warn("Exception when calling " + handler + ": " +
                     CommonUtils.exceptionStr(ex));
      this._inProgressSubmissionRequest = null;
      this._handleSubmissionFailure();
      return;
    }

    return chained;
  },

  _handleSubmissionResult: function _handleSubmissionResult(request) {
    let state = request.state;
    let reason = request.reason || "no reason";
    this._log.info("Got submission request result: " + state);

    if (state == request.SUBMISSION_SUCCESS) {
      if (request.isDelete) {
        this.pendingDeleteRemoteData = false;
        this._log.info("Successful data delete reported.");
      } else {
        this._log.info("Successful data upload reported.");
      }

      this.lastDataSubmissionSuccessfulDate = request.submissionDate;

      let nextSubmissionDate =
        new Date(request.submissionDate.getTime() + MILLISECONDS_PER_DAY);

      
      
      
      if (this.pendingDeleteRemoteData) {
        nextSubmissionDate = this.now();
      }

      this.nextDataSubmissionDate = nextSubmissionDate;
      this.currentDaySubmissionFailureCount = 0;
      return;
    }

    if (state == request.NO_DATA_AVAILABLE) {
      if (request.isDelete) {
        this._log.info("Remote data delete requested but no remote data was stored.");
        this.pendingDeleteRemoteData = false;
        return;
      }

      this._log.info("No data was available to submit. May try later.");
      this._handleSubmissionFailure();
      return;
    }

    
    

    if (state == request.SUBMISSION_FAILURE_SOFT) {
      this._log.warn("Soft error submitting data: " + reason);
      this.lastDataSubmissionFailureDate = this.now();
      this._handleSubmissionFailure();
      return;
    }

    if (state == request.SUBMISSION_FAILURE_HARD) {
      this._log.warn("Hard error submitting data: " + reason);
      this.lastDataSubmissionFailureDate = this.now();
      this._moveScheduleForward24h();
      return;
    }

    throw new Error("Unknown state on DataSubmissionRequest: " + request.state);
  },

  _handleSubmissionFailure: function _handleSubmissionFailure() {
    if (this.currentDaySubmissionFailureCount >= this.FAILURE_BACKOFF_INTERVALS.length) {
      this._log.warn("Reached the limit of daily submission attempts. " +
                     "Rescheduling for tomorrow.");
      this._moveScheduleForward24h();
      return false;
    }

    let offset = this.FAILURE_BACKOFF_INTERVALS[this.currentDaySubmissionFailureCount];
    this.nextDataSubmissionDate = this._futureDate(offset);
    this.currentDaySubmissionFailureCount++;
    return true;
  },

  _moveScheduleForward24h: function _moveScheduleForward24h() {
    let d = this._futureDate(MILLISECONDS_PER_DAY);
    this._log.info("Setting next scheduled data submission for " + d);

    this.nextDataSubmissionDate = d;
    this.currentDaySubmissionFailureCount = 0;
  },

  _futureDate: function _futureDate(offset) {
    return new Date(this.now().getTime() + offset);
  },
});

