






































EXPORTED_SYMBOLS = ["TestPilotSetup", "POPUP_SHOW_ON_NEW",
                    "POPUP_SHOW_ON_FINISH", "POPUP_SHOW_ON_RESULTS",
                    "ALWAYS_SUBMIT_DATA", "RUN_AT_ALL_PREF"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const EXTENSION_ID = "testpilot@labs.mozilla.com";
const VERSION_PREF ="extensions.testpilot.lastversion";
const FIRST_RUN_PREF ="extensions.testpilot.firstRunUrl";
const RUN_AT_ALL_PREF = "extensions.testpilot.runStudies";
const POPUP_SHOW_ON_NEW = "extensions.testpilot.popup.showOnNewStudy";
const POPUP_SHOW_ON_FINISH = "extensions.testpilot.popup.showOnStudyFinished";
const POPUP_SHOW_ON_RESULTS = "extensions.testpilot.popup.showOnNewResults";
const POPUP_CHECK_INTERVAL = "extensions.testpilot.popup.delayAfterStartup";
const POPUP_REMINDER_INTERVAL = "extensions.testpilot.popup.timeBetweenChecks";
const ALWAYS_SUBMIT_DATA = "extensions.testpilot.alwaysSubmitData";
const UPDATE_CHANNEL_PREF = "app.update.channel";
const LOG_FILE_NAME = "TestPilotErrorLog.log";
const RANDOM_DEPLOY_PREFIX = "extensions.testpilot.deploymentRandomizer";

let TestPilotSetup = {
  didReminderAfterStartup: false,
  startupComplete: false,
  _shortTimer: null,
  _longTimer: null,
  _remoteExperimentLoader: null, 
  taskList: [],
  version: "",

  
  __application: null,
  get _application() {
    if (this.__application == null) {
      this.__application = Cc["@mozilla.org/fuel/application;1"]
                             .getService(Ci.fuelIApplication);
    }
    return this.__application;
  },

  get _prefs() {
    return this._application.prefs;
  },

  __loader: null,
  get _loader() {
    if (this.__loader == null) {
      let Cuddlefish = {};
      Components.utils.import("resource://testpilot/modules/lib/cuddlefish.js",
                        Cuddlefish);
      let repo = this._logRepo;
      this.__loader = new Cuddlefish.Loader(
          {rootPaths: ["resource://testpilot/modules/",
                     "resource://testpilot/modules/lib/"],
           console: repo.getLogger("TestPilot.Loader")
      });
    }
    return this.__loader;
  },

  __feedbackManager: null,
  get _feedbackManager() {
    if (this.__feedbackManager == null) {
      let FeedbackModule = {};
      Cu.import("resource://testpilot/modules/feedback.js", FeedbackModule);
      this.__feedbackManager = FeedbackModule.FeedbackManager;
    }
    return this.__feedbackManager;
  },

  __dataStoreModule: null,
  get _dataStoreModule() {
    if (this.__dataStoreModule == null) {
      this.__dataStoreModule = {};
      Cu.import("resource://testpilot/modules/experiment_data_store.js",
                  this._dataStoreModule);
    }
    return this.__dataStoreModule;
  },

  __extensionUpdater: null,
  get _extensionUpdater() {
    if (this.__extensionUpdater == null) {
      let ExUpdate = {};
      Cu.import("resource://testpilot/modules/extension-update.js",
                   ExUpdate);
      this.__extensionUpdater = ExUpdate.TestPilotExtensionUpdate;
    }
    return this.__extensionUpdater;
  },

  __logRepo: null,
  get _logRepo() {
    
    
    if (this.__logRepo == null) {
      let Log4MozModule = {};
      Cu.import("resource://testpilot/modules/log4moz.js", Log4MozModule);
      let props = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties);
      let logFile = props.get("ProfD", Components.interfaces.nsIFile);
      logFile.append(LOG_FILE_NAME);
      let formatter = new Log4MozModule.Log4Moz.BasicFormatter;
      let root = Log4MozModule.Log4Moz.repository.rootLogger;
      root.level = Log4MozModule.Log4Moz.Level["All"];
      let appender = new Log4MozModule.Log4Moz.RotatingFileAppender(logFile, formatter);
      root.addAppender(appender);
      this.__logRepo = Log4MozModule.Log4Moz.repository;
    }
    return this.__logRepo;
  },

  __logger: null,
  get _logger() {
    if (this.__logger == null) {
      this.__logger = this._logRepo.getLogger("TestPilot.Setup");
    }
    return this.__logger;
  },

  __taskModule: null,
  get _taskModule() {
    if (this.__taskModule == null) {
      this.__taskModule = {};
      Cu.import("resource://testpilot/modules/tasks.js", this.__taskModule);
    }
    return this.__taskModule;
  },

  __stringBundle: null,
  get _stringBundle() {
    if (this.__stringBundle == null) {
      this.__stringBundle =
      Cc["@mozilla.org/intl/stringbundle;1"].
        getService(Ci.nsIStringBundleService).
          createBundle("chrome://testpilot/locale/main.properties");
    }
    return this.__stringBundle;
  },

  __obs: null,
  get _obs() {
    if (this.__obs == null) {
      this.__obs = this._loader.require("observer-service");
    }
    return this.__obs;
  },

  _isBetaChannel: function TPS__isBetaChannel() {
    
    let channel = this._prefs.getValue(UPDATE_CHANNEL_PREF, "");
    return (channel == "beta") || (channel == "betatest") || (channel == "aurora");
  },

  _setPrefDefaultsForVersion: function TPS__setPrefDefaultsForVersion() {
    


    let ps = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefService);
    let prefBranch = ps.getDefaultBranch("");
    

    if (this._isBetaChannel()) {
      prefBranch.setBoolPref(POPUP_SHOW_ON_NEW, true);
      prefBranch.setIntPref(POPUP_CHECK_INTERVAL, 600000);
    } else {
      prefBranch.setBoolPref(POPUP_SHOW_ON_NEW, false);
      prefBranch.setIntPref(POPUP_CHECK_INTERVAL, 180000);
    }
  },

  globalStartup: function TPS__doGlobalSetup() {
    
    
    let logger = this._logger;
    logger.trace("TestPilotSetup.globalStartup was called.");

    try {
    this._setPrefDefaultsForVersion();
    if (!this._prefs.getValue(RUN_AT_ALL_PREF, true)) {
      logger.trace("Test Pilot globally disabled: Not starting up.");
      return;
    }

    
    var self = this;
    this._obs.add("testpilot:task:changed", this.onTaskStatusChanged, self);
    this._obs.add(
      "testpilot:task:dataAutoSubmitted", this._onTaskDataAutoSubmitted, self);
    
    this._obs.add("quit-application", this.globalShutdown, self);
    
    this._obs.add("private-browsing", this.onPrivateBrowsingMode, self);

    
    
    
    logger.trace("Setting interval for showing reminders...");

    this._shortTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._shortTimer.initWithCallback(
      { notify: function(timer) { self._doHousekeeping();} },
      this._prefs.getValue(POPUP_CHECK_INTERVAL, 180000),
      Ci.nsITimer.TYPE_REPEATING_SLACK
    );
    this._longTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._longTimer.initWithCallback(
      { notify: function(timer) {
          self.reloadRemoteExperiments(function() {
            self._notifyUserOfTasks();
	  });
      }}, this._prefs.getValue(POPUP_REMINDER_INTERVAL, 86400000),
      Ci.nsITimer.TYPE_REPEATING_SLACK);

      this.getVersion(function() {
        

        if ((self._prefs.getValue(VERSION_PREF, "") == "") &&
           (!self._interfaceBuilder.channelUsesFeedback())) {
            self._prefs.setValue(VERSION_PREF, self.version);
            let browser = self._getFrontBrowserWindow().getBrowser();
            let url = self._prefs.getValue(FIRST_RUN_PREF, "");
            let tab = browser.addTab(url);
            browser.selectedTab = tab;
        }

        
        
        self.checkForTasks(function() {
          

         self.startupComplete = true;
         logger.trace("I'm in the callback from checkForTasks.");
         
         for (let i = 0; i < self.taskList.length; i++) {
           self.taskList[i].onAppStartup();
         }
         self._obs.notify("testpilot:startup:complete", "", null);
         

         logger.trace("Testpilot startup complete.");
      });
    });
    } catch(e) {
      logger.error("Error in testPilot startup: " + e);
    }
  },

  globalShutdown: function TPS_globalShutdown() {
    let logger = this._logger;
    logger.trace("Global shutdown.  Unregistering everything.");
    let self = this;
    for (let i = 0; i < self.taskList.length; i++) {
      self.taskList[i].onAppShutdown();
      self.taskList[i].onExperimentShutdown();
    }
    this.taskList = [];
    this._loader.unload();
    this._obs.remove("testpilot:task:changed", this.onTaskStatusChanged, self);
    this._obs.remove(
      "testpilot:task:dataAutoSubmitted", this._onTaskDataAutoSubmitted, self);
    this._obs.remove("quit-application", this.globalShutdown, self);
    this._obs.remove("private-browsing", this.onPrivateBrowsingMode, self);
    this._loader.unload();
    this._shortTimer.cancel();
    this._longTimer.cancel();
    logger.trace("Done unregistering everything.");
  },

  _getFrontBrowserWindow: function TPS__getFrontWindow() {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
               getService(Ci.nsIWindowMediator);
    
    return wm.getMostRecentWindow("navigator:browser");
  },

  onPrivateBrowsingMode: function TPS_onPrivateBrowsingMode(topic, data) {
    for (let i = 0; i < this.taskList.length; i++) {
      if (data == "enter") {
        this.taskList[i].onEnterPrivateBrowsing();
      } else if (data == "exit") {
        this.taskList[i].onExitPrivateBrowsing();
      }
    }
  },

  onWindowUnload: function TPS__onWindowRegistered(window) {
    this._logger.trace("Called TestPilotSetup.onWindow unload!");
    for (let i = 0; i < this.taskList.length; i++) {
      this.taskList[i].onWindowClosed(window);
    }
  },

  onWindowLoad: function TPS_onWindowLoad(window) {
    this._logger.trace("Called TestPilotSetup.onWindowLoad!");
    
    let self = this;

    
    
    let appcontent = window.document.getElementById("appcontent");
    if (appcontent) {
      appcontent.addEventListener("DOMContentLoaded", function(event) {
        let newUrl =  event.originalTarget.URL;
        self._feedbackManager.fillInFeedbackPage(newUrl, window);
        for (i = 0; i < self.taskList.length; i++) {
          self.taskList[i].onUrlLoad(newUrl, event);
        }
      }, true);
    }

    
    for (let i = 0; i < this.taskList.length; i++) {
      this.taskList[i].onNewWindow(window);
    }
  },

  addTask: function TPS_addTask(testPilotTask) {
    
    
    this.taskList.push(testPilotTask);
  },

  _showNotification: function TPS__showNotification(task, fragile, text, title,
                                                    iconClass, showSubmit,
						    showAlwaysSubmitCheckbox,
                                                    linkText, linkUrl,
						    isExtensionUpdate,
                                                    onCloseCallback) {
    



    
    
    let window = this._getFrontBrowserWindow();
    let doc = window.document;
    let popup = doc.getElementById("pilot-notification-popup");

    let anchor;
    if (this._isBetaChannel()) {
      


      anchor = doc.getElementById("feedback-menu-button");
      popup.setAttribute("class", "tail-up");
    } else {
      anchor = doc.getElementById("pilot-notifications-button");
      popup.setAttribute("class", "tail-down");
    }
    let textLabel = doc.getElementById("pilot-notification-text");
    let titleLabel = doc.getElementById("pilot-notification-title");
    let icon = doc.getElementById("pilot-notification-icon");
    let submitBtn = doc.getElementById("pilot-notification-submit");
    let closeBtn = doc.getElementById("pilot-notification-close");
    let link = doc.getElementById("pilot-notification-link");
    let alwaysSubmitCheckbox =
      doc.getElementById("pilot-notification-always-submit-checkbox");
    let self = this;

    
    if (isExtensionUpdate) {
      popup.setAttribute("tpisextensionupdate", "true");
    }
    popup.setAttribute("noautohide", !fragile);
    titleLabel.setAttribute("value", title);
    while (textLabel.lastChild) {
      textLabel.removeChild(textLabel.lastChild);
    }
    textLabel.appendChild(doc.createTextNode(text));
    if (iconClass) {
      
      icon.setAttribute("class", iconClass);
    }

    alwaysSubmitCheckbox.setAttribute("hidden", !showAlwaysSubmitCheckbox);
    if (showSubmit) {
      if (isExtensionUpdate) {
        submitBtn.setAttribute("label",
	  this._stringBundle.GetStringFromName(
	    "testpilot.notification.update"));
	submitBtn.onclick = function() {
          this._extensionUpdater.check(EXTENSION_ID);
          self._hideNotification(window, onCloseCallback);
	};
      } else {
        submitBtn.setAttribute("label",
	  this._stringBundle.GetStringFromName("testpilot.submit"));
        
        submitBtn.onclick = function() {
          self._hideNotification(window, onCloseCallback);
          if (showAlwaysSubmitCheckbox && alwaysSubmitCheckbox.checked) {
            self._prefs.setValue(ALWAYS_SUBMIT_DATA, true);
          }
          task.upload( function(success) {
            if (success) {
              self._showNotification(
		task, true,
                self._stringBundle.GetStringFromName(
		  "testpilot.notification.thankYouForUploadingData.message"),
                self._stringBundle.GetStringFromName(
		  "testpilot.notification.thankYouForUploadingData"),
		"study-submitted", false, false,
                self._stringBundle.GetStringFromName("testpilot.moreInfo"),
		task.defaultUrl);
            } else {
              
            }
          });
        };
      }
    }
    submitBtn.setAttribute("hidden", !showSubmit);

    
    if (linkText && (linkUrl || task)) {
      link.setAttribute("value", linkText);
      link.setAttribute("class", "notification-link");
      link.onclick = function(event) {
        if (event.button == 0) {
	  if (task) {
            task.loadPage();
	  } else {
            self._openChromeless(linkUrl);
	  }
          self._hideNotification(window, onCloseCallback);
        }
      };
      link.setAttribute("hidden", false);
    } else {
      link.setAttribute("hidden", true);
    }

    closeBtn.onclick = function() {
      self._hideNotification(window, onCloseCallback);
    };

    
    popup.hidden = false;
    popup.setAttribute("open", "true");
    popup.openPopup( anchor, "after_end");
  },

  _openChromeless: function TPS__openChromeless(url) {
    let window = this._getFrontBrowserWindow();
    window.TestPilotWindowUtils.openChromeless(url);
  },

  _hideNotification: function TPS__hideNotification(window, onCloseCallback) {
    



    let popup = window.document.getElementById("pilot-notification-popup");
    popup.hidden = true;
    popup.setAttribute("open", "false");
    popup.removeAttribute("tpisextensionupdate");
    popup.hidePopup();
    if (onCloseCallback) {
      onCloseCallback();
    }
  },

  _isShowingUpdateNotification : function() {
    let window = this._getFrontBrowserWindow();
    let popup = window.document.getElementById("pilot-notification-popup");

    return popup.hasAttribute("tpisextensionupdate");
  },

  _notifyUserOfTasks: function TPS__notifyUser() {
    
    
    let i, task;
    let TaskConstants = this._taskModule.TaskConstants;

    
    if (this._isShowingUpdateNotification()) {
      return;
    }

    
    if (this._prefs.getValue(POPUP_SHOW_ON_FINISH, false)) {
      for (i = 0; i < this.taskList.length; i++) {
        task = this.taskList[i];
        if (task.status == TaskConstants.STATUS_FINISHED) {
          if (!this._prefs.getValue(ALWAYS_SUBMIT_DATA, false)) {
            this._showNotification(
	      task, false,
	      this._stringBundle.formatStringFromName(
		"testpilot.notification.readyToSubmit.message", [task.title],
		1),
	      this._stringBundle.GetStringFromName(
		"testpilot.notification.readyToSubmit"),
	      "study-finished", true, true,
	      this._stringBundle.GetStringFromName("testpilot.moreInfo"),
	      task.defaultUrl);
            
            
            return;
          }
        }
      }
    }

    
    
    if (this._prefs.getValue(POPUP_SHOW_ON_NEW, false)) {
      for (i = 0; i < this.taskList.length; i++) {
        task = this.taskList[i];
        if (task.status == TaskConstants.STATUS_PENDING ||
            task.status == TaskConstants.STATUS_NEW) {
          if (task.taskType == TaskConstants.TYPE_EXPERIMENT) {
	    this._showNotification(
	      task, false,
	      this._stringBundle.formatStringFromName(
		"testpilot.notification.newTestPilotStudy.pre.message",
		[task.title], 1),
	      this._stringBundle.GetStringFromName(
		"testpilot.notification.newTestPilotStudy"),
	      "new-study", false, false,
	      this._stringBundle.GetStringFromName("testpilot.moreInfo"),
	      task.defaultUrl, false, function() {
                

                task.changeStatus(TaskConstants.STATUS_STARTING, true);
                TestPilotSetup.reloadRemoteExperiments();
              });
            return;
          } else if (task.taskType == TaskConstants.TYPE_SURVEY) {
	    this._showNotification(
	      task, false,
	      this._stringBundle.formatStringFromName(
		"testpilot.notification.newTestPilotSurvey.message",
		[task.title], 1),
              this._stringBundle.GetStringFromName(
		"testpilot.notification.newTestPilotSurvey"),
	      "new-study", false, false,
	      this._stringBundle.GetStringFromName("testpilot.moreInfo"),
	      task.defaultUrl);
            task.changeStatus(TaskConstants.STATUS_IN_PROGRESS, true);
            return;
          }
        }
      }
    }

    
    if (this._prefs.getValue(POPUP_SHOW_ON_RESULTS, false)) {
      for (i = 0; i < this.taskList.length; i++) {
        task = this.taskList[i];
        if (task.taskType == TaskConstants.TYPE_RESULTS &&
            task.status == TaskConstants.STATUS_NEW) {
	  this._showNotification(
	    task, true,
	    this._stringBundle.formatStringFromName(
	      "testpilot.notification.newTestPilotResults.message",
	      [task.title], 1),
            this._stringBundle.GetStringFromName(
	      "testpilot.notification.newTestPilotResults"),
	    "new-results", false, false,
	    this._stringBundle.GetStringFromName("testpilot.moreInfo"),
	    task.defaultUrl);
          
          
          task.changeStatus(TaskConstants.STATUS_ARCHIVED, true);
          return;
        }
      }
    }
  },

  _doHousekeeping: function TPS__doHousekeeping() {
    
    for (let i = 0; i < this.taskList.length; i++) {
      let task = this.taskList[i];
      task.checkDate();
    }
    
    if (!this.didReminderAfterStartup) {
      this._logger.trace("Doing reminder after startup...");
      this.didReminderAfterStartup = true;
      this._notifyUserOfTasks();
    }
  },

  onTaskStatusChanged: function TPS_onTaskStatusChanged() {
    this._notifyUserOfTasks();
  },

  _onTaskDataAutoSubmitted: function(subject, data) {
    this._showNotification(
      subject, true,
      this._stringBundle.formatStringFromName(
	"testpilot.notification.autoUploadedData.message",
	[subject.title], 1),
      this._stringBundle.GetStringFromName(
	"testpilot.notification.autoUploadedData"),
      "study-submitted", false, false,
      this._stringBundle.GetStringFromName("testpilot.moreInfo"),
      subject.defaultUrl);
  },

  getVersion: function TPS_getVersion(callback) {
    
    
    
    if (this._application.extensions) {
      this.version = this._application.extensions.get(EXTENSION_ID).version;
      callback();
    } else {
      let self = this;
      self._application.getExtensions(function(extensions) {
        self.version = extensions.get(EXTENSION_ID).version;
        callback();
      });
    }
  },

  _isNewerThanMe: function TPS__isNewerThanMe(versionString) {
    let result = Cc["@mozilla.org/xpcom/version-comparator;1"]
                   .getService(Ci.nsIVersionComparator)
                   .compare(this.version, versionString);
    if (result < 0) {
      return true; 
    } else {
      return false; 
    }
  },

  _isNewerThanFirefox: function TPS__isNewerThanFirefox(versionString) {
    let result = Cc["@mozilla.org/xpcom/version-comparator;1"]
                   .getService(Ci.nsIVersionComparator)
                   .compare(this._application.version, versionString);
    if (result < 0) {
      return true; 
    } else {
      return false; 
    }
  },

  _experimentRequirementsAreMet: function TPS__requirementsMet(experiment) {
    




    let logger = this._logger;
    try {
      let minTpVer, minFxVer, expName, runOrNotFunc, randomDeployment;
      

      let info = experiment.experimentInfo ?
                   experiment.experimentInfo :
                   experiment.surveyInfo;
      if (!info) {
        
        logger.warn("Study lacks minimum metadata to run.");
        return false;
      }
      minTpVer = info.minTPVersion;
      minFxVer = info.minFXVersion;
      expName =  info.testName;
      runOrNotFunc = info.runOrNotFunc;
      randomDeployment = info.randomDeployment;

      
      if (minTpVer && this._isNewerThanMe(minTpVer)) {
        logger.warn("Not loading " + expName);
        logger.warn("Because it requires Test Pilot version " + minTpVer);

        
        if (!this._isShowingUpdateNotification()) {
          this._showNotification(
	    null, false,
	    this._stringBundle.GetStringFromName(
	      "testpilot.notification.extensionUpdate.message"),
	    this._stringBundle.GetStringFromName(
	      "testpilot.notification.extensionUpdate"),
	    "update-extension", true, false, "", "", true);
	}
        return false;
      }

      
      if (minFxVer && this._isNewerThanFirefox(minFxVer)) {
        logger.warn("Not loading " + expName);
        logger.warn("Because it requires Firefox version " + minFxVer);
        return false;
      }

      
      if (randomDeployment) {
        


        let prefName = RANDOM_DEPLOY_PREFIX + "." + randomDeployment.rolloutCode;
        let myRoll = this._prefs.getValue(prefName, null);
        if (myRoll == null) {
          myRoll = Math.floor(Math.random()*100);
          this._prefs.setValue(prefName, myRoll);
        }
        if (myRoll < randomDeployment.minRoll) {
          return false;
        }
        if (myRoll > randomDeployment.maxRoll) {
          return false;
        }
      }

      

      if (runOrNotFunc) {
        return runOrNotFunc();
      }
    } catch (e) {
      logger.warn("Error in requirements check " +  e);
    }
    return true;
  },

  checkForTasks: function TPS_checkForTasks(callback) {
    let logger = this._logger;
    if (! this._remoteExperimentLoader ) {
      logger.trace("Now requiring remote experiment loader:");
      let remoteLoaderModule = this._loader.require("remote-experiment-loader");
      logger.trace("Now instantiating remoteExperimentLoader:");
      let rel = new remoteLoaderModule.RemoteExperimentLoader(this._logRepo);
      this._remoteExperimentLoader = rel;
    }

    let self = this;
    this._remoteExperimentLoader.checkForUpdates(
      function(success) {
        logger.info("Getting updated experiments... Success? " + success);
        
        
        let experiments = self._remoteExperimentLoader.getExperiments();

        for (let filename in experiments) {
          if (!self._experimentRequirementsAreMet(experiments[filename])) {
            continue;
          }
          try {
            
            
            logger.trace("Attempting to load experiment " + filename);

            let task;
            
            if (experiments[filename].surveyInfo != undefined) {
              let sInfo = experiments[filename].surveyInfo;
              
              
              if (!sInfo.surveyQuestions) {
                task = new self._taskModule.TestPilotWebSurvey(sInfo);
              } else {
                task = new self._taskModule.TestPilotBuiltinSurvey(sInfo);
              }
            } else {
              
              let expInfo = experiments[filename].experimentInfo;
              let dsInfo = experiments[filename].dataStoreInfo;
              let dataStore = new self._dataStoreModule.ExperimentDataStore(
                dsInfo.fileName, dsInfo.tableName, dsInfo.columns );
              let webContent = experiments[filename].webContent;
              task = new self._taskModule.TestPilotExperiment(expInfo,
                                                              dataStore,
                                                              experiments[filename].handlers,
                                                              webContent);
            }
            self.addTask(task);
            logger.info("Loaded task " + filename);
          } catch (e) {
            logger.warn("Failed to load task " + filename + ": " + e);
          }
        } 

        
        let results = self._remoteExperimentLoader.getStudyResults();
        for (let r in results) {
          let studyResult = new self._taskModule.TestPilotStudyResults(results[r]);
          self.addTask(studyResult);
        }

        

        let legacyStudies = self._remoteExperimentLoader.getLegacyStudies();
        for (let l in legacyStudies) {
          let legacyStudy = new self._taskModule.TestPilotLegacyStudy(legacyStudies[l]);
          self.addTask(legacyStudy);
        }

        if (callback) {
          callback();
        }
      }
    );
  },

  reloadRemoteExperiments: function TPS_reloadRemoteExperiments(callback) {
    for (let i = 0; i < this.taskList.length; i++) {
      this.taskList[i].onExperimentShutdown();
    }

    this.taskList = [];
    this._loader.unload();

    this.checkForTasks(callback);
  },

  getTaskById: function TPS_getTaskById(id) {
    for (let i = 0; i < this.taskList.length; i++) {
      let task = this.taskList[i];
      if (task.id == id) {
	return task;
      }
    }
    return null;
  },

  getAllTasks: function TPS_getAllTasks() {
    return this.taskList;
  }
};
