


'use strict';

const { merge } = require('sdk/util/object');
const { version, platform } = require('sdk/system');
const { getMostRecentBrowserWindow, isBrowser } = require('sdk/window/utils');
const { WindowTracker } = require('sdk/deprecated/window-utils');
const { close, focus } = require('sdk/window/helpers');
const { when } = require('sdk/system/unload');

function replaceWindow(remote) {
  let next = null;
  let old = getMostRecentBrowserWindow();
  let promise = new Promise(resolve => {
    let tracker = WindowTracker({
      onTrack: window => {
        if (window !== next)
          return;
        resolve(window);
        tracker.unload();
      }
    });
  })
  next = old.OpenBrowserWindow({ remote });
  return promise.then(focus).then(_ => close(old));
}


merge(module.exports, require('./test-tab-events'));
merge(module.exports, require('./test-tab-observer'));
merge(module.exports, require('./test-tab-utils'));


if (!version.endsWith('a1')) {
  module.exports = {};
}


if (platform === 'linux') {
  module.exports = {};
  require('sdk/test/runner').runTestsFromModule(module);
}
else {
  replaceWindow(true).then(_ =>
    require('sdk/test/runner').runTestsFromModule(module));

  when(_ => replaceWindow(false));
}
