



"use strict";

this.EXPORTED_SYMBOLS = ["AitcStorage", "AitcQueue"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");















this.AitcQueue = function AitcQueue(filename, cb) {
  if (!cb) {
    throw new Error("AitcQueue constructor called without callback");
  }

  this._log = Log4Moz.repository.getLogger("Service.AITC.Storage.Queue");
  this._log.level = Log4Moz.Level[Preferences.get(
    "services.aitc.storage.log.level"
  )];

  this._queue = [];
  this._writeLock = false;
  this._filePath = "webapps/" + filename;

  this._log.info("AitcQueue instance loading");

  CommonUtils.jsonLoad(this._filePath, this, function jsonLoaded(data) {
    if (data && Array.isArray(data)) {
      this._queue = data;
    }
    this._log.info("AitcQueue instance created");
    cb(true);
  });
}
AitcQueue.prototype = {
  


  enqueue: function enqueue(obj, cb) {
    this._log.info("Adding to queue " + obj);

    if (!cb) {
      throw new Error("enqueue called without callback");
    }

    let self = this;
    this._queue.push(obj);

    try {
      this._putFile(this._queue, function _enqueuePutFile(err) {
        if (err) {
          
          self._queue.pop();
          cb(err, false);
          return;
        }
        
        cb(null, true);
        return;
      });
    } catch (e) {
      self._queue.pop();
      cb(e, false);
    }
  },

  


  dequeue: function dequeue(cb) {
    this._log.info("Removing head of queue");

    if (!cb) {
      throw new Error("dequeue called without callback");
    }
    if (!this._queue.length) {
      throw new Error("Queue is empty");
    }

    let self = this;
    let obj = this._queue.shift();

    try {
      this._putFile(this._queue, function _dequeuePutFile(err) {
        if (!err) {
          
          cb(null, true);
          return;
        }
        
        self._queue.unshift(obj);
        cb(err, false);
      });
    } catch (e) {
      self._queue.unshift(obj);
      cb(e, false);
    }
  },

  


  peek: function peek() {
    this._log.info("Peek called when length " + this._queue.length);
    if (!this._queue.length) {
      throw new Error("Queue is empty");
    }
    return this._queue[0];
  },

  


  get length() {
    return this._queue.length;
  },

  



  _putFile: function _putFile(value, cb) {
    if (this._writeLock) {
      throw new Error("_putFile already in progress");
    }

    this._writeLock = true;
    this._log.info("Writing queue to disk");
    CommonUtils.jsonSave(this._filePath, this, value, function jsonSaved(err) {
      if (err) {
        let msg = new Error("_putFile failed with " + err);
        this._writeLock = false;
        cb(msg);
        return;
      }
      this._log.info("_putFile succeeded");
      this._writeLock = false;
      cb(null);
    });
  },
};





function AitcStorageImpl() {
  this._log = Log4Moz.repository.getLogger("Service.AITC.Storage");
  this._log.level = Log4Moz.Level[Preferences.get(
    "services.aitc.storage.log.level"
  )];
  this._log.info("Loading AitC storage module");
}
AitcStorageImpl.prototype = {
  









  processApps: function processApps(remoteApps, callback) {
    let self = this;
    this._log.info("Server check got " + remoteApps.length + " apps");

    
    
    DOMApplicationRegistry.getAllWithoutManifests(
      function _processAppsGotLocalApps(localApps) {
        let changes = self._determineLocalChanges(localApps, remoteApps);
        self._processChanges(changes, callback);
      }
    );
  },

  


















  _determineLocalChanges: function _determineChanges(localApps, remoteApps) {
    let changes = new Map();
    changes.deleteIDs = [];
    changes.installs  = {};

    
    
    
    
    if (!Object.keys(remoteApps).length) {
      this._log.warn("Empty set of remote apps. Not taking any action.");
      return changes;
    }

    
    
    let deletes       = {};
    let remoteOrigins = {};

    let localOrigins = {};
    for (let [id, app] in Iterator(localApps)) {
      localOrigins[app.origin] = id;
    }

    for (let remoteApp of remoteApps) {
      let origin = remoteApp.origin;
      remoteOrigins[origin] = true;

      
      
      if (remoteApp.hidden) {
        if (origin in localOrigins) {
          deletes[localOrigins[origin]] = true;
        }

        continue;
      }

      
      
      if (!localApps[origin]) {
        changes.installs[DOMApplicationRegistry.makeAppId()] = remoteApp;
        continue;
      }

      
      
      if (localApps[origin].installTime < remoteApp.installTime) {
        changes.installs[localApps[origin]] = remoteApp;
        continue;
      }
    }

    
    
    for (let [id, app] in Iterator(localApps)) {
      if (!(app.origin in remoteOrigins)) {
        deletes[id] = true;
      }
    }

    changes.deleteIDs = Object.keys(deletes);

    return changes;
  },

  







  _processChanges: function _processChanges(changes, cb) {

    if (!changes.deleteIDs.length && !Object.keys(changes.installs).length) {
      this._log.info("No changes to be applied.");
      cb();
      return;
    }

    
    let installs = [];
    for (let [id, record] in Iterator(changes.installs)) {
      installs.push({id: id, value: record});
    }

    let uninstalls = [];
    for (let id of changes.deleteIDs) {
      this._log.info("Uninstalling app: " + id);
      uninstalls.push({id: id, hidden: true});
    }

    
    
    
    
    
    
    
    let doInstalls = function doInstalls() {
      if (!installs.length) {
        if (cb) {
          try {
            cb();
          } catch (ex) {
            this._log.warn("Exception when invoking callback: " +
                           CommonUtils.exceptionStr(ex));
          } finally {
            cb = null;
          }
        }
        return;
      }

      this._applyInstalls(installs, cb);

      
      installs = [];
      cb       = null;
    }.bind(this);

    if (uninstalls.length) {
      DOMApplicationRegistry.updateApps(uninstalls, function onComplete() {
        doInstalls();
        return;
      });
    } else {
      doInstalls();
    }
  },

  



  _applyInstalls: function _applyInstalls(toInstall, callback) {
    let done = 0;
    let total = toInstall.length;
    this._log.info("Applying " + total + " installs to registry");

    




    let self = this;
    function _checkIfDone() {
      done += 1;
      self._log.debug(done + "/" + total + " apps processed");
      if (done == total) {
        callback();
      }
    }

    function _makeManifestCallback(appObj) {
      return function(err, manifest) {
        if (err) {
          self._log.warn("Could not fetch manifest for " + appObj.name);
          _checkIfDone();
          return;
        }
        appObj.value.manifest = manifest;
        DOMApplicationRegistry.updateApps([appObj], _checkIfDone);
      }
    }

    






    for each (let app in toInstall) {
      let url = app.value.manifestURL;
      if (url[0] == "/") {
        url = app.value.origin + app.value.manifestURL;
      }
      this._getManifest(url, _makeManifestCallback(app));
    }
  },

  



  _getManifest: function _getManifest(url, callback) {
    let req = new RESTRequest(url);
    req.timeout = 20;

    let self = this;
    req.get(function(error) {
      if (error) {
        callback(error, null);
        return;
      }
      if (!req.response.success) {
        callback(new Error("Non-200 while fetching manifest"), null);
        return;
      }

      let err = null;
      let manifest = null;
      try {
        manifest = JSON.parse(req.response.body);
        if (!manifest.name) {
          self._log.warn(
            "_getManifest got invalid manifest: " + req.response.body
          );
          err = new Error("Invalid manifest fetched");
        }
      } catch (e) {
        self._log.warn(
          "_getManifest got invalid JSON response: " + req.response.body
        );
        err = new Error("Invalid manifest fetched");
      }

      callback(err, manifest);
    });
  },

};

XPCOMUtils.defineLazyGetter(this, "AitcStorage", function() {
  return new AitcStorageImpl();
});
