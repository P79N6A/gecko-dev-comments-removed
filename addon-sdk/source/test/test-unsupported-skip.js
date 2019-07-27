


'use strict';

const app = require('sdk/system/xul-app');






if (!app.is('Firefox')) {
  require('./fixtures/loader/unsupported/firefox');
}
else {
  require('./fixtures/loader/unsupported/fennec');
}

exports.testRunning = function (assert) {
  assert.fail('Tests should not run in unsupported applications');
};

require('sdk/test').run(exports);
