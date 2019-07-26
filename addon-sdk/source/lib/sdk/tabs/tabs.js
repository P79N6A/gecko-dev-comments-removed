


'use strict';

module.metadata = {
  'stability': 'unstable',
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

if (require('../system/xul-app').name == 'Fennec') {
  module.exports = require('../windows/tabs-fennec').tabs;
}
else {
  module.exports = require('./tabs-firefox');
}
