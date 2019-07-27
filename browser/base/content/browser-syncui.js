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


let gSyncUI = {
  DEFAULT_EOL_URL: "https://www.mozilla.org/firefox/?utm_source=synceol",

  _obs: ["weave:service:sync:start",
         "weave:service:quota:remaining",
         "weave:service:setup-complete",
         "weave:service:login:start",
         "weave:service:login:finish",
         "weave:service:logout:finish",
         "weave:service:start-over",
         "weave:service:start-over:finish",
         "weave:ui:login:error",
         "weave:ui:sync:error",
         "weave:ui:sync:finish",
         "weave:ui:clear-error",
         "weave:eol",
  ],

  _unloaded: false,

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
  },

  _needsSetup: function SUI__needsSetup() {
    
    
    
    
    
    
    if (Weave.Status._authManager._signedInUser) {
      
      
      if (!Weave.Status._authManager._signedInUser.verified) {
        return true;
      }
    }

    let firstSync = "";
    try {
      firstSync = Services.prefs.getCharPref("services.sync.firstSync");
    } catch (e) { }

    return Weave.Status.checkSetup() == Weave.CLIENT_NOT_CONFIGURED ||
           firstSync == "notReady";
  },

  _loginFailed: function () {
    return Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED;
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
    } else if (needsSetup) {
      document.getElementById("sync-setup-state").hidden = false;
    } else {
      document.getElementById("sync-syncnow-state").hidden = false;
    }

    if (!gBrowser)
      return;

    let syncButton = document.getElementById("sync-button");
    let panelHorizontalButton = document.getElementById("PanelUI-fxa-status");
    [syncButton, panelHorizontalButton].forEach(function(button) {
      if (!button)
        return;
      button.removeAttribute("status");
    });

    if (needsSetup && syncButton)
      syncButton.removeAttribute("tooltiptext");

    this._updateLastSyncTime();
  },


  
  onActivityStart: function SUI_onActivityStart() {
    if (!gBrowser)
      return;

    ["sync-button", "PanelUI-fxa-status"].forEach(function(id) {
      let button = document.getElementById(id);
      if (!button)
        return;
      button.setAttribute("status", "active");
    });
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
    
    if (Weave.Status.login == Weave.LOGIN_FAILED_NOT_READY) {
      this.updateUI();
      return;
    }

    let title = this._stringBundle.GetStringFromName("error.login.title");

    let description;
    if (Weave.Status.sync == Weave.PROLONGED_SYNC_FAILURE) {
      
      let lastSync =
        Services.prefs.getIntPref("services.sync.errorhandler.networkFailureReportTimeout") / 86400;
      description =
        this._stringBundle.formatStringFromName("error.sync.prolonged_failure", [lastSync], 1);
    } else {
      let reason = Weave.Utils.getErrorString(Weave.Status.login);
      description =
        this._stringBundle.formatStringFromName("error.sync.description", [reason], 1);
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
    this.updateUI();
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

  onEOLNotice: function (data) {
    let code = data.code;
    let kind = (code == "hard-eol") ? "error" : "warning";
    let url = data.url || gSyncUI.DEFAULT_EOL_URL;

    let title = this._stringBundle.GetStringFromName(kind + ".sync.eol.label");
    let description = this._stringBundle.formatStringFromName(kind + ".sync.eol.description",
                                                              [this._getAppName()],
                                                              1);

    let buttons = [];
    buttons.push(new Weave.NotificationButton(
      this._stringBundle.GetStringFromName("sync.eol.learnMore.label"),
      this._stringBundle.GetStringFromName("sync.eol.learnMore.accesskey"),
      function() {
        window.openUILinkIn(url, "tab");
        return true;
      }
    ));

    let priority = (kind == "error") ? Weave.Notifications.PRIORITY_WARNING :
                                       Weave.Notifications.PRIORITY_INFO;
    let notification = new Weave.Notification(title, description, null, priority, buttons);
    Weave.Notifications.replaceTitle(notification);
  },

  openServerStatus: function () {
    let statusURL = Services.prefs.getCharPref("services.sync.statusURL");
    window.openUILinkIn(statusURL, "tab");
  },

  
  doSync: function SUI_doSync() {
    let needsSetup = this._needsSetup();
    let loginFailed = this._loginFailed();

    if (!(loginFailed || needsSetup)) {
      setTimeout(function () Weave.Service.errorHandler.syncAndReportErrors(), 0);
    }

    Services.obs.notifyObservers(null, "cloudsync:user-sync", null);
  },

  handleToolbarButton: function SUI_handleStatusbarButton() {
    if (this._needsSetup())
      this.openSetup();
    else
      this.doSync();
  },

  
  

  









  openSetup: function SUI_openSetup(wizardType) {
    let xps = Components.classes["@mozilla.org/weave/service;1"]
                                .getService(Components.interfaces.nsISupports)
                                .wrappedJSObject;
    if (xps.fxAccountsEnabled) {
      fxAccounts.getSignedInUser().then(userData => {
        if (userData) {
          this.openPrefs();
        } else {
          switchToTabHavingURI("about:accounts", true);
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

  openSignInAgainPage: function () {
    switchToTabHavingURI("about:accounts?action=reauth", true);
  },

  
  _updateLastSyncTime: function SUI__updateLastSyncTime() {
    if (!gBrowser)
      return;

    let syncButton = document.getElementById("sync-button");
    if (!syncButton)
      return;

    let lastSync;
    try {
      lastSync = Services.prefs.getCharPref("services.sync.lastSync");
    }
    catch (e) { };
    if (!lastSync || this._needsSetup()) {
      syncButton.removeAttribute("tooltiptext");
      return;
    }

    
    let lastSyncDate = new Date(lastSync).toLocaleFormat("%a %H:%M");
    let lastSyncLabel =
      this._stringBundle.formatStringFromName("lastSync2.label", [lastSyncDate], 1);

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

  onSyncError: function SUI_onSyncError() {
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
        this.onActivityStart();
        break;
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
      case "weave:service:login:start":
        this.onActivityStart();
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
      case "weave:eol":
        this.onEOLNotice(subject);
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

