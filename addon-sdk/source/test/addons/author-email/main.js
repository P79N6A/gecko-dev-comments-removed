


'use strict';

const { id } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports.testContributors = function*(assert) {
  let addon = yield getAddonByID(id);
  assert.equal(addon.creator.name, 'test <test@mozilla.com>', '< and > characters work');
}

require('sdk/test/runner').runTestsFromModule(module);
