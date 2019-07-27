



#ifndef MERGED_COMPARTMENT

"use strict";

this.EXPORTED_SYMBOLS = [
  "ProfileCreationTimeAccessor",
  "ProfileMetadataProvider",
];

const {utils: Cu, classes: Cc, interfaces: Ci} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

Cu.import("resource://gre/modules/Metrics.jsm");

#endif

const DEFAULT_PROFILE_MEASUREMENT_NAME = "age";
const REQUIRED_UINT32_TYPE = {type: "TYPE_UINT32"};

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");




this.ProfileCreationTimeAccessor = function(profile, log) {
  this.profilePath = profile || OS.Constants.Path.profileDir;
  if (!this.profilePath) {
    throw new Error("No profile directory.");
  }
  this._log = log || {"debug": function (s) { dump(s + "\n"); }};
}
this.ProfileCreationTimeAccessor.prototype = {
  











  get created() {
    if (this._created) {
      return Promise.resolve(this._created);
    }

    function onSuccess(times) {
      if (times && times.created) {
        return this._created = times.created;
      }
      return onFailure.call(this, null, times);
    }

    function onFailure(err, times) {
      return this.computeAndPersistTimes(times)
                 .then(function onSuccess(created) {
                         return this._created = created;
                       }.bind(this));
    }

    return this.readTimes()
               .then(onSuccess.bind(this),
                     onFailure.bind(this));
  },

  



  getPath: function (file) {
    return OS.Path.join(this.profilePath, file);
  },

  



  readTimes: function (file="times.json") {
    return CommonUtils.readJSON(this.getPath(file));
  },

  



  writeTimes: function (contents, file="times.json") {
    return CommonUtils.writeJSON(contents, this.getPath(file));
  },

  



  computeAndPersistTimes: function (existingContents, file="times.json") {
    let path = this.getPath(file);
    function onOldest(oldest) {
      let contents = existingContents || {};
      contents.created = oldest;
      return this.writeTimes(contents, path)
                 .then(function onSuccess() {
                   return oldest;
                 });
    }

    return this.getOldestProfileTimestamp()
               .then(onOldest.bind(this));
  },

  



  getOldestProfileTimestamp: function () {
    let self = this;
    let oldest = Date.now() + 1000;
    let iterator = new OS.File.DirectoryIterator(this.profilePath);
    self._log.debug("Iterating over profile " + this.profilePath);
    if (!iterator) {
      throw new Error("Unable to fetch oldest profile entry: no profile iterator.");
    }

    function onEntry(entry) {
      function onStatSuccess(info) {
        
        
        let date = info.winBirthDate || info.macBirthDate;
        if (!date || !date.getTime()) {
          
          
          
          
          self._log.debug("No birth date. Using mtime.");
          date = info.lastModificationDate;
        }

        if (date) {
          let timestamp = date.getTime();
          self._log.debug("Using date: " + entry.path + " = " + date);
          if (timestamp < oldest) {
            oldest = timestamp;
          }
        }
      }

      function onStatFailure(e) {
        
        self._log.debug("Stat failure: " + CommonUtils.exceptionStr(e));
      }

      return OS.File.stat(entry.path)
                    .then(onStatSuccess, onStatFailure);
    }

    let promise = iterator.forEach(onEntry);

    function onSuccess() {
      iterator.close();
      return oldest;
    }

    function onFailure(reason) {
      iterator.close();
      throw new Error("Unable to fetch oldest profile entry: " + reason);
    }

    return promise.then(onSuccess, onFailure);
  },
}




function ProfileMetadataMeasurement() {
  Metrics.Measurement.call(this);
}
ProfileMetadataMeasurement.prototype = {
  __proto__: Metrics.Measurement.prototype,

  name: DEFAULT_PROFILE_MEASUREMENT_NAME,
  version: 1,

  fields: {
    
    profileCreation: {type: Metrics.Storage.FIELD_LAST_NUMERIC},
  },
};







function truncate(msec) {
  return Math.floor(msec / MILLISECONDS_PER_DAY);
}




this.ProfileMetadataProvider = function() {
  Metrics.Provider.call(this);
}
this.ProfileMetadataProvider.prototype = {
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.profile",

  measurementTypes: [ProfileMetadataMeasurement],

  pullOnly: true,

  getProfileCreationDays: function () {
    let accessor = new ProfileCreationTimeAccessor(null, this._log);

    return accessor.created
                   .then(truncate);
  },

  collectConstantData: function () {
    let m = this.getMeasurement(DEFAULT_PROFILE_MEASUREMENT_NAME, 1);

    return Task.spawn(function collectConstant() {
      let createdDays = yield this.getProfileCreationDays();

      yield this.enqueueStorageOperation(function storeDays() {
        return m.setLastNumeric("profileCreation", createdDays);
      });
    }.bind(this));
  },
};

