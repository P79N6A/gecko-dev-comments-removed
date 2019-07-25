




































EXPORTED_SYMBOLS = ["TaskConstants", "TestPilotBuiltinSurvey",
                    "TestPilotExperiment", "TestPilotStudyResults",
                    "TestPilotLegacyStudy", "TestPilotWebSurvey"];

const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://testpilot/modules/Observers.js");
Components.utils.import("resource://testpilot/modules/metadata.js");
Components.utils.import("resource://testpilot/modules/log4moz.js");
Components.utils.import("resource://testpilot/modules/string_sanitizer.js");

const STATUS_PREF_PREFIX = "extensions.testpilot.taskstatus.";
const START_DATE_PREF_PREFIX = "extensions.testpilot.startDate.";
const RECUR_PREF_PREFIX = "extensions.testpilot.reSubmit.";
const RECUR_TIMES_PREF_PREFIX = "extensions.testpilot.recurCount.";
const SURVEY_ANSWER_PREFIX = "extensions.testpilot.surveyAnswers.";
const EXPIRATION_DATE_FOR_DATA_SUBMISSION_PREFIX =
  "extensions.testpilot.expirationDateForDataSubmission.";
const DATE_FOR_DATA_DELETION_PREFIX =
  "extensions.testpilot.dateForDataDeletion.";
const GUID_PREF_PREFIX = "extensions.testpilot.taskGUID.";
const RETRY_INTERVAL_PREF = "extensions.testpilot.uploadRetryInterval";
const TIME_FOR_DATA_DELETION = 7 * (24 * 60 * 60 * 1000); 
const DATA_UPLOAD_PREF = "extensions.testpilot.dataUploadURL";
const DEFAULT_THUMBNAIL_URL = "chrome://testpilot/skin/badge-default.png";


const TaskConstants = {
  
  
  
 STATUS_NEW: 0, 
 STATUS_PENDING : 1,  
 STATUS_STARTING: 2,  
 STATUS_IN_PROGRESS : 3, 
 STATUS_FINISHED : 4, 
 STATUS_CANCELLED : 5, 
 STATUS_SUBMITTED : 6, 
 STATUS_RESULTS : 7, 
 STATUS_ARCHIVED: 8, 
 STATUS_MISSED: 9, 

 TYPE_EXPERIMENT : 1,
 TYPE_SURVEY : 2,
 TYPE_RESULTS : 3,
 TYPE_LEGACY: 4,

 ALWAYS_SUBMIT: 1,
 NEVER_SUBMIT: -1,
 ASK_EACH_TIME: 0
};




let Application = Cc["@mozilla.org/fuel/application;1"]
                  .getService(Ci.fuelIApplication);


