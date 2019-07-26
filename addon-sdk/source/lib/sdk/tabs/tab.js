


'use strict';

module.metadata = {
  'stability': 'unstable',
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

if (require('../system/xul-app').name == 'Fennec') {
  module.exports = require('./tab-fennec');
}
else {
  module.exports = require('./tab-firefox');
}
