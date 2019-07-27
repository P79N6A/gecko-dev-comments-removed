















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_inputter.js");
}


var KeyEvent = require('gcli/util/util').KeyEvent;

var latestEvent;
var latestData;

var outputted = function(ev) {
  latestEvent = ev;

  ev.output.promise.then(function() {
    latestData = ev.output.data;
  });
};


exports.setup = function(options) {
  options.requisition.commandOutputManager.onOutput.add(outputted);
};

exports.shutdown = function(options) {
  options.requisition.commandOutputManager.onOutput.remove(outputted);
};

exports.testOutput = function(options) {
  latestEvent = undefined;
  latestData = undefined;

  var terminal = options.terminal;
  if (!terminal) {
    assert.log('Skipping testInputter.testOutput due to lack of terminal.');
    return;
  }

  var focusManager = terminal.focusManager;

  terminal.setInput('tss');

  var ev0 = { keyCode: KeyEvent.DOM_VK_RETURN };
  terminal.onKeyDown(ev0);

  assert.is(terminal.getInputState().typed,
            'tss',
            'terminal should do nothing on RETURN keyDown');
  assert.is(latestEvent, undefined, 'no events this test');
  assert.is(latestData, undefined, 'no data this test');

  var ev1 = { keyCode: KeyEvent.DOM_VK_RETURN };
  return terminal.handleKeyUp(ev1).then(function() {
    assert.ok(latestEvent != null, 'events this test');
    assert.is(latestData.name, 'tss', 'last command is tss');

    assert.is(terminal.getInputState().typed,
              '',
              'terminal should exec on RETURN keyUp');

    assert.ok(focusManager._recentOutput, 'recent output happened');

    var ev2 = { keyCode: KeyEvent.DOM_VK_F1 };
    return terminal.handleKeyUp(ev2).then(function() {
      assert.ok(!focusManager._recentOutput, 'no recent output happened post F1');
      assert.ok(focusManager._helpRequested, 'F1 = help');

      var ev3 = { keyCode: KeyEvent.DOM_VK_ESCAPE };
      return terminal.handleKeyUp(ev3).then(function() {
        assert.ok(!focusManager._helpRequested, 'ESCAPE = anti help');
      });
    });

  });
};
