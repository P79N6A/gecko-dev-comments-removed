



Components.utils.import("resource://services-sync/main.js");

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

  init: function () {
    
    let xps = Components.classes["@mozilla.org/weave/service;1"]
                                .getService(Components.interfaces.nsISupports)
                                .wrappedJSObject;

    if (xps.ready) {
      this._init();
      return;
    }

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
    if (Weave.Status.service == Weave.CLIENT_NOT_CONFIGURED ||
        Weave.Svc.Prefs.get("firstSync", "") == "notReady" ||
        Weave.Status.login == Weave.LOGIN_FAILED_INVALID_PASSPHRASE ||
        Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED) {
      this.page = PAGE_NO_ACCOUNT;
    } else {
      this.page = PAGE_HAS_ACCOUNT;
      document.getElementById("accountName").value = Weave.Service.identity.account;
      document.getElementById("syncComputerName").value = Weave.Service.clientsEngine.localName;
      document.getElementById("tosPP").hidden = this._usingCustomServer;
    }
  },

  openAccountsPage: function () {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    win.switchToTabHavingURI("about:accounts", true);
  }
};
