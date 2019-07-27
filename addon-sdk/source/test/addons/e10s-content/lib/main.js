


'use strict';

const { merge } = require('sdk/util/object');
const { version } = require('sdk/system');

const SKIPPING_TESTS = {
  "test skip": (assert) => assert.pass("nothing to test here")
};

merge(module.exports, require('./test-content-script'));
merge(module.exports, require('./test-content-worker'));
merge(module.exports, require('./test-page-worker'));


if (!version.endsWith('a1')) {
  module.exports = SKIPPING_TESTS;
}

require('sdk/test/runner').runTestsFromModule(module);
