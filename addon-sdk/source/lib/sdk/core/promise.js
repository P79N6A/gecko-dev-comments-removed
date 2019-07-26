



;(function(id, factory) { 
  if (typeof(define) === 'function') { 
    define(factory);
  } else if (typeof(require) === 'function') { 
    factory.call(this, require, exports, module);
  } else if (~String(this).indexOf('BackstagePass')) { 
    this[factory.name] = {};
    factory(function require(uri) {
      var imports = {};
      this['Components'].utils.import(uri, imports);
      return imports;
    }, this[factory.name], { uri: __URI__, id: id });
    this.EXPORTED_SYMBOLS = [factory.name];
  } else {  
    var globals = this
    factory(function require(id) {
      return globals[id];
    }, (globals[id] = {}), { uri: document.location.href + '#' + id, id: id });
  }
}).call(this, 'loader', function Promise(require, exports, module) {

'use strict';

module.metadata = {
  "stability": "unstable"
};

function resolution(value) {
  



  return { then: function then(resolve) { resolve(value) } }
}

function rejection(reason) {
  



  return { then: function then(resolve, reject) { reject(reason) } }
}

function attempt(f) {
  




  return function effort(options) {
    try { return f(options) }
    catch(error) { return rejection(error) }
  }
}

function isPromise(value) {
  



  return value && typeof(value.then) === 'function'
}

function defer(prototype) {
  

































  var pending = [], result
  prototype = (prototype || prototype === null) ? prototype : Object.prototype

  
  var promise = Object.create(prototype, {
    then: { value: function then(resolve, reject) {
      
      var deferred = defer(prototype)
      
      resolve = resolve ? attempt(resolve) : resolution
      reject = reject ? attempt(reject) : rejection

      
      
      function resolved(value) { deferred.resolve(resolve(value)) }
      function rejected(reason) { deferred.resolve(reject(reason)) }

      
      
      if (pending) pending.push([ resolved, rejected ])
      else result.then(resolved, rejected)

      return deferred.promise
    }}
  })

  var deferred = {
    promise: promise,
    resolve: function resolve(value) {
      



      if (pending) {
        
        
        
        
        result = isPromise(value) ? value : resolution(value)
        
        while (pending.length) result.then.apply(result, pending.shift())
        
        pending = null
      }
    },
    reject: function reject(reason) {
      



      deferred.resolve(rejection(reason))
    }
  }

  return deferred
}
exports.defer = defer

function resolve(value, prototype) {
  



  var deferred = defer(prototype)
  deferred.resolve(value)
  return deferred.promise
}
exports.resolve = resolve

function reject(reason, prototype) {
  




  var deferred = defer(prototype)
  deferred.reject(reason)
  return deferred.promise
}
exports.reject = reject

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
exports.promised = promised

})
