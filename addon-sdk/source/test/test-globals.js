


'use strict';

Object.defineProperty(this, 'global', { value: this });

exports.testGlobals = function(assert) {
  
  
  assert.equal(typeof module, 'object', 'have "module", good');
  assert.equal(typeof exports, 'object', 'have "exports", good');
  assert.equal(typeof require, 'function', 'have "require", good');
  assert.equal(typeof dump, 'function', 'have "dump", good');
  assert.equal(typeof console, 'object', 'have "console", good');

  
  assert.ok(!('packaging' in global), "no 'packaging', good");
  assert.ok(!('memory' in global), "no 'memory', good");
  assert.ok(/test-globals\.js$/.test(module.uri),
     'should contain filename');
};

exports.testComponent = function (assert) {
  assert.throws(() => {
    Components;
  }, /`Components` is not available/, 'using `Components` throws');
};

require('sdk/test').run(exports);
