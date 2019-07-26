






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testInputter.js</p>";

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





var KeyEvent = require('gcli/util').KeyEvent;



var latestEvent = undefined;
var latestOutput = undefined;
var latestData = undefined;

var outputted = function(ev) {
  function updateData() {
    latestData = latestOutput.data;
  }

  if (latestOutput != null) {
    ev.output.onChange.remove(updateData);
  }

  latestEvent = ev;
  latestOutput = ev.output;

  ev.output.onChange.add(updateData);
};

exports.setup = function(options) {
  options.display.requisition.commandOutputManager.onOutput.add(outputted);
  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  options.display.requisition.commandOutputManager.onOutput.remove(outputted);
};

exports.testOutput = function(options) {
  latestEvent = undefined;
  latestOutput = undefined;
  latestData = undefined;

  var inputter = options.display.inputter;
  var focusManager = options.display.focusManager;

  inputter.setInput('tss');

  inputter.onKeyDown({
    keyCode: KeyEvent.DOM_VK_RETURN
  });

  assert.is(inputter.element.value, 'tss', 'inputter should do nothing on RETURN keyDown');
  assert.is(latestEvent, undefined, 'no events this test');
  assert.is(latestData, undefined, 'no data this test');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_RETURN
  });

  assert.ok(latestEvent != null, 'events this test');
  assert.is(latestData.command.name, 'tss', 'last command is tss');

  assert.is(inputter.element.value, '', 'inputter should exec on RETURN keyUp');

  assert.ok(focusManager._recentOutput, 'recent output happened');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_F1
  });

  assert.ok(!focusManager._recentOutput, 'no recent output happened post F1');
  assert.ok(focusManager._helpRequested, 'F1 = help');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_ESCAPE
  });

  assert.ok(!focusManager._helpRequested, 'ESCAPE = anti help');

  latestOutput.onClose();
};



