



































let WeaveGlue = {
  autoConnect: false,

  init: function init() {
    Components.utils.import("resource://services-sync/main.js");

    this._addListeners();

    
    if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
      Weave.Service.keyGenEnabled = false;

      this.autoConnect = Services.prefs.getBoolPref("services.sync.autoconnect");
      if (this.autoConnect) {
        
        this._settings.account.collapsed = true;
        this._settings.pass.collapsed = true;
        this._settings.secret.collapsed = true;
        this._settings.connect.collapsed = false;
        this._settings.device.collapsed = false;
        this._settings.disconnect.collapsed = true;
        this._settings.sync.collapsed = false;

        this._settings.connect.firstChild.disabled = true;
        this._settings.sync.firstChild.disabled = true;

        let bundle = Services.strings.createBundle("chrome://weave/locale/services/sync.properties");
        this._settings.connect.setAttribute("title", bundle.GetStringFromName("connecting.label"));

        try {
          this._settings.device.value = Services.prefs.getCharPref("services.sync.client.name");
        } catch(e) {}
      }
    }
  },

  connect: function connect() {
    
    if (this._settings.account.value != Weave.Service.account)
      Weave.Service.startOver();

    
    this._settings.connect.removeAttribute("desc");

    
    Weave.Service.account = this._settings.account.value;
    Weave.Service.login(Weave.Service.username, this._settings.pass.value, this.normalizePassphrase(this._settings.secret.value));
    Weave.Service.persistLogin();
  },

  disconnect: function disconnect() {
    Weave.Service.logout();
  },

  sync: function sync() {
    Weave.Service.sync();
  },

  _addListeners: function _addListeners() {
    let topics = ["weave:service:sync:start", "weave:service:sync:finish",
      "weave:service:sync:error", "weave:service:login:start",
      "weave:service:login:finish", "weave:service:login:error",
      "weave:service:logout:finish"];

    
    topics.forEach(function(topic) {
      Services.obs.addObserver(WeaveGlue, topic, false);
    });

    
    addEventListener("unload", function() {
      topics.forEach(function(topic) {
        Services.obs.removeObserver(WeaveGlue, topic, false);
      });
    }, false);
  },

  get _settings() {
    
    let syncButton = document.getElementById("sync-syncButton");
    if (syncButton == null)
      return;

    
    let settings = {};
    let ids = ["account", "pass", "secret", "device", "connect", "disconnect", "sync"];
    ids.forEach(function(id) {
      settings[id] = document.getElementById("sync-" + id);
    });

    
    delete this._settings;
    return this._settings = settings;
  },

  observe: function observe(aSubject, aTopic, aData) {
    let loggedIn = Weave.Service.isLoggedIn;
    document.getElementById("cmd_remoteTabs").setAttribute("disabled", !loggedIn);

    
    
    loggedIn = loggedIn || this.autoConnect;

    
    Util.forceOnline();

    
    if (this._settings == null)
      return;

    
    let account = this._settings.account;
    let pass = this._settings.pass;
    let secret = this._settings.secret;
    let connect = this._settings.connect;
    let device = this._settings.device;
    let disconnect = this._settings.disconnect;
    let sync = this._settings.sync;
    let syncStr = Weave.Str.sync;

    
    account.collapsed = loggedIn;
    pass.collapsed = loggedIn;
    secret.collapsed = loggedIn;
    connect.collapsed = loggedIn;
    device.collapsed = !loggedIn;
    disconnect.collapsed = !loggedIn;
    sync.collapsed = !loggedIn;

    
    setTimeout(function() {
      
      if (Weave.Service.locked) {
        connect.firstChild.disabled = true;
        sync.firstChild.disabled = true;

        if (aTopic == "weave:service:login:start")
          connect.setAttribute("title", syncStr.get("connecting.label"));

        if (aTopic == "weave:service:sync:start")
          sync.setAttribute("title", syncStr.get("lastSyncInProgress.label"));
      } else {
        connect.firstChild.disabled = false;
        sync.firstChild.disabled = false;
        connect.setAttribute("title", syncStr.get("disconnected.label"));
      }
    }, 0);

    
    let parent = connect.parentNode;
    if (!loggedIn)
      parent = parent.parentNode;
    parent.appendChild(disconnect);
    parent.appendChild(sync);

    
    let connectedStr = syncStr.get("connected.label", [Weave.Service.account]);
    disconnect.setAttribute("title", connectedStr);

    
    let lastSync = Weave.Svc.Prefs.get("lastSync");
    if (lastSync != null) {
      let syncDate = new Date(lastSync).toLocaleFormat("%a %R");
      let dateStr = syncStr.get("lastSync.label", [syncDate]);
      sync.setAttribute("title", dateStr);
    }

    
    if (aTopic == "weave:service:login:error")
      connect.setAttribute("desc", Weave.Utils.getErrorString(Weave.Status.login));
    else
      connect.removeAttribute("desc");

    
    if (aTopic == "weave:service:login:finish" || aTopic == "weave:service:login:error")
      this.autoConnect = false;

    
    if (aTopic =="weave:service:sync:error") {
      let clientOutdated = false, remoteOutdated = false;
      if (Weave.Status.sync == Weave.VERSION_OUT_OF_DATE) {
        clientOutdated = true;
      } else if (Weave.Status.sync == Weave.DESKTOP_VERSION_OUT_OF_DATE) {
        remoteOutdated = true;
      } else if (Weave.Status.service == Weave.SYNC_FAILED_PARTIAL) {
        
        for (let [engine, reason] in Iterator(Weave.Status.engines)) {
           clientOutdated = clientOutdated || reason == Weave.VERSION_OUT_OF_DATE;
           remoteOutdated = remoteOutdated || reason == Weave.DESKTOP_VERSION_OUT_OF_DATE;
        }
      }

      if (clientOutdated || remoteOutdated) {
        let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
        let brand = Services.strings.createBundle("chrome://branding/locale/brand.properties");
        let brandName = brand.GetStringFromName("brandShortName");

        let type = clientOutdated ? "client" : "remote";
        let message = bundle.GetStringFromName("sync.update." + type);
        message = message.replace("#1", brandName);
        message = message.replace("#2", Services.appinfo.version);
        let title = bundle.GetStringFromName("sync.update.title")
        let button = bundle.GetStringFromName("sync.update.button")
        let close = bundle.GetStringFromName("sync.update.close")

        let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_IS_STRING +
                    Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_IS_STRING;
        let choice = Services.prompt.confirmEx(window, title, message, flags, button, close, null, null, {});
        if (choice == 0)
          Browser.addTab("https://services.mozilla.com/update/", true, Browser.selectedTab);
      }
    }

    
    account.value = Weave.Service.account || "";
    pass.value = Weave.Service.password || "";
    let pp = Weave.Service.passphrase || "";
    if (pp.length == 20)
      pp = this.hyphenatePassphrase(pp);
    secret.value = pp;
    device.value = Weave.Clients.localName || "";
  },

  changeName: function changeName(aInput) {
    
    Weave.Clients.localName = aInput.value;
    aInput.value = Weave.Clients.localName;
  },

  hyphenatePassphrase: function(passphrase) {
    
    return passphrase.slice(0, 5) + '-'
         + passphrase.slice(5, 10) + '-'
         + passphrase.slice(10, 15) + '-'
         + passphrase.slice(15, 20);
  },

  normalizePassphrase: function(pp) {
    
    if (pp.length == 23 && pp[5] == '-' && pp[11] == '-' && pp[17] == '-')
      return pp.slice(0, 5) + pp.slice(6, 11) + pp.slice(12, 17) + pp.slice(18, 23);
    return pp;
  }
};
