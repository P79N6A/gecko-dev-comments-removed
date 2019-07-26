


'use strict';

const { getTabForContentWindow } = require('./utils');
const { Tab } = require('./tab');

function getTabForWindow(win) {
  let tab = getTabForContentWindow(win);
  
  if (!tab)
    return null;

  return Tab({ tab: tab });
}
exports.getTabForWindow = getTabForWindow;
