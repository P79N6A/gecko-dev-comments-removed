



"use strict";

const EXPORTED_SYMBOLS = ["AitcStorage", "AitcQueue"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/rest.js");















function AitcQueue(filename, cb) {
  if (!cb) {
    throw new Error("AitcQueue constructor called without callback");
  }

  this._log = Log4Moz.repository.getLogger("Service.AITC.Storage.Queue");
  this._log.level = Log4Moz.Level[Preferences.get(
    "services.aitc.storage.log.level"
  )];

  this._queue = [];
  this._writeLock = false;
  this._file = FileUtils.getFile("ProfD", ["webapps", filename], true);

  this._log.info("AitcQueue instance loading");

  let self = this;
  if (this._file.exists()) {
    this._getFile(function gotFile(data) {
      if (data && Array.isArray(data)) {
        self._queue = data;
      }
      self._log.info("AitcQueue instance created");
      cb(true);
    });
  } else {
    self._log.info("AitcQueue instance created");
    cb(true);
  }
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

  



  _getFile: function _getFile(cb) {
    let channel = NetUtil.newChannel(this._file);
    channel.contentType = "application/json";

    let self = this;
    NetUtil.asyncFetch(channel, function _asyncFetched(stream, res) {
      if (!Components.isSuccessCode(res)) {
        self._log.error("Could not read from json file " + this._file.path);
        cb(null);
        return;
      }

      let data = [];
      try {
        data = JSON.parse(
          NetUtil.readInputStreamToString(stream, stream.available())
        );
        stream.close();
        cb(data);
      } catch (e) {
        self._log.error("Could not parse JSON " + e);
        cb(null);
      }
    });
  },

  



  _putFile: function _putFile(value, cb) {
    if (this._writeLock) {
      throw new Error("_putFile already in progress");
    }

    this._writeLock = true;
    try {
      let ostream = FileUtils.openSafeFileOutputStream(this._file);

      let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                      createInstance(Ci.nsIScriptableUnicodeConverter);
      converter.charset = "UTF-8";
      let istream = converter.convertToInputStream(JSON.stringify(value));

      
      let self = this;
      this._log.info("Writing queue to disk");
      NetUtil.asyncCopy(istream, ostream, function _asyncCopied(result) {
        self._writeLock = false;
        if (Components.isSuccessCode(result)) {
          self._log.info("asyncCopy succeeded");
          cb(null);
        } else {
          let msg = new Error("asyncCopy failed with " + result);
          self._log.info(msg);
          cb(msg);
        }
      });
    } catch (e) {
      this._writeLock = false;
      cb(msg);
    }
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
        self._processApps(remoteApps, localApps, callback);
      }
    );
  },

  



















  _processApps: function _processApps(remoteApps, lApps, callback) {
    let toDelete = {};
    let localApps = {};

    
    
    
    
    if (!Object.keys(remoteApps).length) {
      this._log.warn("Empty set of remote apps to _processApps, returning");
      callback();
      return;
    }

    
    for (let [id, app] in Iterator(lApps)) {
      app.id = id;
      toDelete[app.origin] = app;
      localApps[app.origin] = app;
    }

    
    let toInstall = [];
    for each (let app in remoteApps) {
      
      let origin = app.origin;
      if (!app.hidden) {
        delete toDelete[origin];
      }

      
      if (app.hidden && !localApps[origin]) {
        continue;
      }

      
      
      let id;
      if (!localApps[origin]) {
        id = DOMApplicationRegistry.makeAppId();
      }
      if (localApps[origin] &&
          (localApps[origin].installTime < app.installTime)) {
        id = localApps[origin].id;
      }

      
      if (id) {
        toInstall.push({id: id, value: app});
      }
    }

    
    let toUninstall = [];
    for (let origin in toDelete) {
      toUninstall.push({id: toDelete[origin].id, hidden: true});
    }

    
    if (toUninstall.length) {
      this._log.info("Applying uninstalls to registry");

      let self = this;
      DOMApplicationRegistry.updateApps(toUninstall, function() {
        
        if (toInstall.length) {
          self._applyInstalls(toInstall, callback);
          return;
        }
        callback();
      });

      return;
    }

    
    if (toInstall.length) {
      this._applyInstalls(toInstall, callback);
      return;
    }

    this._log.info("There were no changes to be applied, returning");
    callback();
    return;
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
