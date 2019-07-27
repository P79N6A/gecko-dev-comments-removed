



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
};






function shouldArchivePings() {
  return Preferences.get(PREF_ARCHIVE_ENABLED, false);
}

let TelemetryArchiveImpl = {
  _logger: null,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
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

    return TelemetryStorage.saveArchivedPing(ping);
  },

  _buildArchivedPingList: function(archivedPingsMap) {
    let list = [for (p of archivedPingsMap) {
      id: p[0],
      timestampCreated: p[1].timestampCreated,
      type: p[1].type,
    }];

    list.sort((a, b) => a.timestampCreated - b.timestampCreated);

    return list;
  },

  promiseArchivedPingList: function() {
    this._log.trace("promiseArchivedPingList");

    return TelemetryStorage.loadArchivedPingList().then(loadedInfo => {
      return this._buildArchivedPingList(loadedInfo);
    });
  },

  promiseArchivedPingById: function(id) {
    this._log.trace("promiseArchivedPingById - id: " + id);
    return TelemetryStorage.loadArchivedPing(id);
  },
};
