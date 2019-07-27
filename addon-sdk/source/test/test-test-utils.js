



'use strict'

const { setTimeout } = require('sdk/timers');
const { waitUntil, cleanUI } = require('sdk/test/utils');
const tabs = require('sdk/tabs');
const fixtures = require("./fixtures");
const testURI = fixtures.url("test.html");

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

exports.testCleanUIWithExtraTabAndWindow = function*(assert) {
  let tab = yield new Promise(resolve => {
    tabs.open({
      url: testURI,
      inNewWindow: true,
      onReady: resolve
    });
  });

  assert.equal(tabs.length, 2, 'there are two tabs open');

  yield cleanUI()
  assert.pass("the ui was cleaned");
  assert.equal(tabs.length, 1, 'there is only one tab open');
}

exports.testCleanUIWithOnlyExtraTab = function*(assert) {
  let tab = yield new Promise(resolve => {
    tabs.open({
      url: testURI,
      inBackground: true,
      onReady: resolve
    });
  });

  assert.equal(tabs.length, 2, 'there are two tabs open');

  yield cleanUI();
  assert.pass("the ui was cleaned.");
  assert.equal(tabs.length, 1, 'there is only one tab open');
}

require('sdk/test').run(exports);
