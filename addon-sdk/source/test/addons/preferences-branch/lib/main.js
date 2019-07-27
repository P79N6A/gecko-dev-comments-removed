


'use strict';

const { id, preferencesBranch } = require('sdk/self');
const simple = require('sdk/simple-prefs');
const service = require('sdk/preferences/service');
const { getAddonByID } = require('sdk/addon/manager');

exports.testPreferencesBranch = function(assert) {
  assert.equal(preferencesBranch, 'human-readable', 'preferencesBranch is human-readable');
  assert.equal(simple.prefs.test42, true, 'test42 is true');

  simple.prefs.test43 = 'movie';
  assert.equal(service.get('extensions.human-readable.test43'), 'movie', 'test43 is a movie');

}


exports.testSelfID = function*(assert) {
  assert.equal(typeof(id), 'string', 'self.id is a string');
  assert.ok(id.length > 0, 'self.id not empty');
  let addon = yield getAddonByID(id);
  assert.ok(addon, 'found addon with self.id');
}

require('sdk/test/runner').runTestsFromModule(module);
