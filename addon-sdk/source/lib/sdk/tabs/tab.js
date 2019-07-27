


'use strict';

module.metadata = {
  'stability': 'unstable',
  'engines': {
    'Firefox': '*',
    'Fennec': '*'
  }
};

const { getTargetWindow } = require("../content/mod");
const { getTabContentWindow, isTab } = require("./utils");
const { viewFor } = require("../view/core");

if (require('../system/xul-app').name == 'Fennec') {
  module.exports = require('./tab-fennec');
}
else {
  module.exports = require('./tab-firefox');
}

getTargetWindow.when(isTab, tab => getTabContentWindow(tab));

getTargetWindow.when(x => x instanceof module.exports.Tab,
  tab => getTabContentWindow(viewFor(tab)));
