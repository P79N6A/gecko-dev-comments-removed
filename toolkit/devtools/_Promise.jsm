






this.EXPORTED_SYMBOLS = [ "Promise" ];








this.Promise = function Promise(aTrace) {
  this._status = Promise.PENDING;
  this._value = undefined;
  this._onSuccessHandlers = [];
  this._onErrorHandlers = [];
  this._trace = aTrace;

  
  if (Promise.Debug._debug) {
    this._id = Promise.Debug._nextId++;
    Promise.Debug._outstanding[this._id] = this;
  }
}




Promise.Debug = {
  





  setDebug: function(value) {
    Promise.Debug._debug = value;
    if (!value) {
      Promise.Debug._outstanding = [];
      Promise.Debug._recent = [];
    }
  },

  _debug: false,

  


  _nextId: 0,

  


  _outstanding: [],

  


  _recent: []
};







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
    Promise._error("Promise complete.", "Attempted ", name, "() with ", data);
    Promise._error("Previous status: ", this._status, ", value =", this._value);
    throw new Error('Promise already complete');
  }

  if (list.length == 0 && status == Promise.ERROR) {
    var frame;
    var text;

    
    
    Promise._error("Promise rejection ignored and silently dropped", data);
    if (data.stack) {
      if (data.fileName && data.lineNumber) {
        Promise._error("Error originating at", data.fileName,
                       ", line", data.lineNumber );
      }
      try {
        for (frame = data.stack; frame; frame = frame.caller) {
          text += frame + "\n";
        }
        Promise._error("Attempting to extract exception stack", text);
      } catch (x) {
        Promise._error("Could not extract exception stack.");
      }
    } else {
      Promise._error("Exception stack not available.");
    }
    if (Components && Components.stack) {
      try {
        text = "";
        for (frame = Components.stack; frame; frame = frame.caller) {
          text += frame + "\n";
        }
        Promise._error("Attempting to extract current stack", text);
      } catch (x) {
        Promise._error("Could not extract current stack.");
      }
    } else {
      Promise._error("Current stack not available.");
    }
  }


  this._status = status;
  this._value = data;

  
  list.forEach(function(handler) {
    handler.call(null, this._value);
  }, this);
  delete this._onSuccessHandlers;
  delete this._onErrorHandlers;

  
  
  delete Promise.Debug._outstanding[this._id];
  
  
  






  return this;
};









Promise._error = null;
if (typeof console != "undefined" && console.warn) {
  Promise._error = function() {
    var args = Array.prototype.slice.call(arguments);
    args.unshift("Promise");
    console.warn.call(console, args);
  };
} else {
  Promise._error = function() {
    var i;
    var len = arguments.length;
    dump("Promise: ");
    for (i = 0; i < len; ++i) {
      dump(arguments[i]+" ");
    }
    dump("\n");
  };
}







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



























Promise.prototype.trap = function(aTrap) {
  var promise = new Promise();
  var resolve = Promise.prototype.resolve.bind(promise);
  var reject = function(aRejection) {
    try {
      
      var result = aTrap.call(aTrap, aRejection);
      promise.resolve(result);
    } catch (x) {
      promise.reject(x);
    }
  };
  this.then(resolve, reject);
  return promise;
};




















Promise.prototype.always = function(aTrap) {
  var promise = new Promise();
  var resolve = function(result) {
    try {
      aTrap.call(aTrap);
      promise.resolve(result);
    } catch (x) {
      promise.reject(x);
    }
  };
  var reject = function(result) {
    try {
      aTrap.call(aTrap);
      promise.reject(result);
    } catch (x) {
      promise.reject(result);
    }
  };
  this.then(resolve, reject);
  return promise;
};


Promise.prototype.toString = function() {
  var status;
  switch (this._status) {
  case Promise.PENDING:
    status = "pending";
    break;
  case Promise.SUCCESS:
    status = "resolved";
    break;
  case Promise.ERROR:
    status = "rejected";
    break;
  default:
    status = "invalid status: "+this._status;
  }
  return "[Promise " + this._id + " (" + status + ")]";
};
