


'use strict';

const { id } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports.testDevelopers = function*(assert) {
  let addon = yield getAddonByID(id);
  let count = 0;
  addon.developers.forEach(({ name }) => {
    assert.equal(name, ++count == 1 ? 'A' : 'B', 'The developers keys are correct');
  });
  assert.equal(count, 2, 'The key count is correct');
  assert.equal(addon.developers.length, 2, 'The key length is correct');
}

require('sdk/test/runner').runTestsFromModule(module);
