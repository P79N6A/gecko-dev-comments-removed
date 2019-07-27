


'use strict';

const { Cc, Ci, Cu, components } = require('chrome');
const { gc } = require('sdk/test/memory');

exports.testGC = function*(assert) {
  let weakref;

  if (true) {
    let tempObj = {};
    weakref = Cu.getWeakReference(tempObj);
    assert.equal(weakref.get(), tempObj, 'the weakref returned the tempObj');
  }

  let arg = yield gc();

  assert.equal(arg, undefined, 'there is no argument');
  assert.pass('gc() returns a promise which eventually resolves');
  assert.equal(weakref.get(), undefined, 'the weakref returned undefined');
};

require('sdk/test').run(exports);
