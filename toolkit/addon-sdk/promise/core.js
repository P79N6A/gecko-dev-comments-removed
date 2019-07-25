



;(function(id, factory) { 
  if (typeof(define) === 'function') { 
    define(factory);
  } else if (typeof(require) === 'function') { 
    factory.call(this, require, exports, module);
  } else if (String(this).indexOf('BackstagePass') >= 0) { 
    this[factory.name] = {};
    factory(function require(uri) {
      var imports = {};
      this['Components'].utils.import(uri, imports);
      return imports;
    }, this[factory.name], { uri: __URI__, id: id });
    this.EXPORTED_SYMBOLS = [factory.name];
  } else {  
    var globals = this;
    factory(function require(id) {
      return globals[id];
    }, (globals[id] = {}), { uri: document.location.href + '#' + id, id: id });
  }
}).call(this, 'promise/core', function Promise(require, exports, module) {

'use strict';






function fulfilled(value) {
  return { then: function then(fulfill) { fulfill(value); } };
}






function rejected(reason) {
  return { then: function then(fulfill, reject) { reject(reason); } };
}





function attempt(f) {
  return function effort(input) {
    try {
      return f(input);
    }
    catch(error) {
      return rejected(error);
    }
  };
}





function isPromise(value) {
  return value && typeof(value.then) === 'function';
}





























function defer() {
  
  
  var observers = [];

  
  
  
  
  var result = null;

  var deferred = {
    promise: {
      then: function then(onFulfill, onError) {
        var deferred = defer();

        
        
        
        
        
        
        
        onFulfill = onFulfill ? attempt(onFulfill) : fulfilled;
        onError = onError ? attempt(onError) : rejected;

        
        
        function resolveDeferred(value) { deferred.resolve(onFulfill(value)); }
        function rejectDeferred(reason) { deferred.resolve(onError(reason)); }

        
        
        
        
        
        if (observers) {
          observers.push({ resolve: resolveDeferred, reject: rejectDeferred });
        }
        
        else {
          result.then(resolveDeferred, rejectDeferred);
        }

        return deferred.promise;
      }
    },
    






    resolve: function resolve(value) {
      if (!result) {
        
        
        
        
        result = isPromise(value) ? value : fulfilled(value);

        
        
        
        
        
        
        while (observers.length) {
          var observer = observers.shift();
          result.then(observer.resolve, observer.reject);
        }

        
        
        observers = null;
      }
    },
    




    reject: function reject(reason) {
      
      
      
      
      
      
      
      
      
      deferred.resolve(rejected(reason));
    }
  };

  return deferred;
}
exports.defer = defer;




function resolve(value) {
  var deferred = defer();
  deferred.resolve(value);
  return deferred.promise;
}
exports.resolve = resolve;




function reject(reason) {
  var deferred = defer();
  deferred.reject(reason);
  return deferred.promise;
}
exports.reject = reject;

});
