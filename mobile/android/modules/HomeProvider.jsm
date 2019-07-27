




"use strict";

this.EXPORTED_SYMBOLS = [ "HomeProvider" ];

const { utils: Cu, classes: Cc, interfaces: Ci } = Components;

Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");






const SCHEMA_VERSION = 2;


const MAX_SAVE_COUNT = 100;

XPCOMUtils.defineLazyGetter(this, "DB_PATH", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "home.sqlite");
});

const PREF_STORAGE_LAST_SYNC_TIME_PREFIX = "home.storage.lastSyncTime.";
const PREF_SYNC_UPDATE_MODE = "home.sync.updateMode";
const PREF_SYNC_CHECK_INTERVAL_SECS = "home.sync.checkIntervalSecs";

XPCOMUtils.defineLazyGetter(this, "gSyncCheckIntervalSecs", function() {
  return Services.prefs.getIntPref(PREF_SYNC_CHECK_INTERVAL_SECS);
});

XPCOMUtils.defineLazyServiceGetter(this,
  "gUpdateTimerManager", "@mozilla.org/updates/timer-manager;1", "nsIUpdateTimerManager");




const SQL = {
  createItemsTable:
    "CREATE TABLE items (" +
      "_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
      "dataset_id TEXT NOT NULL, " +
      "url TEXT," +
      "title TEXT," +
      "description TEXT," +
      "image_url TEXT," +
      "filter TEXT," +
      "created INTEGER" +
    ")",

  dropItemsTable:
    "DROP TABLE items",

  insertItem:
    "INSERT INTO items (dataset_id, url, title, description, image_url, filter, created) " +
      "VALUES (:dataset_id, :url, :title, :description, :image_url, :filter, :created)",

  deleteFromDataset:
    "DELETE FROM items WHERE dataset_id = :dataset_id"
}





function isUsingWifi() {
  let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
  return (network.linkType === Ci.nsINetworkLinkService.LINK_TYPE_WIFI || network.linkType === Ci.nsINetworkLinkService.LINK_TYPE_ETHERNET);
}

function getNowInSeconds() {
  return Math.round(Date.now() / 1000);
}

function getLastSyncPrefName(datasetId) {
  return PREF_STORAGE_LAST_SYNC_TIME_PREFIX + datasetId;
}


var gTimerRegistered = false;


var gSyncCallbacks = {};






function syncTimerCallback(timer) {
  for (let datasetId in gSyncCallbacks) {
    let lastSyncTime = 0;
    try {
      lastSyncTime = Services.prefs.getIntPref(getLastSyncPrefName(datasetId));
    } catch(e) { }

    let now = getNowInSeconds();
    let { interval: interval, callback: callback } = gSyncCallbacks[datasetId];

    if (lastSyncTime < now - interval) {
      let success = HomeProvider.requestSync(datasetId, callback);
      if (success) {
        Services.prefs.setIntPref(getLastSyncPrefName(datasetId), now);
      }
    }
  }
}

this.HomeStorage = function(datasetId) {
  this.datasetId = datasetId;
};

this.ValidationError = function(message) {
  this.name = "ValidationError";
  this.message = message;
};
ValidationError.prototype = new Error();
ValidationError.prototype.constructor = ValidationError;

this.HomeProvider = Object.freeze({
  ValidationError: ValidationError,

  







  getStorage: function(datasetId) {
    return new HomeStorage(datasetId);
  },

  







  requestSync: function(datasetId, callback) {
    
    if ((Services.prefs.getIntPref(PREF_SYNC_UPDATE_MODE) === 1) && !isUsingWifi()) {
      Cu.reportError("HomeProvider: Failed to sync because device is not on a local network");
      return false;
    }

    callback(datasetId);
    return true;
  },

  






  addPeriodicSync: function(datasetId, interval, callback) {
    
    if (interval < gSyncCheckIntervalSecs) {
      Cu.reportError("HomeProvider: Warning for dataset " + datasetId +
        " : Sync notifications are throttled to " + gSyncCheckIntervalSecs + " seconds");
    }

    gSyncCallbacks[datasetId] = {
      interval: interval,
      callback: callback
    };

    if (!gTimerRegistered) {
      gUpdateTimerManager.registerTimer("home-provider-sync-timer", syncTimerCallback, gSyncCheckIntervalSecs);
      gTimerRegistered = true;
    }
  },

  




  removePeriodicSync: function(datasetId) {
    delete gSyncCallbacks[datasetId];
    Services.prefs.clearUserPref(getLastSyncPrefName(datasetId));
    
  }
});

