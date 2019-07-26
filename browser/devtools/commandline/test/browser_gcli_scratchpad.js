






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testScratchpad.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';



var origScratchpad;

exports.setup = function(options) {
  origScratchpad = options.display.inputter.scratchpad;
  options.display.inputter.scratchpad = stubScratchpad;
};

exports.shutdown = function(options) {
  options.display.inputter.scratchpad = origScratchpad;
};

var stubScratchpad = {
  shouldActivate: function(ev) {
    return true;
  },
  activatedCount: 0,
  linkText: 'scratchpad.linkText'
};
stubScratchpad.activate = function(value) {
  stubScratchpad.activatedCount++;
  return true;
};


exports.testActivate = function(options) {
  var ev = {};
  stubScratchpad.activatedCount = 0;
  options.display.inputter.handleKeyUp(ev);
  assert.is(stubScratchpad.activatedCount, 1, 'scratchpad is activated');
};



