















'use strict';

var Cu = require('chrome').Cu;
var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;


















var promise = require('resource://gre/modules/devtools/deprecated-sync-thenables.js', {});




var Promise = function(executor) {
  this.deferred = promise.defer();
  try {
    executor.call(null, this.deferred.resolve, this.deferred.reject);
  }
  catch (ex) {
    this.deferred.reject(ex);
  }
}

var async = true;







Promise.prototype.then = function(onResolve, onReject) {
  return new Promise(function(resolve, reject) {
    setTimeout(function() {
      try {
        resolve(this.deferred.promise.then(onResolve, onReject));
      }
      catch (ex) {
        reject(ex);
      }
    }.bind(this), 0);
  }.bind(this));
};

Promise.all = promise.all;
Promise.resolve = promise.resolve;
Promise.defer = promise.defer;

exports.Promise = Promise;








var nextTimeoutId = 1; 

var timeoutTable = new Map(); 

var setTimeout = function(callback, millis) {
  let id = nextTimeoutId++;
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(function setTimeout_timer() {
    timeoutTable.delete(id);
    callback.call(undefined);
  }, millis, timer.TYPE_ONE_SHOT);

  timeoutTable.set(id, timer);
  return id;
}

var clearTimeout = function(aId) {
  if (timeoutTable.has(aId)) {
    timeoutTable.get(aId).cancel();
    timeoutTable.delete(aId);
  }
}
