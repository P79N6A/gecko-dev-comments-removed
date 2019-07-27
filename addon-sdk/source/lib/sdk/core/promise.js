


'use strict';









const PROMISE_URI = 'resource://gre/modules/Promise.jsm';

getEnvironment.call(this, function ({ require, exports, module, Cu }) {

const Promise = Cu.import(PROMISE_URI, {}).Promise;
const { Debugging, defer, resolve, all, reject, race } = Promise;

module.metadata = {
  'stability': 'unstable'
};

let promised = (function() {
  
  
  

  var call = Function.call;
  var concat = Array.prototype.concat;

  
  
  function execute (args) call.apply(call, args)

  
  
  function promisedConcat(promises, unknown) {
    return promises.then(function (values) {
      return resolve(unknown)
        .then(function (value) values.concat([value]));
    });
  }

  return function promised(f, prototype) {
    











    return function promised(...args) {
      
      return [f, this, ...args].
        
        reduce(promisedConcat, resolve([], prototype)).
        
        then(execute);
    };
  };
})();

exports.promised = promised;
exports.all = all;
exports.defer = defer;
exports.resolve = resolve;
exports.reject = reject;
exports.race = race;
exports.Promise = Promise;
exports.Debugging = Debugging;
});

function getEnvironment (callback) {
  let Cu, _exports, _module, _require;

  
  if (typeof(require) === 'function') {
    Cu = require('chrome').Cu;
    _exports = exports;
    _module = module;
    _require = require;
  }
  
  else if (String(this).indexOf('BackstagePass') >= 0) {
    Cu = this['Components'].utils;
    _exports = this.Promise = {};
    _module = { uri: __URI__, id: 'promise/core' };
    _require = uri => {
      let imports = {};
      Cu.import(uri, imports);
      return imports;
    };
    this.EXPORTED_SYMBOLS = ['Promise'];
  
  } else if (~String(this).indexOf('Sandbox')) {
    Cu = this['Components'].utils;
    _exports = this;
    _module = { id: 'promise/core' };
    _require = uri => {};
  }

  callback({
    Cu: Cu,
    exports: _exports,
    module: _module,
    require: _require
  });
}

