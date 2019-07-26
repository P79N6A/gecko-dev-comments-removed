



"use strict";

this.EXPORTED_SYMBOLS = [
  "ProfileCreationTimeAccessor",
  "ProfileMetadataProvider",
];

const {utils: Cu, classes: Cc, interfaces: Ci} = Components;

const DEFAULT_PROFILE_MEASUREMENT_NAME = "org.mozilla.profile";
const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
const REQUIRED_UINT32_TYPE = {type: "TYPE_UINT32"};

Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/utils.js");




function ProfileCreationTimeAccessor(profile) {
  this.profilePath = profile || OS.Constants.Path.profileDir;
  if (!this.profilePath) {
    throw new Error("No profile directory.");
  }
}
ProfileCreationTimeAccessor.prototype = {
  











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
    let oldest = Date.now() + 1000;
    let iterator = new OS.File.DirectoryIterator(this.profilePath);
dump("Iterating over profile " + this.profilePath);
    if (!iterator) {
      throw new Error("Unable to fetch oldest profile entry: no profile iterator.");
    }

    function onEntry(entry) {
      if ("winLastWriteDate" in entry) {
        
        
        let timestamp = entry.winCreationDate.getTime();
        if (timestamp < oldest) {
          oldest = timestamp;
        }
        return;
      }

      
      function onStatSuccess(info) {
        let date = info.creationDate;
        let timestamp = date.getTime();
        dump("CREATION DATE: " + entry.path + " = " + date);
        if (timestamp < oldest) {
          oldest = timestamp;
        }
      }
      return OS.File.stat(entry.path)
                    .then(onStatSuccess);
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




function ProfileMetadataMeasurement(name=DEFAULT_PROFILE_MEASUREMENT_NAME) {
  MetricsMeasurement.call(this, name, 1);
}
ProfileMetadataMeasurement.prototype = {
  __proto__: MetricsMeasurement.prototype,

  fields: {
    
    "profileCreation": REQUIRED_UINT32_TYPE,
  },
};







function truncate(msec) {
  return Math.floor(msec / MILLISECONDS_PER_DAY);
}




function ProfileMetadataProvider(name="ProfileMetadataProvider") {
  MetricsProvider.call(this, name);
}
ProfileMetadataProvider.prototype = {
  __proto__: MetricsProvider.prototype,

  getProfileCreationDays: function () {
    let accessor = new ProfileCreationTimeAccessor();

    return accessor.created
                   .then(truncate);
  },

  collectConstantMeasurements: function () {
    let result = this.createResult();
    result.expectMeasurement("org.mozilla.profile");
    result.populate = this._populateConstants.bind(this);
    return result;
  },

  _populateConstants: function (result) {
    let name = DEFAULT_PROFILE_MEASUREMENT_NAME;
    result.addMeasurement(new ProfileMetadataMeasurement(name));
    function onSuccess(days) {
      result.setValue(name, "profileCreation", days);
      result.finish();
    }
    function onFailure(ex) {
      result.addError(ex);
      result.finish();
    }
    return this.getProfileCreationDays()
               .then(onSuccess, onFailure);
  },
};

