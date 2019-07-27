


'use strict';

const { Class } = require('../core/heritage');
const { BrowserWindow } = require('../window/browser');
const { WindowTracker } = require('../deprecated/window-utils');
const { isBrowser, getMostRecentBrowserWindow } = require('../window/utils');
const { windowNS } = require('../window/namespace');
const { on, off, once, emit } = require('../event/core');
const { method } = require('../lang/functional');
const { EventTarget } = require('../event/target');
const { List, addListItem } = require('../util/list');

const ERR_FENNEC_MSG = 'This method is not yet supported by Fennec, consider using require("sdk/tabs") instead';



let BrowserWindows = Class({
  implements: [ List ],
  extends: EventTarget,
  initialize: function() {
    List.prototype.initialize.apply(this);
  },
  get activeWindow() {
    let window = getMostRecentBrowserWindow();
    return window ? getBrowserWindow({window: window}) : null;
  },
  open: function open(options) {
    throw new Error(ERR_FENNEC_MSG);
    return null;
  }
});
const browserWindows = exports.browserWindows = BrowserWindows();






function getRegisteredWindow(chromeWindow) {
  for (let window of browserWindows) {
    if (chromeWindow === windowNS(window).window)
      return window;
  }

  return null;
}







function getBrowserWindow(options) {
  let window = null;

  
  if ('window' in options)
    window = getRegisteredWindow(options.window);
  if (window)
    return window;

  
  var window = BrowserWindow(options);
  addListItem(browserWindows, window);
  return window;
}

WindowTracker({
  onTrack: function onTrack(chromeWindow) {
    if (!isBrowser(chromeWindow)) return;
    let window = getBrowserWindow({ window: chromeWindow });
    emit(browserWindows, 'open', window);
  },
  onUntrack: function onUntrack(chromeWindow) {
    if (!isBrowser(chromeWindow)) return;
    let window = getBrowserWindow({ window: chromeWindow });
    emit(browserWindows, 'close', window);
  }
});
