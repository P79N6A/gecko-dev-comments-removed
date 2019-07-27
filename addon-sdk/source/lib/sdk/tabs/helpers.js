


'use strict';

module.metadata = {
  'stability': 'unstable'
};





const { getTabForContentWindow, getTabForBrowser: getRawTabForBrowser } = require('./utils');
const { modelFor } = require('../model/core');

exports.getTabForRawTab = modelFor;

function getTabForBrowser(browser) {
  return modelFor(getRawTabForBrowser(browser)) || null;
}
exports.getTabForBrowser = getTabForBrowser;
