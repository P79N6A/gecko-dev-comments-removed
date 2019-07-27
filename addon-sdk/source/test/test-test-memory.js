


'use strict';

const { Cc, Ci, Cu, components } = require('chrome');
const { gc } = require('sdk/test/memory');

exports.testGC = function(assert, done) {
  let weakref;
  let (tempObj = {}) {
    weakref = Cu.getWeakReference(tempObj);
    assert.equal(weakref.get(), tempObj, 'the weakref returned the tempObj');
  }

  gc().then(function(arg) {
    assert.equal(arg, undefined, 'there is no argument');
    assert.pass('gc() returns a promise which eventually resolves');
    assert.equal(weakref.get(), undefined, 'the weakref returned undefined');
  }).then(done).then(null, assert.fail);
};

require('sdk/test').run(exports);
