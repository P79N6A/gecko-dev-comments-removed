



'use strict';

const { safeMerge: merge } = require('sdk/util/object');
const app = require("sdk/system/xul-app");



if (app.is('Firefox')) {
  merge(module.exports,
    require('./test-places-events'),
    require('./test-places-bookmarks'),
    require('./test-places-favicon'),
    require('./test-places-history'),
    require('./test-places-host'),
    require('./test-places-utils')
  );
} else {
  exports['test unsupported'] = (assert) => {
    assert.pass('This application is unsupported.');
  };
}

require('sdk/test/runner').runTestsFromModule(module);
