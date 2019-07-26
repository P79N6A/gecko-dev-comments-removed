


'use strict';

const { isPrivateBrowsingSupported } = require('sdk/self');

exports.testIsPrivateBrowsingTrue = function(assert) {
  assert.ok(isPrivateBrowsingSupported,
            'isPrivateBrowsingSupported property is true');
};

require('sdk/test/runner').runTestsFromModule(module);
