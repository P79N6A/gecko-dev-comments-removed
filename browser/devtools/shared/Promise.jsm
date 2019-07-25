






var EXPORTED_SYMBOLS = [ "Promise" ];





function Promise() {
  this._status = Promise.PENDING;
  this._value = undefined;
  this._onSuccessHandlers = [];
  this._onErrorHandlers = [];

  
  this._id = Promise._nextId++;
  Promise._outstanding[this._id] = this;
}




Promise._nextId = 0;




Promise._outstanding = [];




Promise._recent = [];






Promise.ERROR = -1;
Promise.PENDING = 0;
Promise.SUCCESS = 1;




Promise.prototype.isPromise = true;




Promise.prototype.isComplete = function() {
  return this._status != Promise.PENDING;
};




Promise.prototype.isResolved = function() {
  return this._status == Promise.SUCCESS;
};




Promise.prototype.isRejected = function() {
  return this._status == Promise.ERROR;
};





Promise.prototype.then = function(onSuccess, onError) {
  if (typeof onSuccess === 'function') {
    if (this._status === Promise.SUCCESS) {
      onSuccess.call(null, this._value);
    }
    else if (this._status === Promise.PENDING) {
      this._onSuccessHandlers.push(onSuccess);
    }
  }

  if (typeof onError === 'function') {
    if (this._status === Promise.ERROR) {
      onError.call(null, this._value);
    }
    else if (this._status === Promise.PENDING) {
      this._onErrorHandlers.push(onError);
    }
  }

  return this;
};





Promise.prototype.chainPromise = function(onSuccess) {
  var chain = new Promise();
  chain._chainedFrom = this;
  this.then(function(data) {
    try {
      chain.resolve(onSuccess(data));
    }
    catch (ex) {
      chain.reject(ex);
    }
  }, function(ex) {
    chain.reject(ex);
  });
  return chain;
};




Promise.prototype.resolve = function(data) {
  return this._complete(this._onSuccessHandlers,
                        Promise.SUCCESS, data, 'resolve');
};




Promise.prototype.reject = function(data) {
  return this._complete(this._onErrorHandlers, Promise.ERROR, data, 'reject');
};





Promise.prototype._complete = function(list, status, data, name) {
  
  if (this._status != Promise.PENDING) {
    if (typeof 'console' === 'object') {
      console.error('Promise complete. Attempted ' + name + '() with ', data);
      console.error('Prev status = ', this._status, ', value = ', this._value);
    }
    throw new Error('Promise already complete');
  }

  this._status = status;
  this._value = data;

  
  list.forEach(function(handler) {
    handler.call(null, this._value);
  }, this);
  delete this._onSuccessHandlers;
  delete this._onErrorHandlers;

  
  
  delete Promise._outstanding[this._id];
  
  
  






  return this;
};







Promise.group = function(promiseList) {
  if (!Array.isArray(promiseList)) {
    promiseList = Array.prototype.slice.call(arguments);
  }

  
  if (promiseList.length === 0) {
    return new Promise().resolve([]);
  }

  var groupPromise = new Promise();
  var results = [];
  var fulfilled = 0;

  var onSuccessFactory = function(index) {
    return function(data) {
      results[index] = data;
      fulfilled++;
      
      if (groupPromise._status !== Promise.ERROR) {
        if (fulfilled === promiseList.length) {
          groupPromise.resolve(results);
        }
      }
    };
  };

  promiseList.forEach(function(promise, index) {
    var onSuccess = onSuccessFactory(index);
    var onError = groupPromise.reject.bind(groupPromise);
    promise.then(onSuccess, onError);
  });

  return groupPromise;
};
