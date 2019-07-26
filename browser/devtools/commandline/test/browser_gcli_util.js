






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testUtil.js</p>";

function test() {
  var tests = Object.keys(exports);
  
  tests.sort(function(t1, t2) {
    if (t1 == "setup" || t2 == "shutdown") return -1;
    if (t2 == "setup" || t1 == "shutdown") return 1;
    return 0;
  });
  info("Running tests: " + tests.join(", "))
  tests = tests.map(function(test) { return exports[test]; });
  DeveloperToolbarTest.test(TEST_URI, tests, true);
}



var util = require('gcli/util');


exports.testFindCssSelector = function(options) {
  var nodes = options.window.document.querySelectorAll('*');
  for (var i = 0; i < nodes.length; i++) {
    var selector = util.findCssSelector(nodes[i]);
    var matches = options.window.document.querySelectorAll(selector);

    assert.is(matches.length, 1, 'multiple matches for ' + selector);
    assert.is(matches[0], nodes[i], 'non-matching selector: ' + selector);
  }
};



