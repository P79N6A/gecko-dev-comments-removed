



'use strict'

const { defer: async } = require('sdk/lang/functional');
const { before, after } = require('sdk/test/utils');

let AFTER_RUN = 0;
let BEFORE_RUN = 0;






exports.testABeforeAsync = function (assert, done) {
  assert.equal(BEFORE_RUN, 1, 'before function was called');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
  async(done)();
};

exports.testABeforeNameAsync = function (assert, done) {
  assert.equal(BEFORE_RUN, 2, 'before function was called with name');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
  async(done)();
};

exports.testAfterAsync = function (assert, done) {
  assert.equal(AFTER_RUN, 1, 'after function was called previously');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
  async(done)();
};

exports.testAfterNameAsync = function (assert, done) {
  assert.equal(AFTER_RUN, 2, 'after function was called with name');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
  async(done)();
};

exports.testSyncABefore = function (assert) {
  assert.equal(BEFORE_RUN, 1, 'before function was called for sync test');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
};

exports.testSyncAfter = function (assert) {
  assert.equal(AFTER_RUN, 1, 'after function was called for sync test');
  BEFORE_RUN = 0;
  AFTER_RUN = 0;
};

before(exports, (name, done) => {
  if (name === 'testABeforeNameAsync')
    BEFORE_RUN = 2;
  else
    BEFORE_RUN = 1;
  async(done)();
});

after(exports, (name, done) => {
  
  
  if (name === 'testAfterAsync')
    AFTER_RUN = 2;
  else
    AFTER_RUN = 1;
  async(done)();
});

require('sdk/test').run(exports);
