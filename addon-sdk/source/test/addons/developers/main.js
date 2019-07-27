


'use strict';

const { Cu } = require('chrome');
const { id } = require('sdk/self');
const { AddonManager } = Cu.import('resource://gre/modules/AddonManager.jsm', {});

exports.testDevelopers = function(assert, done) {
  AddonManager.getAddonByID(id, (addon) => {
    let count = 0;
    addon.developers.forEach(({ name }) => {
      count++;
      assert.equal(name, count == 1 ? 'A' : 'B', 'The developers keys are correct');
    });
    assert.equal(count, 2, 'The key count is correct');
    assert.equal(addon.developers.length, 2, 'The key length is correct');
    done();
  });
}

require('sdk/test/runner').runTestsFromModule(module);
