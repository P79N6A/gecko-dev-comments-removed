





































let WeaveGlue = {
  setupData: null,
  autoConnect: false,
  jpake: null,

  init: function init() {
    this._bundle = Services.strings.createBundle("chrome://browser/locale/sync.properties");
    this._msg = document.getElementById("prefs-messages");

    this._addListeners();

    this.setupData = { account: "", password: "" , synckey: "", serverURL: "" };

    let enableSync = Services.prefs.getBoolPref("browser.sync.enabled");
    if (enableSync)
      this._elements.connect.collapsed = false;

    
    if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
      this.autoConnect = Services.prefs.getBoolPref("services.sync.autoconnect");
      if (enableSync && this.autoConnect) {
        
        this._elements.connect.firstChild.disabled = true;
        this._elements.connect.setAttribute("title", this._bundle.GetStringFromName("connecting.label"));

        try {
          this._elements.device.value = Services.prefs.getCharPref("services.sync.client.name");
        } catch(e) {}
      }
    } else if (Weave.Status.login != Weave.LOGIN_FAILED_NO_USERNAME) {
      this.loadSetupData();
    }
  },

  abortEasySetup: function abortEasySetup() {
    document.getElementById("syncsetup-code1").value = "....";
    document.getElementById("syncsetup-code2").value = "....";
    document.getElementById("syncsetup-code3").value = "....";
    if (!this.jpake)
      return;

    this.jpake.abort();
    this.jpake = null;
  },

  _resetScrollPosition: function _resetScrollPosition() {
    let scrollboxes = document.getElementsByClassName("syncsetup-scrollbox");
    for (let i = 0; i < scrollboxes.length; i++) {
      let sbo = scrollboxes[i].boxObject.QueryInterface(Ci.nsIScrollBoxObject);
      try {
        sbo.scrollTo(0, 0);
      } catch(e) {}
    }
  },

  open: function open() {
    let container = document.getElementById("syncsetup-container");
    if (!container.hidden)
      return;

    
    let nls = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
    if (!nls.isLinkUp) {
      Services.prompt.alert(window,
                             this._bundle.GetStringFromName("sync.setup.error.title"),
                             this._bundle.GetStringFromName("sync.setup.error.network"));
      return;
    }

    
    this.abortEasySetup();

    
    container.hidden = false;
    document.getElementById("syncsetup-simple").hidden = false;
    document.getElementById("syncsetup-fallback").hidden = true;

    BrowserUI.pushDialog(this);

    let self = this;
    this.jpake = new Weave.JPAKEClient({
      displayPIN: function displayPIN(aPin) {
        document.getElementById("syncsetup-code1").value = aPin.slice(0, 4);
        document.getElementById("syncsetup-code2").value = aPin.slice(4, 8);
        document.getElementById("syncsetup-code3").value = aPin.slice(8);
      },

      onComplete: function onComplete(aCredentials) {
        self.close();
        self.setupData = aCredentials;
        self.connect();
      },

      onAbort: function onAbort(aError) {
        self.jpake = null;

        if (aError == "jpake.error.userabort" || container.hidden)
          return;

        
        let brandShortName = Strings.brand.GetStringFromName("brandShortName");
        let tryAgain = self._bundle.GetStringFromName("sync.setup.tryagain");
        let manualSetup = self._bundle.GetStringFromName("sync.setup.manual");
        let buttonFlags = Ci.nsIPrompt.BUTTON_POS_1_DEFAULT +
                         (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                         (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1) +
                         (Ci.nsIPrompt.BUTTON_TITLE_CANCEL    * Ci.nsIPrompt.BUTTON_POS_2);

        let button = Services.prompt.confirmEx(window,
                               self._bundle.GetStringFromName("sync.setup.error.title"),
                               self._bundle.formatStringFromName("sync.setup.error.nodata", [brandShortName], 1),
                               buttonFlags, tryAgain, manualSetup, null, "", {});
        switch (button) {
          case 0:
            
            container.hidden = true;
            self.open();
            break;
          case 1:
            self.openManual();
            break;
          case 2:
          default:
            self.close();
            break;
        }
      }
    });
    this.jpake.receiveNoPIN();
  },

  openManual: function openManual() {
    this.abortEasySetup();

    
    this._resetScrollPosition();

    document.getElementById("syncsetup-simple").hidden = true;
    document.getElementById("syncsetup-fallback").hidden = false;

    
    if (this.setupData && "account" in this.setupData) {
      this._elements.account.value = this.setupData.account;
      this._elements.password.value = this.setupData.password;
      let pp = this.setupData.synckey;
      if (Weave.Utils.isPassphrase(pp))
        pp = Weave.Utils.hyphenatePassphrase(pp);
      this._elements.synckey.value = pp;
      if (this.setupData.serverURL && this.setupData.serverURL.length) {
        this._elements.usecustomserver.checked = true;
        this._elements.customserver.disabled = false;
        this._elements.customserver.value = this.setupData.serverURL;
      } else {
        this._elements.usecustomserver.checked = false;
        this._elements.customserver.disabled = true;
        this._elements.customserver.value = "";
      }
    }

    this.canConnect();
  },

  close: function close() {
    if (this.jpake)
      this.abortEasySetup();

    
    this._resetScrollPosition();

    
    this.setupData = {
      account: this._elements.account.value.trim(),
      password: this._elements.password.value.trim(),
      synckey: Weave.Utils.normalizePassphrase(this._elements.synckey.value.trim()),
      serverURL: this._validateServer(this._elements.customserver.value.trim())
    };

    
    this._elements.account.value = "";
    this._elements.password.value = "";
    this._elements.synckey.value = "";
    this._elements.usecustomserver.checked = false;
    this._elements.customserver.disabled = true;
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

  canConnect: function canConnect() {
    let account = this._elements.account.value;
    let password = this._elements.password.value;
    let synckey = this._elements.synckey.value;

    let disabled = !(account && password && synckey);
    document.getElementById("syncsetup-button-connect").disabled = disabled;
  },

  showDetails: function showDetails() {
    
    let show = this._elements.sync.collapsed;
    this._elements.details.checked = show;
    this._elements.sync.collapsed = !show;
    this._elements.device.collapsed = !show;
    this._elements.disconnect.collapsed = !show;
  },

  toggleSyncEnabled: function toggleSyncEnabled() {
    let enabled = this._elements.autosync.value;
    if (enabled) {
      
      if (this.setupData) {
        if (this.setupData.serverURL && this.setupData.serverURL.length)
          Weave.Service.serverURL = this.setupData.serverURL;

        
        
        this.observe(null, "", "");

        
        
        Weave.Service.login(Weave.Service.username, this.setupData.password, this.setupData.synckey);
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

    
    if (this.setupData.serverURL && this.setupData.serverURL.length)
      Weave.Service.serverURL = this.setupData.serverURL;

    
    Weave.Service.account = this.setupData.account;
    Weave.Service.login(Weave.Service.username, this.setupData.password, this.setupData.synckey);
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

    
    let panel = document.getElementById("prefs-container");
    panel.addEventListener("ToolPanelHidden", function onHide(aEvent) {
      panel.removeEventListener(aEvent.type, onHide, false);
      let notification = WeaveGlue._msg.getNotificationWithValue("undo-disconnect");
      if (notification)
        notification.close();
    }, false);

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

    
    if (!this.setupData && this.autoConnect && aTopic == "weave:service:login:finish")
      this.loadSetupData();

    
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
  },

  _validateServer: function _validateServer(aURL) {
    let uri = Weave.Utils.makeURI(aURL);

    if (!uri && aURL)
      uri = Weave.Utils.makeURI("https://" + aURL);

    if (!uri)
      return "";
    return uri.spec;
  },

  openTutorial: function _openTutorial() {
    WeaveGlue.close();

    let formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
    let url = formatter.formatURLPref("app.sync.tutorialURL");
    BrowserUI.newTab(url, Browser.selectedTab);
  },

  loadSetupData: function _loadSetupData() {
    this.setupData = {};
    this.setupData.account = Weave.Service.account || "";
    this.setupData.password = Weave.Service.password || "";
    this.setupData.synckey = Weave.Service.passphrase || "";

    let serverURL = Weave.Service.serverURL;
    let defaultPrefs = Services.prefs.getDefaultBranch(null);
    if (serverURL == defaultPrefs.getCharPref("services.sync.serverURL"))
      serverURL = "";
    this.setupData.serverURL = serverURL;
  }
};