var gDatabaseEnsured = false;




function createDatabase(db) {
  return Task.spawn(function create_database_task() {
    yield db.execute(SQL.createItemsTable);
  });
}




function upgradeDatabase(db, oldVersion, newVersion) {
  return Task.spawn(function upgrade_database_task() {
    for (let v = oldVersion + 1; v <= newVersion; v++) {
      switch(v) {
        case 2:
          
          
          yield db.execute(SQL.dropItemsTable);
          yield db.execute(SQL.createItemsTable);
          break;
      }
    }
  });
}









function getDatabaseConnection() {
  return Task.spawn(function get_database_connection_task() {
    let db = yield Sqlite.openConnection({ path: DB_PATH });
    if (gDatabaseEnsured) {
      throw new Task.Result(db);
    }

    try {
      
      let dbVersion = parseInt(yield db.getSchemaVersion());

      
      
      if (dbVersion === 0) {
        yield createDatabase(db);
      } else if (dbVersion < SCHEMA_VERSION) {
        yield upgradeDatabase(db, dbVersion, SCHEMA_VERSION);
      }

      yield db.setSchemaVersion(SCHEMA_VERSION);
    } catch(e) {
      
      yield db.close();
      throw e;
    }

    gDatabaseEnsured = true;
    throw new Task.Result(db);
  });
}







function validateItem(datasetId, item) {
  if (!item.url) {
    throw new ValidationError('HomeStorage: All rows must have an URL: datasetId = ' +
                              datasetId);
  }

  if (!item.image_url && !item.title && !item.description) {
    throw new ValidationError('HomeStorage: All rows must have at least an image URL, ' +
                              'or a title or a description: datasetId = ' + datasetId);
  }
}

var gRefreshTimers = {};





function refreshDataset(datasetId) {
  
  if (gRefreshTimers[datasetId]) {
    return;
  }

  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(function(timer) {
    delete gRefreshTimers[datasetId];

    Messaging.sendRequest({
      type: "HomePanels:RefreshDataset",
      datasetId: datasetId
    });
  }, 100, Ci.nsITimer.TYPE_ONE_SHOT);

  gRefreshTimers[datasetId] = timer;
}

HomeStorage.prototype = {
  


















  save: function(data, options) {
    if (data && data.length > MAX_SAVE_COUNT) {
      throw "save failed for dataset = " + this.datasetId +
        ": you cannot save more than " + MAX_SAVE_COUNT + " items at once";
    }

    return Task.spawn(function save_task() {
      let db = yield getDatabaseConnection();
      try {
        yield db.executeTransaction(function save_transaction() {
          if (options && options.replace) {
            yield db.executeCached(SQL.deleteFromDataset, { dataset_id: this.datasetId });
          }

          
          for (let item of data) {
            validateItem(this.datasetId, item);

            
            let params = {
              dataset_id: this.datasetId,
              url: item.url,
              title: item.title,
              description: item.description,
              image_url: item.image_url,
              filter: item.filter,
              created: Date.now()
            };
            yield db.executeCached(SQL.insertItem, params);
          }
        }.bind(this));
      } finally {
        yield db.close();
      }

      refreshDataset(this.datasetId);
    }.bind(this));
  },

  





  deleteAll: function() {
    return Task.spawn(function delete_all_task() {
      let db = yield getDatabaseConnection();
      try {
        let params = { dataset_id: this.datasetId };
        yield db.executeCached(SQL.deleteFromDataset, params);
      } finally {
        yield db.close();
      }

      refreshDataset(this.datasetId);
    }.bind(this));
  }
};
