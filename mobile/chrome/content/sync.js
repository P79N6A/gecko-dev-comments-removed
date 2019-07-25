



































let WeaveGlue = {
  init: function init() {
    Components.utils.import("resource://services-sync/main.js");

    this._addListeners();

    
    if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED)
      Weave.Service.keyGenEnabled = false;

    
    this.loadInputs();
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

    
    this.loadInputs();
  },

  loadInputs: function loadInputs() {
    this._settings.account.value = Weave.Service.account || "";
    this._settings.pass.value = Weave.Service.password || "";
    let pp = Weave.Service.passphrase || "";
    if (pp.length == 20)
      pp = this.hyphenatePassphrase(pp);
    this._settings.secret.value = pp;
    this._settings.device.value = Weave.Clients.localName || "";
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
