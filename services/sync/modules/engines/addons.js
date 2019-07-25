
































































"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-sync/addonsreconciler.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/async.js");
Cu.import("resource://services-sync/ext/Preferences.js");

Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/AddonRepository.jsm");

const EXPORTED_SYMBOLS = ["AddonsEngine"];


const PRUNE_ADDON_CHANGES_THRESHOLD = 60 * 60 * 24 * 7 * 1000;






























function AddonRecord(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
AddonRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Addon"
};

Utils.deferGetSet(AddonRecord, "cleartext", ["addonID",
                                             "applicationID",
                                             "enabled",
                                             "source"]);











function AddonsEngine() {
  SyncEngine.call(this, "Addons");

  this._reconciler = new AddonsReconciler();
}
AddonsEngine.prototype = {
  __proto__:              SyncEngine.prototype,
  _storeObj:              AddonsStore,
  _trackerObj:            AddonsTracker,
  _recordObj:             AddonRecord,
  version:                1,

  _reconciler:            null,

  


  _findDupe: function _findDupe(item) {
    let id = item.addonID;

    
    
    let addons = this._reconciler.addons;
    if (!(id in addons)) {
      return null;
    }

    let addon = addons[id];
    if (addon.guid != item.id) {
      return addon.guid;
    }

    return null;
  },

  



  getChangedIDs: function getChangedIDs() {
    let changes = {};
    for (let [id, modified] in Iterator(this._tracker.changedIDs)) {
      changes[id] = modified;
    }

    let lastSyncDate = new Date(this.lastSync * 1000);

    
    
    let reconcilerChanges = this._reconciler.getChangesSinceDate(lastSyncDate);
    let addons = this._reconciler.addons;
    for each (let change in reconcilerChanges) {
      let changeTime = change[0];
      let id = change[2];

      if (!(id in addons)) {
        continue;
      }

      
      if (id in changes && changeTime < changes[id]) {
          continue;
      }

      if (!this._store.isAddonSyncable(addons[id])) {
        continue;
      }

      this._log.debug("Adding changed add-on from changes log: " + id);
      let addon = addons[id];
      changes[addon.guid] = changeTime.getTime() / 1000;
    }

    return changes;
  },

  










  _syncStartup: function _syncStartup() {
    
    
    
    this._refreshReconcilerState();

    SyncEngine.prototype._syncStartup.call(this);
  },

  







  _syncCleanup: function _syncCleanup() {
    let ms = 1000 * this.lastSync - PRUNE_ADDON_CHANGES_THRESHOLD;
    this._reconciler.pruneChangesBeforeDate(new Date(ms));

    SyncEngine.prototype._syncCleanup.call(this);
  },

  





  _refreshReconcilerState: function _refreshReconcilerState() {
    this._log.debug("Refreshing reconciler state");
    let cb = Async.makeSpinningCallback();
    this._reconciler.refreshGlobalState(cb);
    cb.wait();
  }
};