var TestPilotTask = {
  _id: null,
  _title: null,
  _status: null,
  _url: null,

  _taskInit: function TestPilotTask__taskInit(id, title, url, summary, thumb) {
    this._id = id;
    this._title = title;
    this._status = Application.prefs.getValue(STATUS_PREF_PREFIX + this._id,
                                              TaskConstants.STATUS_NEW);
    this._url = url;
    this._summary = summary;
    this._thumbnail = thumb;
    this._logger = Log4Moz.repository.getLogger("TestPilot.Task_"+this._id);
  },

  get title() {
    return this._title;
  },

  get id() {
    return this._id;
  },

  get version() {
    return this._versionNumber;
  },

  get taskType() {
    return null;
  },

  get status() {
    return this._status;
  },

  get webContent() {
    return this._webContent;
  },

  get summary() {
    if (this._summary) {
      return this._summary;
    } else {
      return this._title;
    }
  },

  get thumbnail() {
    if (this._thumbnail) {
      return this._thumbnail;
    } else {
      return DEFAULT_THUMBNAIL_URL;
    }
  },

  
  get infoPageUrl() {
    return this._url;
  },

  get currentStatusUrl() {
    return this._url;
  },

  get defaultUrl() {
    return this.infoPageUrl;
  },

  get uploadUrl() {
    let url = Application.prefs.getValue(DATA_UPLOAD_PREF, "");
    return url + this._id;
  },

  

  onExperimentStartup: function TestPilotTask_onExperimentStartup() {
    
    
  },

  onExperimentShutdown: function TestPilotTask_onExperimentShutdown() {
    
    
  },

  doExperimentCleanup: function TestPilotTask_onExperimentCleanup() {
    
    
  },

  onAppStartup: function TestPilotTask_onAppStartup() {
    
  },

  onAppShutdown: function TestPilotTask_onAppShutdown() {
    
  },

  onEnterPrivateBrowsing: function TestPilotTask_onEnterPrivate() {
  },

  onExitPrivateBrowsing: function TestPilotTask_onExitPrivate() {
  },

  onNewWindow: function TestPilotTask_onNewWindow(window) {
  },

  onWindowClosed: function TestPilotTask_onWindowClosed(window) {
  },

  onUrlLoad: function TestPilotTask_onUrlLoad(url) {
  },

  onDetailPageOpened: function TestPilotTask_onDetailPageOpened(){
    
  },

  checkDate: function TestPilotTask_checkDate() {
  },

  changeStatus: function TPS_changeStatus(newStatus, suppressNotification) {
    
    
    
    let logger = Log4Moz.repository.getLogger("TestPilot.Task");
    logger.info("Changing task " + this._id + " status to " + newStatus);
    this._status = newStatus;
    
    Application.prefs.setValue(STATUS_PREF_PREFIX + this._id, newStatus);
    
    if (!suppressNotification) {
      Observers.notify("testpilot:task:changed", "", null);
    }
  },

  loadPage: function TestPilotTask_loadPage() {
    
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Ci.nsIWindowMediator);
    let window = wm.getMostRecentWindow("navigator:browser");
    window.TestPilotWindowUtils.openChromeless(this.defaultUrl);
    

    if (this._status == TaskConstants.STATUS_NEW) {
      this.changeStatus(TaskConstants.STATUS_PENDING);
    } else if (this._status == TaskConstants.STATUS_STARTING) {
      this.changeStatus(TaskConstants.STATUS_IN_PROGRESS);
    } else if (this._status == TaskConstants.STATUS_RESULTS) {
      this.changeStatus(TaskConstants.STATUS_ARCHIVED);
    }

    this.onDetailPageOpened();
  },

  getGuid: function TPS_getGuid(id) {
    
    
    let guid = Application.prefs.getValue(GUID_PREF_PREFIX + id, "");
    if (guid == "") {
      let uuidGenerator =
        Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
      guid = uuidGenerator.generateUUID().toString();
      
      if (guid.indexOf("{") == 0) {
        guid = guid.substring(1, (guid.length - 1));
      }
      Application.prefs.setValue(GUID_PREF_PREFIX + id, guid);
    }
    return guid;
  }
};

