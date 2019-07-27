



"use strict";

this.EXPORTED_SYMBOLS = ["ProfileAge"];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");






this.ProfileAge = function(profile, log) {
  this.profilePath = profile || OS.Constants.Path.profileDir;
  if (!this.profilePath) {
    throw new Error("No profile directory.");
  }
  this._log = log || {"debug": function (s) { dump(s + "\n"); }};
}
this.ProfileAge.prototype = {
  











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
