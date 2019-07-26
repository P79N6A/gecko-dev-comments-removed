



'use strict';

module.metadata = {
  'stability': 'unstable'
};

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
