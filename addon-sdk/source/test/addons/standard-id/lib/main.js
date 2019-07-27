



'use strict';

const { id, preferencesBranch } = require('sdk/self');
const simple = require('sdk/simple-prefs');
const service = require('sdk/preferences/service');
const { getAddonByID } = require('sdk/addon/manager');

exports.testStandardID = function(assert) {
  assert.equal(id, 'standard-id@jetpack', 'standard ID is standard');

  assert.equal(simple.prefs.test13, 26, 'test13 is 26');

  simple.prefs.test14 = '15';
  assert.equal(service.get('extensions.standard-id@jetpack.test14'), '15', 'test14 is 15');
  assert.equal(service.get('extensions.standard-id@jetpack.test14'), simple.prefs.test14, 'simple test14 also 15');
}


exports.testSelfID = function*(assert) {
  assert.equal(typeof(id), 'string', 'self.id is a string');
  assert.ok(id.length > 0, 'self.id not empty');
  let addon = yield getAddonByID(id);
  assert.ok(addon, 'found addon with self.id');
}

require('sdk/test/runner').runTestsFromModule(module);
