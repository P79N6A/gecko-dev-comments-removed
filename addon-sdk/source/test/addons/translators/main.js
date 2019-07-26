


'use strict';

const { Cc, Ci, Cu, Cm, components } = require('chrome');
const self = require('sdk/self');
const { AddonManager } = Cu.import('resource://gre/modules/AddonManager.jsm', {});


exports.testTranslators = function(assert, done) {
  AddonManager.getAddonByID(self.id, function(addon) {
    let count = 0;
    addon.translators.forEach(function({ name }) {
      count++;
      assert.equal(name, 'Erik Vold', 'The translator keys are correct');
    });
      assert.equal(count, 1, 'The translator key count is correct');
      assert.equal(addon.translators.length, 1, 'The translator key length is correct');
    done();
  });
}

require('sdk/test/runner').runTestsFromModule(module);
