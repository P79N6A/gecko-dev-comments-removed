



'use strict';

module.metadata = {
  'engines': {
    'Firefox': '*'
  }
};

const { safeMerge: merge } = require('sdk/util/object');

merge(module.exports,
  require('./tests/test-places-bookmarks'),
  require('./tests/test-places-events'),
  require('./tests/test-places-favicon'),
  require('./tests/test-places-history'),
  require('./tests/test-places-host'),
  require('./tests/test-places-utils')
);

require('sdk/test/runner').runTestsFromModule(module);
