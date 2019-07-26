



'use strict'

const { setTimeout } = require('sdk/timers');
const { waitUntil } = require('sdk/test/utils');

exports.testWaitUntil = function (assert, done) {
  let bool = false;
  let finished = false;
  waitUntil(() => {
    if (finished)
      assert.fail('interval should be cleared after predicate is truthy');
    return bool;
  }).then(function () {
    assert.ok(bool,
      'waitUntil shouldn\'t call until predicate is truthy');
    finished = true;
    done();
  });
  setTimeout(() => { bool = true; }, 20);
};

exports.testWaitUntilInterval = function (assert, done) {
  let bool = false;
  let finished = false;
  let counter = 0;
  waitUntil(() => {
    if (finished)
      assert.fail('interval should be cleared after predicate is truthy');
    counter++;
    return bool;
  }, 50).then(function () {
    assert.ok(bool,
      'waitUntil shouldn\'t call until predicate is truthy');
    assert.equal(counter, 1,
      'predicate should only be called once with a higher interval');
    finished = true;
    done();
  });
  setTimeout(() => { bool = true; }, 10);
};

require('sdk/test').run(exports);
