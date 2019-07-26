



'use strict';

const { safeMerge: merge } = require('sdk/util/object');
const app = require("sdk/system/xul-app");



if (app.is('Firefox')) {
  merge(module.exports,
    require('./tests/test-places-bookmarks'),
    require('./tests/test-places-events'),
    require('./tests/test-places-favicon'),
    require('./tests/test-places-history'),
    require('./tests/test-places-host'),
    require('./tests/test-places-utils')
  );
} else {
  exports['test unsupported'] = (assert) => {
    assert.pass('This application is unsupported.');
  };
}

require('sdk/test/runner').runTestsFromModule(module);
