












var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFocus.js</p>";

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








exports.setup = function(options) {
  mockCommands.setup();
  helpers.setup(options);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  helpers.shutdown(options);
};

exports.testBasic = function(options) {
  if (options.isJsdom) {
    test.log('jsdom does not pass on focus events properly, skipping testBasic');
    return;
  }

  helpers.focusInput();
  helpers.exec({ typed: 'help' });

  helpers.setInput('tsn deep');
  helpers.check({
    input:  'tsn deep',
    hints:          '',
    markup: 'IIIVIIII',
    cursor: 8,
    status: 'ERROR',
    outputState: 'false:default',
    tooltipState: 'false:default'
  });

  helpers.pressReturn();
  helpers.check({
    input:  'tsn deep',
    hints:          '',
    markup: 'IIIVIIII',
    cursor: 8,
    status: 'ERROR',
    outputState: 'false:default',
    tooltipState: 'true:isError'
  });
};



