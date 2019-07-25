

















































"use strict";

const Cu = Components.utils;

Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://gre/modules/AddonManager.jsm");

const DEFAULT_STATE_FILE = "addonsreconciler";

const CHANGE_INSTALLED   = 1;
const CHANGE_UNINSTALLED = 2;
const CHANGE_ENABLED     = 3;
const CHANGE_DISABLED    = 4;

const EXPORTED_SYMBOLS = ["AddonsReconciler", "CHANGE_INSTALLED",
                          "CHANGE_UNINSTALLED", "CHANGE_ENABLED",
                          "CHANGE_DISABLED"];
















































































function AddonsReconciler() {
  this._log = Log4Moz.repository.getLogger("Sync.AddonsReconciler");
  let level = Svc.Prefs.get("log.logger.addonsreconciler", "Debug");
  this._log.level = Log4Moz.Level[level];

  AddonManager.addAddonListener(this);
  AddonManager.addInstallListener(this);
  this._listening = true;

  Svc.Obs.add("xpcom-shutdown", this.stopListening.bind(this));
};
AddonsReconciler.prototype = {
  
  _listening: false,

  



  _stateLoaded: false,

  
  _log: null,

  






  _addons: {},

  




  _changes: [],

  


  _listeners: [],

  




  get addons() {
    this._ensureStateLoaded();
    return this._addons;
  },

  
















  loadState: function loadState(path, callback) {
    let file = path || DEFAULT_STATE_FILE;
    Utils.jsonLoad(file, this, function(json) {
      this._addons = {};
      this._changes = [];

      if (!json) {
        this._log.debug("No data seen in loaded file: " + file);
        if (callback) {
          callback(null, false);
        }

        return;
      }

      let version = json.version;
      if (!version || version != 1) {
        this._log.error("Could not load JSON file because version not " +
                        "supported: " + version);
        if (callback) {
          callback(null, false);
        }

        return;
      }

      this._addons = json.addons;
      for each (let record in this._addons) {
        record.modified = new Date(record.modified);
      }

      for each (let [time, change, id] in json.changes) {
        this._changes.push([new Date(time), change, id]);
      }

      if (callback) {
        callback(null, true);
      }
    });
  },

  









  saveState: function saveState(path, callback) {
    let file = path || DEFAULT_STATE_FILE;
    let state = {version: 1, addons: {}, changes: []};

    for (let [id, record] in Iterator(this._addons)) {
      state.addons[id] = {};
      for (let [k, v] in Iterator(record)) {
        if (k == "modified") {
          state.addons[id][k] = v.getTime();
        }
        else {
          state.addons[id][k] = v;
        }
      }
    }

    for each (let [time, change, id] in this._changes) {
      state.changes.push([time.getTime(), change, id]);
    }

    this._log.info("Saving reconciler state to file: " + file);
    Utils.jsonSave(file, this, state, callback);
  },

  











  addChangeListener: function addChangeListener(listener) {
    if (this._listeners.indexOf(listener) == -1) {
      this._log.debug("Adding change listener.");
      this._listeners.push(listener);
    }
  },

  





  removeChangeListener: function removeChangeListener(listener) {
    this._listeners = this._listeners.filter(function(element) {
      if (element == listener) {
        this._log.debug("Removing change listener.");
        return false;
      } else {
        return true;
      }
    }.bind(this));
  },

  








  stopListening: function stopListening() {
    if (this._listening) {
      this._log.debug("Stopping listening and removing AddonManager listeners.");
      AddonManager.removeInstallListener(this);
      AddonManager.removeAddonListener(this);
      this._listening = false;
    }
  },

  


  refreshGlobalState: function refreshGlobalState(callback) {
    this._log.info("Refreshing global state from AddonManager.");
    this._ensureStateLoaded();

    let installs;

    AddonManager.getAllAddons(function (addons) {
      let ids = {};

      for each (let addon in addons) {
        ids[addon.id] = true;
        this.rectifyStateFromAddon(addon);
      }

      
      
      for (let [id, addon] in Iterator(this._addons)) {
        if (id in ids) {
          continue;
        }

        
        
        

        if (!installs) {
          let cb = Async.makeSyncCallback();
          AddonManager.getAllInstalls(cb);
          installs = Async.waitForSyncCallback(cb);
        }

        let installFound = false;
        for each (let install in installs) {
          if (install.addon && install.addon.id == id &&
              install.state == AddonManager.STATE_INSTALLED) {

            installFound = true;
            break;
          }
        }

        if (installFound) {
          continue;
        }

        if (addon.installed) {
          addon.installed = false;
          this._log.debug("Adding change because add-on not present in " +
                          "Add-on Manager: " + id);
          this._addChange(new Date(), CHANGE_UNINSTALLED, addon);
        }
      }

      this.saveState(null, callback);
    }.bind(this));
  },

  











  rectifyStateFromAddon: function rectifyStateFromAddon(addon) {
    this._log.debug("Rectifying state for addon: " + addon.id);
    this._ensureStateLoaded();

    let id = addon.id;
    let enabled = !addon.userDisabled;
    let guid = addon.syncGUID;
    let now = new Date();

    if (!(id in this._addons)) {
      let record = {
        id: id,
        guid: guid,
        enabled: enabled,
        installed: true,
        modified: now,
        type: addon.type,
        scope: addon.scope,
        foreignInstall: addon.foreignInstall
      };
      this._addons[id] = record;
      this._log.debug("Adding change because add-on not present locally: " +
                      id);
      this._addChange(now, CHANGE_INSTALLED, record);
      return;
    }

    let record = this._addons[id];

    if (!record.installed) {
      
      
      if (!(addon.pendingOperations & AddonManager.PENDING_UNINSTALL)) {
        record.installed = true;
        record.modified = now;
      }
    }

    if (record.enabled != enabled) {
      record.enabled = enabled;
      record.modified = now;
      let change = enabled ? CHANGE_ENABLED : CHANGE_DISABLED;
      this._log.debug("Adding change because enabled state changed: " + id);
      this._addChange(new Date(), change, record);
    }

    if (record.guid != guid) {
      record.guid = guid;
      
      
      
    }
  },

  









  _addChange: function _addChange(date, change, state) {
    this._log.info("Change recorded for " + state.id);
    this._changes.push([date, change, state.id]);

    for each (let listener in this._listeners) {
      try {
        listener.changeListener.call(listener, date, change, state);
      } catch (ex) {
        this._log.warn("Exception calling change listener: " +
                       Utils.exceptionStr(ex));
      }
    }
  },

  









  getChangesSinceDate: function getChangesSinceDate(date) {
    this._ensureStateLoaded();

    let length = this._changes.length;
    for (let i = 0; i < length; i++) {
      if (this._changes[i][0] >= date) {
        return this._changes.slice(i);
      }
    }

    return [];
  },

  





  pruneChangesBeforeDate: function pruneChangesBeforeDate(date) {
    this._ensureStateLoaded();

    while (this._changes.length > 0) {
      if (this._changes[0][0] >= date) {
        return;
      }

      delete this._changes[0];
    }
  },

  




  getAllSyncGUIDs: function getAllSyncGUIDs() {
    let result = {};
    for (let id in this.addons) {
      result[id] = true;
    }

    return result;
  },

  








  getAddonStateFromSyncGUID: function getAddonStateFromSyncGUID(guid) {
    for each (let addon in this.addons) {
      if (addon.guid == guid) {
        return addon;
      }
    }

    return null;
  },

  





  _ensureStateLoaded: function _ensureStateLoaded() {
    if (this._stateLoaded) {
      return;
    }

    let cb = Async.makeSpinningCallback();
    this.loadState(null, cb);
    cb.wait();
    this._stateLoaded = true;
  },

  


  _handleListener: function _handlerListener(action, addon, requiresRestart) {
    
    
    try {
      let id = addon.id;
      this._log.debug("Add-on change: " + action + " to " + id);

      
      
      
      
      if (requiresRestart === false) {
        this._log.debug("Ignoring " + action + " for restartless add-on.");
        return;
      }

      switch (action) {
        case "onEnabling":
        case "onEnabled":
        case "onDisabling":
        case "onDisabled":
        case "onInstalled":
        case "onInstallEnded":
        case "onOperationCancelled":
          this.rectifyStateFromAddon(addon);
          break;

        case "onUninstalling":
        case "onUninstalled":
          let id = addon.id;
          let addons = this.addons;
          if (id in addons) {
            let now = new Date();
            let record = addons[id];
            record.installed = false;
            record.modified = now;
            this._log.debug("Adding change because of uninstall listener: " +
                            id);
            this._addChange(now, CHANGE_UNINSTALLED, record);
          }
      }

      let cb = Async.makeSpinningCallback();
      this.saveState(null, cb);
      cb.wait();
    }
    catch (ex) {
      this._log.warn("Exception: " + Utils.exceptionStr(ex));
    }
  },

  
  onEnabling: function onEnabling(addon, requiresRestart) {
    this._handleListener("onEnabling", addon, requiresRestart);
  },
  onEnabled: function onEnabled(addon) {
    this._handleListener("onEnabled", addon);
  },
  onDisabling: function onDisabling(addon, requiresRestart) {
    this._handleListener("onDisabling", addon, requiresRestart);
  },
  onDisabled: function onDisabled(addon) {
    this._handleListener("onDisabled", addon);
  },
  onInstalling: function onInstalling(addon, requiresRestart) {
    this._handleListener("onInstalling", addon, requiresRestart);
  },
  onInstalled: function onInstalled(addon) {
    this._handleListener("onInstalled", addon);
  },
  onUninstalling: function onUninstalling(addon, requiresRestart) {
    this._handleListener("onUninstalling", addon, requiresRestart);
  },
  onUninstalled: function onUninstalled(addon) {
    this._handleListener("onUninstalled", addon);
  },
  onOperationCancelled: function onOperationCancelled(addon) {
    this._handleListener("onOperationCancelled", addon);
  },

  
  onInstallEnded: function onInstallEnded(install, addon) {
    this._handleListener("onInstallEnded", addon);
  }
};
