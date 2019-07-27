


'use strict';

const { merge } = require('sdk/util/object');
const { version } = require('sdk/system');

const SKIPPING_TESTS = {
  "test skip": (assert) => assert.pass("nothing to test here")
};

merge(module.exports, require('./test-tab'));
merge(module.exports, require('./test-tab-events'));
merge(module.exports, require('./test-tab-observer'));
merge(module.exports, require('./test-tab-utils'));


if (!version.endsWith('a1')) {
  module.exports = SKIPPING_TESTS;
}

require('sdk/test/runner').runTestsFromModule(module);
