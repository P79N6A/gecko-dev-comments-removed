















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testUtil.js</p>";

function test() {
  return Task.spawn(function() {
    let options = yield helpers.openTab(TEST_URI);
    yield helpers.openToolbar(options);
    gcli.addItems(mockCommands.items);

    yield helpers.runTests(options, exports);

    gcli.removeItems(mockCommands.items);
    yield helpers.closeToolbar(options);
    yield helpers.closeTab(options);
  }).then(finish, helpers.handleError);
}




var util = require('gcli/util/util');

exports.testFindCssSelector = function(options) {
  if (options.isPhantomjs || options.isNoDom) {
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
