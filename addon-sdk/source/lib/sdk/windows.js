


'use strict';

module.metadata = {
  'stability': 'stable',
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

if (require('./system/xul-app').is('Firefox')) {
  module.exports = require('./windows/firefox');
}
else if (require('./system/xul-app').is('Fennec')) {
  module.exports = require('./windows/fennec');
}
