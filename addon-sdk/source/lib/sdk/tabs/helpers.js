


'use strict';

module.metadata = {
  'stability': 'unstable'
};





const { getTabForContentWindow, getTabForBrowser: getRawTabForBrowser } = require('./utils');
const { modelFor } = require('../model/core');

function getTabForWindow(win) {
  let tab = getTabForContentWindow(win);
  
  if (!tab)
    return null;

  return modelFor(tab);
}
exports.getTabForWindow = getTabForWindow;

exports.getTabForRawTab = modelFor;

function getTabForBrowser(browser) {
  return modelFor(getRawTabForBrowser(browser)) || null;
}
exports.getTabForBrowser = getTabForBrowser;
