# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

#ifdef MOZ_SERVICES_CLOUDSYNC
XPCOMUtils.defineLazyModuleGetter(this, "CloudSync",
                                  "resource://gre/modules/CloudSync.jsm");
#else
let CloudSync = null;
#endif

XPCOMUtils.defineLazyModuleGetter(this, "ReadingListScheduler",
                                  "resource:///modules/readinglist/Scheduler.jsm");


let gSyncUI = {
  _obs: ["weave:service:sync:start",
         "weave:service:sync:finish",
         "weave:service:sync:error",
         "weave:service:quota:remaining",
         "weave:service:setup-complete",
         "weave:service:login:start",
         "weave:service:login:finish",
         "weave:service:login:error",
         "weave:service:logout:finish",
         "weave:service:start-over",
         "weave:service:start-over:finish",
         "weave:ui:login:error",
         "weave:ui:sync:error",
         "weave:ui:sync:finish",
         "weave:ui:clear-error",

         "readinglist:sync:start",
         "readinglist:sync:finish",
         "readinglist:sync:error",
  ],

  _unloaded: false,
  
  _numActiveSyncTasks: 0,

  init: function () {
    Cu.import("resource://services-common/stringbundle.js");

    
    
    let xps = Components.classes["@mozilla.org/weave/service;1"]
                                .getService(Components.interfaces.nsISupports)
                                .wrappedJSObject;
    if (xps.ready) {
      this.initUI();
      return;
    }

    Services.obs.addObserver(this, "weave:service:ready", true);

    
    
    window.addEventListener("unload", function onUnload() {
      gSyncUI._unloaded = true;
      window.removeEventListener("unload", onUnload, false);
      Services.obs.removeObserver(gSyncUI, "weave:service:ready");

      if (Weave.Status.ready) {
        gSyncUI._obs.forEach(function(topic) {
          Services.obs.removeObserver(gSyncUI, topic);
        });
      }
    }, false);
  },

  initUI: function SUI_initUI() {
    
    if (gBrowser) {
      this._obs.push("weave:notification:added");
    }

    this._obs.forEach(function(topic) {
      Services.obs.addObserver(this, topic, true);
    }, this);

    if (gBrowser && Weave.Notifications.notifications.length) {
      this.initNotifications();
    }
    this.updateUI();
  },

  initNotifications: function SUI_initNotifications() {
    const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    let notificationbox = document.createElementNS(XULNS, "notificationbox");
    notificationbox.id = "sync-notifications";
    notificationbox.setAttribute("flex", "1");

    let bottombox = document.getElementById("browser-bottombox");
    bottombox.insertBefore(notificationbox, bottombox.firstChild);

    
    notificationbox.clientTop;

    
    Services.obs.removeObserver(this, "weave:notification:added");
    let idx = this._obs.indexOf("weave:notification:added");
    if (idx >= 0) {
      this._obs.splice(idx, 1);
    }
  },

  _needsSetup() {
    
    
    
    
    
    
    if (Weave.Status._authManager._signedInUser !== undefined) {
      
      
      
      
      
      return !Weave.Status._authManager._signedInUser ||
             !Weave.Status._authManager._signedInUser.verified;
    }

    
    
    let firstSync = "";
    try {
      firstSync = Services.prefs.getCharPref("services.sync.firstSync");
    } catch (e) { }

    return Weave.Status.checkSetup() == Weave.CLIENT_NOT_CONFIGURED ||
           firstSync == "notReady";
  },

  _loginFailed: function () {
    this.log.debug("_loginFailed has sync state=${sync}, readinglist state=${rl}",
                   { sync: Weave.Status.login, rl: ReadingListScheduler.state});
    return Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED ||
           ReadingListScheduler.state == ReadingListScheduler.STATE_ERROR_AUTHENTICATION;
  },

  updateUI: function SUI_updateUI() {
    let needsSetup = this._needsSetup();
    let loginFailed = this._loginFailed();

    
    document.getElementById("sync-reauth-state").hidden = true;
    document.getElementById("sync-setup-state").hidden = true;
    document.getElementById("sync-syncnow-state").hidden = true;

    if (CloudSync && CloudSync.ready && CloudSync().adapters.count) {
      document.getElementById("sync-syncnow-state").hidden = false;
    } else if (loginFailed) {
      document.getElementById("sync-reauth-state").hidden = false;
      this.showLoginError();
    } else if (needsSetup) {
      document.getElementById("sync-setup-state").hidden = false;
    } else {
      document.getElementById("sync-syncnow-state").hidden = false;
    }

    if (!gBrowser)
      return;

    let syncButton = document.getElementById("sync-button");
    if (needsSetup && syncButton)
      syncButton.removeAttribute("tooltiptext");

    this._updateLastSyncTime();
  },


  
  onActivityStart() {
    if (!gBrowser)
      return;

    this.log.debug("onActivityStart with numActive", this._numActiveSyncTasks);
    if (++this._numActiveSyncTasks == 1) {
      let button = document.getElementById("sync-button");
      if (button) {
        button.setAttribute("status", "active");
      }
      button = document.getElementById("PanelUI-fxa-status");
      if (button) {
        button.setAttribute("syncstatus", "active");
      }
    }
  },

  onActivityStop() {
    if (!gBrowser)
      return;
    this.log.debug("onActivityStop with numActive", this._numActiveSyncTasks);
    if (--this._numActiveSyncTasks) {
      if (this._numActiveSyncTasks < 0) {
        
        
        
        this.log.error("mismatched onActivityStart/Stop calls",
                       new Error("active=" + this._numActiveSyncTasks));
      }
      return; 
    }

    let syncButton = document.getElementById("sync-button");
    if (syncButton) {
      syncButton.removeAttribute("status");
    }
    let panelHorizontalButton = document.getElementById("PanelUI-fxa-status");
    if (panelHorizontalButton) {
      panelHorizontalButton.removeAttribute("syncstatus");
    }
  },

  onLoginFinish: function SUI_onLoginFinish() {
    
    let title = this._stringBundle.GetStringFromName("error.login.title");
    this.clearError(title);
  },

  onSetupComplete: function SUI_onSetupComplete() {
    this.onLoginFinish();
  },

  onLoginError: function SUI_onLoginError() {
    
    
    Weave.Notifications.removeAll();

    
    if (this._needsSetup()) {
      this.updateUI();
      return;
    }
    
    
    
    
    
    if (Weave.Status.login == Weave.LOGIN_FAILED_NOT_READY ||
        Weave.Status.login == Weave.LOGIN_FAILED_NETWORK_ERROR ||
        Weave.Status.login == Weave.LOGIN_FAILED_SERVER_ERROR) {
      this.updateUI();
      return;
    }
    this.showLoginError();
    this.updateUI();
  },

  showLoginError() {
    
    let title = this._stringBundle.GetStringFromName("error.login.title");

    let description;
    if (Weave.Status.sync == Weave.PROLONGED_SYNC_FAILURE ||
        this.isProlongedReadingListError()) {
      this.log.debug("showLoginError has a prolonged login error");
      
      let lastSync =
        Services.prefs.getIntPref("services.sync.errorhandler.networkFailureReportTimeout") / 86400;
      description =
        this._stringBundle.formatStringFromName("error.sync.prolonged_failure", [lastSync], 1);
    } else {
      let reason = Weave.Utils.getErrorString(Weave.Status.login);
      description =
        this._stringBundle.formatStringFromName("error.sync.description", [reason], 1);
      this.log.debug("showLoginError has a non-prolonged error", reason);
    }

    let buttons = [];
    buttons.push(new Weave.NotificationButton(
      this._stringBundle.GetStringFromName("error.login.prefs.label"),
      this._stringBundle.GetStringFromName("error.login.prefs.accesskey"),
      function() { gSyncUI.openPrefs(); return true; }
    ));

    let notification = new Weave.Notification(title, description, null,
                                              Weave.Notifications.PRIORITY_WARNING, buttons);
    Weave.Notifications.replaceTitle(notification);
  },

  onLogout: function SUI_onLogout() {
    this.updateUI();
  },

  onStartOver: function SUI_onStartOver() {
    this.clearError();
  },

  onQuotaNotice: function onQuotaNotice(subject, data) {
    let title = this._stringBundle.GetStringFromName("warning.sync.quota.label");
    let description = this._stringBundle.GetStringFromName("warning.sync.quota.description");
    let buttons = [];
    buttons.push(new Weave.NotificationButton(
      this._stringBundle.GetStringFromName("error.sync.viewQuotaButton.label"),
      this._stringBundle.GetStringFromName("error.sync.viewQuotaButton.accesskey"),
      function() { gSyncUI.openQuotaDialog(); return true; }
    ));

    let notification = new Weave.Notification(
      title, description, null, Weave.Notifications.PRIORITY_WARNING, buttons);
    Weave.Notifications.replaceTitle(notification);
  },

  _getAppName: function () {
    let brand = new StringBundle("chrome://branding/locale/brand.properties");
    return brand.get("brandShortName");
  },

  openServerStatus: function () {
    let statusURL = Services.prefs.getCharPref("services.sync.statusURL");
    window.openUILinkIn(statusURL, "tab");
  },

  
  doSync: function SUI_doSync() {
    let needsSetup = this._needsSetup();

    if (!needsSetup) {
      setTimeout(function () Weave.Service.errorHandler.syncAndReportErrors(), 0);
    }

    Services.obs.notifyObservers(null, "cloudsync:user-sync", null);
    Services.obs.notifyObservers(null, "readinglist:user-sync", null);
  },

  handleToolbarButton: function SUI_handleStatusbarButton() {
    if (this._needsSetup())
      this.openSetup();
    else
      this.doSync();
  },

  
  

  











  openSetup: function SUI_openSetup(wizardType, entryPoint = "syncbutton") {
    let xps = Components.classes["@mozilla.org/weave/service;1"]
                                .getService(Components.interfaces.nsISupports)
                                .wrappedJSObject;
    if (xps.fxAccountsEnabled) {
      fxAccounts.getSignedInUser().then(userData => {
        if (userData) {
          this.openPrefs();
        } else {
          
          if (UITour.tourBrowsersByWindow.get(window) &&
              UITour.tourBrowsersByWindow.get(window).has(gBrowser.selectedBrowser)) {
            entryPoint = "uitour";
          }
          switchToTabHavingURI("about:accounts?entrypoint=" + entryPoint, true, {
            replaceQueryString: true
          });
        }
      });
    } else {
      let win = Services.wm.getMostRecentWindow("Weave:AccountSetup");
      if (win)
        win.focus();
      else {
        window.openDialog("chrome://browser/content/sync/setup.xul",
                          "weaveSetup", "centerscreen,chrome,resizable=no",
                          wizardType);
      }
    }
  },

  openAddDevice: function () {
    if (!Weave.Utils.ensureMPUnlocked())
      return;

    let win = Services.wm.getMostRecentWindow("Sync:AddDevice");
    if (win)
      win.focus();
    else
      window.openDialog("chrome://browser/content/sync/addDevice.xul",
                        "syncAddDevice", "centerscreen,chrome,resizable=no");
  },

  openQuotaDialog: function SUI_openQuotaDialog() {
    let win = Services.wm.getMostRecentWindow("Sync:ViewQuota");
    if (win)
      win.focus();
    else
      Services.ww.activeWindow.openDialog(
        "chrome://browser/content/sync/quota.xul", "",
        "centerscreen,chrome,dialog,modal");
  },

  openPrefs: function SUI_openPrefs() {
    openPreferences("paneSync");
  },

  openSignInAgainPage: function (entryPoint = "syncbutton") {
    gFxAccounts.openSignInAgainPage(entryPoint);
  },

  
  _updateLastSyncTime: function SUI__updateLastSyncTime() {
    if (!gBrowser)
      return;

    let syncButton = document.getElementById("sync-button");
    if (!syncButton)
      return;

    let lastSync;
    try {
      lastSync = new Date(Services.prefs.getCharPref("services.sync.lastSync"));
    }
    catch (e) { };
    
    try {
      let lastRLSync = new Date(Services.prefs.getCharPref("readinglist.scheduler.lastSync"));
      if (!lastSync || lastRLSync > lastSync) {
        lastSync = lastRLSync;
      }
    }
    catch (e) { };
    if (!lastSync || this._needsSetup()) {
      syncButton.removeAttribute("tooltiptext");
      return;
    }

    
    let lastSyncDateString = lastSync.toLocaleFormat("%a %H:%M");
    let lastSyncLabel =
      this._stringBundle.formatStringFromName("lastSync2.label", [lastSyncDateString], 1);

    syncButton.setAttribute("tooltiptext", lastSyncLabel);
  },

  clearError: function SUI_clearError(errorString) {
    Weave.Notifications.removeAll(errorString);
    this.updateUI();
  },

  onSyncFinish: function SUI_onSyncFinish() {
    let title = this._stringBundle.GetStringFromName("error.sync.title");

    
    this.clearError(title);
  },

  
  
  
  isProlongedReadingListError() {
    let lastSync, threshold, prolonged;
    try {
      lastSync = new Date(Services.prefs.getCharPref("readinglist.scheduler.lastSync"));
      threshold = new Date(Date.now() - Services.prefs.getIntPref("services.sync.errorhandler.networkFailureReportTimeout") * 1000);
      prolonged = lastSync <= threshold;
    } catch (ex) {
      
      prolonged = false;
    }
    this.log.debug("isProlongedReadingListError has last successful sync at ${lastSync}, threshold is ${threshold}, prolonged=${prolonged}",
                   {lastSync, threshold, prolonged});
    return prolonged;
  },

  onRLSyncError() {
    
    
    
    
    
    
    this.log.debug("onRLSyncError with readingList state", ReadingListScheduler.state);
    if (ReadingListScheduler.state == ReadingListScheduler.STATE_ERROR_AUTHENTICATION) {
      this.onLoginError();
      return;
    }
    
    if (!this.isProlongedReadingListError()) {
      this.log.debug("onRLSyncError has a non-authentication, non-prolonged error, so not showing any error UI");
      return;
    }
    
    
    this.log.debug("onRLSyncError has a prolonged error");
    let title = this._stringBundle.GetStringFromName("error.sync.title");
    
    
    
    let lastSync =
      Services.prefs.getIntPref("services.sync.errorhandler.networkFailureReportTimeout") / 86400;
    let description =
      this._stringBundle.formatStringFromName("error.sync.prolonged_failure", [lastSync], 1);
    let priority = Weave.Notifications.PRIORITY_INFO;
    let buttons = [
      new Weave.NotificationButton(
        this._stringBundle.GetStringFromName("error.sync.tryAgainButton.label"),
        this._stringBundle.GetStringFromName("error.sync.tryAgainButton.accesskey"),
        function() { gSyncUI.doSync(); return true; }
      ),
    ];
    let notification =
      new Weave.Notification(title, description, null, priority, buttons);
    Weave.Notifications.replaceTitle(notification);

    this.updateUI();
  },

  onSyncError: function SUI_onSyncError() {
    this.log.debug("onSyncError");
    let title = this._stringBundle.GetStringFromName("error.sync.title");

    if (Weave.Status.login != Weave.LOGIN_SUCCEEDED) {
      this.onLoginError();
      return;
    }

    let description;
    if (Weave.Status.sync == Weave.PROLONGED_SYNC_FAILURE) {
      
      let lastSync =
        Services.prefs.getIntPref("services.sync.errorhandler.networkFailureReportTimeout") / 86400;
      description =
        this._stringBundle.formatStringFromName("error.sync.prolonged_failure", [lastSync], 1);
    } else {
      let error = Weave.Utils.getErrorString(Weave.Status.sync);
      description =
        this._stringBundle.formatStringFromName("error.sync.description", [error], 1);
    }
    let priority = Weave.Notifications.PRIORITY_WARNING;
    let buttons = [];

    
    
    
    let outdated = Weave.Status.sync == Weave.VERSION_OUT_OF_DATE;
    for (let [engine, reason] in Iterator(Weave.Status.engines))
      outdated = outdated || reason == Weave.VERSION_OUT_OF_DATE;

    if (outdated) {
      description = this._stringBundle.GetStringFromName(
        "error.sync.needUpdate.description");
      buttons.push(new Weave.NotificationButton(
        this._stringBundle.GetStringFromName("error.sync.needUpdate.label"),
        this._stringBundle.GetStringFromName("error.sync.needUpdate.accesskey"),
        function() { window.openUILinkIn("https://services.mozilla.com/update/", "tab"); return true; }
      ));
    }
    else if (Weave.Status.sync == Weave.OVER_QUOTA) {
      description = this._stringBundle.GetStringFromName(
        "error.sync.quota.description");
      buttons.push(new Weave.NotificationButton(
        this._stringBundle.GetStringFromName(
          "error.sync.viewQuotaButton.label"),
        this._stringBundle.GetStringFromName(
          "error.sync.viewQuotaButton.accesskey"),
        function() { gSyncUI.openQuotaDialog(); return true; } )
      );
    }
    else if (Weave.Status.enforceBackoff) {
      priority = Weave.Notifications.PRIORITY_INFO;
      buttons.push(new Weave.NotificationButton(
        this._stringBundle.GetStringFromName("error.sync.serverStatusButton.label"),
        this._stringBundle.GetStringFromName("error.sync.serverStatusButton.accesskey"),
        function() { gSyncUI.openServerStatus(); return true; }
      ));
    }
    else {
      priority = Weave.Notifications.PRIORITY_INFO;
      buttons.push(new Weave.NotificationButton(
        this._stringBundle.GetStringFromName("error.sync.tryAgainButton.label"),
        this._stringBundle.GetStringFromName("error.sync.tryAgainButton.accesskey"),
        function() { gSyncUI.doSync(); return true; }
      ));
    }

    let notification =
      new Weave.Notification(title, description, null, priority, buttons);
    Weave.Notifications.replaceTitle(notification);

    this.updateUI();
  },

  observe: function SUI_observe(subject, topic, data) {
    this.log.debug("observed", topic);
    if (this._unloaded) {
      Cu.reportError("SyncUI observer called after unload: " + topic);
      return;
    }

    
    if (subject && typeof subject == "object" &&
        ("wrappedJSObject" in subject) &&
        ("observersModuleSubjectWrapper" in subject.wrappedJSObject)) {
      subject = subject.wrappedJSObject.object;
    }

    
    switch (topic) {
      case "weave:service:sync:start":
      case "weave:service:login:start":
      case "readinglist:sync:start":
        this.onActivityStart();
        break;
      case "weave:service:sync:finish":
      case "weave:service:sync:error":
      case "weave:service:login:finish":
      case "weave:service:login:error":
      case "readinglist:sync:finish":
      case "readinglist:sync:error":
        this.onActivityStop();
        break;
    }
    
    
    
    switch (topic) {
      case "weave:ui:sync:finish":
        this.onSyncFinish();
        break;
      case "weave:ui:sync:error":
        this.onSyncError();
        break;
      case "weave:service:quota:remaining":
        this.onQuotaNotice();
        break;
      case "weave:service:setup-complete":
        this.onSetupComplete();
        break;
      case "weave:service:login:finish":
        this.onLoginFinish();
        break;
      case "weave:ui:login:error":
        this.onLoginError();
        break;
      case "weave:service:logout:finish":
        this.onLogout();
        break;
      case "weave:service:start-over":
        this.onStartOver();
        break;
      case "weave:service:start-over:finish":
        this.updateUI();
        break;
      case "weave:service:ready":
        this.initUI();
        break;
      case "weave:notification:added":
        this.initNotifications();
        break;
      case "weave:ui:clear-error":
        this.clearError();
        break;

      case "readinglist:sync:error":
        this.onRLSyncError();
        break;
      case "readinglist:sync:finish":
        this.clearError();
        break;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ])
};

XPCOMUtils.defineLazyGetter(gSyncUI, "_stringBundle", function() {
  
  
  return Cc["@mozilla.org/intl/stringbundle;1"].
         getService(Ci.nsIStringBundleService).
         createBundle("chrome://weave/locale/services/sync.properties");
});

XPCOMUtils.defineLazyGetter(gSyncUI, "log", function() {
  return Log.repository.getLogger("browserwindow.syncui");
});
