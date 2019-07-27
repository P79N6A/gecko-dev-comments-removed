


'use strict';

const { Cu } = require('chrome');
const self = require('sdk/self');
const { AddonManager } = Cu.import('resource://gre/modules/AddonManager.jsm', {});

exports.testContributors = function(assert, done) {
  AddonManager.getAddonByID(self.id, (addon) => {
    assert.equal(addon.creator.name, 'test <test@mozilla.com>', '< and > characters work');
    done();
  });
}

require('sdk/test/runner').runTestsFromModule(module);
