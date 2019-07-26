






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testCompletion.js</p>";

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

exports.testActivate = function(options) {
  if (!options.display) {
    test.log('No display. Skipping activate tests');
    return;
  }

  helpers.setInput('');
  helpers.check({
    hints: ''
  });

  helpers.setInput(' ');
  helpers.check({
    hints: ''
  });

  helpers.setInput('tsr');
  helpers.check({
    hints: ' <text>'
  });

  helpers.setInput('tsr ');
  helpers.check({
    hints: '<text>'
  });

  helpers.setInput('tsr b');
  helpers.check({
    hints: ''
  });

  helpers.setInput('tsb');
  helpers.check({
    hints: ' [toggle]'
  });

  helpers.setInput('tsm');
  helpers.check({
    hints: ' <abc> <txt> <num>'
  });

  helpers.setInput('tsm ');
  helpers.check({
    hints: 'a <txt> <num>'
  });

  helpers.setInput('tsm a');
  helpers.check({
    hints: ' <txt> <num>'
  });

  helpers.setInput('tsm a ');
  helpers.check({
    hints: '<txt> <num>'
  });

  helpers.setInput('tsm a  ');
  helpers.check({
    hints: '<txt> <num>'
  });

  helpers.setInput('tsm a  d');
  helpers.check({
    hints: ' <num>'
  });

  helpers.setInput('tsm a "d d"');
  helpers.check({
    hints: ' <num>'
  });

  helpers.setInput('tsm a "d ');
  helpers.check({
    hints: ' <num>'
  });

  helpers.setInput('tsm a "d d" ');
  helpers.check({
    hints: '<num>'
  });

  helpers.setInput('tsm a "d d ');
  helpers.check({
    hints: ' <num>'
  });

  helpers.setInput('tsm d r');
  helpers.check({
    hints: ' <num>'
  });

  helpers.setInput('tsm a d ');
  helpers.check({
    hints: '<num>'
  });

  helpers.setInput('tsm a d 4');
  helpers.check({
    hints: ''
  });

  helpers.setInput('tsg');
  helpers.check({
    hints: ' <solo> [options]'
  });

  helpers.setInput('tsg ');
  helpers.check({
    hints: 'aaa [options]'
  });

  helpers.setInput('tsg a');
  helpers.check({
    hints: 'aa [options]'
  });

  helpers.setInput('tsg b');
  helpers.check({
    hints: 'bb [options]'
  });

  helpers.setInput('tsg d');
  helpers.check({
    hints: ' [options] -> ccc'
  });

  helpers.setInput('tsg aa');
  helpers.check({
    hints: 'a [options]'
  });

  helpers.setInput('tsg aaa');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsg aaa ');
  helpers.check({
    hints: '[options]'
  });

  helpers.setInput('tsg aaa d');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsg aaa dddddd');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsg aaa dddddd ');
  helpers.check({
    hints: '[options]'
  });

  helpers.setInput('tsg aaa "d');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsg aaa "d d');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsg aaa "d d"');
  helpers.check({
    hints: ' [options]'
  });

  helpers.setInput('tsn ex ');
  helpers.check({
    hints: ''
  });

  helpers.setInput('selarr');
  helpers.check({
    hints: ' -> tselarr'
  });

  helpers.setInput('tselar 1');
  helpers.check({
    hints: ''
  });

  helpers.setInput('tselar 1', 7);
  helpers.check({
    hints: ''
  });

  helpers.setInput('tselar 1', 6);
  helpers.check({
    hints: ' -> tselarr'
  });

  helpers.setInput('tselar 1', 5);
  helpers.check({
    hints: ' -> tselarr'
  });
};

exports.testLong = function(options) {
  helpers.setInput('tslong --sel');
  helpers.check({
    input:  'tslong --sel',
    hints:              ' <selection> <msg> [options]',
    markup: 'VVVVVVVIIIII'
  });

  helpers.pressTab();
  helpers.check({
    input:  'tslong --sel ',
    hints:               'space <msg> [options]',
    markup: 'VVVVVVVIIIIIV'
  });

  helpers.setInput('tslong --sel ');
  helpers.check({
    input:  'tslong --sel ',
    hints:               'space <msg> [options]',
    markup: 'VVVVVVVIIIIIV'
  });

  helpers.setInput('tslong --sel s');
  helpers.check({
    input:  'tslong --sel s',
    hints:                'pace <msg> [options]',
    markup: 'VVVVVVVIIIIIVI'
  });

  helpers.setInput('tslong --num ');
  helpers.check({
    input:  'tslong --num ',
    hints:               '<number> <msg> [options]',
    markup: 'VVVVVVVIIIIIV'
  });

  helpers.setInput('tslong --num 42');
  helpers.check({
    input:  'tslong --num 42',
    hints:                 ' <msg> [options]',
    markup: 'VVVVVVVVVVVVVVV'
  });

  helpers.setInput('tslong --num 42 ');
  helpers.check({
    input:  'tslong --num 42 ',
    hints:                  '<msg> [options]',
    markup: 'VVVVVVVVVVVVVVVV'
  });

  helpers.setInput('tslong --num 42 --se');
  helpers.check({
    input:  'tslong --num 42 --se',
    hints:                      'l <msg> [options]',
    markup: 'VVVVVVVVVVVVVVVVIIII'
  });

  helpers.pressTab();
  helpers.check({
    input:  'tslong --num 42 --sel ',
    hints:                        'space <msg> [options]',
    markup: 'VVVVVVVVVVVVVVVVIIIIIV'
  });

  helpers.pressTab();
  helpers.check({
    input:  'tslong --num 42 --sel space ',
    hints:                              '<msg> [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV'
  });

  helpers.setInput('tslong --num 42 --sel ');
  helpers.check({
    input:  'tslong --num 42 --sel ',
    hints:                        'space <msg> [options]',
    markup: 'VVVVVVVVVVVVVVVVIIIIIV'
  });

  helpers.setInput('tslong --num 42 --sel space ');
  helpers.check({
    input:  'tslong --num 42 --sel space ',
    hints:                              '<msg> [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV'
  });
};

exports.testNoTab = function(options) {
  helpers.setInput('tss');
  helpers.pressTab();
  helpers.check({
    input:  'tss ',
    markup: 'VVVV',
    hints: ''
  });

  helpers.pressTab();
  helpers.check({
    input:  'tss ',
    markup: 'VVVV',
    hints: ''
  });

  helpers.setInput('xxxx');
  helpers.check({
    input:  'xxxx',
    markup: 'EEEE',
    hints: ''
  });

  helpers.pressTab();
  helpers.check({
    input:  'xxxx',
    markup: 'EEEE',
    hints: ''
  });
};

exports.testOutstanding = function(options) {
  
  







};

exports.testCompleteIntoOptional = function(options) {
  
  helpers.setInput('tso ');
  helpers.check({
    typed:  'tso ',
    hints:      '[text]',
    markup: 'VVVV',
    status: 'VALID'
  });

  helpers.setInput('tso');
  helpers.pressTab();
  helpers.check({
    typed:  'tso ',
    hints:      '[text]',
    markup: 'VVVV',
    status: 'VALID'
  });
};



