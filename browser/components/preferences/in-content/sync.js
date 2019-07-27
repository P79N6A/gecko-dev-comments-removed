



Components.utils.import("resource://services-sync/main.js");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "FxAccountsCommon", function () {
  return Components.utils.import("resource://gre/modules/FxAccountsCommon.js", {});
});

XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
  "resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "fxaMigrator",
  "resource://services-sync/FxaMigrator.jsm");

const PAGE_NO_ACCOUNT = 0;
const PAGE_HAS_ACCOUNT = 1;
const PAGE_NEEDS_UPDATE = 2;
const PAGE_PLEASE_WAIT = 3;
const FXA_PAGE_LOGGED_OUT = 4;
const FXA_PAGE_LOGGED_IN = 5;



const FXA_LOGIN_VERIFIED = 0;

const FXA_LOGIN_UNVERIFIED = 1;

const FXA_LOGIN_FAILED = 2;

let gSyncPane = {
  prefArray: ["engine.bookmarks", "engine.passwords", "engine.prefs",
              "engine.tabs", "engine.history"],

  get page() {
    return document.getElementById("weavePrefsDeck").selectedIndex;
  },

  set page(val) {
    document.getElementById("weavePrefsDeck").selectedIndex = val;
  },

  get _usingCustomServer() {
    return Weave.Svc.Prefs.isSet("serverURL");
  },

  needsUpdate: function () {
    this.page = PAGE_NEEDS_UPDATE;
    let label = document.getElementById("loginError");
    label.textContent = Weave.Utils.getErrorString(Weave.Status.login);
    label.className = "error";
  },

  init: function () {
    this._setupEventListeners();

    
    let xps = Components.classes["@mozilla.org/weave/service;1"]
                                .getService(Components.interfaces.nsISupports)
                                .wrappedJSObject;

    if (xps.ready) {
      this._init();
      return;
    }

    
    
    this.page = PAGE_PLEASE_WAIT;

    let onUnload = function () {
      window.removeEventListener("unload", onUnload, false);
      try {
        Services.obs.removeObserver(onReady, "weave:service:ready");
      } catch (e) {}
    };

    let onReady = function () {
      Services.obs.removeObserver(onReady, "weave:service:ready");
      window.removeEventListener("unload", onUnload, false);
      this._init();
    }.bind(this);

    Services.obs.addObserver(onReady, "weave:service:ready", false);
    window.addEventListener("unload", onUnload, false);

    xps.ensureLoaded();
  },

  _init: function () {
    let topics = ["weave:service:login:error",
                  "weave:service:login:finish",
                  "weave:service:start-over:finish",
                  "weave:service:setup-complete",
                  "weave:service:logout:finish",
                  FxAccountsCommon.ONVERIFIED_NOTIFICATION,
                  FxAccountsCommon.ON_PROFILE_CHANGE_NOTIFICATION,
                  ];
    let migrateTopic = "fxa-migration:state-changed";

    
    
    
    topics.forEach(function (topic) {
      Weave.Svc.Obs.add(topic, this.updateWeavePrefs, this);
    }, this);
    
    Weave.Svc.Obs.add(migrateTopic, this.updateMigrationState, this);

    window.addEventListener("unload", function() {
      topics.forEach(function (topic) {
        Weave.Svc.Obs.remove(topic, this.updateWeavePrefs, this);
      }, gSyncPane);
      Weave.Svc.Obs.remove(migrateTopic, gSyncPane.updateMigrationState, gSyncPane);
    }, false);

    XPCOMUtils.defineLazyGetter(this, '_stringBundle', () => {
      return Services.strings.createBundle("chrome://browser/locale/preferences/preferences.properties");
    }),

    XPCOMUtils.defineLazyGetter(this, '_accountsStringBundle', () => {
      return Services.strings.createBundle("chrome://browser/locale/accounts.properties");
    }),

    this.updateWeavePrefs();

    this._initProfileImageUI();
  },

  _setupEventListeners: function() {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gSyncPane));
    }

    setEventListener("noAccountSetup", "click", function (aEvent) {
      aEvent.stopPropagation();
      gSyncPane.openSetup(null);
    });
    setEventListener("noAccountPair", "click", function (aEvent) {
      aEvent.stopPropagation();
      gSyncPane.openSetup('pair');
    });
    setEventListener("syncViewQuota", "command", gSyncPane.openQuotaDialog);
    setEventListener("syncChangePassword", "command",
      () => gSyncUtils.changePassword());
    setEventListener("syncResetPassphrase", "command",
      () => gSyncUtils.resetPassphrase());
    setEventListener("syncReset", "command", gSyncPane.resetSync);
    setEventListener("syncAddDeviceLabel", "click", function () {
      gSyncPane.openAddDevice();
      return false;
    });
    setEventListener("syncEnginesList", "select", function () {
      if (this.selectedCount)
        this.clearSelection();
    });
    setEventListener("syncComputerName", "change", function (e) {
      gSyncUtils.changeName(e.target);
    });
    setEventListener("unlinkDevice", "click", function () {
      gSyncPane.startOver(true);
      return false;
    });
    setEventListener("tosPP-normal-ToS", "click", gSyncPane.openToS);
    setEventListener("tosPP-normal-PP", "click", gSyncPane.openPrivacyPolicy);
    setEventListener("loginErrorUpdatePass", "click", function () {
      gSyncPane.updatePass();
      return false;
    });
    setEventListener("loginErrorResetPass", "click", function () {
      gSyncPane.resetPass();
      return false;
    });
    setEventListener("loginErrorStartOver", "click", function () {
      gSyncPane.startOver(true);
      return false;
    });
    setEventListener("noFxaSignUp", "click", function () {
      gSyncPane.signUp();
      return false;
    });
    setEventListener("noFxaSignIn", "click", function () {
      gSyncPane.signIn();
      return false;
    });
    setEventListener("noFxaUseOldSync", "click", function () {
      gSyncPane.openOldSyncSupportPage();
      return false;
    });
    setEventListener("verifiedManage", "command",
      gSyncPane.manageFirefoxAccount);
    setEventListener("fxaUnlinkButton", "click", function () {
      gSyncPane.unlinkFirefoxAccount(true);
    });
    setEventListener("verifyFxaAccount", "command",
      gSyncPane.verifyFirefoxAccount);
    setEventListener("unverifiedUnlinkFxaAccount", "click", function () {
      
      gSyncPane.unlinkFirefoxAccount(false);
    });
    setEventListener("rejectReSignIn", "command",
      gSyncPane.reSignIn);
    setEventListener("rejectUnlinkFxaAccount", "click", function () {
      gSyncPane.unlinkFirefoxAccount(true);
    });
    setEventListener("fxaSyncComputerName", "change", function (e) {
      gSyncUtils.changeName(e.target);
    });
    setEventListener("tosPP-small-ToS", "click", gSyncPane.openToS);
    setEventListener("tosPP-small-PP", "click", gSyncPane.openPrivacyPolicy);
    setEventListener("sync-migrate-upgrade", "click", function () {
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      fxaMigrator.createFxAccount(win);
    });
    setEventListener("sync-migrate-unlink", "click", function () {
      gSyncPane.startOverMigration();
    });
    setEventListener("sync-migrate-forget", "click", function () {
      fxaMigrator.forgetFxAccount();
    });
    setEventListener("sync-migrate-resend", "click", function () {
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      fxaMigrator.resendVerificationMail(win);
    });
  },

  _initProfileImageUI: function () {
    try {
      if (Services.prefs.getBoolPref("identity.fxaccounts.profile_image.enabled")) {
        document.getElementById("fxaProfileImage").hidden = false;
      }
    } catch (e) { }
  },

  updateWeavePrefs: function () {
    
    
    
    
    Services.obs.notifyObservers(null, "fxa-migration:state-request", null);

    let service = Components.classes["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;
    
    
    if (service.fxAccountsEnabled) {
      
      
      if (Services.prefs.getBoolPref("browser.readinglist.enabled")) {
        document.getElementById("readinglist-engine").removeAttribute("hidden");
      }
      
      this.page = PAGE_PLEASE_WAIT;

      fxAccounts.getSignedInUser().then(data => {
        if (!data) {
          this.page = FXA_PAGE_LOGGED_OUT;
          return false;
        }
        this.page = FXA_PAGE_LOGGED_IN;
        
        
        let fxaLoginStatus = document.getElementById("fxaLoginStatus");
        let enginesListDisabled;
        
        if (!data.verified) {
          fxaLoginStatus.selectedIndex = FXA_LOGIN_UNVERIFIED;
          enginesListDisabled = true;
        
        
        
        
        
        
        } else if (Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
          fxaLoginStatus.selectedIndex = FXA_LOGIN_FAILED;
          enginesListDisabled = true;
        
        
        } else {
          fxaLoginStatus.selectedIndex = FXA_LOGIN_VERIFIED;
          enginesListDisabled = false;
        }
        document.getElementById("fxaEmailAddress1").textContent = data.email;
        document.getElementById("fxaEmailAddress2").textContent = data.email;
        document.getElementById("fxaEmailAddress3").textContent = data.email;
        document.getElementById("fxaSyncComputerName").value = Weave.Service.clientsEngine.localName;
        let engines = document.getElementById("fxaSyncEngines")
        for (let checkbox of engines.querySelectorAll("checkbox")) {
          checkbox.disabled = enginesListDisabled;
        }

        
        document.getElementById("fxaProfileImage").style.removeProperty("background-image");

        
        
        return data.verified;
      }).then(shouldGetProfile => {
        if (shouldGetProfile) {
          return fxAccounts.getSignedInUserProfile();
        }
      }).then(data => {
        if (data && data.avatar) {
          
          
          
          let img = new Image();
          img.onload = () => {
            let bgImage = "url('" + data.avatar + "')";
            document.getElementById("fxaProfileImage").style.backgroundImage = bgImage;
          };
          img.src = data.avatar;
        }
      }, err => {
        FxAccountsCommon.log.error(err);
      }).catch(err => {
        
        Cu.reportError(String(err));
      });

    
    
    
    
    } else if (Weave.Status.service == Weave.CLIENT_NOT_CONFIGURED ||
               Weave.Svc.Prefs.get("firstSync", "") == "notReady") {
      this.page = PAGE_NO_ACCOUNT;
    
    
    } else if (Weave.Status.login == Weave.LOGIN_FAILED_INVALID_PASSPHRASE ||
               Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
      this.needsUpdate();
    } else {
      this.page = PAGE_HAS_ACCOUNT;
      document.getElementById("accountName").textContent = Weave.Service.identity.account;
      document.getElementById("syncComputerName").value = Weave.Service.clientsEngine.localName;
      document.getElementById("tosPP-normal").hidden = this._usingCustomServer;
    }
  },

  updateMigrationState: function(subject, state) {
    let selIndex;
    switch (state) {
      case fxaMigrator.STATE_USER_FXA: {
        let sb = this._accountsStringBundle;
        
        
        
        
        let email = subject ? subject.QueryInterface(Components.interfaces.nsISupportsString).data : null;
        let elt = document.getElementById("sync-migrate-upgrade-description");
        elt.textContent = email ?
                          sb.formatStringFromName("signInAfterUpgradeOnOtherDevice.description",
                                                  [email], 1) :
                          sb.GetStringFromName("needUserLong");

        
        if (!email) {
          let learnMoreLink = document.createElement("label");
          learnMoreLink.className = "text-link";
          let { text, href } = fxaMigrator.learnMoreLink;
          learnMoreLink.setAttribute("value", text);
          learnMoreLink.href = href;
          elt.appendChild(learnMoreLink);
        }

        
        let button = document.getElementById("sync-migrate-upgrade");
        button.setAttribute("label",
                            sb.GetStringFromName(email
                                                 ? "signInAfterUpgradeOnOtherDevice.label"
                                                 : "upgradeToFxA.label"));
        button.setAttribute("accesskey",
                            sb.GetStringFromName(email
                                                 ? "signInAfterUpgradeOnOtherDevice.accessKey"
                                                 : "upgradeToFxA.accessKey"));
        
        button = document.getElementById("sync-migrate-unlink");
        if (email) {
          button.hidden = true;
        } else {
          button.setAttribute("label", sb.GetStringFromName("unlinkMigration.label"));
          button.setAttribute("accesskey", sb.GetStringFromName("unlinkMigration.accessKey"));
        }
        selIndex = 0;
        break;
      }
      case fxaMigrator.STATE_USER_FXA_VERIFIED: {
        let sb = this._accountsStringBundle;
        let email = subject.QueryInterface(Components.interfaces.nsISupportsString).data;
        let label = sb.formatStringFromName("needVerifiedUserLong", [email], 1);
        let elt = document.getElementById("sync-migrate-verify-label");
        elt.setAttribute("value", label);
        
        let button = document.getElementById("sync-migrate-resend");
        button.setAttribute("label", sb.GetStringFromName("resendVerificationEmail.label"));
        button.setAttribute("accesskey", sb.GetStringFromName("resendVerificationEmail.accessKey"));
        
        button = document.getElementById("sync-migrate-forget");
        button.setAttribute("label", sb.GetStringFromName("forgetMigration.label"));
        button.setAttribute("accesskey", sb.GetStringFromName("forgetMigration.accessKey"));
        selIndex = 1;
        break;
      }
      default:
        if (state) { 
          Cu.reportError("updateMigrationState has unknown state: " + state);
        }
        document.getElementById("sync-migration").hidden = true;
        return;
    }
    document.getElementById("sync-migration").hidden = false;
    document.getElementById("sync-migration-deck").selectedIndex = selIndex;
  },

  
  onPreferenceChanged: function() {
    let prefElts = document.querySelectorAll("#syncEnginePrefs > preference");
    let syncEnabled = false;
    for (let elt of prefElts) {
      if (elt.name.startsWith("services.sync.") && elt.value) {
        syncEnabled = true;
        break;
      }
    }
    Services.prefs.setBoolPref("services.sync.enabled", syncEnabled);
  },

  startOver: function (showDialog) {
    if (showDialog) {
      let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_IS_STRING +
                  Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_CANCEL + 
                  Services.prompt.BUTTON_POS_1_DEFAULT;
      let buttonChoice =
        Services.prompt.confirmEx(window,
                                  this._stringBundle.GetStringFromName("syncUnlink.title"),
                                  this._stringBundle.GetStringFromName("syncUnlink.label"),
                                  flags,
                                  this._stringBundle.GetStringFromName("syncUnlinkConfirm.label"),
                                  null, null, null, {});

      
      if (buttonChoice == 1)
        return;
    }

    Weave.Service.startOver();
    this.updateWeavePrefs();
  },

  
  
  startOverMigration: function () {
    let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_IS_STRING +
                Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_CANCEL +
                Services.prompt.BUTTON_POS_1_DEFAULT;
    let sb = this._accountsStringBundle;
    let buttonChoice =
      Services.prompt.confirmEx(window,
                                sb.GetStringFromName("unlinkVerificationTitle"),
                                sb.GetStringFromName("unlinkVerificationDescription"),
                                flags,
                                sb.GetStringFromName("unlinkVerificationConfirm"),
                                null, null, null, {});

    
    if (buttonChoice == 1)
      return;

    fxaMigrator.recordTelemetry(fxaMigrator.TELEMETRY_UNLINKED);
    Weave.Service.startOver();
    this.updateWeavePrefs();
  },

  updatePass: function () {
    if (Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED)
      gSyncUtils.changePassword();
    else
      gSyncUtils.updatePassphrase();
  },

  resetPass: function () {
    if (Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED)
      gSyncUtils.resetPassword();
    else
      gSyncUtils.resetPassphrase();
  },

  








  openSetup: function (wizardType) {
    let service = Components.classes["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;

    if (service.fxAccountsEnabled) {
      this.openContentInBrowser("about:accounts?entrypoint=preferences", {
        replaceQueryString: true
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

  openContentInBrowser: function(url, options) {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    if (!win) {
      
      
      
      gSyncUtils._openLink(url);
      return;
    }
    win.switchToTabHavingURI(url, true, options);
  },


  openPrivacyPolicy: function(aEvent) {
    aEvent.stopPropagation();
    gSyncUtils.openPrivacyPolicy();
  },

  openToS: function(aEvent) {
    aEvent.stopPropagation();
    gSyncUtils.openToS();
  },

  signUp: function() {
    this.openContentInBrowser("about:accounts?action=signup&entrypoint=preferences", {
      replaceQueryString: true
    });
  },

  signIn: function() {
    this.openContentInBrowser("about:accounts?action=signin&entrypoint=preferences", {
      replaceQueryString: true
    });
  },

  reSignIn: function() {
    this.openContentInBrowser("about:accounts?action=reauth&entrypoint=preferences", {
      replaceQueryString: true
    });
  },

  openChangeProfileImage: function() {
    fxAccounts.promiseAccountsChangeProfileURI("avatar")
      .then(url => {
        this.openContentInBrowser(url, {
          replaceQueryString: true
        });
      });
  },

  manageFirefoxAccount: function() {
    fxAccounts.promiseAccountsManageURI()
      .then(url => {
        this.openContentInBrowser(url, {
          replaceQueryString: true
        });
      });
  },

  verifyFirefoxAccount: function() {
    fxAccounts.resendVerificationEmail().then(() => {
      fxAccounts.getSignedInUser().then(data => {
        let sb = this._accountsStringBundle;
        let title = sb.GetStringFromName("verificationSentTitle");
        let heading = sb.formatStringFromName("verificationSentHeading",
                                              [data.email], 1);
        let description = sb.GetStringFromName("verificationSentDescription");

        let factory = Cc["@mozilla.org/prompter;1"]
                        .getService(Ci.nsIPromptFactory);
        let prompt = factory.getPrompt(window, Ci.nsIPrompt);
        let bag = prompt.QueryInterface(Ci.nsIWritablePropertyBag2);
        bag.setPropertyAsBool("allowTabModal", true);

        prompt.alert(title, heading + "\n\n" + description);
      });
    });
  },

  openOldSyncSupportPage: function() {
    let url = Services.urlFormatter.formatURLPref("app.support.baseURL") + "old-sync";
    this.openContentInBrowser(url);
  },

  unlinkFirefoxAccount: function(confirm) {
    if (confirm) {
      
      let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
      let continueLabel = sb.GetStringFromName("continue.label");
      let title = sb.GetStringFromName("disconnect.verify.title");
      let brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
      let brandShortName = brandBundle.GetStringFromName("brandShortName");
      let body = sb.GetStringFromName("disconnect.verify.heading") +
                 "\n\n" +
                 sb.formatStringFromName("disconnect.verify.description",
                                         [brandShortName], 1);
      let ps = Services.prompt;
      let buttonFlags = (ps.BUTTON_POS_0 * ps.BUTTON_TITLE_IS_STRING) +
                        (ps.BUTTON_POS_1 * ps.BUTTON_TITLE_CANCEL) +
                        ps.BUTTON_POS_1_DEFAULT;

      let factory = Cc["@mozilla.org/prompter;1"]
                      .getService(Ci.nsIPromptFactory);
      let prompt = factory.getPrompt(window, Ci.nsIPrompt);
      let bag = prompt.QueryInterface(Ci.nsIWritablePropertyBag2);
      bag.setPropertyAsBool("allowTabModal", true);

      let pressed = prompt.confirmEx(title, body, buttonFlags,
                                     continueLabel, null, null, null, {});

      if (pressed != 0) { 
        return;
      }
    }
    fxAccounts.signOut().then(() => {
      this.updateWeavePrefs();
    });
  },

  openQuotaDialog: function () {
    let win = Services.wm.getMostRecentWindow("Sync:ViewQuota");
    if (win)
      win.focus();
    else 
      window.openDialog("chrome://browser/content/sync/quota.xul", "",
                        "centerscreen,chrome,dialog,modal");
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

  resetSync: function () {
    this.openSetup("reset");
  },
};

