




'use strict';

module.metadata = {
  'stability': 'unstable'
};

function getTestNames (exports)
  Object.keys(exports).filter(name => /^test/.test(name))

function isAsync (fn) fn.length > 1








function before (exports, beforeFn) {
  getTestNames(exports).map(name => {
    let testFn = exports[name];
    if (!isAsync(testFn) && !isAsync(beforeFn)) {
      exports[name] = function (assert) {
        beforeFn(name);
        testFn(assert);
      };
    }
    else if (isAsync(testFn) && !isAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name);
        testFn(assert, done);
      }
    }
    else if (!isAsync(testFn) && isAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, () => {
          testFn(assert);
          done();
        });
      }
    } else if (isAsync(testFn) && isAsync(beforeFn)) {
      exports[name] = function (assert, done) {
        beforeFn(name, () => {
          testFn(assert, done);
        });
      }
    }
  });
}
exports.before = before;








function after (exports, afterFn) {
  getTestNames(exports).map(name => {
    let testFn = exports[name];
    if (!isAsync(testFn) && !isAsync(afterFn)) {
      exports[name] = function (assert) {
        testFn(assert);
        afterFn(name);
      };
    }
    else if (isAsync(testFn) && !isAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert, () => {
          afterFn(name);
          done();
        });
      }
    }
    else if (!isAsync(testFn) && isAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert);
        afterFn(name, done);
      }
    } else if (isAsync(testFn) && isAsync(afterFn)) {
      exports[name] = function (assert, done) {
        testFn(assert, () => {
          afterFn(name, done);
        });
      }
    }
  });
}
exports.after = after;
