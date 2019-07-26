


'use strict';

Object.defineProperty(this, 'global', { value: this });

exports.testGlobals = function(assert) {
  
  assert.equal(typeof module, 'object', 'have "module" global');
  assert.equal(typeof exports, 'object', 'have "exports" global');
  assert.equal(typeof require, 'function', 'have "require" global');
  assert.equal(typeof dump, 'function', 'have "dump" global');
  assert.equal(typeof console, 'object', 'have "console" global');

  
  assert.ok(!('packaging' in global), 'no "packaging" global was found');
  assert.ok(!('memory' in global), 'no "memory" global was found');

  assert.ok(/test-globals\.js$/.test(module.uri), 'should contain filename');
};

require("test").run(exports);
