# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let gFxAccounts = {

  PREF_SYNC_START_DOORHANGER: "services.sync.ui.showSyncStartDoorhanger",
  DOORHANGER_ACTIVATE_DELAY_MS: 5000,
  SYNC_MIGRATION_NOTIFICATION_TITLE: "fxa-migration",

  _initialized: false,
  _inCustomizationMode: false,
  
  
  
  
  _expectingNotifyClose: false,

  get weave() {
    delete this.weave;
    return this.weave = Cc["@mozilla.org/weave/service;1"]
                          .getService(Ci.nsISupports)
                          .wrappedJSObject;
  },

  get topics() {
    
    delete this.topics;
    return this.topics = [
      "weave:service:ready",
      "weave:service:sync:start",
      "weave:service:login:error",
      "weave:service:setup-complete",
      "fxa-migration:state-changed",
      this.FxAccountsCommon.ONVERIFIED_NOTIFICATION,
      this.FxAccountsCommon.ONLOGOUT_NOTIFICATION,
      "weave:notification:removed",
    ];
  },

  get button() {
    delete this.button;
    return this.button = document.getElementById("PanelUI-fxa-status");
  },

  get strings() {
    delete this.strings;
    return this.strings = Services.strings.createBundle(
      "chrome://browser/locale/accounts.properties"
    );
  },

  get loginFailed() {
    
    
    let service = Cc["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;
    if (!service.ready) {
      return false;
    }
    
    
    
    return Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED;
  },

  get isActiveWindow() {
    let fm = Services.focus;
    return fm.activeWindow == window;
  },

  init: function () {
    
    if (this._initialized || !window.toolbar.visible) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.addObserver(this, topic, false);
    }

    addEventListener("activate", this);
    gNavToolbox.addEventListener("customizationstarting", this);
    gNavToolbox.addEventListener("customizationending", this);

    
    
    Services.obs.notifyObservers(null, "fxa-migration:state-request", null);

    this._initialized = true;

    this.updateUI();
  },

  uninit: function () {
    if (!this._initialized) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.removeObserver(this, topic);
    }

    this._initialized = false;
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case this.FxAccountsCommon.ONVERIFIED_NOTIFICATION:
        Services.prefs.setBoolPref(this.PREF_SYNC_START_DOORHANGER, true);
        break;
      case "weave:service:sync:start":
        this.onSyncStart();
        break;
      case "fxa-migration:state-changed":
        this.onMigrationStateChanged(data, subject);
        break;
      case "weave:notification:removed":
        
        
        let notif = subject.wrappedJSObject.object;
        if (notif.title == this.SYNC_MIGRATION_NOTIFICATION_TITLE &&
            !this._expectingNotifyClose) {
          
          this.fxaMigrator.recordTelemetry(this.fxaMigrator.TELEMETRY_DECLINED);
        }
        break;
      default:
        this.updateUI();
        break;
    }
  },

  onSyncStart: function () {
    if (!this.isActiveWindow) {
      return;
    }

    let showDoorhanger = false;

    try {
      showDoorhanger = Services.prefs.getBoolPref(this.PREF_SYNC_START_DOORHANGER);
    } catch (e) {  }

    if (showDoorhanger) {
      Services.prefs.clearUserPref(this.PREF_SYNC_START_DOORHANGER);
      this.showSyncStartedDoorhanger();
    }
  },

  onMigrationStateChanged: function (newState, email) {
    this._migrationInfo = !newState ? null : {
      state: newState,
      email: email ? email.QueryInterface(Ci.nsISupportsString).data : null,
    };
    this.updateUI();
  },

  handleEvent: function (event) {
    if (event.type == "activate") {
      
      
      
      
      
      setTimeout(() => this.onSyncStart(), this.DOORHANGER_ACTIVATE_DELAY_MS);
    } else {
      this._inCustomizationMode = event.type == "customizationstarting";
      this.updateAppMenuItem();
    }
  },

  showDoorhanger: function (id) {
    let panel = document.getElementById(id);
    let anchor = document.getElementById("PanelUI-menu-button");

    let iconAnchor =
      document.getAnonymousElementByAttribute(anchor, "class",
                                              "toolbarbutton-icon");

    panel.hidden = false;
    panel.openPopup(iconAnchor || anchor, "bottomcenter topright");
  },

  showSyncStartedDoorhanger: function () {
    this.showDoorhanger("sync-start-panel");
  },

  showSyncFailedDoorhanger: function () {
    this.showDoorhanger("sync-error-panel");
  },

  updateUI: function () {
    this.updateAppMenuItem();
    this.updateMigrationNotification();
  },

  updateAppMenuItem: function () {
    if (this._migrationInfo) {
      this.updateAppMenuItemForMigration();
      return;
    }

    
    if (!this.weave.fxAccountsEnabled) {
      
      
      
      
      this.button.hidden = true;
      this.button.removeAttribute("fxastatus");
      return;
    }

    
    this.button.hidden = false;

    
    if (this._inCustomizationMode) {
      this.button.setAttribute("disabled", "true");
    } else {
      this.button.removeAttribute("disabled");
    }

    let defaultLabel = this.button.getAttribute("defaultlabel");
    let errorLabel = this.button.getAttribute("errorlabel");

    
    
    let doUpdate = userData => {
      
      this.button.setAttribute("label", defaultLabel);
      this.button.removeAttribute("tooltiptext");
      this.button.removeAttribute("fxastatus");

      if (!this._inCustomizationMode) {
        if (this.loginFailed) {
          this.button.setAttribute("fxastatus", "error");
          this.button.setAttribute("label", errorLabel);
        } else if (userData) {
          this.button.setAttribute("fxastatus", "signedin");
          this.button.setAttribute("label", userData.email);
          this.button.setAttribute("tooltiptext", userData.email);
        }
      }
    }
    fxAccounts.getSignedInUser().then(userData => {
      doUpdate(userData);
    }).then(null, error => {
      
      
      
      
      doUpdate(null);
    });
  },

  updateAppMenuItemForMigration: Task.async(function* () {
    let status = null;
    let label = null;
    switch (this._migrationInfo.state) {
      case this.fxaMigrator.STATE_USER_FXA:
        status = "migrate-signup";
        label = this.strings.formatStringFromName("needUserShort",
          [this.button.getAttribute("fxabrandname")], 1);
        break;
      case this.fxaMigrator.STATE_USER_FXA_VERIFIED:
        status = "migrate-verify";
        label = this.strings.formatStringFromName("needVerifiedUserShort",
                                                  [this._migrationInfo.email],
                                                  1);
        break;
    }
    this.button.label = label;
    this.button.hidden = false;
    this.button.setAttribute("fxastatus", status);
  }),

  updateMigrationNotification: Task.async(function* () {
    if (!this._migrationInfo) {
      this._expectingNotifyClose = true;
      Weave.Notifications.removeAll(this.SYNC_MIGRATION_NOTIFICATION_TITLE);
      
      
      
      
      this._expectingNotifyClose = false;
      return;
    }
    let note = null;
    switch (this._migrationInfo.state) {
      case this.fxaMigrator.STATE_USER_FXA: {
        
        
        
        
        let msg, upgradeLabel, upgradeAccessKey, learnMoreLink;
        if (this._migrationInfo.email) {
          msg = this.strings.formatStringFromName("signInAfterUpgradeOnOtherDevice.description",
                                                  [this._migrationInfo.email],
                                                  1);
          upgradeLabel = this.strings.GetStringFromName("signInAfterUpgradeOnOtherDevice.label");
          upgradeAccessKey = this.strings.GetStringFromName("signInAfterUpgradeOnOtherDevice.accessKey");
        } else {
          msg = this.strings.GetStringFromName("needUserLong");
          upgradeLabel = this.strings.GetStringFromName("upgradeToFxA.label");
          upgradeAccessKey = this.strings.GetStringFromName("upgradeToFxA.accessKey");
          learnMoreLink = this.fxaMigrator.learnMoreLink;
        }
        note = new Weave.Notification(
          undefined, msg, undefined, Weave.Notifications.PRIORITY_WARNING, [
            new Weave.NotificationButton(upgradeLabel, upgradeAccessKey, () => {
              this._expectingNotifyClose = true;
              this.fxaMigrator.createFxAccount(window);
            }),
          ], learnMoreLink
        );
        break;
      }
      case this.fxaMigrator.STATE_USER_FXA_VERIFIED: {
        let msg =
          this.strings.formatStringFromName("needVerifiedUserLong",
                                            [this._migrationInfo.email], 1);
        let resendLabel =
          this.strings.GetStringFromName("resendVerificationEmail.label");
        let resendAccessKey =
          this.strings.GetStringFromName("resendVerificationEmail.accessKey");
        note = new Weave.Notification(
          undefined, msg, undefined, Weave.Notifications.PRIORITY_INFO, [
            new Weave.NotificationButton(resendLabel, resendAccessKey, () => {
              this._expectingNotifyClose = true;
              this.fxaMigrator.resendVerificationMail();
            }),
          ]
        );
        break;
      }
    }
    note.title = this.SYNC_MIGRATION_NOTIFICATION_TITLE;
    Weave.Notifications.replaceTitle(note);
  }),

  onMenuPanelCommand: function (event) {
    let button = event.originalTarget;

    switch (button.getAttribute("fxastatus")) {
    case "signedin":
      this.openPreferences();
      break;
    case "error":
      this.openSignInAgainPage("menupanel");
      break;
    case "migrate-signup":
    case "migrate-verify":
      
      
      this.openPreferences();
      break;
    default:
      this.openAccountsPage(null, { entryPoint: "menupanel" });
      break;
    }

    PanelUI.hide();
  },

  openPreferences: function () {
    openPreferences("paneSync");
  },

  openAccountsPage: function (action, urlParams={}) {
    
    
    
    if (UITour.tourBrowsersByWindow.get(window) &&
        UITour.tourBrowsersByWindow.get(window).has(gBrowser.selectedBrowser)) {
      urlParams.entryPoint = "uitour";
    }
    let params = new URLSearchParams();
    if (action) {
      params.set("action", action);
    }
    for (let name in urlParams) {
      if (urlParams[name] !== undefined) {
        params.set(name, urlParams[name]);
      }
    }
    let url = "about:accounts?" + params;
    switchToTabHavingURI(url, true, {
      replaceQueryString: true
    });
  },

  openSignInAgainPage: function (entryPoint) {
    this.openAccountsPage("reauth", { entryPoint: entryPoint });
  },
};

XPCOMUtils.defineLazyGetter(gFxAccounts, "FxAccountsCommon", function () {
  return Cu.import("resource://gre/modules/FxAccountsCommon.js", {});
});

XPCOMUtils.defineLazyModuleGetter(gFxAccounts, "fxaMigrator",
  "resource://services-sync/FxaMigrator.jsm");
