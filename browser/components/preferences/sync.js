



Components.utils.import("resource://services-sync/main.js");
Components.utils.import("resource://gre/modules/Services.jsm");

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
  _stringBundle: null,
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
    label.value = Weave.Utils.getErrorString(Weave.Status.login);
    label.className = "error";
  },

  init: function () {
    
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
                  "weave:service:start-over",
                  "weave:service:setup-complete",
                  "weave:service:logout:finish"];

    
    
    
    topics.forEach(function (topic) {
      Weave.Svc.Obs.add(topic, this.updateWeavePrefs, this);
    }, this);
    window.addEventListener("unload", function() {
      topics.forEach(function (topic) {
        Weave.Svc.Obs.remove(topic, this.updateWeavePrefs, this);
      }, gSyncPane);
    }, false);

    this._stringBundle =
      Services.strings.createBundle("chrome://browser/locale/preferences/preferences.properties");
    this.updateWeavePrefs();
  },

  updateWeavePrefs: function () {
    let service = Components.classes["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;
    
    
    if (service.fxAccountsEnabled) {
      
      this.page = PAGE_PLEASE_WAIT;
      Components.utils.import("resource://gre/modules/FxAccounts.jsm");
      fxAccounts.getSignedInUser().then(data => {
        if (!data) {
          this.page = FXA_PAGE_LOGGED_OUT;
          return;
        }
        this.page = FXA_PAGE_LOGGED_IN;
        
        
        let fxaLoginStatus = document.getElementById("fxaLoginStatus");
        let enginesListDisabled;
        
        if (!data.verified) {
          fxaLoginStatus.selectedIndex = FXA_LOGIN_UNVERIFIED;
          enginesListDisabled = true;
        
        } else if (Weave.Status.login != Weave.LOGIN_SUCCEEDED) {
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
        let enginesList = document.getElementById("fxaSyncEnginesList")
        enginesList.disabled = enginesListDisabled;
        
        
        for (let checkbox of enginesList.querySelectorAll("checkbox")) {
          checkbox.disabled = enginesListDisabled;
        }
      });
    
    
    
    
    } else if (Weave.Status.service == Weave.CLIENT_NOT_CONFIGURED ||
               Weave.Svc.Prefs.get("firstSync", "") == "notReady") {
      this.page = PAGE_NO_ACCOUNT;
    
    
    } else if (Weave.Status.login == Weave.LOGIN_FAILED_INVALID_PASSPHRASE ||
               Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
      this.needsUpdate();
    } else {
      this.page = PAGE_HAS_ACCOUNT;
      document.getElementById("accountName").value = Weave.Service.identity.account;
      document.getElementById("syncComputerName").value = Weave.Service.clientsEngine.localName;
      document.getElementById("tosPP").hidden = this._usingCustomServer;
    }
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

      
      if (buttonChoice == 1) {
        return;
      }
    }

    Weave.Service.startOver();
    this.updateWeavePrefs();
  },

  updatePass: function () {
    if (Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
      gSyncUtils.changePassword();
    } else {
      gSyncUtils.updatePassphrase();
    }
  },

  resetPass: function () {
    if (Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
      gSyncUtils.resetPassword();
    } else {
      gSyncUtils.resetPassphrase();
    }
  },

  








  openSetup: function (wizardType) {
    let service = Components.classes["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;

    if (service.fxAccountsEnabled) {
      this.openContentInBrowser("about:accounts");
    } else {
      let win = Services.wm.getMostRecentWindow("Weave:AccountSetup");
      if (win) {
        win.focus();
      } else {
        window.openDialog("chrome://browser/content/sync/setup.xul",
                          "weaveSetup", "centerscreen,chrome,resizable=no",
                          wizardType);
      }
    }
  },

  openContentInBrowser: function(url) {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    if (!win) {
      
      
      
      gSyncUtils._openLink(url);
      return;
    }
    win.switchToTabHavingURI(url, true);
    
    window.close();
  },

  reSignIn: function() {
    this.openContentInBrowser("about:accounts");
  },

  manageFirefoxAccount: function() {
    let url = Services.prefs.getCharPref("identity.fxaccounts.settings.uri");
    this.openContentInBrowser(url);
  },

  verifyFirefoxAccount: function() {
    Components.utils.import("resource://gre/modules/FxAccounts.jsm");
    fxAccounts.resendVerificationEmail().then(() => {
      fxAccounts.getSignedInUser().then(data => {
        let sb = this._stringBundle;
        let title = sb.GetStringFromName("firefoxAccountsVerificationSentTitle");
        let heading = sb.formatStringFromName("firefoxAccountsVerificationSentHeading",
                                              [data.email], 1);
        let description = sb.GetStringFromName("firefoxAccountVerificationSentDescription");

        Services.prompt.alert(window, title, heading + "\n\n" + description);
      });
    });
  },

  openOldSyncSupportPage: function() {
    let url = Services.urlFormatter.formatURLPref('app.support.baseURL') + "old-sync"
    this.openContentInBrowser(url);
  },

  unlinkFirefoxAccount: function(confirm) {
    Components.utils.import('resource://gre/modules/FxAccounts.jsm');
    fxAccounts.signOut().then(() => {
      this.updateWeavePrefs();
    });
  },

  openQuotaDialog: function () {
    let win = Services.wm.getMostRecentWindow("Sync:ViewQuota");
    if (win) {
      win.focus();
    } else {
      window.openDialog("chrome://browser/content/sync/quota.xul", "",
                        "centerscreen,chrome,dialog,modal");
    }
  },

  openAddDevice: function () {
    if (!Weave.Utils.ensureMPUnlocked()) {
      return;
    }

    let win = Services.wm.getMostRecentWindow("Sync:AddDevice");
    if (win) {
      win.focus();
    } else {
      window.openDialog("chrome://browser/content/sync/addDevice.xul",
                        "syncAddDevice", "centerscreen,chrome,resizable=no");
    }
  },

  resetSync: function () {
    this.openSetup("reset");
  },
};

