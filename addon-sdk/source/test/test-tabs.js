


'use strict';

module.metadata = {
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

const app = require('sdk/system/xul-app');

if (app.is('Fennec')) {
  module.exports = require('./tabs/test-fennec-tabs');
}
else {
  module.exports = require('./tabs/test-firefox-tabs');
}