function TestPilotExperiment(expInfo, dataStore, handlers, webContent, dateOverrideFunc) {
  
  this._init(expInfo, dataStore, handlers, webContent, dateOverrideFunc);
}
TestPilotExperiment.prototype = {
  _init: function TestPilotExperiment__init(expInfo,
					    dataStore,
					    handlers,
                                            webContent,
                                            dateOverrideFunc) {
    













    
    
    if (dateOverrideFunc) {
      this._now = dateOverrideFunc;
    } else {
      this._now = Date.now;
    }
    this._taskInit(expInfo.testId, expInfo.testName, expInfo.testInfoUrl,
                   expInfo.summary, expInfo.thumbnail);
    this._webContent = webContent;
    this._dataStore = dataStore;
    this._versionNumber = expInfo.versionNumber;
    this._optInRequired = expInfo.optInRequired;
    
    this._recursAutomatically = expInfo.recursAutomatically;
    this._recurrenceInterval = expInfo.recurrenceInterval;

    let prefName = START_DATE_PREF_PREFIX + this._id;
    let startDateString = Application.prefs.getValue(prefName, false);
    if (startDateString) {
      
      
      this._startDate = Date.parse(startDateString);
    } else {
      
      
      if (expInfo.startDate) {
        this._startDate = Date.parse(expInfo.startDate);
        Application.prefs.setValue(prefName, expInfo.startDate);
      } else {
        this._startDate = this._now();
        Application.prefs.setValue(prefName, (new Date(this._startDate)).toString());
      }
    }

    
    let duration = expInfo.duration || 7; 
    this._endDate = this._startDate + duration * (24 * 60 * 60 * 1000);
    this._logger.info("Start date is " + this._startDate.toString());
    this._logger.info("End date is " + this._endDate.toString());

    this._handlers = handlers;
    this._uploadRetryTimer = null;
    this._startedUpHandlers = false;

    
    
    this.checkDate();

    if (this.experimentIsRunning) {
      this.onExperimentStartup();
    }
  },

  get taskType() {
    return TaskConstants.TYPE_EXPERIMENT;
  },

  get endDate() {
    return this._endDate;
  },

  get startDate() {
    return this._startDate;
  },

  get dataStore() {
    return this._dataStore;
  },

  get currentStatusUrl() {
    let param = "?eid=" + this._id;
    return "chrome://testpilot/content/status.html" + param;
  },

  get defaultUrl() {
    return this.currentStatusUrl;
  },

  get recurPref() {
    let prefName = RECUR_PREF_PREFIX + this._id;
    return Application.prefs.getValue(prefName, TaskConstants.ASK_EACH_TIME);
  },

  getDataStoreAsJSON: function(callback) {
    this._dataStore.getAllDataAsJSON(false, callback);
  },

  getWebContent: function TestPilotExperiment_getWebContent(callback) {
    let content = "";
    let waitForData = false;
    let self = this;

    switch (this._status) {
      case TaskConstants.STATUS_NEW:
      case TaskConstants.STATUS_PENDING:
        content = this.webContent.upcomingHtml;
      break;
      case TaskConstants.STATUS_STARTING:
      case TaskConstants.STATUS_IN_PROGRESS:
        content = this.webContent.inProgressHtml;
      break;
      case TaskConstants.STATUS_FINISHED:
	waitForData = true;
	this._dataStore.haveData(function(withData) {
	  if (withData) {
	    content = self.webContent.completedHtml;
	  } else {
	    
            let stringBundle =
              Components.classes["@mozilla.org/intl/stringbundle;1"].
                getService(Components.interfaces.nsIStringBundleService).
	          createBundle("chrome://testpilot/locale/main.properties");
	    let link =
	      '<a href="' + self.infoPageUrl + '">' + self.title + '</a>';
	    content =
	      '<h2>' + stringBundle.formatStringFromName(
	        "testpilot.finishedTask.finishedStudy", [link], 1) + '</h2>' +
	      '<p>' + stringBundle.GetStringFromName(
	        "testpilot.finishedTask.allRelatedDataDeleted") + '</p>';
	  }
	  callback(content);
	});
      break;
      case TaskConstants.STATUS_CANCELLED:
	if (this._expirationDateForDataSubmission.length == 0) {
          content = this.webContent.canceledHtml;
	} else {
          content = this.webContent.dataExpiredHtml;
	}
      break;
      case TaskConstants.STATUS_SUBMITTED:
	if (this._dateForDataDeletion.length > 0) {
          content = this.webContent.remainDataHtml;
	} else {
          content = this.webContent.deletedRemainDataHtml;
	}
      break;
    }
    
    if (!waitForData) {
      callback(content);
    }
  },

  getDataPrivacyContent: function(callback) {
    let content = "";
    let waitForData = false;
    let self = this;

    switch (this._status) {
      case TaskConstants.STATUS_STARTING:
      case TaskConstants.STATUS_IN_PROGRESS:
        content = this.webContent.inProgressDataPrivacyHtml;
      break;
      case TaskConstants.STATUS_FINISHED:
	waitForData = true;
	this._dataStore.haveData(function(withData) {
	  if (withData) {
	    content = self.webContent.completedDataPrivacyHtml;
	  }
	  callback(content);
	});
      break;
      case TaskConstants.STATUS_CANCELLED:
	if (this._expirationDateForDataSubmission.length == 0) {
          content = this.webContent.canceledDataPrivacyHtml;
	} else {
          content = this.webContent.dataExpiredDataPrivacyHtml;
	}
      break;
      case TaskConstants.STATUS_SUBMITTED:
	if (this._dateForDataDeletion.length > 0) {
          content = this.webContent.remainDataDataPrivacyHtml;
	} else {
          content = this.webContent.deletedRemainDataDataPrivacyHtml;
	}
      break;
    }
    if (!waitForData) {
      callback(content);
    }
  },

  experimentIsRunning: function TestPilotExperiment_isRunning() {
    
    return (this._status == TaskConstants.STATUS_STARTING ||
            this._status == TaskConstants.STATUS_IN_PROGRESS);
  },

  
  onNewWindow: function TestPilotExperiment_onNewWindow(window) {
    this._logger.trace("Experiment.onNewWindow called.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onNewWindow(window);
      } catch(e) {
        this._dataStore.logException("onNewWindow: " + e);
      }
    }
  },

  onWindowClosed: function TestPilotExperiment_onWindowClosed(window) {
    this._logger.trace("Experiment.onWindowClosed called.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onWindowClosed(window);
      } catch(e) {
        this._dataStore.logException("onWindowClosed: " + e);
      }
    }
  },

  onAppStartup: function TestPilotExperiment_onAppStartup() {
    this._logger.trace("Experiment.onAppStartup called.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onAppStartup();
      } catch(e) {
        this._dataStore.logException("onAppStartup: " + e);
      }
    }
  },

  onAppShutdown: function TestPilotExperiment_onAppShutdown() {
    this._logger.trace("Experiment.onAppShutdown called.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onAppShutdown();
      } catch(e) {
        this._dataStore.logException("onAppShutdown: " + e);
      }
    }
  },

  onExperimentStartup: function TestPilotExperiment_onStartup() {
    this._logger.trace("Experiment.onExperimentStartup called.");
    
    if (this.experimentIsRunning() && !this._startedUpHandlers) {
      this._logger.trace("  ... starting up handlers!");
      try {
        this._handlers.onExperimentStartup(this._dataStore);
      } catch(e) {
        this._dataStore.logException("onExperimentStartup: " + e);
      }
      this._startedUpHandlers = true;
    }
  },

  onExperimentShutdown: function TestPilotExperiment_onShutdown() {
    this._logger.trace("Experiment.onExperimentShutdown called.");
    if (this.experimentIsRunning() && this._startedUpHandlers) {
      try {
        this._handlers.onExperimentShutdown();
      } catch(e) {
        this._dataStore.logException("onExperimentShutdown: " + e);
      }
      this._startedUpHandlers = false;
    }
  },

  doExperimentCleanup: function TestPilotExperiment_doExperimentCleanup() {
    if (this._handlers.doExperimentCleanup) {
      this._logger.trace("Doing experiment cleanup.");
      try {
        this._handlers.doExperimentCleanup();
      } catch(e) {
        this._dataStore.logException("doExperimentCleanup: " + e);
      }
    }
  },

  onEnterPrivateBrowsing: function TestPilotExperiment_onEnterPrivate() {
    this._logger.trace("Task is entering private browsing.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onEnterPrivateBrowsing();
      } catch(e) {
        this._dataStore.logException("onEnterPrivateBrowsing: " + e);
      }
    }
  },

  onExitPrivateBrowsing: function TestPilotExperiment_onExitPrivate() {
    this._logger.trace("Task is exiting private browsing.");
    if (this.experimentIsRunning()) {
      try {
        this._handlers.onExitPrivateBrowsing();
      } catch(e) {
        this._dataStore.logException("onExitPrivateBrowsing: " + e);
      }
    }
  },

  getStudyMetadata: function TestPilotExperiment_getStudyMetadata() {
    try {
      if (this._handlers.getStudyMetadata) {
        let metadata = this._handlers.getStudyMetadata();
        if (metadata.length) {
          
          return metadata;
        }
      }
    } catch(e) {
      this._logger.warn("Error in getStudyMetadata: " + e);
    }
    return null;
  },

  _reschedule: function TestPilotExperiment_reschedule() {
    
    
    let ms = this._recurrenceInterval * (24 * 60 * 60 * 1000);
    
    this._startDate += ms;
    this._endDate += ms;
    let prefName = START_DATE_PREF_PREFIX + this._id;
    Application.prefs.setValue(prefName,
                               (new Date(this._startDate)).toString());
  },

  get _numTimesRun() {
    
    
    
    if (this._recursAutomatically) {
      return Application.prefs.getValue(RECUR_TIMES_PREF_PREFIX + this._id,
                                        1);
    } else {
      return 0;
    }
  },

  set _expirationDateForDataSubmission(date) {
    if (date) {
      Application.prefs.setValue(
        EXPIRATION_DATE_FOR_DATA_SUBMISSION_PREFIX + this._id,
        (new Date(date)).toString());
    } else {
      Application.prefs.setValue(
        EXPIRATION_DATE_FOR_DATA_SUBMISSION_PREFIX + this._id, "");
    }
  },

  get _expirationDateForDataSubmission() {
    return Application.prefs.getValue(
      EXPIRATION_DATE_FOR_DATA_SUBMISSION_PREFIX + this._id, "");
  },

  set _dateForDataDeletion(date) {
    if (date) {
      Application.prefs.setValue(
        DATE_FOR_DATA_DELETION_PREFIX + this._id, (new Date(date)).toString());
    } else {
      Application.prefs.setValue(DATE_FOR_DATA_DELETION_PREFIX + this._id, "");
    }
  },

  get _dateForDataDeletion() {
    return Application.prefs.getValue(
      DATE_FOR_DATA_DELETION_PREFIX + this._id, "");
  },

  checkDate: function TestPilotExperiment_checkDate() {
    
    
    let currentDate = this._now();

    
    if (this._recursAutomatically &&
        this._status >= TaskConstants.STATUS_FINISHED &&
        currentDate >= this._startDate &&
	currentDate <= this._endDate) {
      
      
      if (this.recurPref == TaskConstants.NEVER_SUBMIT) {
        this._logger.info("recurPref is never submit, so I'm rescheduling.");
        this._reschedule();
      } else {
        
        this.changeStatus(TaskConstants.STATUS_NEW, true);

        
        let numTimesRun = this._numTimesRun;
        numTimesRun++;
        this._logger.trace("Test recurring... incrementing " + RECUR_TIMES_PREF_PREFIX + this._id + " to " + numTimesRun);
        Application.prefs.setValue( RECUR_TIMES_PREF_PREFIX + this._id,
                                    numTimesRun );
        this._logger.trace("Incremented it.");
      }
    }

    
    
    if (!this._optInRequired &&
        !Application.prefs.getValue("extensions.testpilot.popup.showOnNewStudy",
                                    false) &&
        (this._status == TaskConstants.STATUS_NEW ||
         this._status == TaskConstants.STATUS_PENDING)) {
      this._logger.info("Skipping pending and going straight to starting.");
      this.changeStatus(TaskConstants.STATUS_STARTING, true);
    }

    
    
    if ( this._status == TaskConstants.STATUS_STARTING &&
        currentDate >= this._startDate &&
        currentDate <= this._endDate) {
      this._logger.info("Study now starting.");
      
      let self = this;
      this._dataStore.wipeAllData(function() {
        
        self.changeStatus(TaskConstants.STATUS_IN_PROGRESS, true);
        self.onExperimentStartup();
      });
    }

    
    if (this._status < TaskConstants.STATUS_FINISHED &&
	currentDate > this._endDate) {
      let self = this;
      let setDataDeletionDate = true;
      this._logger.info("Passed End Date - Switched Task Status to Finished");
      this.changeStatus(TaskConstants.STATUS_FINISHED);
      this.onExperimentShutdown();
      this.doExperimentCleanup();

      if (this._recursAutomatically) {
        this._reschedule();
        
        
        if (this.recurPref == TaskConstants.ALWAYS_SUBMIT) {
          this._logger.info("Automatically Uploading Data");
          this.upload(function(success) {
            Observers.notify("testpilot:task:dataAutoSubmitted", self, null);
	  });
        } else if (this.recurPref == TaskConstants.NEVER_SUBMIT) {
          this._logger.info("Automatically opting out of uploading data");
          this.changeStatus(TaskConstants.STATUS_CANCELLED, true);
          this._dataStore.wipeAllData();
	  setDataDeletionDate = false;
        } else {
          if (Application.prefs.getValue(
              "extensions.testpilot.alwaysSubmitData", false)) {
            this.upload(function(success) {
	      if (success) {
                Observers.notify(
		  "testpilot:task:dataAutoSubmitted", self, null);
	      }
	    });
          }
	}
      } else {
        if (Application.prefs.getValue(
            "extensions.testpilot.alwaysSubmitData", false)) {
          this.upload(function(success) {
	    if (success) {
              Observers.notify("testpilot:task:dataAutoSubmitted", self, null);
	    }
	  });
        }
      }
      if (setDataDeletionDate) {
	let date = this._endDate + TIME_FOR_DATA_DELETION;
        this._dateForDataDeletion = date;
        this._expirationDateForDataSubmission = date;
      } else {
        this._dateForDataDeletion = null;
        this._expirationDateForDataSubmission = null;
      }
    } else {
      
      if (this._status == TaskConstants.STATUS_FINISHED) {
	if (Application.prefs.getValue(
	    "extensions.testpilot.alwaysSubmitData", false)) {
          this.upload(function(success) {
	    if (success) {
              Observers.notify("testpilot:task:dataAutoSubmitted", self, null);
	    }
	  });
        } else if (this._expirationDateForDataSubmission.length > 0) {
	  let expirationDate = Date.parse(this._expirationDateForDataSubmission);
	  if (currentDate > expirationDate) {
            this.changeStatus(TaskConstants.STATUS_CANCELLED, true);
            this._dataStore.wipeAllData();
	    this._dateForDataDeletion = null;
	    
	    
	  }
	}
      } else if (this._status == TaskConstants.STATUS_SUBMITTED) {
	if (this._dateForDataDeletion.length > 0) {
	  let deleteDate = Date.parse(this._dateForDataDeletion);
	  if (currentDate > deleteDate) {
            this._dataStore.wipeAllData();
	    this._dateForDataDeletion = null;
	  }
	}
      }
    }
  },

  _prependMetadataToJSON: function TestPilotExperiment__prependToJson(callback) {
    let json = {};
    let self = this;
    MetadataCollector.getMetadata(function(md) {
      json.metadata = md;
      json.metadata.task_guid = self.getGuid(self._id);
      json.metadata.event_headers = self._dataStore.getPropertyNames();
      let moreMd = self.getStudyMetadata();
      if (moreMd) {
        for (let i = 0; i < moreMd.length; i++) {
          if (moreMd[i].name && moreMd[i].value) {
            json.metadata[ moreMd[i].name ] = moreMd[i].value; 
            
          }
        }
      }
      self._dataStore.getJSONRows(function(rows) {
        json.events = rows;
        self._dataStore.getExceptionsAsJson(function(errs) {
          json.exceptions = errs;
          callback( JSON.stringify(json) );
        });
      });
    });
  },

  
  
  upload: function TestPilotExperiment_upload(callback, retryCount) {
    
    

    


    if (this._status >= TaskConstants.STATUS_SUBMITTED) {
      callback(true);
      return;
    }

    
    let self = this;
    let url = self.uploadUrl;
    self._logger.info("Posting data to url " + url + "\n");
    self._prependMetadataToJSON( function(dataString) {
      let req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance( Ci.nsIXMLHttpRequest );
      req.open('POST', url, true);
      req.setRequestHeader("Content-type", "application/json");
      req.setRequestHeader("Content-length", dataString.length);
      req.setRequestHeader("Connection", "close");
      req.onreadystatechange = function(aEvt) {
        if (req.readyState == 4) {
          if (req.status == 200 || req.status == 201 || req.status == 202) {
            let location = req.getResponseHeader("Location");
  	    self._logger.info("DATA WAS POSTED SUCCESSFULLY " + location);
            if (self._uploadRetryTimer) {
              self._uploadRetryTimer.cancel(); 
            }
            self.changeStatus(TaskConstants.STATUS_SUBMITTED);
            self._dateForDataDeletion = self._now() + TIME_FOR_DATA_DELETION;
            self._expirationDateForDataSubmission = null;
            callback(true);
          } else {
          







            
            
            self._logger.warn("ERROR POSTING DATA: " + req.responseText);
            self._uploadRetryTimer = Cc["@mozilla.org/timer;1"]
              .createInstance(Ci.nsITimer);

            if (!retryCount) {
	      retryCount = 0;
            }
            let interval =
	      Application.prefs.getValue(RETRY_INTERVAL_PREF, 3600000); 
            let delay =
              parseInt(Math.random() * Math.pow(2, retryCount) * interval);
            self._uploadRetryTimer.initWithCallback(
              { notify: function(timer) {
		self.upload(callback, retryCount++);
	      } }, (interval + delay), Ci.nsITimer.TYPE_ONE_SHOT);
            callback(false);
          }
        }
      };
      req.send(dataString);
    });
  },

  optOut: function TestPilotExperiment_optOut(reason, callback) {
    
    
    let url = Application.prefs.getValue(DATA_UPLOAD_PREF, "") + "opt-out";
    let logger = this._logger;

    this.onExperimentShutdown();
    this.changeStatus(TaskConstants.STATUS_CANCELLED);
    this._dataStore.wipeAllData();
    this.doExperimentCleanup();
    this._dateForDataDeletion = null;
    this._expirationDateForDataSubmission = null;
    logger.info("Opting out of test with reason " + reason);
    if (reason) {
      
      
      let answer = {id: this._id,
                    reason: reason};
      let dataString = JSON.stringify(answer);
      var req =
        Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
	  createInstance(Ci.nsIXMLHttpRequest);
      logger.trace("Posting " + dataString + " to " + url);
      req.open('POST', url, true);
      req.setRequestHeader("Content-type", "application/json");
      req.setRequestHeader("Content-length", dataString.length);
      req.setRequestHeader("Connection", "close");
      req.onreadystatechange = function(aEvt) {
        if (req.readyState == 4) {
          if (req.status == 200 || req.status == 201 || req.status == 202) {
	    logger.info("Quit reason posted successfully " + req.responseText);
    	    callback(true);
	  } else {
	    logger.warn(req.status + " posting error " + req.responseText);
	    callback(false);
	  }
	}
      };
      logger.trace("Sending quit reason.");
      req.send(dataString);
    } else {
      callback(false);
    }
  },

  setRecurPref: function TPE_setRecurPrefs(value) {
    
    let prefName = RECUR_PREF_PREFIX + this._id;
    this._logger.info("Setting recur pref to " + value);
    Application.prefs.setValue(prefName, value);
  }
};
TestPilotExperiment.prototype.__proto__ = TestPilotTask;

