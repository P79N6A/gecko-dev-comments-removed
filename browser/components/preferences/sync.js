






































Components.utils.import("resource://services-sync/service.js");
Components.utils.import("resource://gre/modules/Services.jsm");

const PAGE_NO_ACCOUNT = 0;
const PAGE_HAS_ACCOUNT = 1;

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

  onLoginStart: function () {
    if (this.page == PAGE_NO_ACCOUNT)
      return;

    document.getElementById("loginFeedbackRow").hidden = true;
    document.getElementById("connectThrobber").hidden = false;
  },

  onLoginError: function () {
    if (this.page == PAGE_NO_ACCOUNT)
      return;

    document.getElementById("connectThrobber").hidden = true;
    document.getElementById("loginFeedbackRow").hidden = false;
    let label = document.getElementById("loginError");
    label.value = Weave.Utils.getErrorString(Weave.Status.login);
    label.className = "error";
  },

  onLoginFinish: function () {
    document.getElementById("connectThrobber").hidden = true;
    this.updateWeavePrefs();
  },

  init: function () {
    let obs = [
      ["weave:service:login:start",   "onLoginStart"],
      ["weave:service:login:error",   "onLoginError"],
      ["weave:service:login:finish",  "onLoginFinish"],
      ["weave:service:start-over",    "updateWeavePrefs"],
      ["weave:service:setup-complete","updateWeavePrefs"],
      ["weave:service:logout:finish", "updateWeavePrefs"]];

    
    let self = this;
    let addRem = function(add) {
      obs.forEach(function([topic, func]) {
        
        
        if (add)
          Weave.Svc.Obs.add(topic, self[func], self);
        else
          Weave.Svc.Obs.remove(topic, self[func], self);
      });
    };
    addRem(true);
    window.addEventListener("unload", function() addRem(false), false);

    this._stringBundle =
      Services.strings.createBundle("chrome://browser/locale/preferences/preferences.properties");;
    this.updateWeavePrefs();
  },

  updateWeavePrefs: function () {
    if (Weave.Status.service == Weave.CLIENT_NOT_CONFIGURED ||
        Weave.Svc.Prefs.get("firstSync", "") == "notReady")
      this.page = PAGE_NO_ACCOUNT;
    else {
      this.page = PAGE_HAS_ACCOUNT;
      document.getElementById("currentAccount").value = Weave.Service.account;
      document.getElementById("syncComputerName").value = Weave.Clients.localName;
      if (Weave.Status.service == Weave.LOGIN_FAILED)
        this.onLoginError();
      this.updateConnectButton();
      document.getElementById("tosPP").hidden = this._usingCustomServer;
    }
  },

  updateConnectButton: function () {
    let str = Weave.Service.isLoggedIn ? this._stringBundle.GetStringFromName("disconnect.label")
                                       : this._stringBundle.GetStringFromName("connect.label");
    document.getElementById("connectButton").label = str;
  },

  handleConnectCommand: function () {
    Weave.Service.isLoggedIn ? Weave.Service.logout() : Weave.Service.login();
  },

  startOver: function (showDialog) {
    if (showDialog) {
      let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_IS_STRING +
                  Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_CANCEL;
      let buttonChoice =
        Services.prompt.confirmEx(window,
                                  this._stringBundle.GetStringFromName("differentAccount.title"),
                                  this._stringBundle.GetStringFromName("differentAccount.label"),
                                  flags,
                                  this._stringBundle.GetStringFromName("differentAccountConfirm.label"),
                                  null, null, null, {});

      
      if (buttonChoice == 1)
        return;
    }

    this.handleExpanderClick();
    Weave.Service.startOver();
    this.updateWeavePrefs();
    document.getElementById("manageAccountExpander").className = "expander-down";
    document.getElementById("manageAccountControls").hidden = true;
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

  handleExpanderClick: function () {
    
    
    
    let prefwindow = document.documentElement;
    let pane = document.getElementById("paneSync");
    if (prefwindow._shouldAnimate)
      prefwindow._currentHeight = pane.contentHeight;

    let expander = document.getElementById("manageAccountExpander");
    let expand = expander.className == "expander-down";
    expander.className =
       expand ? "expander-up" : "expander-down";
    document.getElementById("manageAccountControls").hidden = !expand;

    
    if (prefwindow._shouldAnimate)
      prefwindow.animate("null", pane);
  },

  openSetup: function (resetSync) {
    var win = Services.wm.getMostRecentWindow("Weave:AccountSetup");
    if (win)
      win.focus();
    else {
      window.openDialog("chrome://browser/content/syncSetup.xul",
                        "weaveSetup", "centerscreen,chrome,resizable=no", resetSync);
    }
  },

  openQuotaDialog: function () {
    let win = Services.wm.getMostRecentWindow("Sync:ViewQuota");
    if (win)
      win.focus();
    else 
      window.openDialog("chrome://browser/content/syncQuota.xul", "",
                        "centerscreen,chrome,dialog,modal");
  },

  resetSync: function () {
    this.openSetup(true);
  }
}

