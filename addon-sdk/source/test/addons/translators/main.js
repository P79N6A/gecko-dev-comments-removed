


'use strict';

const { id } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports.testTranslators = function*(assert) {
  let addon = yield getAddonByID(id);
  let count = 0;
  addon.translators.forEach(function({ name }) {
    count++;
    assert.equal(name, 'Erik Vold', 'The translator keys are correct');
  });
  assert.equal(count, 1, 'The translator key count is correct');
  assert.equal(addon.translators.length, 1, 'The translator key length is correct');
}

require('sdk/test/runner').runTestsFromModule(module);