function TestPilotBuiltinSurvey(surveyInfo) {
  this._init(surveyInfo);
}
TestPilotBuiltinSurvey.prototype = {
  _init: function TestPilotBuiltinSurvey__init(surveyInfo) {
    this._taskInit(surveyInfo.surveyId,
                   surveyInfo.surveyName,
                   surveyInfo.surveyUrl,
                   surveyInfo.summary,
                   surveyInfo.thumbnail);
    this._studyId = surveyInfo.uploadWithExperiment; 
    this._versionNumber = surveyInfo.versionNumber;
    this._questions = surveyInfo.surveyQuestions;
    this._explanation = surveyInfo.surveyExplanation;
  },

  get taskType() {
    return TaskConstants.TYPE_SURVEY;
  },

  get surveyExplanation() {
    return this._explanation;
  },

  get surveyQuestions() {
    return this._questions;
  },

  get currentStatusUrl() {
    let param = "?eid=" + this._id;
    return "chrome://testpilot/content/take-survey.html" + param;
  },

  get defaultUrl() {
    return this.currentStatusUrl;
  },

  get relatedStudyId() {
    return this._studyId;
  },

  onDetailPageOpened: function TPS_onDetailPageOpened() {
    if (this._status < TaskConstants.STATUS_IN_PROGRESS) {
      this.changeStatus( TaskConstants.STATUS_IN_PROGRESS, true );
    }
  },

  get oldAnswers() {
    let surveyResults =
      Application.prefs.getValue(SURVEY_ANSWER_PREFIX + this._id, null);
    if (!surveyResults) {
      return null;
    } else {
      this._logger.info("Trying to json.parse this: " + surveyResults);
      return sanitizeJSONStrings( JSON.parse(surveyResults) );
    }
  },

  store: function TestPilotSurvey_store(surveyResults, callback) {
    





    surveyResults = sanitizeJSONStrings(surveyResults);
    let prefName = SURVEY_ANSWER_PREFIX + this._id;
    
    if (this._versionNumber) {
      surveyResults["version_number"] = this._versionNumber;
    }
    Application.prefs.setValue(prefName, JSON.stringify(surveyResults));
    if (this._studyId) {
      this._upload(callback, 0);
    } else {
      this.changeStatus(TaskConstants.STATUS_SUBMITTED);
      callback(true);
    }
  },

  _prependMetadataToJSON: function TestPilotSurvey__prependToJson(callback) {
    let json = {};
    let self = this;
    MetadataCollector.getMetadata(function(md) {
      json.metadata = md;
      if (self._studyId) {
        
        
        json.metadata.task_guid = self.getGuid(self._studyId);
      }
      let pref = SURVEY_ANSWER_PREFIX + self._id;
      let surveyAnswers = JSON.parse(Application.prefs.getValue(pref, "{}"));
      json.survey_data = sanitizeJSONStrings(surveyAnswers);
      callback(JSON.stringify(json));
    });
  },

  
  
  _upload: function TestPilotSurvey__upload(callback, retryCount) {
    let self = this;
    this._prependMetadataToJSON(function(params) {
      let req =
        Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
          createInstance(Ci.nsIXMLHttpRequest);
      let url = self.uploadUrl;
      req.open("POST", url, true);
      req.setRequestHeader("Content-type", "application/json");
      req.setRequestHeader("Content-length", params.length);
      req.setRequestHeader("Connection", "close");
      req.onreadystatechange = function(aEvt) {
        if (req.readyState == 4) {
          if (req.status == 200 || req.status == 201 ||
             req.status == 202) {
            self._logger.info(
	    "DATA WAS POSTED SUCCESSFULLY " + req.responseText);
            if (self._uploadRetryTimer) {
              self._uploadRetryTimer.cancel(); 
	    }
            self.changeStatus(TaskConstants.STATUS_SUBMITTED);
	    callback(true);
	  } else {
	    self._logger.warn(req.status + " ERROR POSTING DATA: " + req.responseText);
	    self._uploadRetryTimer =
	      Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

	    if (!retryCount) {
              retryCount = 0;
	    }
	    let interval =
              Application.prefs.getValue(RETRY_INTERVAL_PREF, 3600000); 
	    let delay =
	      parseInt(Math.random() * Math.pow(2, retryCount) * interval);
	    self._uploadRetryTimer.initWithCallback(
              { notify: function(timer) {
	          self._upload(callback, retryCount++);
	        } }, (interval + delay), Ci.nsITimer.TYPE_ONE_SHOT);
	    callback(false);
	  }
        }
      };
      req.send(params);
    });
  }
};
TestPilotBuiltinSurvey.prototype.__proto__ = TestPilotTask;

