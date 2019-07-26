



;(function(id, factory) { 
  if (typeof(define) === 'function') { 
    define(factory);
  } else if (typeof(require) === 'function') { 
    factory.call(this, require, exports, module);
  } else if (String(this).indexOf('BackstagePass') >= 0) { 
    this[factory.name] = {};
    try {
      this.console = this['Components'].utils
          .import('resource://gre/modules/devtools/Console.jsm', {}).console;
    }
    catch (ex) {
      
    }
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

module.metadata = {
  "stability": "unstable"
};






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
      if (exports._reportErrors && typeof(console) === 'object') {
        console.error(error)
      }
      return rejected(error)
    }
  };
}





function isPromise(value) {
  return value && typeof(value.then) === 'function';
}

































function defer(prototype) {
  
  
  var observers = [];

  
  
  
  
  var result = null;

  prototype = (prototype || prototype === null) ? prototype : Object.prototype

  
  var promise = Object.create(prototype, {
    then: { value: function then(onFulfill, onError) {
      var deferred = defer(prototype);

      
      
      
      
      
      
      
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
    }}
  })

  var deferred = {
    promise: promise,
    






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






function resolve(value, prototype) {
  var deferred = defer(prototype);
  deferred.resolve(value);
  return deferred.promise;
}
exports.resolve = resolve;






function reject(reason, prototype) {
  var deferred = defer(prototype);
  deferred.reject(reason);
  return deferred.promise;
}
exports.reject = reject;

var promised = (function() {
  
  
  

  var call = Function.call
  var concat = Array.prototype.concat

  
  
  function execute(args) { return call.apply(call, args) }

  
  
  function promisedConcat(promises, unknown) {
    return promises.then(function(values) {
      return resolve(unknown).then(function(value) {
        return values.concat([ value ])
      })
    })
  }

  return function promised(f, prototype) {
    











    return function promised() {
      
      return concat.apply([ f, this ], arguments).
        
        reduce(promisedConcat, resolve([], prototype)).
        
        then(execute)
    }
  }
})()
exports.promised = promised;

});
