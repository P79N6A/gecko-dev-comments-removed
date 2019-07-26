


'use strict';

let { Loader } = require('sdk/test/loader');

exports.testReady = function(assert, done) {
  let loader = Loader(module);
  let { ready, window } = loader.require('sdk/addon/window');
  let windowIsReady = false;

  ready.then(function() {
    assert.equal(windowIsReady, false, 'ready promise was resolved only once');
    windowIsReady = true;

    loader.unload();
    done();
  }).then(null, assert.fail);
}

require('sdk/test').run(exports);