function TestPilotWebSurvey(surveyInfo) {
  this._init(surveyInfo);
}
TestPilotWebSurvey.prototype = {
  _init: function TestPilotWebSurvey__init(surveyInfo) {
    this._taskInit(surveyInfo.surveyId,
                   surveyInfo.surveyName,
                   surveyInfo.surveyUrl,
                   surveyInfo.summary,
                   surveyInfo.thumbnail);
    this._logger.info("Initing survey.  This._status is " + this._status);
  },

  get taskType() {
    return TaskConstants.TYPE_SURVEY;
  },

  get defaultUrl() {
    return this.infoPageUrl;
  },

  onDetailPageOpened: function TPWS_onDetailPageOpened() {
    



    if (this._status < TaskConstants.STATUS_SUBMITTED) {
      this.changeStatus( TaskConstants.STATUS_SUBMITTED, true );
    }
  }
};
TestPilotWebSurvey.prototype.__proto__ = TestPilotTask;


function TestPilotStudyResults(resultsInfo) {
  this._init(resultsInfo);
};
TestPilotStudyResults.prototype = {
  _init: function TestPilotStudyResults__init(resultsInfo) {
    this._taskInit( resultsInfo.id,
                    resultsInfo.title,
                    resultsInfo.url,
                    resultsInfo.summary,
                    resultsInfo.thumbnail);
    this._studyId = resultsInfo.studyId; 
    this._pubDate = Date.parse(resultsInfo.date);
  },

  get taskType() {
    return TaskConstants.TYPE_RESULTS;
  },

  get publishDate() {
    return this._pubDate;
  },

  get relatedStudyId() {
    return this._studyId;
  }
};
TestPilotStudyResults.prototype.__proto__ = TestPilotTask;

