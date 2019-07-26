






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testUtil.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';

var util = require('util/util');


exports.testFindCssSelector = function(options) {
  if (options.isPhantomjs) {
    assert.log('Skipping tests due to issues with querySelectorAll.');
    return;
  }

  var nodes = options.window.document.querySelectorAll('*');
  for (var i = 0; i < nodes.length; i++) {
    var selector = util.findCssSelector(nodes[i]);
    var matches = options.window.document.querySelectorAll(selector);

    assert.is(matches.length, 1, 'multiple matches for ' + selector);
    assert.is(matches[0], nodes[i], 'non-matching selector: ' + selector);
  }
};



