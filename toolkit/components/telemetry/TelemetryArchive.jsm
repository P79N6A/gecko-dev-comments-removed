



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryArchive"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetryArchive::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_ARCHIVE_ENABLED = PREF_BRANCH + "archive.enabled";

XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStorage",
                                  "resource://gre/modules/TelemetryStorage.jsm");

this.TelemetryArchive = {
  










  promiseArchivedPingList: function() {
    return TelemetryArchiveImpl.promiseArchivedPingList();
  },

  





  promiseArchivedPingById: function(id) {
    return TelemetryArchiveImpl.promiseArchivedPingById(id);
  },

  





  promiseArchivePing: function(ping) {
    return TelemetryArchiveImpl.promiseArchivePing(ping);
  },

  


  _testReset: function() {
    TelemetryArchiveImpl._testReset();
  },
};






function shouldArchivePings() {
  return Preferences.get(PREF_ARCHIVE_ENABLED, false);
}

let TelemetryArchiveImpl = {
  _logger: null,

  
  
  _archivedPings: new Map(),
  
  _scannedArchiveDirectory: false,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
  },

  _testReset: function() {
    this._archivedPings = new Map();
    this._scannedArchiveDirectory = false;
  },

  promiseArchivePing: function(ping) {
    if (!shouldArchivePings()) {
      this._log.trace("promiseArchivePing - archiving is disabled");
      return Promise.resolve();
    }

    for (let field of ["creationDate", "id", "type"]) {
      if (!(field in ping)) {
        this._log.warn("promiseArchivePing - missing field " + field)
        return Promise.reject(new Error("missing field " + field));
      }
    }

    const creationDate = new Date(ping.creationDate);
    if (this._archivedPings.has(ping.id)) {
      const data = this._archivedPings.get(ping.id);
      if (data.timestampCreated > creationDate.getTime()) {
        this._log.error("promiseArchivePing - trying to overwrite newer ping with the same id");
        return Promise.reject(new Error("trying to overwrite newer ping with the same id"));
      } else {
        this._log.warn("promiseArchivePing - overwriting older ping with the same id");
      }
    }

    this._archivedPings.set(ping.id, {
      timestampCreated: creationDate.getTime(),
      type: ping.type,
    });

    return TelemetryStorage.saveArchivedPing(ping);
  },

  _buildArchivedPingList: function() {
    let list = [for (p of this._archivedPings) {
      id: p[0],
      timestampCreated: p[1].timestampCreated,
      type: p[1].type,
    }];

    list.sort((a, b) => a.timestampCreated - b.timestampCreated);

    return list;
  },

  promiseArchivedPingList: function() {
    this._log.trace("promiseArchivedPingList");

    if (this._scannedArchiveDirectory) {
      return Promise.resolve(this._buildArchivedPingList())
    }

    return TelemetryStorage.loadArchivedPingList().then((loadedInfo) => {
      
      
      for (let [id, info] of loadedInfo) {
        this._log.trace("promiseArchivedPingList - id: " + id + ", info: " + info);
        this._archivedPings.set(id, {
          timestampCreated: info.timestampCreated,
          type: info.type,
        });
      }

      this._scannedArchiveDirectory = true;
      return this._buildArchivedPingList();
    });
  },

  promiseArchivedPingById: function(id) {
    this._log.trace("promiseArchivedPingById - id: " + id);
    const data = this._archivedPings.get(id);
    if (!data) {
      this._log.trace("promiseArchivedPingById - no ping with id: " + id);
      return Promise.reject(new Error("TelemetryArchive.promiseArchivedPingById - no ping with id " + id));
    }

    return TelemetryStorage.loadArchivedPing(id, data.timestampCreated, data.type);
  },
};
