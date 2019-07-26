


'use strict';

module.metadata = {
  'stability': 'unstable',
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

if (require('../system/xul-app').is('Firefox')) {
  module.exports = require('./tabs-firefox');
}
else if (require('../system/xul-app').is('Fennec')) {
  module.exports = require('../windows/tabs-fennec').tabs;
}
