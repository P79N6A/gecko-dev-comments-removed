


'use strict';

const { Cu } = require('chrome');
const self = require('sdk/self');
const { AddonManager } = Cu.import('resource://gre/modules/AddonManager.jsm', {});

exports.testContributors = function(assert, done) {
  AddonManager.getAddonByID(self.id, function(addon) {
    let count = 0;
    addon.contributors.forEach(function({ name }) {
      count++;
      assert.equal(name, count == 1 ? 'A' : 'B', 'The contributors keys are correct');
    });
    assert.equal(count, 2, 'The key count is correct');
    assert.equal(addon.contributors.length, 2, 'The key length is correct');
    done();
  });
}

require('sdk/test/runner').runTestsFromModule(module);