function TestPilotLegacyStudy(studyInfo) {
  this._init(studyInfo);
};
TestPilotLegacyStudy.prototype = {
  _init: function TestPilotLegacyStudy__init(studyInfo) {
    let stat = Application.prefs.getValue(STATUS_PREF_PREFIX + studyInfo.id,
                                          null);
    this._taskInit( studyInfo.id,
                    studyInfo.name,
                    studyInfo.url,
                    studyInfo.summary,
                    studyInfo.thumbnail );

    



    switch (stat) {
    case TaskConstants.STATUS_CANCELLED:
    case TaskConstants.STATUS_ARCHIVED:
    case TaskConstants.STATUS_MISSED:
      
      break;
    case TaskConstants.STATUS_SUBMITTED:
      
      this.changeStatus(TaskConstants.STATUS_ARCHIVED, true);
      break;
    default:
      
      this.changeStatus(TaskConstants.STATUS_MISSED, true);
    }

    if (studyInfo.duration) {
      let prefName = START_DATE_PREF_PREFIX + this._id;
      let startDateString = Application.prefs.getValue(prefName, null);
      if (startDateString) {
        this._startDate = Date.parse(startDateString);
        this._endDate = this._startDate + duration * (24 * 60 * 60 * 1000);
      }
    }
  },

  get taskType() {
    return TaskConstants.TYPE_LEGACY;
  }
  
  
};
TestPilotLegacyStudy.prototype.__proto__ = TestPilotTask;