



































let WeaveGlue = {
  autoConnect: false,

  init: function init() {
    Components.utils.import("resource://services-sync/main.js");

    this._bundle = Services.strings.createBundle("chrome://browser/locale/sync.properties");
    this._msg = document.getElementById("prefs-messages");

    this._addListeners();

    
    if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
      Weave.Service.keyGenEnabled = false;

      this.autoConnect = Services.prefs.getBoolPref("services.sync.autoconnect");
      if (this.autoConnect) {
        
        this._elements.connect.collapsed = false;
        this._elements.sync.collapsed = false;

        this._elements.connect.firstChild.disabled = true;
        this._elements.sync.firstChild.disabled = true;

        this._elements.connect.setAttribute("title", this._bundle.GetStringFromName("connecting.label"));
        this._elements.autosync.value = true;

        try {
          this._elements.device.value = Services.prefs.getCharPref("services.sync.client.name");
        } catch(e) {}
      }
    }
  },

  show: function show() {
    
    document.getElementById("syncsetup-container").hidden = false;
    document.getElementById("syncsetup-jpake").hidden = false;
    document.getElementById("syncsetup-manual").hidden = true;
  },

  close: function close() {
    
    document.getElementById("syncsetup-container").hidden = true;
  },

  showDetails: function showDetails() {
    
    let show = this._elements.details.checked;
    this._elements.autosync.collapsed = show;
    this._elements.device.collapsed = show;
    this._elements.disconnect.collapsed = show;
  },

  connect: function connect() {
    
    if (this._elements.account.value != Weave.Service.account)
      Weave.Service.startOver();

    
    this._elements.connect.removeAttribute("desc");

    
    Weave.Service.account = this._elements.account.value;
    Weave.Service.login(Weave.Service.username, this._elements.password.value, this.normalizePassphrase(this._elements.synckey.value));
    Weave.Service.persistLogin();
  },

  disconnect: function disconnect() {
    let message = this._bundle.GetStringFromName("notificationDisconnect.label");
    let button = this._bundle.GetStringFromName("notificationDisconnect.button");
    let buttons = [ {
      label: button,
      accessKey: "",
      callback: function() { WeaveGlue.connect(); }
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
    let setupids = ["account", "password", "synckey", "customserver"];
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

    
    let account = this._elements.account;
    let password = this._elements.password;
    let synckey = this._elements.synckey;
    let connect = this._elements.connect;
    let connected = this._elements.connected;
    let autosync = this._elements.autosync;
    let device = this._elements.device;
    let disconnect = this._elements.disconnect;
    let sync = this._elements.sync;

    
    connect.collapsed = loggedIn;
    connected.collapsed = !loggedIn;
    sync.collapsed = !loggedIn;

    if (connected.collapsed) {
      connect.setAttribute("title", this._bundle.GetStringFromName("notconnected.label"));
      this._elements.details.checked = false;
      this._elements.autosync.collapsed = true;
      this._elements.device.collapsed = true;
      this._elements.disconnect.collapsed = true;
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

    
    let parent = connect.parentNode;
    if (!loggedIn)
      parent = parent.parentNode;
    parent.appendChild(disconnect);
    parent.appendChild(sync);

    
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

    
    account.value = Weave.Service.account || "";
    password.value = Weave.Service.password || "";
    let pp = Weave.Service.passphrase || "";
    if (pp.length == 20)
      pp = this.hyphenatePassphrase(pp);
    synckey.value = pp;
    device.value = Weave.Clients.localName || "";
  },

  changeName: function changeName(aInput) {
    
    Weave.Clients.localName = aInput.value;
    aInput.value = Weave.Clients.localName;
  },

  changeSync: function changeSync() {
    
  },

  showMessage: function showMessage(aMsg, aValue, aButtons) {
    let notification = this._msg.getNotificationWithValue(aValue);
    if (notification)
      return;

    this._msg.appendNotification(aMsg, aValue, "", this._msg.PRIORITY_WARNING_LOW, aButtons);
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
