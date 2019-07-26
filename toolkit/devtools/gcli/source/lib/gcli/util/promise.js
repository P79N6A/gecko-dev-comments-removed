















'use strict';




var util = {
  errorHandler: function(ex) {
    if (ex instanceof Error) {
      
      if (ex.stack.indexOf(ex.message) !== -1) {
        console.error(ex.stack);
      }
      else {
        console.error('' + ex);
        console.error(ex.stack);
      }
    }
    else {
      console.error(ex);
    }
  }
};






function fulfilled(value) {
  return { then: function then(fulfill) { fulfill(value); } };
}






function rejected(reason) {
  return { then: function then(fulfill, reject) { reject(reason); } };
}





function isPromise(value) {
  return value && typeof(value.then) === 'function';
}

































function defer(prototype) {
  
  
  var observers = [];

  
  
  
  
  var result = null;

  prototype = (prototype || prototype === null) ? prototype : Object.prototype;

  
  var promise = Object.create(prototype, {
    then: { value: function then(onFulfill, onError) {
      var deferred = defer(prototype);

      function resolve(value) {
        
        
        
        try {
          deferred.resolve(onFulfill ? onFulfill(value) : value);
        }
        
        
        catch(error) {
          if (exports._reportErrors && typeof(console) === 'object') {
            util.errorHandler(error);
          }
          
          
          deferred.resolve(rejected(error));
        }
      }

      function reject(reason) {
        try {
          if (onError) { deferred.resolve(onError(reason)); }
          else { deferred.resolve(rejected(reason)); }
        }
        catch(error) {
          if (exports._reportErrors && typeof(console) === 'object') {
            util.errorHandler(error);
          }
          deferred.resolve(rejected(error));
        }
      }

      
      
      
      
      
      if (observers) {
        observers.push({ resolve: resolve, reject: reject });
      }
      
      else {
        result.then(resolve, reject);
      }

      return deferred.promise;
    }},
    done: { value: function() {
      this.then(null, util.errorHandler);
    }},
  });

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
  
  
  

  var call = Function.call;
  var concat = Array.prototype.concat;

  
  
  function execute(args) { return call.apply(call, args); }

  
  
  function promisedConcat(promises, unknown) {
    return promises.then(function(values) {
      return resolve(unknown).then(function(value) {
        return values.concat([ value ]);
      });
    });
  }

  return function promised(f, prototype) {
    











    return function promised() {
      
      return concat.apply([ f, this ], arguments).
        
        reduce(promisedConcat, resolve([], prototype)).
        
        then(execute);
    };
  };
})();
exports.promised = promised;

var all = promised(Array);
exports.all = all;
