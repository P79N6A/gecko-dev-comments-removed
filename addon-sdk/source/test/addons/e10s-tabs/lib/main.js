


'use strict';

const { merge } = require('sdk/util/object');

merge(module.exports, require('./test-tab'));
merge(module.exports, require('./test-tab-events'));
merge(module.exports, require('./test-tab-observer'));
merge(module.exports, require('./test-tab-utils'));

require('sdk/test/runner').runTestsFromModule(module);
