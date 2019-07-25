



































let WeaveGlue = {
  init: function init() {
    Components.utils.import("resource://services-sync/service.js");

    this._addListeners();

    
    this._updateOptions();

    
    Weave.Service.keyGenEnabled = false;
  },

  openRemoteTabs: function openRemoteTabs() {
    BrowserUI.newOrSelectTab("about:sync-tabs", null);
  },

  connect: function connect() {
    Weave.Service.login(this._settings.user.value, this._settings.pass.value,
      this._settings.secret.value);
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

    
    let addRem = function(add) topics.forEach(function(topic) Weave.Svc.
      Obs[add ? "add" : "remove"](topic, WeaveGlue._updateOptions, WeaveGlue));

    
    addRem(true);
    addEventListener("unload", function() addRem(false), false);
  },

  get _settings() {
    
    let syncButton = document.getElementById("sync-syncButton");
    if (syncButton == null)
      return;

    
    let settings = {};
    let ids = ["user", "pass", "secret", "device", "connect", "disconnect", "sync"];
    ids.forEach(function(id) {
      settings[id] = document.getElementById("sync-" + id);
    });

    
    delete this._settings;
    return this._settings = settings;
  },

  _updateOptions: function _updateOptions() {
    let loggedIn = Weave.Service.isLoggedIn;
    document.getElementById("remotetabs-button").disabled = !loggedIn;

    
    Util.forceOnline();

    
    if (this._settings == null)
      return;

    
    let user = this._settings.user;
    let pass = this._settings.pass;
    let secret = this._settings.secret;
    let connect = this._settings.connect;
    let device = this._settings.device;
    let disconnect = this._settings.disconnect;
    let sync = this._settings.sync;
    let syncStr = Weave.Str.sync;

    
    user.collapsed = loggedIn;
    pass.collapsed = loggedIn;
    secret.collapsed = loggedIn;
    connect.collapsed = loggedIn;
    device.collapsed = !loggedIn;
    disconnect.collapsed = !loggedIn;
    sync.collapsed = !loggedIn;

    
    setTimeout(Weave.Utils.bind2(this, function() {
      
      if (Weave.Service.locked) {
        connect.firstChild.disabled = true;
        sync.firstChild.disabled = true;
        connect.setAttribute("title", syncStr.get("connecting.label"));
        sync.setAttribute("title", syncStr.get("lastSyncInProgress.label"));
      } else {
        connect.firstChild.disabled = false;
        sync.firstChild.disabled = false;
        connect.setAttribute("title", syncStr.get("disconnected.label"));
      }
    }), 0);

    
    let parent = connect.parentNode;
    if (!loggedIn)
      parent = parent.parentNode;
    parent.appendChild(disconnect);
    parent.appendChild(sync);

    
    let connectedStr = syncStr.get("connected.label", [Weave.Service.username]);
    disconnect.setAttribute("title", connectedStr);

    
    let lastSync = Weave.Svc.Prefs.get("lastSync");
    if (lastSync != null) {
      let syncDate = new Date(lastSync).toLocaleFormat("%a %R");
      let dateStr = syncStr.get("lastSync.label", [syncDate]);
      sync.setAttribute("title", dateStr);
    }

    
    let login = Weave.Status.login;
    if (login == Weave.LOGIN_SUCCEEDED)
      connect.removeAttribute("desc");
    else if (login != null)
      connect.setAttribute("desc", Weave.Str.errors.get(login));

    
    user.value = Weave.Service.username || "";
    pass.value = Weave.Service.password || "";
    secret.value = Weave.Service.passphrase || "";
    device.value = Weave.Clients.localName || "";
  },

  changeName: function changeName(input) {
    
    Weave.Clients.localName = input.value;
    input.value = Weave.Clients.localName;
  }
};
