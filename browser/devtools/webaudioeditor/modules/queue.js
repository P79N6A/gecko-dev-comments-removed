


"use strict";

const Promise = require("sdk/core/promise");


















function Queue () {
  this._messages = [];
  this._processing = false;
  this._currentProcess = null;
  this._process = this._process.bind(this);
  this._asyncHandler = this._asyncHandler.bind(this);
}
exports.Queue = Queue;





Queue.prototype.addHandler = function (fn) {
  return (...args) => {
    this._messages.push([fn, ...args]);
    if (!this._processing) {
      this._process();
    }
  }
};

Queue.prototype._process = function () {
  if (this._messages.length === 0) {
    this._processing = false;
    return;
  }

  this._processing = true;

  let [fn, ...args] = this._messages.shift();
  let result = fn.apply(null, args);
  if (result && result.then) {
    
    
    this._currentProcess = result.then(this._asyncHandler, this._asyncHandler);
  } else {
    this._process();
  }
};




Queue.prototype._asyncHandler = function () {
  this._currentProcess = null;
  this._process();
};




Queue.prototype.getMessageCount = function () {
  return this._messages.length;
};






Queue.prototype.clear = function () {
  this._messages.length = 0;
  this._processing = false;

  
  
  if (this._currentProcess) {
    return this._currentProcess;
  }
  return Promise.resolve();
};
