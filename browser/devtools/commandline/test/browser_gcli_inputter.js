






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testInputter.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';

var KeyEvent = require('util/util').KeyEvent;



var latestEvent;
var latestData;

var outputted = function(ev) {
  latestEvent = ev;

  ev.output.promise.then(function() {
    latestData = ev.output.data;
    ev.output.onClose();
  });
};


exports.setup = function(options) {
  mockCommands.setup();
  options.display.requisition.commandOutputManager.onOutput.add(outputted);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  options.display.requisition.commandOutputManager.onOutput.remove(outputted);
};

exports.testOutput = function(options) {
  latestEvent = undefined;
  latestData = undefined;

  var inputter = options.display.inputter;
  var focusManager = options.display.focusManager;

  inputter.setInput('tss');

  var ev0 = { keyCode: KeyEvent.DOM_VK_RETURN };
  inputter.onKeyDown(ev0);

  assert.is(inputter.element.value, 'tss', 'inputter should do nothing on RETURN keyDown');
  assert.is(latestEvent, undefined, 'no events this test');
  assert.is(latestData, undefined, 'no data this test');

  var ev1 = { keyCode: KeyEvent.DOM_VK_RETURN };
  return inputter.handleKeyUp(ev1).then(function() {
    assert.ok(latestEvent != null, 'events this test');
    assert.is(latestData, 'Exec: tss ', 'last command is tss');

    assert.is(inputter.element.value, '', 'inputter should exec on RETURN keyUp');

    assert.ok(focusManager._recentOutput, 'recent output happened');

    var ev2 = { keyCode: KeyEvent.DOM_VK_F1 };
    return inputter.handleKeyUp(ev2).then(function() {
      assert.ok(!focusManager._recentOutput, 'no recent output happened post F1');
      assert.ok(focusManager._helpRequested, 'F1 = help');

      var ev3 = { keyCode: KeyEvent.DOM_VK_ESCAPE };
      return inputter.handleKeyUp(ev3).then(function() {
        assert.ok(!focusManager._helpRequested, 'ESCAPE = anti help');
      });
    });

  });
};



