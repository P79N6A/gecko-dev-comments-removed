



#ifndef MERGED_COMPARTMENT

"use strict";

this.EXPORTED_SYMBOLS = [
  "ProfileTimesAccessor",
  "ProfileMetadataProvider",
];

const {utils: Cu, classes: Cc, interfaces: Ci} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

Cu.import("resource://gre/modules/Metrics.jsm");

#endif

const DEFAULT_PROFILE_MEASUREMENT_NAME = "age";
const DEFAULT_PROFILE_MEASUREMENT_VERSION = 2;
const REQUIRED_UINT32_TYPE = {type: "TYPE_UINT32"};

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");




this.ProfileTimesAccessor = function(profile, log) {
  this.profilePath = profile || OS.Constants.Path.profileDir;
  if (!this.profilePath) {
    throw new Error("No profile directory.");
  }
  this._log = log || {"debug": function (s) { dump(s + "\n"); }};
}
this.ProfileTimesAccessor.prototype = {
  











  get created() {
    function onSuccess(times) {
      if (times.created) {
        return times.created;
      }
      return onFailure.call(this, null, times);
    }

    function onFailure(err, times) {
      return this.computeAndPersistCreated(times)
                 .then(function onSuccess(created) {
                         return created;
                       }.bind(this));
    }

    return this.getTimes()
               .then(onSuccess.bind(this),
                     onFailure.bind(this));
  },

  



  getPath: function (file) {
    return OS.Path.join(this.profilePath, file);
  },

  



  getTimes: function (file="times.json") {
    if (this._times) {
      return Promise.resolve(this._times);
    }
    return this.readTimes(file).then(
      times => {
        return this.times = times || {};
      }
    );
  },

  



  readTimes: function (file="times.json") {
    return CommonUtils.readJSON(this.getPath(file));
  },

  



  writeTimes: function (contents, file="times.json") {
    return CommonUtils.writeJSON(contents, this.getPath(file));
  },

  



  computeAndPersistCreated: function (existingContents, file="times.json") {
    let path = this.getPath(file);
    function onOldest(oldest) {
      let contents = existingContents || {};
      contents.created = oldest;
      this._times = contents;
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

  






  recordProfileReset: function (time=Date.now(), file="times.json") {
    return this.getTimes(file).then(
      times => {
        times.reset = time;
        return this.writeTimes(times, file);
      }
    );
  },

  


  get reset() {
    return this.getTimes().then(
      times => times.reset
    );
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


function ProfileMetadataMeasurement2() {
  Metrics.Measurement.call(this);
}
ProfileMetadataMeasurement2.prototype = {
  __proto__: Metrics.Measurement.prototype,

  name: DEFAULT_PROFILE_MEASUREMENT_NAME,
  version: DEFAULT_PROFILE_MEASUREMENT_VERSION,

  fields: {
    
    profileCreation: {type: Metrics.Storage.FIELD_LAST_NUMERIC},
    
    profileReset: {type: Metrics.Storage.FIELD_LAST_NUMERIC},
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

  measurementTypes: [ProfileMetadataMeasurement2],

  pullOnly: true,

  getProfileDays: Task.async(function* () {
    let result = {};
    let accessor = new ProfileTimesAccessor(null, this._log);

    let created = yield accessor.created;
    result["profileCreation"] = truncate(created);
    let reset = yield accessor.reset;
    if (reset) {
      result["profileReset"] = truncate(reset);
    }
    return result;
  }),

  collectConstantData: function () {
    let m = this.getMeasurement(DEFAULT_PROFILE_MEASUREMENT_NAME,
                                DEFAULT_PROFILE_MEASUREMENT_VERSION);

    return Task.spawn(function* collectConstants() {
      let days = yield this.getProfileDays();

      yield this.enqueueStorageOperation(function storeDays() {
        return Task.spawn(function* () {
          yield m.setLastNumeric("profileCreation", days["profileCreation"]);
          if (days["profileReset"]) {
            yield m.setLastNumeric("profileReset", days["profileReset"]);
          }
        });
      });
    }.bind(this));
  },
};

