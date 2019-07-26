






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testIntro.js</p>";

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



  
  
  var canon = require('gcli/canon');

  exports.setup = function(options) {
    helpers.setup(options);
  };

  exports.shutdown = function(options) {
    helpers.shutdown(options);
  };

  exports.testIntroStatus = function(options) {
    if (canon.getCommand('intro') == null) {
      assert.log('Skipping testIntroStatus; missing intro command.');
      return;
    }

    helpers.setInput('intro');
    helpers.check({
      typed:  'intro',
      markup: 'VVVVV',
      status: 'VALID',
      hints: ''
    });

    helpers.setInput('intro foo');
    helpers.check({
      typed:  'intro foo',
      markup: 'VVVVVVEEE',
      status: 'ERROR',
      hints: ''
    });
  };

  exports.testIntroExec = function(options) {
    if (canon.getCommand('intro') == null) {
      assert.log('Skipping testIntroStatus; missing intro command.');
      return;
    }

    helpers.exec({
      typed: 'intro',
      args: { },
      outputMatch: [
        /command\s*line/,
        /help/,
        /F1/,
        /Escape/
      ]
    });
  };


