


'use strict';

const { merge } = require('sdk/util/object');
const { get } = require('sdk/preferences/service');

merge(module.exports, require('./test-tab'));
merge(module.exports, require('./test-tab-events'));
merge(module.exports, require('./test-tab-observer'));
merge(module.exports, require('./test-tab-utils'));


if (get('app.update.channel') !== 'nightly') {
  module.exports = {};
}

require('sdk/test/runner').runTestsFromModule(module);
