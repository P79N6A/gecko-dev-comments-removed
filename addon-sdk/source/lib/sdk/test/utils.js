


'use strict';

module.metadata = {
  'stability': 'unstable'
};

const { defer } = require('../core/promise');
const { setInterval, clearInterval } = require('../timers');
const { getTabs, closeTab } = require("../tabs/utils");
const { windows: getWindows } = require("../window/utils");
const { close: closeWindow } = require("../window/helpers");

function getTestNames (exports)
  Object.keys(exports).filter(name => /^test/.test(name))

function isTestAsync (fn) fn.length > 1
function isHelperAsync (fn) fn.length > 2








function before (exports, beforeFn) {
  getTestNames(exports).map(name => {
    let testFn = exports[name];
    if (!isTestAsync(testFn) && !isHelperAsync(beforeFn)) {
      exports[name] = function (assert) {
        beforeFn(name, assert);
        testFn(assert);
      };
    }
    else if (isTestAsync(testFn) && !isHelperAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, assert);
        testFn(assert, done);
      };
    }
    else if (!isTestAsync(testFn) && isHelperAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, assert, () => {
          testFn(assert);
          done();
        });
      };
    } else if (isTestAsync(testFn) && isHelperAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, assert, () => {
          testFn(assert, done);
        });
      };
    }
  });
}
exports.before = before;








function after (exports, afterFn) {
  getTestNames(exports).map(name => {
    let testFn = exports[name];
    if (!isTestAsync(testFn) && !isHelperAsync(afterFn)) {
      exports[name] = function (assert) {
        testFn(assert);
        afterFn(name, assert);
      };
    }
    else if (isTestAsync(testFn) && !isHelperAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert, () => {
          afterFn(name, assert);
          done();
        });
      };
    }
    else if (!isTestAsync(testFn) && isHelperAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert);
        afterFn(name, assert, done);
      };
    } else if (isTestAsync(testFn) && isHelperAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert, () => {
          afterFn(name, assert, done);
        });
      };
    }
  });
}
exports.after = after;

function waitUntil (predicate, delay) {
  let { promise, resolve } = defer();
  let interval = setInterval(() => {
    if (!predicate()) return;
    clearInterval(interval);
    resolve();
  }, delay || 10);
  return promise;
}
exports.waitUntil = waitUntil;

let cleanUI = function cleanUI() {
  let { promise, resolve } = defer();

  let windows = getWindows(null, { includePrivate: true });
  if (windows.length > 1) {
    return closeWindow(windows[1]).then(cleanUI);
  }

  getTabs(windows[0]).slice(1).forEach(closeTab);

  resolve();

  return promise;
}
exports.cleanUI = cleanUI;
