



































let WeaveGlue = {
  setupData: null,
  autoConnect: false,

  init: function init() {
    Components.utils.import("resource://services-sync/main.js");

    this._bundle = Services.strings.createBundle("chrome://browser/locale/sync.properties");
    this._msg = document.getElementById("prefs-messages");

    this._addListeners();

    this.setupData = { account: "", password: "" , syncKey: "", customServer: "" };

    let enableSync = Services.prefs.getBoolPref("browser.sync.enabled");
    if (enableSync)
      this._elements.connect.collapsed = false;

    
    if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
      Weave.Service.keyGenEnabled = false;

      this.autoConnect = Services.prefs.getBoolPref("services.sync.autoconnect");
      if (enableSync && this.autoConnect) {
        
        this._elements.connect.firstChild.disabled = true;
        this._elements.connect.setAttribute("title", this._bundle.GetStringFromName("connecting.label"));

        try {
          this._elements.device.value = Services.prefs.getCharPref("services.sync.client.name");
        } catch(e) {}
      }
    }
  },

  open: function open() {
    
    document.getElementById("syncsetup-container").hidden = false;
    document.getElementById("syncsetup-jpake").hidden = false;
    document.getElementById("syncsetup-manual").hidden = true;

    BrowserUI.pushDialog(this);
  },

  openManual: function openManual() {
    
    let scrollbox = document.getElementById("syncsetup-scrollbox").boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    scrollbox.scrollTo(0, 0);

    document.getElementById("syncsetup-jpake").hidden = true;
    document.getElementById("syncsetup-manual").hidden = false;

    
    if (this.setupData && "account" in this.setupData) {
      this._elements.account.value = this.setupData.account;
      this._elements.password.value = this.setupData.password;
      this._elements.synckey.value = this.setupData.syncKey;
      if (this.setupData.customServer && this.setupData.customServer.length) {
        this._elements.usecustomserver.checked = true;
        this._elements.customserver.disabled = false;
        this._elements.customserver.value = this.setupData.customServer;
      } else {
        this._elements.usecustomserver.checked = false;
        this._elements.customserver.disabled = true;
        this._elements.customserver.value = "";
      }
    }
  },
  
  close: function close() {
    let scrollbox = document.getElementById("syncsetup-scrollbox").boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    scrollbox.scrollTo(0, 0);

    
    this.setupData = {};
    this.setupData.account = this._elements.account.value;
    this.setupData.password = this._elements.password.value;
    this.setupData.syncKey = this._elements.synckey.value;
    this.setupData.customServer = this._elements.customserver.value;

    
    this._elements.account.value = "";
    this._elements.password.value = "";
    this._elements.synckey.value = "";
    this._elements.usecustomserver.checked = false;
    this._elements.customserver.disable = true;
    this._elements.customserver.value = "";

    
    document.getElementById("syncsetup-container").hidden = true;
    BrowserUI.popDialog();
  },

  toggleCustomServer: function toggleCustomServer() {
    let useCustomServer = this._elements.usecustomserver.checked;
    this._elements.customserver.disabled = !useCustomServer;
    if (!useCustomServer)
      this._elements.customserver.value = "";
  },
  
  showDetails: function showDetails() {
    
    let show = this._elements.details.checked;
    this._elements.sync.collapsed = !show;
    this._elements.device.collapsed = !show;
    this._elements.disconnect.collapsed = !show;
  },

  toggleSyncEnabled: function toggleSyncEnabled() {
    let enabled = this._elements.autosync.value;
    if (enabled) {
      
      if (this.setupData) {
        if (this.setupData.customServer && this.setupData.customServer.length)
          Weave.Service.serverURL = this.setupData.customServer;
        Weave.Service.login(Weave.Service.username, this.setupData.password, Weave.Utils.normalizePassphrase(this.setupData.syncKey));
      } else {
        
        this._elements.connected.collapsed = true;
        this._elements.connect.collapsed = false;
      }
    } else {
      this._elements.connect.collapsed = true;
      this._elements.connected.collapsed = true;
      Weave.Service.logout();
    }

    
    let notification = this._msg.getNotificationWithValue("undo-disconnect");
    if (notification)
      notification.close();
  },

  connect: function connect(aSetupData) {
    
    if (aSetupData)
      this.setupData = aSetupData;

    
    if (this.setupData.account != Weave.Service.account)
      Weave.Service.startOver();

    
    this._elements.connect.removeAttribute("desc");

    
    if (this.setupData.customServer && this.setupData.customServer.length)
      Weave.Service.serverURL = this.setupData.customServer;

    
    Weave.Service.account = this.setupData.account;
    Weave.Service.login(Weave.Service.username, this.setupData.password, Weave.Utils.normalizePassphrase(this.setupData.syncKey));
    Weave.Service.persistLogin();
  },

  disconnect: function disconnect() {
    
    let undoData = this.setupData;

    
    this.setupData = null;
    Weave.Service.startOver();

    let message = this._bundle.GetStringFromName("notificationDisconnect.label");
    let button = this._bundle.GetStringFromName("notificationDisconnect.button");
    let buttons = [ {
      label: button,
      accessKey: "",
      callback: function() { WeaveGlue.connect(undoData); }
    } ];
    this.showMessage(message, "undo-disconnect", buttons);

    
    setTimeout(function(self) {
      let notification = self._msg.getNotificationWithValue("undo-disconnect");
      if (notification)
        notification.close();
    }, 10000, this);

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

  get _elements() {
    
    let syncButton = document.getElementById("sync-syncButton");
    if (syncButton == null)
      return null;

    
    let elements = {};
    let setupids = ["account", "password", "synckey", "usecustomserver", "customserver"];
    setupids.forEach(function(id) {
      elements[id] = document.getElementById("syncsetup-" + id);
    });

    let settingids = ["device", "connect", "connected", "disconnect", "sync", "autosync", "details"];
    settingids.forEach(function(id) {
      elements[id] = document.getElementById("sync-" + id);
    });

    
    delete this._elements;
    return this._elements = elements;
  },

  observe: function observe(aSubject, aTopic, aData) {
    let loggedIn = Weave.Service.isLoggedIn;
    document.getElementById("cmd_remoteTabs").setAttribute("disabled", !loggedIn);

    
    
    loggedIn = loggedIn || this.autoConnect;

    
    Util.forceOnline();

    
    if (this._elements == null)
      return;

    
    let connect = this._elements.connect;
    let connected = this._elements.connected;
    let autosync = this._elements.autosync;
    let details = this._elements.details;
    let device = this._elements.device;
    let disconnect = this._elements.disconnect;
    let sync = this._elements.sync;

    let syncEnabled = this._elements.autosync.value;

    
    if (syncEnabled) {
      connect.collapsed = loggedIn;
      connected.collapsed = !loggedIn;
    } else {
      connect.collapsed = true;
      connected.collapsed = true;
    }

    if (!loggedIn) {
      connect.setAttribute("title", this._bundle.GetStringFromName("notconnected.label"));
      connect.firstChild.disabled = false;
      details.checked = false;
      sync.collapsed = true;
      device.collapsed = true;
      disconnect.collapsed = true;
    }

    
    setTimeout(function(self) {
      
      if (Weave.Service.locked) {
        connect.firstChild.disabled = true;
        sync.firstChild.disabled = true;

        if (aTopic == "weave:service:login:start")
          connect.setAttribute("title", self._bundle.GetStringFromName("connecting.label"));

        if (aTopic == "weave:service:sync:start")
          sync.setAttribute("title", self._bundle.GetStringFromName("lastSyncInProgress.label"));
      } else {
        connect.firstChild.disabled = false;
        sync.firstChild.disabled = false;
      }
    }, 0, this);

    
    let accountStr = this._bundle.formatStringFromName("account.label", [Weave.Service.account], 1);
    disconnect.setAttribute("title", accountStr);

    
    let lastSync = Weave.Svc.Prefs.get("lastSync");
    if (lastSync != null) {
      let syncDate = new Date(lastSync).toLocaleFormat("%a %R");
      let dateStr = this._bundle.formatStringFromName("lastSync.label", [syncDate], 1);
      sync.setAttribute("title", dateStr);
    }

    
    if (aTopic == "weave:service:login:error")
      connect.setAttribute("desc", Weave.Utils.getErrorString(Weave.Status.login));
    else
      connect.removeAttribute("desc");

    
    if (!this.setupData && this.autoConnect && aTopic == "weave:service:login:finish") {
      this.setupData = {};
      this.setupData.account = Weave.Service.account || "";
      this.setupData.password = Weave.Service.password || "";

      let pp = Weave.Service.passphrase || "";
      if (pp.length == 20)
        pp = Weave.Utils.hyphenatePassphrase(pp);
      this.setupData.syncKey = pp;

      let serverURL = Weave.Service.serverURL;
      let defaultPrefs = Services.prefs.getDefaultBranch(null);
      if (serverURL == defaultPrefs.getCharPref("services.sync.serverURL"))
        serverURL = "";
      this.setupData.customServer = serverURL;
    }
    
    
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
        let brand = Services.strings.createBundle("chrome://branding/locale/brand.properties");
        let brandName = brand.GetStringFromName("brandShortName");

        let type = clientOutdated ? "client" : "remote";
        let message = this._bundle.GetStringFromName("sync.update." + type);
        message = message.replace("#1", brandName);
        message = message.replace("#2", Services.appinfo.version);
        let title = this._bundle.GetStringFromName("sync.update.title")
        let button = this._bundle.GetStringFromName("sync.update.button")
        let close = this._bundle.GetStringFromName("sync.update.close")

        let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_IS_STRING +
                    Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_IS_STRING;
        let choice = Services.prompt.confirmEx(window, title, message, flags, button, close, null, null, {});
        if (choice == 0)
          Browser.addTab("https://services.mozilla.com/update/", true, Browser.selectedTab);
      }
    }

    device.value = Weave.Clients.localName || "";
  },

  changeName: function changeName(aInput) {
    
    Weave.Clients.localName = aInput.value;
    aInput.value = Weave.Clients.localName;
  },

  showMessage: function showMessage(aMsg, aValue, aButtons) {
    let notification = this._msg.getNotificationWithValue(aValue);
    if (notification)
      return;

    this._msg.appendNotification(aMsg, aValue, "", this._msg.PRIORITY_WARNING_LOW, aButtons);
  }
};
