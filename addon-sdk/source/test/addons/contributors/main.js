


'use strict';

const { id } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports.testContributors = function*(assert) {
  let addon = yield getAddonByID(id);
  let count = 0;
  addon.contributors.forEach(({ name }) => {
    assert.equal(name, ++count == 1 ? 'A' : 'B', 'The contributors keys are correct');
  });
  assert.equal(count, 2, 'The key count is correct');
  assert.equal(addon.contributors.length, 2, 'The key length is correct');
}

require('sdk/test/runner').runTestsFromModule(module);
