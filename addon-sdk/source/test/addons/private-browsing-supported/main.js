


'use strict';

const { merge } = require('sdk/util/object');
const app = require('sdk/system/xul-app');
const { isGlobalPBSupported } = require('sdk/private-browsing/utils');

merge(module.exports,
  require('./test-tabs'),
  require('./test-page-mod'),
  require('./test-private-browsing'),
  require('./test-sidebar'),
  isGlobalPBSupported ? require('./test-global-private-browsing') : {}
);




if (!app.is('Fennec')) {
  merge(module.exports,
    require('./test-selection'),
    require('./test-panel'),
    require('./test-window-tabs'),
    require('./test-windows')
  );
}

require('sdk/test/runner').runTestsFromModule(module);
