


'use strict';

module.metadata = {
  'stability': 'unstable'
};

const { defer } = require('../core/promise');
const { setInterval, clearInterval } = require('../timers');
const { getTabs, closeTab } = require("../tabs/utils");
const { windows: getWindows } = require("../window/utils");
const { close: closeWindow } = require("../window/helpers");
const { isGenerator } = require("../lang/type");

const { Task } = require("resource://gre/modules/Task.jsm");

function getTestNames (exports)
  Object.keys(exports).filter(name => /^test/.test(name))

function isTestAsync (fn) fn.length > 1
function isHelperAsync (fn) fn.length > 2








function before (exports, beforeFn) {
  getTestNames(exports).map(name => {
    let testFn = exports[name];

    
    if (isGenerator(testFn) && isGenerator(beforeFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(beforeFn.bind(null, name, assert));
        yield Task.spawn(testFn.bind(null, assert));
      }
    }
    else if (isGenerator(testFn) && !isHelperAsync(beforeFn)) {
      exports[name] = function*(assert) {
        beforeFn(name, assert);
        yield Task.spawn(testFn.bind(null, assert));
      }
    }
    else if (isGenerator(testFn) && isHelperAsync(beforeFn)) {
      exports[name] = function*(assert) {
        yield new Promise(resolve => beforeFn(name, assert, resolve));
        yield Task.spawn(testFn.bind(null, assert));
      }
    }
    
    else if (!isTestAsync(testFn) && isGenerator(beforeFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(beforeFn.bind(null, name, assert));
        testFn(assert);
      };
    }
    else if (!isTestAsync(testFn) && !isHelperAsync(beforeFn)) {
      exports[name] = function (assert) {
        beforeFn(name, assert);
        testFn(assert);
      };
    }
    else if (!isTestAsync(testFn) && isHelperAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, assert, () => {
          testFn(assert);
          done();
        });
      };
    }
    
    else if (isTestAsync(testFn) && isGenerator(beforeFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(beforeFn.bind(null, name, assert));
        yield new Promise(resolve => testFn(assert, resolve));
      };
    }
    else if (isTestAsync(testFn) && !isHelperAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, assert);
        testFn(assert, done);
      };
    }
    else if (isTestAsync(testFn) && isHelperAsync(beforeFn)) {
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

    
    if (isGenerator(testFn) && isGenerator(afterFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(testFn.bind(null, assert));
        yield Task.spawn(afterFn.bind(null, name, assert));
      }
    }
    else if (isGenerator(testFn) && !isHelperAsync(afterFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(testFn.bind(null, assert));
        afterFn(name, assert);
      }
    }
    else if (isGenerator(testFn) && isHelperAsync(afterFn)) {
      exports[name] = function*(assert) {
        yield Task.spawn(testFn.bind(null, assert));
        yield new Promise(resolve => afterFn(name, assert, resolve));
      }
    }
    
    else if (!isTestAsync(testFn) && isGenerator(afterFn)) {
      exports[name] = function*(assert) {
        testFn(assert);
        yield Task.spawn(afterFn.bind(null, name, assert));
      };
    }
    else if (!isTestAsync(testFn) && !isHelperAsync(afterFn)) {
      exports[name] = function (assert) {
        testFn(assert);
        afterFn(name, assert);
      };
    }
    else if (!isTestAsync(testFn) && isHelperAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert);
        afterFn(name, assert, done);
      };
    }
    
    else if (isTestAsync(testFn) && isGenerator(afterFn)) {
      exports[name] = function*(assert) {
        yield new Promise(resolve => testFn(assert, resolve));
        yield Task.spawn(afterFn.bind(null, name, assert));
      };
    }
    else if (isTestAsync(testFn) && !isHelperAsync(afterFn)) {
      exports[name] = function*(assert) {
        yield new Promise(resolve => testFn(assert, resolve));
        afterFn(name, assert);
      };
    }
    else if (isTestAsync(testFn) && isHelperAsync(afterFn)) {
      exports[name] = function*(assert) {
        yield new Promise(resolve => testFn(assert, resolve));
        yield new Promise(resolve => afterFn(name, assert, resolve));
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
