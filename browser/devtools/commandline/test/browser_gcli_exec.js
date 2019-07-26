






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testExec.js</p>";

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




var Requisition = require('gcli/cli').Requisition;
var CommandOutputManager = require('gcli/canon').CommandOutputManager;

var nodetype = require('gcli/types/node');



var actualExec;
var actualOutput;
var hideExec = false;
var skip = 'skip';

var environment = { value: 'example environment data' };
var commandOutputManager = new CommandOutputManager();
var requisition = new Requisition(environment, null, commandOutputManager);

exports.setup = function(options) {
  mockCommands.setup();
  mockCommands.onCommandExec.add(commandExeced);

  commandOutputManager.onOutput.add(commandOutputed);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  mockCommands.onCommandExec.remove(commandExeced);

  commandOutputManager.onOutput.remove(commandOutputed);
};

function commandExeced(ev) {
  actualExec = ev;
}

function commandOutputed(ev) {
  actualOutput = ev.output;
}

function exec(command, expectedArgs) {
  var outputObject = requisition.exec({ typed: command, hidden: hideExec });

  assert.is(command.indexOf(actualExec.command.name), 0, 'Command name: ' + command);

  assert.is(command, outputObject.typed, 'outputObject.command for: ' + command);
  assert.ok(outputObject.completed, 'outputObject.completed false for: ' + command);

  if (expectedArgs == null) {
    assert.ok(false, 'expectedArgs == null for ' + command);
    return;
  }
  if (actualExec.args == null) {
    assert.ok(false, 'actualExec.args == null for ' + command);
    return;
  }

  assert.is(Object.keys(expectedArgs).length, Object.keys(actualExec.args).length,
          'Arg count: ' + command);
  Object.keys(expectedArgs).forEach(function(arg) {
    var expectedArg = expectedArgs[arg];
    var actualArg = actualExec.args[arg];

    if (expectedArg === skip) {
      return;
    }

    if (Array.isArray(expectedArg)) {
      if (!Array.isArray(actualArg)) {
        assert.ok(false, 'actual is not an array. ' + command + '/' + arg);
        return;
      }

      assert.is(expectedArg.length, actualArg.length,
              'Array length: ' + command + '/' + arg);
      for (var i = 0; i < expectedArg.length; i++) {
        assert.is(expectedArg[i], actualArg[i],
                'Member: "' + command + '/' + arg + '/' + i);
      }
    }
    else {
      assert.is(expectedArg, actualArg, 'Command: "' + command + '" arg: ' + arg);
    }
  });

  assert.is(environment, actualExec.context.environment, 'Environment');

  if (!hideExec) {
    assert.is(false, actualOutput.error, 'output error is false');
    assert.is(command, actualOutput.typed, 'command is typed');
    assert.ok(typeof actualOutput.canonical === 'string', 'canonical exists');

    assert.is(actualExec.args, actualOutput.args, 'actualExec.args is actualOutput.args');
  }
}


exports.testExec = function(options) {
  hideExec = options.hideExec;

  exec('tss', {});

  
  exec('tsv option1 10', { optionType: mockCommands.option1, optionValue: '10' });
  exec('tsv option2 10', { optionType: mockCommands.option2, optionValue: 10 });

  exec('tsr fred', { text: 'fred' });
  exec('tsr fred bloggs', { text: 'fred bloggs' });
  exec('tsr "fred bloggs"', { text: 'fred bloggs' });

  exec('tsb', { toggle: false });
  exec('tsb --toggle', { toggle: true });

  exec('tsu 10', { num: 10 });
  exec('tsu --num 10', { num: 10 });

  
  
  exec('tsj { 1 + 1 }', { javascript: '1 + 1' });

  var origDoc = nodetype.getDocument();
  nodetype.setDocument(mockDoc);
  exec('tse :root', { node: mockBody, nodes: skip, nodes2: skip });
  nodetype.setDocument(origDoc);

  exec('tsn dif fred', { text: 'fred' });
  exec('tsn exten fred', { text: 'fred' });
  exec('tsn extend fred', { text: 'fred' });

  exec('tselarr 1', { num: '1', arr: [ ] });
  exec('tselarr 1 a', { num: '1', arr: [ 'a' ] });
  exec('tselarr 1 a b', { num: '1', arr: [ 'a', 'b' ] });

  exec('tsm a 10 10', { abc: 'a', txt: '10', num: 10 });

  
  exec('tsg aaa', { solo: 'aaa', txt1: null, bool: false, txt2: 'd', num: 42 });
};

var mockBody = {
  style: {}
};

var mockDoc = {
  querySelectorAll: function(css) {
    if (css === ':root') {
      return {
        length: 1,
        item: function(i) {
          return mockBody;
        }
      };
    }
    else {
      return {
        length: 0,
        item: function() { return null; }
      };
    }
  }
};



