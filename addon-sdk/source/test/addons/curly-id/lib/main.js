


'use strict';

const simple = require('sdk/simple-prefs');
const service = require('sdk/preferences/service');
const { id, preferencesBranch } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports.testCurlyID = function(assert) {
  assert.equal(id, '{34a1eae1-c20a-464f-9b0e-000000000000}', 'curly ID is curly');
  assert.equal(simple.prefs.test13, 26, 'test13 is 26');

  simple.prefs.test14 = '15';
  assert.equal(service.get('extensions.{34a1eae1-c20a-464f-9b0e-000000000000}.test14'), '15', 'test14 is 15');
  assert.equal(service.get('extensions.{34a1eae1-c20a-464f-9b0e-000000000000}.test14'), simple.prefs.test14, 'simple test14 also 15');
}


exports.testSelfID = function*(assert) {
  assert.equal(typeof(id), 'string', 'self.id is a string');
  assert.ok(id.length > 0, 'self.id not empty');

  let addon = yield getAddonByID(id);
  assert.ok(addon, 'found addon with self.id');
}

require('sdk/test/runner').runTestsFromModule(module);
