



"use strict";

let { Ci, Cu } = require("chrome");
let Services = require("Services");
let { ActorPool } = require("devtools/server/actors/common");
let { TabSources } = require("./utils/TabSources");
let makeDebugger = require("./utils/make-debugger");
let { ConsoleAPIListener } = require("devtools/toolkit/webconsole/utils");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dbg_assert, update } = DevToolsUtils;

loader.lazyRequireGetter(this, "AddonThreadActor", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "unwrapDebuggerObjectGlobal", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "mapURIToAddonID", "devtools/server/actors/utils/map-uri-to-addon-id");
loader.lazyRequireGetter(this, "WebConsoleActor", "devtools/server/actors/webconsole", true);

loader.lazyImporter(this, "AddonManager", "resource://gre/modules/AddonManager.jsm");

function BrowserAddonActor(aConnection, aAddon) {
  this.conn = aConnection;
  this._addon = aAddon;
  this._contextPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._contextPool);
  this.threadActor = null;
  this._global = null;

  this._shouldAddNewGlobalAsDebuggee = this._shouldAddNewGlobalAsDebuggee.bind(this);

  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: this._findDebuggees.bind(this),
    shouldAddNewGlobalAsDebuggee: this._shouldAddNewGlobalAsDebuggee
  });

  AddonManager.addAddonListener(this);
}
exports.BrowserAddonActor = BrowserAddonActor;

BrowserAddonActor.prototype = {
  actorPrefix: "addon",

  get exited() {
    return !this._addon;
  },

  get id() {
    return this._addon.id;
  },

  get url() {
    return this._addon.sourceURI ? this._addon.sourceURI.spec : undefined;
  },

  get attached() {
    return this.threadActor;
  },

  get global() {
    return this._global;
  },

  get sources() {
    if (!this._sources) {
      dbg_assert(this.threadActor, "threadActor should exist when creating sources.");
      this._sources = new TabSources(this.threadActor, this._allowSource);
    }
    return this._sources;
  },


  form: function BAA_form() {
    dbg_assert(this.actorID, "addon should have an actorID.");
    if (!this._consoleActor) {
      this._consoleActor = new AddonConsoleActor(this._addon, this.conn, this);
      this._contextPool.addActor(this._consoleActor);
    }

    return {
      actor: this.actorID,
      id: this.id,
      name: this._addon.name,
      url: this.url,
      debuggable: this._addon.isDebuggable,
      consoleActor: this._consoleActor.actorID,

      traits: {
        highlightable: false,
        networkMonitor: false,
      },
    };
  },

  disconnect: function BAA_disconnect() {
    this.conn.removeActorPool(this._contextPool);
    this._contextPool = null;
    this._consoleActor = null;
    this._addon = null;
    this._global = null;
    AddonManager.removeAddonListener(this);
  },

  setOptions: function BAA_setOptions(aOptions) {
    if ("global" in aOptions) {
      this._global = aOptions.global;
    }
  },

  onDisabled: function BAA_onDisabled(aAddon) {
    if (aAddon != this._addon) {
      return;
    }

    this._global = null;
  },

  onUninstalled: function BAA_onUninstalled(aAddon) {
    if (aAddon != this._addon) {
      return;
    }

    if (this.attached) {
      this.onDetach();
      this.conn.send({ from: this.actorID, type: "tabDetached" });
    }

    this.disconnect();
  },

  onAttach: function BAA_onAttach() {
    if (this.exited) {
      return { type: "exited" };
    }

    if (!this.attached) {
      this.threadActor = new AddonThreadActor(this.conn, this);
      this._contextPool.addActor(this.threadActor);
    }

    return { type: "tabAttached", threadActor: this.threadActor.actorID };
  },

  onDetach: function BAA_onDetach() {
    if (!this.attached) {
      return { error: "wrongState" };
    }

    this._contextPool.removeActor(this.threadActor);

    this.threadActor = null;
    this._sources = null;

    return { type: "detached" };
  },

  preNest: function() {
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.suppressEventHandling(true);
      windowUtils.suspendTimeouts();
    }
  },

  postNest: function() {
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.resumeTimeouts();
      windowUtils.suppressEventHandling(false);
    }
  },

  



  _shouldAddNewGlobalAsDebuggee: function (aGlobal) {
    const global = unwrapDebuggerObjectGlobal(aGlobal);
    try {
      
      let metadata = Cu.getSandboxMetadata(global);
      if (metadata) {
        return metadata.addonID === this.id;
      }
    } catch (e) {}

    if (global instanceof Ci.nsIDOMWindow) {
      let id = {};
      if (mapURIToAddonID(global.document.documentURIObject, id)) {
        return id.value === this.id;
      }
      return false;
    }

    
    
    let uridescriptor = aGlobal.getOwnPropertyDescriptor("__URI__");
    if (uridescriptor && "value" in uridescriptor && uridescriptor.value) {
      let uri;
      try {
        uri = Services.io.newURI(uridescriptor.value, null, null);
      }
      catch (e) {
        DevToolsUtils.reportException(
          "BrowserAddonActor.prototype._shouldAddNewGlobalAsDebuggee",
          new Error("Invalid URI: " + uridescriptor.value)
        );
        return false;
      }

      let id = {};
      if (mapURIToAddonID(uri, id)) {
        return id.value === this.id;
      }
    }

    return false;
  },

  




  _allowSource: function(aSource) {
    
    if (aSource.url === "resource://gre/modules/addons/XPIProvider.jsm") {
      return false;
    }

    return true;
  },

  



  _findDebuggees: function (dbg) {
    return dbg.findAllGlobals().filter(this._shouldAddNewGlobalAsDebuggee);
  }
};

BrowserAddonActor.prototype.requestTypes = {
  "attach": BrowserAddonActor.prototype.onAttach,
  "detach": BrowserAddonActor.prototype.onDetach
};













function AddonConsoleActor(aAddon, aConnection, aParentActor)
{
  this.addon = aAddon;
  WebConsoleActor.call(this, aConnection, aParentActor);
}

AddonConsoleActor.prototype = Object.create(WebConsoleActor.prototype);

update(AddonConsoleActor.prototype, {
  constructor: AddonConsoleActor,

  actorPrefix: "addonConsole",

  


  addon: null,

  


  get window() {
    return this.parentActor.global;
  },

  


  disconnect: function ACA_disconnect()
  {
    WebConsoleActor.prototype.disconnect.call(this);
    this.addon = null;
  },

  







  onStartListeners: function ACA_onStartListeners(aRequest)
  {
    let startedListeners = [];

    while (aRequest.listeners.length > 0) {
      let listener = aRequest.listeners.shift();
      switch (listener) {
        case "ConsoleAPI":
          if (!this.consoleAPIListener) {
            this.consoleAPIListener =
              new ConsoleAPIListener(null, this, "addon/" + this.addon.id);
            this.consoleAPIListener.init();
          }
          startedListeners.push(listener);
          break;
      }
    }
    return {
      startedListeners: startedListeners,
      nativeConsoleAPI: true,
      traits: this.traits,
    };
  },
});

AddonConsoleActor.prototype.requestTypes = Object.create(WebConsoleActor.prototype.requestTypes);
AddonConsoleActor.prototype.requestTypes.startListeners = AddonConsoleActor.prototype.onStartListeners;