function AddonsStore(name) {
  Store.call(this, name);
}
AddonsStore.prototype = {
  __proto__: Store.prototype,

  
  _syncableTypes: ["extension", "theme"],

  get reconciler() {
    return this.engine._reconciler;
  },

  get engine() {
    
    
    
    return Engines.get("addons");
  },

  


  applyIncoming: function applyIncoming(record) {
    
    if (!record.deleted) {
      
      
      if (record.applicationID != Services.appinfo.ID) {
        this._log.info("Ignoring incoming record from other App ID: " +
                        record.id);
        return;
      }

      
      
      if (record.source != "amo") {
        this._log.info("Ignoring unknown add-on source (" + record.source + ")" +
                       " for " + record.id);
        return;
      }
    }

    Store.prototype.applyIncoming.call(this, record);
  },


  


  create: function create(record) {
    
    
    

    let cb = Async.makeSpinningCallback();
    this.installAddonsFromIDs([record.addonID], cb);

    
    
    let results = cb.wait();

    let addon;
    for each (let a in results.addons) {
      if (a.id == record.addonID) {
        addon = a;
        break;
      }
    }

    
    if (!addon) {
      throw new Error("Add-on not found after install: " + record.addonID);
    }

    this._log.info("Add-on installed: " + record.addonID);
    this._log.info("Setting add-on Sync GUID to remote: " + record.id);
    addon.syncGUID = record.id;

    cb = Async.makeSpinningCallback();
    this.updateUserDisabled(addon, !record.enabled, cb);
    cb.wait();
  },

  


  remove: function remove(record) {
    
    let addon = this.getAddonByGUID(record.id);
    if (!addon) {
      
      
      
      return;
    }

    this._log.info("Uninstalling add-on: " + addon.id);
    let cb = Async.makeSpinningCallback();
    this.uninstallAddon(addon, cb);
    cb.wait();
  },

  


  update: function update(record) {
    let addon = this.getAddonByID(record.addonID);

    
    
    
    if (!addon) {
      this._log.warn("Requested to update record but add-on not found: " +
                     record.addonID);
      return;
    }

    let cb = Async.makeSpinningCallback();
    this.updateUserDisabled(addon, !record.enabled, cb);
    cb.wait();
  },

  


  itemExists: function itemExists(guid) {
    let addon = this.reconciler.getAddonStateFromSyncGUID(guid);

    return !!addon;
  },

  









  createRecord: function createRecord(guid, collection) {
    let record = new AddonRecord(collection, guid);
    record.applicationID = Services.appinfo.ID;

    let addon = this.reconciler.getAddonStateFromSyncGUID(guid);

    
    
    if (!addon || !addon.installed) {
      record.deleted = true;
      return record;
    }

    record.modified = addon.modified.getTime() / 1000;

    record.addonID = addon.id;
    record.enabled = addon.enabled;

    
    record.source = "amo";

    return record;
  },

  




  changeItemID: function changeItemID(oldID, newID) {
    
    
    let state = this.reconciler.getAddonStateFromSyncGUID(oldID);
    if (state) {
      state.guid = newID;
      let cb = Async.makeSpinningCallback();
      this.reconciler.saveState(null, cb);
      cb.wait();
    }

    let addon = this.getAddonByGUID(oldID);
    if (!addon) {
      this._log.debug("Cannot change item ID (" + oldID + ") in Add-on " +
                      "Manager because old add-on not present: " + oldID);
      return;
    }

    addon.syncGUID = newID;
  },

  




  getAllIDs: function getAllIDs() {
    let ids = {};

    let addons = this.reconciler.addons;
    for each (let addon in addons) {
      if (this.isAddonSyncable(addon)) {
        ids[addon.guid] = true;
      }
    }

    return ids;
  },

  





  wipe: function wipe() {
    this._log.info("Processing wipe.");

    this.engine._refreshReconcilerState();

    
    
    for (let guid in this.getAllIDs()) {
      let addon = this.getAddonByGUID(guid);
      if (!addon) {
        this._log.debug("Ignoring add-on because it couldn't be obtained: " +
                        guid);
        continue;
      }

      this._log.info("Uninstalling add-on as part of wipe: " + addon.id);
      Utils.catch(addon.uninstall)();
    }
  },

  



  






  getAddonByID: function getAddonByID(id) {
    let cb = Async.makeSyncCallback();
    AddonManager.getAddonByID(id, cb);
    return Async.waitForSyncCallback(cb);
  },

  






  getAddonByGUID: function getAddonByGUID(guid) {
    let cb = Async.makeSyncCallback();
    AddonManager.getAddonBySyncGUID(guid, cb);
    return Async.waitForSyncCallback(cb);
  },

  






  isAddonSyncable: function isAddonSyncable(addon) {
    
    
    
    
    
    

    
    
    if (!addon) {
      this._log.debug("Null object passed to isAddonSyncable.");
      return false;
    }

    if (this._syncableTypes.indexOf(addon.type) == -1) {
      this._log.debug(addon.id + " not syncable: type not in whitelist: " +
                      addon.type);
      return false;
    }

    if (!(addon.scope & AddonManager.SCOPE_PROFILE)) {
      this._log.debug(addon.id + " not syncable: not installed in profile.");
      return false;
    }

    
    
    
    
    if (addon.foreignInstall) {
      this._log.debug(addon.id + " not syncable: is foreign install.");
      return false;
    }

    
    
    
    if (Svc.Prefs.get("addons.ignoreRepositoryChecking", false)) {
      return true;
    }

    let cb = Async.makeSyncCallback();
    AddonRepository.getCachedAddonByID(addon.id, cb);
    let result = Async.waitForSyncCallback(cb);

    if (!result) {
      this._log.debug(addon.id + " not syncable: add-on not found in add-on " +
                      "repository.");
      return false;
    }

    return this.isSourceURITrusted(result.sourceURI);
  },

  










  isSourceURITrusted: function isSourceURITrusted(uri) {
    
    
    
    let trustedHostnames = Svc.Prefs.get("addons.trustedSourceHostnames", "")
                           .split(",");

    if (!uri) {
      this._log.debug("Undefined argument to isSourceURITrusted().");
      return false;
    }

    
    
    
    if (uri.scheme != "https") {
      this._log.debug("Source URI not HTTPS: " + uri.spec);
      return false;
    }

    if (trustedHostnames.indexOf(uri.host) == -1) {
      this._log.debug("Source hostname not trusted: " + uri.host);
      return false;
    }

    return true;
  },

  














  getInstallFromSearchResult: function getInstallFromSearchResult(addon, cb) {
    
    if (addon.install) {
      cb(null, addon.install);
      return;
    }

    this._log.debug("Manually obtaining install for " + addon.id);

    
    
    
    if (!Svc.Prefs.get("addons.ignoreRepositoryChecking", false)) {
      let scheme = addon.sourceURI.scheme;
      if (scheme != "https") {
        cb(new Error("Insecure source URI scheme: " + scheme), addon.install);
      }
    }

    AddonManager.getInstallForURL(
      addon.sourceURI.spec,
      function handleInstall(install) {
        cb(null, install);
      },
      "application/x-xpinstall",
      undefined,
      addon.name,
      addon.iconURL,
      addon.version
    );
  },

  


















  installAddonFromSearchResult:
    function installAddonFromSearchResult(addon, cb) {
    this._log.info("Trying to install add-on from search result: " + addon.id);

    this.getInstallFromSearchResult(addon, function(error, install) {
      if (error) {
        cb(error, null);
        return;
      }

      if (!install) {
        cb(new Error("AddonInstall not available: " + addon.id), null);
        return;
      }

      try {
        this._log.info("Installing " + addon.id);

        let listener = {
          onInstallEnded: function(install, addon) {
            install.removeListener(listener);

            cb(null, {id: addon.id, install: install, addon: addon});
          },
          onInstallFailed: function(install) {
            install.removeListener(listener);

            cb(new Error("Install failed: " + install.error), null);
          },
          onDownloadFailed: function(install) {
            install.removeListener(listener);

            cb(new Error("Download failed: " + install.error), null);
          }
        };
        install.addListener(listener);
        install.install();
      }
      catch (ex) {
        this._log.error("Error installing add-on: " + Utils.exceptionstr(ex));
        cb(ex, null);
      }
    }.bind(this));
  },

  








  uninstallAddon: function uninstallAddon(addon, callback) {
    let listener = {
      onUninstalling: function(uninstalling, needsRestart) {
        if (addon.id != uninstalling.id) {
          return;
        }

        
        
        if (!needsRestart) {
          return;
        }

        
        
        AddonManager.removeAddonListener(listener);
        callback(null, addon);
      },
      onUninstalled: function(uninstalled) {
        if (addon.id != uninstalled.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(null, addon);
      }
    };
    AddonManager.addAddonListener(listener);
    addon.uninstall();
  },

  















  updateUserDisabled: function updateUserDisabled(addon, value, callback) {
    if (addon.userDisabled == value) {
      callback(null, addon);
      return;
    }

    
    if (Svc.Prefs.get("addons.ignoreUserEnabledChanges", false)) {
      this._log.info("Ignoring enabled state change due to preference: " +
                     addon.id);
      callback(null, addon);
      return;
    }

    let listener = {
      onEnabling: function onEnabling(wrapper, needsRestart) {
        this._log.debug("onEnabling: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        
        if (!needsRestart) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(null, wrapper);
      }.bind(this),

      onEnabled: function onEnabled(wrapper) {
        this._log.debug("onEnabled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(null, wrapper);
      }.bind(this),

      onDisabling: function onDisabling(wrapper, needsRestart) {
        this._log.debug("onDisabling: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        if (!needsRestart) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(null, wrapper);
      }.bind(this),

      onDisabled: function onDisabled(wrapper) {
        this._log.debug("onDisabled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(null, wrapper);
      }.bind(this),

      onOperationCancelled: function onOperationCancelled(wrapper) {
        this._log.debug("onOperationCancelled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        callback(new Error("Operation cancelled"), wrapper);
      }.bind(this)
    };

    
    

    if (!addon.appDisabled) {
      AddonManager.addAddonListener(listener);
    }

    this._log.info("Updating userDisabled flag: " + addon.id + " -> " + value);
    addon.userDisabled = !!value;

    if (!addon.appDisabled) {
      callback(null, addon);
      return;
    }
    
  },

  





















  installAddonsFromIDs: function installAddonsFromIDs(ids, cb) {
    if (!cb) {
      throw new Error("Invalid argument: cb is not defined.");
    }

    AddonRepository.getAddonsByIDs(ids, {
      searchSucceeded: function searchSucceeded(addons, addonsLength, total) {
        this._log.info("Found " + addonsLength + "/" + ids.length +
                       " add-ons during repository search.");

        let ourResult = {
          installedIDs: [],
          installs:     [],
          addons:       [],
          errors:       []
        };

        if (!addonsLength) {
          cb(null, ourResult);
          return;
        }

        let finishedCount = 0;
        let installCallback = function installCallback(error, result) {
          finishedCount++;

          if (error) {
            ourResult.errors.push(error);
          } else {
            ourResult.installedIDs.push(result.id);
            ourResult.installs.push(result.install);
            ourResult.addons.push(result.addon);
          }

          if (finishedCount >= addonsLength) {
            if (ourResult.errors.length > 0) {
              cb(new Error("1 or more add-ons failed to install"), ourResult);
            } else {
              cb(null, ourResult);
            }
          }
        }.bind(this);

        
        
        for (let i = 0; i < addonsLength; i++) {
          this.installAddonFromSearchResult(addons[i], installCallback);
        }

      }.bind(this),

      searchFailed: function searchFailed() {
        cb(new Error("AddonRepository search failed"), null);
      }.bind(this)
    });
  }
};






function AddonsTracker(name) {
  Tracker.call(this, name);

  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);
}
AddonsTracker.prototype = {
  __proto__: Tracker.prototype,

  get reconciler() {
    return Engines.get("addons")._reconciler;
  },

  get store() {
    return Engines.get("addons")._store;
  },

  



  changeListener: function changeHandler(date, change, addon) {
    this._log.debug("changeListener invoked: " + change + " " + addon.id);
    
    if (this.ignoreAll) {
      return;
    }

    if (!this.store.isAddonSyncable(addon)) {
      this._log.debug("Ignoring change because add-on isn't syncable: " +
                      addon.id);
      return;
    }

    this.addChangedID(addon.guid, date.getTime() / 1000);
    this.score += SCORE_INCREMENT_XLARGE;
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "weave:engine:start-tracking":
        this.reconciler.addChangeListener(this);
        break;

      case "weave:engine:stop-tracking":
        this.reconciler.removeChangeListener(this);
        break;
    }
  }
};
