





















































let [ define, require ] = (function() {
  let tempScope = {};
  Components.utils.import("resource://gre/modules/devtools/Require.jsm", tempScope);
  return [ tempScope.define, tempScope.require ];
})();

let console = (function() {
  let tempScope = {};
  Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
  return console;
})();

registerCleanupFunction(function tearDown() {
  define = undefined;
  require = undefined;
  console = undefined;
});
















define('gclitest/index', ['require', 'exports', 'module' , 'gclitest/suite', 'gcli/settings', 'gcli/ui/display'], function(require, exports, module) {

  var examiner = require('gclitest/suite').examiner;
  var settings = require('gcli/settings');
  var Display = require('gcli/ui/display').Display;

  
  var fakeWindow = {
    isFake: true,
    document: { title: 'Fake DOM' }
  };
  fakeWindow.window = fakeWindow;
  examiner.defaultOptions = {
    window: fakeWindow,
    hideExec: true
  };

  
















  exports.run = function(options) {
    options = options || {};
    examiner.mergeDefaultOptions(options);

    examiner.reset();
    examiner.run(options);

    
    
    examiner.defaultOptions = {
      window: options.window,
      display: options.display,
      hideExec: options.hideExec
    };
  };

  








  exports.createDisplay = function(options) {
    options = options || {};
    if (options.settings != null) {
      settings.setDefaults(options.settings);
    }

    window.display = new Display(options);
    var requisition = window.display.requisition;

    
    window.setTimeout(function() {
      var options = {
        window: window,
        display: window.display,
        hideExec: true
      };
      exports.run(options);

      window.createDebugCheck = function() {
        require([ 'gclitest/helpers' ], function(helpers) {
          helpers.setup(options);
          console.log(helpers._createDebugCheck());
          helpers.shutdown(options);
        });
      };

      window.summaryJson = function() {
        var args = [ 'Requisition: ' ];
        var summary = display.requisition._summaryJson;
        Object.keys(summary).forEach(function(name) {
          args.push(' ' + name + '=');
          args.push(summary[name]);
        });
        console.log.apply(console, args);

        console.log('Focus: ' +
                    'tooltip=', display.focusManager._shouldShowTooltip(),
                    'output=', display.focusManager._shouldShowOutput());
      };

      document.addEventListener('keyup', function(ev) {
        if (ev.keyCode === 113 ) {
          window.createDebugCheck();
        }
        if (ev.keyCode === 115 ) {
          window.summaryJson();
        }
      }, true);

      window.testCommands = function() {
        require([ 'gclitest/mockCommands' ], function(mockCommands) {
          mockCommands.setup();
        });
      };
      window.testCommands();
    }, 10);

    return {
      



      exec: requisition.exec.bind(requisition),
      update: requisition.update.bind(requisition),
      destroy: window.display.destroy.bind(window.display)
    };
  };

});
















define('gclitest/suite', ['require', 'exports', 'module' , 'gcli/index', 'test/examiner', 'gclitest/testCanon', 'gclitest/testCli', 'gclitest/testCompletion', 'gclitest/testExec', 'gclitest/testFocus', 'gclitest/testHelp', 'gclitest/testHistory', 'gclitest/testInputter', 'gclitest/testIncomplete', 'gclitest/testIntro', 'gclitest/testJs', 'gclitest/testKeyboard', 'gclitest/testMenu', 'gclitest/testNode', 'gclitest/testPref', 'gclitest/testRequire', 'gclitest/testResource', 'gclitest/testScratchpad', 'gclitest/testSettings', 'gclitest/testSpell', 'gclitest/testSplit', 'gclitest/testTokenize', 'gclitest/testTooltip', 'gclitest/testTypes', 'gclitest/testUtil'], function(require, exports, module) {

  
  require('gcli/index');

  var examiner = require('test/examiner');

  
  
  
  examiner.addSuite('gclitest/testCanon', require('gclitest/testCanon'));
  examiner.addSuite('gclitest/testCli', require('gclitest/testCli'));
  examiner.addSuite('gclitest/testCompletion', require('gclitest/testCompletion'));
  examiner.addSuite('gclitest/testExec', require('gclitest/testExec'));
  examiner.addSuite('gclitest/testFocus', require('gclitest/testFocus'));
  examiner.addSuite('gclitest/testHelp', require('gclitest/testHelp'));
  examiner.addSuite('gclitest/testHistory', require('gclitest/testHistory'));
  examiner.addSuite('gclitest/testInputter', require('gclitest/testInputter'));
  examiner.addSuite('gclitest/testIncomplete', require('gclitest/testIncomplete'));
  examiner.addSuite('gclitest/testIntro', require('gclitest/testIntro'));
  examiner.addSuite('gclitest/testJs', require('gclitest/testJs'));
  examiner.addSuite('gclitest/testKeyboard', require('gclitest/testKeyboard'));
  examiner.addSuite('gclitest/testMenu', require('gclitest/testMenu'));
  examiner.addSuite('gclitest/testNode', require('gclitest/testNode'));
  examiner.addSuite('gclitest/testPref', require('gclitest/testPref'));
  examiner.addSuite('gclitest/testRequire', require('gclitest/testRequire'));
  examiner.addSuite('gclitest/testResource', require('gclitest/testResource'));
  examiner.addSuite('gclitest/testScratchpad', require('gclitest/testScratchpad'));
  examiner.addSuite('gclitest/testSettings', require('gclitest/testSettings'));
  examiner.addSuite('gclitest/testSpell', require('gclitest/testSpell'));
  examiner.addSuite('gclitest/testSplit', require('gclitest/testSplit'));
  examiner.addSuite('gclitest/testTokenize', require('gclitest/testTokenize'));
  examiner.addSuite('gclitest/testTooltip', require('gclitest/testTooltip'));
  examiner.addSuite('gclitest/testTypes', require('gclitest/testTypes'));
  examiner.addSuite('gclitest/testUtil', require('gclitest/testUtil'));

  exports.examiner = examiner;
});
















define('test/examiner', ['require', 'exports', 'module' , 'test/assert', 'test/status'], function(require, exports, module) {
var examiner = exports;

var assert = require('test/assert');
var stati = require('test/status').stati;




examiner.suites = {};




var delay = 10;





examiner.addSuite = function(name, suite) {
  examiner.suites[name] = new Suite(name, suite);
};






examiner.defaultOptions = {};





examiner.mergeDefaultOptions = function(options) {
  Object.keys(examiner.defaultOptions).forEach(function(name) {
    if (options[name] == null) {
      options[name] = examiner.defaultOptions[name];
    }
  });
};




examiner.run = function(options) {
  Object.keys(examiner.suites).forEach(function(suiteName) {
    var suite = examiner.suites[suiteName];
    suite.run(options);
  }.bind(this));

  if (options.detailedResultLog) {
    examiner.detailedResultLog();
  }
  else {
    console.log('Completed test suite');
  }

  return examiner.suites;
};




examiner.runAsync = function(options, callback) {
  this._runAsyncInternal(0, options, callback);
};




examiner._runAsyncInternal = function(i, options, callback) {
  if (i >= Object.keys(examiner.suites).length) {
    if (typeof callback === 'function') {
      callback();
    }
    return;
  }

  var suiteName = Object.keys(examiner.suites)[i];
  examiner.suites[suiteName].runAsync(options, function() {
    setTimeout(function() {
      examiner._runAsyncInternal(i + 1, options, callback);
    }.bind(this), delay);
  }.bind(this));
};




examiner.toRemote = function() {
  return {
    suites: Object.keys(examiner.suites).map(function(suiteName) {
      return examiner.suites[suiteName].toRemote();
    }.bind(this)),
    summary: {
      checks: this.checks,
      status: this.status
    }
  };
};




examiner.reset = function() {
  Object.keys(examiner.suites).forEach(function(suiteName) {
    examiner.suites[suiteName].reset();
  }, this);
};





Object.defineProperty(examiner, 'checks', {
  get: function() {
    return  Object.keys(examiner.suites).reduce(function(current, suiteName) {
      return current + examiner.suites[suiteName].checks;
    }.bind(this), 0);
  },
  enumerable: true
});





Object.defineProperty(examiner, 'status', {
  get: function() {
    return Object.keys(examiner.suites).reduce(function(status, suiteName) {
      var suiteStatus = examiner.suites[suiteName].status;
      return status.index > suiteStatus.index ? status : suiteStatus;
    }.bind(this), stati.notrun);
  },
  enumerable: true
});




examiner.detailedResultLog = function() {
  Object.keys(this.suites).forEach(function(suiteName) {
    var suite = examiner.suites[suiteName];

    console.log(suite.name + ': ' + suite.status.name + ' (funcs=' +
            Object.keys(suite.tests).length +
            ', checks=' + suite.checks + ')');

    Object.keys(suite.tests).forEach(function(testName) {
      var test = suite.tests[testName];
      if (test.status !== stati.pass || test.failures.length !== 0) {
        console.log('- ' + test.name + ': ' + test.status.name);
        test.failures.forEach(function(failure) {
          console.log('  - ' + failure.message);
          if (failure.params) {
            console.log('    - P1: ' + failure.p1);
            console.log('    - P2: ' + failure.p2);
          }
        }.bind(this));
      }
    }.bind(this));
  }.bind(this));

  console.log();
  console.log('Summary: ' + this.status.name + ' (' + this.checks + ' checks)');
};




function Suite(suiteName, suite) {
  this.name = suiteName.replace(/gclitest\//, '');
  this.suite = suite;

  this.tests = {};
  Object.keys(suite).forEach(function(testName) {
    if (testName !== 'setup' && testName !== 'shutdown') {
      var test = new Test(this, testName, suite[testName]);
      this.tests[testName] = test;
    }
  }.bind(this));
}




Suite.prototype.reset = function() {
  Object.keys(this.tests).forEach(function(testName) {
    this.tests[testName].reset();
  }, this);
};




Suite.prototype.run = function(options) {
  if (!this._setup(options)) {
    return;
  }

  Object.keys(this.tests).forEach(function(testName) {
    this.tests[testName].run(options);
  }, this);

  this._shutdown(options);
};




Suite.prototype.runAsync = function(options, callback) {
  if (!this._setup(options)) {
    if (typeof callback === 'function') {
      callback();
    }
    return;
  }

  this._runAsyncInternal(0, options, function() {
    this._shutdown(options);

    if (typeof callback === 'function') {
      callback();
    }
  }.bind(this));
};




Suite.prototype._runAsyncInternal = function(i, options, callback) {
  if (i >= Object.keys(this.tests).length) {
    if (typeof callback === 'function') {
      callback();
    }
    return;
  }

  var testName = Object.keys(this.tests)[i];
  this.tests[testName].runAsync(options, function() {
    setTimeout(function() {
      this._runAsyncInternal(i + 1, options, callback);
    }.bind(this), delay);
  }.bind(this));
};




Suite.prototype.toRemote = function() {
  return {
    name: this.name,
    tests: Object.keys(this.tests).map(function(testName) {
      return this.tests[testName].toRemote();
    }.bind(this))
  };
};





Object.defineProperty(Suite.prototype, 'checks', {
  get: function() {
    return Object.keys(this.tests).reduce(function(prevChecks, testName) {
      return prevChecks + this.tests[testName].checks;
    }.bind(this), 0);
  },
  enumerable: true
});





Object.defineProperty(Suite.prototype, 'status', {
  get: function() {
    return Object.keys(this.tests).reduce(function(prevStatus, testName) {
      var suiteStatus = this.tests[testName].status;
      return prevStatus.index > suiteStatus.index ? prevStatus : suiteStatus;
    }.bind(this), stati.notrun);
  },
  enumerable: true
});




Suite.prototype._setup = function(options) {
  if (typeof this.suite.setup !== 'function') {
    return true;
  }

  try {
    this.suite.setup(options);
    return true;
  }
  catch (ex) {
    this._logToAllTests('' + ex);
    console.error(ex);
    if (ex.stack) {
      console.error(ex.stack);
    }
    return false;
  }
};




Suite.prototype._shutdown = function(options) {
  if (typeof this.suite.shutdown !== 'function') {
    return true;
  }

  try {
    this.suite.shutdown(options);
    return true;
  }
  catch (ex) {
    this._logToAllTests('' + ex);
    console.error(ex);
    if (ex.stack) {
      console.error(ex.stack);
    }
    return false;
  }
};




Suite.prototype._logToAllTests = function(message) {
  var priorCurrentTest = assert.currentTest;
  Object.keys(this.tests).forEach(function(testName) {
    assert.currentTest = this.tests[testName];
    assert.ok(false, message);
  }.bind(this));
  assert.currentTest = priorCurrentTest;
};





function Test(suite, name, func) {
  this.suite = suite;
  this.name = name;
  this.func = func;
  this.title = name.replace(/^test/, '').replace(/([A-Z])/g, ' $1');

  this.failures = [];
  this.status = stati.notrun;
  this.checks = 0;
}




Test.prototype.reset = function() {
  this.failures = [];
  this.status = stati.notrun;
  this.checks = 0;
};




Test.prototype.run = function(options) {
  assert.currentTest = this;
  this.status = stati.executing;
  this.failures = [];
  this.checks = 0;

  try {
    this.func.apply(this.suite, [ options ]);
  }
  catch (ex) {
    assert.ok(false, '' + ex);
    console.error(ex.stack);
    if ((options.isNode || options.isFirefox) && ex.stack) {
      console.error(ex.stack);
    }
  }

  if (this.status === stati.executing) {
    this.status = stati.pass;
  }

  assert.currentTest = null;
};




Test.prototype.runAsync = function(options, callback) {
  setTimeout(function() {
    this.run(options);
    if (typeof callback === 'function') {
      callback();
    }
  }.bind(this), delay);
};




Test.prototype.toRemote = function() {
  return {
    name: this.name,
    title: this.title,
    status: this.status,
    failures: this.failures,
    checks: this.checks
  };
};


});
















define('test/assert', ['require', 'exports', 'module' ], function(require, exports, module) {

  exports.ok = ok;
  exports.is = is;
  exports.log = info;

});
















define('test/status', ['require', 'exports', 'module' ], function(require, exports, module) {

  




  exports.stati = {
    notrun: { index: 0, name: 'Skipped' },
    executing: { index: 1, name: 'Executing' },
    asynchronous: { index: 2, name: 'Waiting' },
    pass: { index: 3, name: 'Pass' },
    fail: { index: 4, name: 'Fail' }
  };

});
















define('gclitest/testCanon', ['require', 'exports', 'module' , 'gclitest/helpers', 'gcli/canon', 'test/assert'], function(require, exports, module) {

  var helpers = require('gclitest/helpers');
  var canon = require('gcli/canon');
  var test = require('test/assert');

  exports.setup = function(options) {
    helpers.setup(options);
  };

  exports.shutdown = function(options) {
    helpers.shutdown(options);
  };

  exports.testAddRemove = function(options) {
    var startCount = canon.getCommands().length;
    var events = 0;

    var canonChange = function(ev) {
      events++;
    };
    canon.onCanonChange.add(canonChange);

    canon.addCommand({
      name: 'testadd',
      exec: function() {
        return 1;
      }
    });

    test.is(canon.getCommands().length, startCount + 1, 'add command success');
    test.is(events, 1, 'add event');
    helpers.exec(options, {
      typed: 'testadd',
      outputMatch: /^1$/
    });

    canon.addCommand({
      name: 'testadd',
      exec: function() {
        return 2;
      }
    });

    test.is(canon.getCommands().length, startCount + 1, 'readd command success');
    test.is(events, 2, 'readd event');
    helpers.exec(options, {
      typed: 'testadd',
      outputMatch: /^2$/
    });

    canon.removeCommand('testadd');

    test.is(canon.getCommands().length, startCount, 'remove command success');
    test.is(events, 3, 'remove event');

    helpers.setInput('testadd');
    helpers.check({
      typed: 'testadd',
      status: 'ERROR'
    });

    canon.addCommand({
      name: 'testadd',
      exec: function() {
        return 3;
      }
    });

    test.is(canon.getCommands().length, startCount + 1, 'rereadd command success');
    test.is(events, 4, 'rereadd event');
    helpers.exec(options, {
      typed: 'testadd',
      outputMatch: /^3$/
    });

    canon.removeCommand({
      name: 'testadd'
    });

    test.is(canon.getCommands().length, startCount, 'reremove command success');
    test.is(events, 5, 'reremove event');

    helpers.setInput('testadd');
    helpers.check({
      typed: 'testadd',
      status: 'ERROR'
    });

    canon.removeCommand({ name: 'nonexistant' });
    test.is(canon.getCommands().length, startCount, 'nonexistant1 command success');
    test.is(events, 5, 'nonexistant1 event');

    canon.removeCommand('nonexistant');
    test.is(canon.getCommands().length, startCount, 'nonexistant2 command success');
    test.is(events, 5, 'nonexistant2 event');

    canon.onCanonChange.remove(canonChange);
  };

});
















define('gclitest/helpers', ['require', 'exports', 'module' , 'test/assert', 'gcli/util'], function(require, exports, module) {


var test = require('test/assert');
var util = require('gcli/util');

var helpers = exports;

helpers._display = undefined;

helpers.setup = function(options) {
  helpers._display = options.display;
};

helpers.shutdown = function(options) {
  helpers._display = undefined;
};




helpers._actual = {
  input: function() {
    return helpers._display.inputter.element.value;
  },

  hints: function() {
    var templateData = helpers._display.completer._getCompleterTemplateData();
    var actualHints = templateData.directTabText +
                      templateData.emptyParameters.join('') +
                      templateData.arrowTabText;
    return actualHints.replace(/\u00a0/g, ' ')
                      .replace(/\u21E5/, '->')
                      .replace(/ $/, '');
  },

  markup: function() {
    var cursor = helpers._display.inputter.element.selectionStart;
    var statusMarkup = helpers._display.requisition.getInputStatusMarkup(cursor);
    return statusMarkup.map(function(s) {
      return Array(s.string.length + 1).join(s.status.toString()[0]);
    }).join('');
  },

  cursor: function() {
    return helpers._display.inputter.element.selectionStart;
  },

  current: function() {
    return helpers._display.requisition.getAssignmentAt(helpers._actual.cursor()).param.name;
  },

  status: function() {
    return helpers._display.requisition.getStatus().toString();
  },

  outputState: function() {
    var outputData = helpers._display.focusManager._shouldShowOutput();
    return outputData.visible + ':' + outputData.reason;
  },

  tooltipState: function() {
    var tooltipData = helpers._display.focusManager._shouldShowTooltip();
    return tooltipData.visible + ':' + tooltipData.reason;
  }
};

helpers._directToString = [ 'boolean', 'undefined', 'number' ];

helpers._createDebugCheck = function() {
  var requisition = helpers._display.requisition;
  var command = requisition.commandAssignment.value;
  var input = helpers._actual.input();
  var padding = Array(input.length + 1).join(' ');

  var output = '';
  output += 'helpers.setInput(\'' + input + '\');\n';
  output += 'helpers.check({\n';
  output += '  input:  \'' + input + '\',\n';
  output += '  hints:  ' + padding + '\'' + helpers._actual.hints() + '\',\n';
  output += '  markup: \'' + helpers._actual.markup() + '\',\n';
  output += '  cursor: ' + helpers._actual.cursor() + ',\n';
  output += '  current: \'' + helpers._actual.current() + '\',\n';
  output += '  status: \'' + helpers._actual.status() + '\',\n';
  output += '  outputState: \'' + helpers._actual.outputState() + '\',\n';

  if (command) {
    output += '  tooltipState: \'' + helpers._actual.tooltipState() + '\',\n';
    output += '  args: {\n';
    output += '    command: { name: \'' + command.name + '\' },\n';

    requisition.getAssignments().forEach(function(assignment) {
      output += '    ' + assignment.param.name + ': { ';

      if (typeof assignment.value === 'string') {
        output += 'value: \'' + assignment.value + '\', ';
      }
      else if (helpers._directToString.indexOf(typeof assignment.value) !== -1) {
        output += 'value: ' + assignment.value + ', ';
      }
      else if (assignment.value === null) {
        output += 'value: ' + assignment.value + ', ';
      }
      else {
        output += '/*value:' + assignment.value + ',*/ ';
      }

      output += 'arg: \'' + assignment.arg + '\', ';
      output += 'status: \'' + assignment.getStatus().toString() + '\', ';
      output += 'message: \'' + assignment.getMessage() + '\'';
      output += ' },\n';
    });

    output += '  }\n';
  }
  else {
    output += '  tooltipState: \'' + helpers._actual.tooltipState() + '\'\n';
  }
  output += '});';

  return output;
};





helpers.setInput = function(typed, cursor) {
  helpers._display.inputter.setInput(typed);

  if (cursor) {
    helpers._display.inputter.setCursor({ start: cursor, end: cursor });
  }

  helpers._display.focusManager.onInputChange();
};




helpers.focusInput = function() {
  helpers._display.inputter.focus();
};




helpers.pressTab = function() {
  helpers.pressKey(9 );
};




helpers.pressReturn = function() {
  helpers.pressKey(13 );
};




helpers.pressKey = function(keyCode) {
  var fakeEvent = {
    keyCode: keyCode,
    preventDefault: function() { },
    timeStamp: new Date().getTime()
  };
  helpers._display.inputter.onKeyDown(fakeEvent);
  helpers._display.inputter.onKeyUp(fakeEvent);
};























helpers.check = function(checks) {
  if ('input' in checks) {
    test.is(helpers._actual.input(), checks.input, 'input');
  }

  if ('cursor' in checks) {
    test.is(helpers._actual.cursor(), checks.cursor, 'cursor');
  }

  if ('current' in checks) {
    test.is(helpers._actual.current(), checks.current, 'current');
  }

  if ('status' in checks) {
    test.is(helpers._actual.status(), checks.status, 'status');
  }

  if ('markup' in checks) {
    test.is(helpers._actual.markup(), checks.markup, 'markup');
  }

  if ('hints' in checks) {
    test.is(helpers._actual.hints(), checks.hints, 'hints');
  }

  if ('tooltipState' in checks) {
    test.is(helpers._actual.tooltipState(), checks.tooltipState, 'tooltipState');
  }

  if ('outputState' in checks) {
    test.is(helpers._actual.outputState(), checks.outputState, 'outputState');
  }

  if (checks.args != null) {
    var requisition = helpers._display.requisition;
    Object.keys(checks.args).forEach(function(paramName) {
      var check = checks.args[paramName];

      var assignment;
      if (paramName === 'command') {
        assignment = requisition.commandAssignment;
      }
      else {
        assignment = requisition.getAssignment(paramName);
      }

      if (assignment == null) {
        test.ok(false, 'Unknown arg: ' + paramName);
        return;
      }

      if ('value' in check) {
        test.is(assignment.value,
                check.value,
                'arg.' + paramName + '.value');
      }

      if ('name' in check) {
        test.is(assignment.value.name,
                check.name,
                'arg.' + paramName + '.name');
      }

      if ('type' in check) {
        test.is(assignment.arg.type,
                check.type,
                'arg.' + paramName + '.type');
      }

      if ('arg' in check) {
        test.is(assignment.arg.toString(),
                check.arg,
                'arg.' + paramName + '.arg');
      }

      if ('status' in check) {
        test.is(assignment.getStatus().toString(),
                check.status,
                'arg.' + paramName + '.status');
      }

      if ('message' in check) {
        test.is(assignment.getMessage(),
                check.message,
                'arg.' + paramName + '.message');
      }
    });
  }
};














helpers.exec = function(options, tests) {
  var requisition = options.display.requisition;
  var inputter = options.display.inputter;

  tests = tests || {};

  if (tests.typed) {
    inputter.setInput(tests.typed);
  }

  var typed = inputter.getInputState().typed;
  var output = requisition.exec({ hidden: true });

  test.is(typed, output.typed, 'output.command for: ' + typed);

  if (tests.completed !== false) {
    test.ok(output.completed, 'output.completed false for: ' + typed);
  }
  else {
    
    
    
  }

  if (tests.args != null) {
    test.is(Object.keys(tests.args).length, Object.keys(output.args).length,
            'arg count for ' + typed);

    Object.keys(output.args).forEach(function(arg) {
      var expectedArg = tests.args[arg];
      var actualArg = output.args[arg];

      if (Array.isArray(expectedArg)) {
        if (!Array.isArray(actualArg)) {
          test.ok(false, 'actual is not an array. ' + typed + '/' + arg);
          return;
        }

        test.is(expectedArg.length, actualArg.length,
                'array length: ' + typed + '/' + arg);
        for (var i = 0; i < expectedArg.length; i++) {
          test.is(expectedArg[i], actualArg[i],
                  'member: "' + typed + '/' + arg + '/' + i);
        }
      }
      else {
        test.is(expectedArg, actualArg, 'typed: "' + typed + '" arg: ' + arg);
      }
    });
  }

  if (!options.window.document.createElement) {
    test.log('skipping output tests (missing doc.createElement) for ' + typed);
    return;
  }

  var div = options.window.document.createElement('div');
  output.toDom(div);
  var displayed = div.textContent.trim();

  if (tests.outputMatch) {
    var doTest = function(match, against) {
      if (!match.test(against)) {
        test.ok(false, "html output for " + typed + " against " + match.source);
        console.log("Actual textContent");
        console.log(against);
      }
    }
    if (Array.isArray(tests.outputMatch)) {
      tests.outputMatch.forEach(function(match) {
        doTest(match, displayed);
      });
    }
    else {
      doTest(tests.outputMatch, displayed);
    }
  }

  if (tests.blankOutput != null) {
    if (!/^$/.test(displayed)) {
      test.ok(false, "html for " + typed + " (textContent sent to info)");
      console.log("Actual textContent");
      console.log(displayed);
    }
  }
};


});
















define('gclitest/testCli', ['require', 'exports', 'module' , 'gcli/cli', 'gcli/types', 'gclitest/mockCommands', 'test/assert'], function(require, exports, module) {


var Requisition = require('gcli/cli').Requisition;
var Status = require('gcli/types').Status;
var mockCommands = require('gclitest/mockCommands');

var test = require('test/assert');

exports.setup = function() {
  mockCommands.setup();
};

exports.shutdown = function() {
  mockCommands.shutdown();
};


var assign1;
var assign2;
var assignC;
var requ;
var debug = false;
var status;
var statuses;

function update(input) {
  if (!requ) {
    requ = new Requisition();
  }
  requ.update(input.typed);

  if (debug) {
    console.log('####### TEST: typed="' + input.typed +
        '" cur=' + input.cursor.start +
        ' cli=', requ);
  }

  status = requ.getStatus();
  assignC = requ.getAssignmentAt(input.cursor.start);
  statuses = requ.getInputStatusMarkup(input.cursor.start).map(function(s) {
    return Array(s.string.length + 1).join(s.status.toString()[0]);
  }).join('');

  if (requ.commandAssignment.value) {
    assign1 = requ.getAssignment(0);
    assign2 = requ.getAssignment(1);
  }
  else {
    assign1 = undefined;
    assign2 = undefined;
  }
}

function verifyPredictionsContains(name, predictions) {
  return predictions.every(function(prediction) {
    return name === prediction.name;
  }, this);
}


exports.testBlank = function() {
  update({ typed: '', cursor: { start: 0, end: 0 } });
  test.is(        '', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is(undefined, requ.commandAssignment.value);

  update({ typed: ' ', cursor: { start: 1, end: 1 } });
  test.is(        'V', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is(undefined, requ.commandAssignment.value);

  update({ typed: ' ', cursor: { start: 0, end: 0 } });
  test.is(        'V', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is(undefined, requ.commandAssignment.value);
};

exports.testIncompleteMultiMatch = function() {
  update({ typed: 't', cursor: { start: 1, end: 1 } });
  test.is(        'I', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.ok(assignC.getPredictions().length > 0);
  verifyPredictionsContains('tsv', assignC.getPredictions());
  verifyPredictionsContains('tsr', assignC.getPredictions());
  test.is(undefined, requ.commandAssignment.value);
};

exports.testIncompleteSingleMatch = function() {
  update({ typed: 'tselar', cursor: { start: 6, end: 6 } });
  test.is(        'IIIIII', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is(1, assignC.getPredictions().length);
  test.is('tselarr', assignC.getPredictions()[0].name);
  test.is(undefined, requ.commandAssignment.value);
};

exports.testTsv = function() {
  update({ typed: 'tsv', cursor: { start: 3, end: 3 } });
  test.is(        'VVV', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is('tsv', requ.commandAssignment.value.name);

  update({ typed: 'tsv ', cursor: { start: 4, end: 4 } });
  test.is(        'VVVV', statuses);
  test.is(Status.ERROR, status);
  test.is(0, assignC.paramIndex);
  test.is('tsv', requ.commandAssignment.value.name);

  update({ typed: 'tsv ', cursor: { start: 2, end: 2 } });
  test.is(        'VVVV', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is('tsv', requ.commandAssignment.value.name);

  update({ typed: 'tsv o', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVI', statuses);
  test.is(Status.ERROR, status);
  test.is(0, assignC.paramIndex);
  test.ok(assignC.getPredictions().length >= 2);
  test.is(mockCommands.option1, assignC.getPredictions()[0].value);
  test.is(mockCommands.option2, assignC.getPredictions()[1].value);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('o', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsv option', cursor: { start: 10, end: 10 } });
  test.is(        'VVVVIIIIII', statuses);
  test.is(Status.ERROR, status);
  test.is(0, assignC.paramIndex);
  test.ok(assignC.getPredictions().length >= 2);
  test.is(mockCommands.option1, assignC.getPredictions()[0].value);
  test.is(mockCommands.option2, assignC.getPredictions()[1].value);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsv option', cursor: { start: 1, end: 1 } });
  test.is(        'VVVVEEEEEE', statuses);
  test.is(Status.ERROR, status);
  test.is(-1, assignC.paramIndex);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsv option ', cursor: { start: 11, end: 11 } });
  test.is(        'VVVVEEEEEEV', statuses);
  test.is(Status.ERROR, status);
  test.is(1, assignC.paramIndex);
  test.is(0, assignC.getPredictions().length);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsv option1', cursor: { start: 11, end: 11 } });
  test.is(        'VVVVVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option1', assign1.arg.text);
  test.is(mockCommands.option1, assign1.value);
  test.is(0, assignC.paramIndex);

  update({ typed: 'tsv option1 ', cursor: { start: 12, end: 12 } });
  test.is(        'VVVVVVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option1', assign1.arg.text);
  test.is(mockCommands.option1, assign1.value);
  test.is(1, assignC.paramIndex);

  update({ typed: 'tsv option1 6', cursor: { start: 13, end: 13 } });
  test.is(        'VVVVVVVVVVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option1', assign1.arg.text);
  test.is(mockCommands.option1, assign1.value);
  test.is('6', assign2.arg.text);
  test.is('6', assign2.value);
  test.is('string', typeof assign2.value);
  test.is(1, assignC.paramIndex);

  update({ typed: 'tsv option2 6', cursor: { start: 13, end: 13 } });
  test.is(        'VVVVVVVVVVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is('option2', assign1.arg.text);
  test.is(mockCommands.option2, assign1.value);
  test.is('6', assign2.arg.text);
  test.is(6, assign2.value);
  test.is('number', typeof assign2.value);
  test.is(1, assignC.paramIndex);
};

exports.testInvalid = function() {
  update({ typed: 'zxjq', cursor: { start: 4, end: 4 } });
  test.is(        'EEEE', statuses);
  test.is('zxjq', requ.commandAssignment.arg.text);
  test.is(0, requ._unassigned.length);
  test.is(-1, assignC.paramIndex);

  update({ typed: 'zxjq ', cursor: { start: 5, end: 5 } });
  test.is(        'EEEEV', statuses);
  test.is('zxjq', requ.commandAssignment.arg.text);
  test.is(0, requ._unassigned.length);
  test.is(-1, assignC.paramIndex);

  update({ typed: 'zxjq one', cursor: { start: 8, end: 8 } });
  test.is(        'EEEEVEEE', statuses);
  test.is('zxjq', requ.commandAssignment.arg.text);
  test.is(1, requ._unassigned.length);
  test.is('one', requ._unassigned[0].arg.text);
};

exports.testSingleString = function() {
  update({ typed: 'tsr', cursor: { start: 3, end: 3 } });
  test.is(        'VVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsr', requ.commandAssignment.value.name);
  test.ok(assign1.arg.type === 'BlankArgument');
  test.is(undefined, assign1.value);
  test.is(undefined, assign2);

  update({ typed: 'tsr ', cursor: { start: 4, end: 4 } });
  test.is(        'VVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsr', requ.commandAssignment.value.name);
  test.ok(assign1.arg.type === 'BlankArgument');
  test.is(undefined, assign1.value);
  test.is(undefined, assign2);

  update({ typed: 'tsr h', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsr', requ.commandAssignment.value.name);
  test.is('h', assign1.arg.text);
  test.is('h', assign1.value);

  update({ typed: 'tsr "h h"', cursor: { start: 9, end: 9 } });
  test.is(        'VVVVVVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsr', requ.commandAssignment.value.name);
  test.is('h h', assign1.arg.text);
  test.is('h h', assign1.value);

  update({ typed: 'tsr h h h', cursor: { start: 9, end: 9 } });
  test.is(        'VVVVVVVVV', statuses);
  test.is('tsr', requ.commandAssignment.value.name);
  test.is('h h h', assign1.arg.text);
  test.is('h h h', assign1.value);
};

exports.testSingleNumber = function() {
  update({ typed: 'tsu', cursor: { start: 3, end: 3 } });
  test.is(        'VVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsu', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsu ', cursor: { start: 4, end: 4 } });
  test.is(        'VVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsu', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsu 1', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsu', requ.commandAssignment.value.name);
  test.is('1', assign1.arg.text);
  test.is(1, assign1.value);
  test.is('number', typeof assign1.value);

  update({ typed: 'tsu x', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVE', statuses);
  test.is(Status.ERROR, status);
  test.is('tsu', requ.commandAssignment.value.name);
  test.is('x', assign1.arg.text);
  test.is(undefined, assign1.value);
};

exports.testElement = function(options) {
  update({ typed: 'tse', cursor: { start: 3, end: 3 } });
  test.is(        'VVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tse', requ.commandAssignment.value.name);
  test.ok(assign1.arg.type === 'BlankArgument');
  test.is(undefined, assign1.value);

  if (!options.isNode) {
    update({ typed: 'tse :root', cursor: { start: 9, end: 9 } });
    test.is(        'VVVVVVVVV', statuses);
    test.is(Status.VALID, status);
    test.is('tse', requ.commandAssignment.value.name);
    test.is(':root', assign1.arg.text);
    if (!options.window.isFake) {
      test.is(options.window.document.documentElement, assign1.value);
    }

    if (!options.window.isFake) {
      var inputElement = options.window.document.getElementById('gcli-input');
      if (inputElement) {
        update({ typed: 'tse #gcli-input', cursor: { start: 15, end: 15 } });
        test.is(        'VVVVVVVVVVVVVVV', statuses);
        test.is(Status.VALID, status);
        test.is('tse', requ.commandAssignment.value.name);
        test.is('#gcli-input', assign1.arg.text);
        test.is(inputElement, assign1.value);
      }
      else {
        test.log('Skipping test that assumes gcli on the web');
      }
    }

    update({ typed: 'tse #gcli-nomatch', cursor: { start: 17, end: 17 } });
    
    
    
    test.is(        'VVVVIIIIIIIIIIIII', statuses);
    test.is(Status.ERROR, status);
    test.is('tse', requ.commandAssignment.value.name);
    test.is('#gcli-nomatch', assign1.arg.text);
    test.is(undefined, assign1.value);
  }
  else {
    test.log('Skipping :root test due to jsdom (from isNode)');
  }

  update({ typed: 'tse #', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVE', statuses);
  test.is(Status.ERROR, status);
  test.is('tse', requ.commandAssignment.value.name);
  test.is('#', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tse .', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVE', statuses);
  test.is(Status.ERROR, status);
  test.is('tse', requ.commandAssignment.value.name);
  test.is('.', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tse *', cursor: { start: 5, end: 5 } });
  test.is(        'VVVVE', statuses);
  test.is(Status.ERROR, status);
  test.is('tse', requ.commandAssignment.value.name);
  test.is('*', assign1.arg.text);
  test.is(undefined, assign1.value);
};

exports.testNestedCommand = function() {
  update({ typed: 'tsn', cursor: { start: 3, end: 3 } });
  test.is(        'III', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn', requ.commandAssignment.arg.text);
  test.is(undefined, assign1);

  update({ typed: 'tsn ', cursor: { start: 4, end: 4 } });
  test.is(        'IIIV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn', requ.commandAssignment.arg.text);
  test.is(undefined, assign1);

  update({ typed: 'tsn x', cursor: { start: 5, end: 5 } });
  
  
  test.is(Status.ERROR, status);
  test.is('tsn x', requ.commandAssignment.arg.text);
  test.is(undefined, assign1);

  update({ typed: 'tsn dif', cursor: { start: 7, end: 7 } });
  test.is(        'VVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn dif', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsn dif ', cursor: { start: 8, end: 8 } });
  test.is(        'VVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn dif', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsn dif x', cursor: { start: 9, end: 9 } });
  test.is(        'VVVVVVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsn dif', requ.commandAssignment.value.name);
  test.is('x', assign1.arg.text);
  test.is('x', assign1.value);

  update({ typed: 'tsn ext', cursor: { start: 7, end: 7 } });
  test.is(        'VVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn ext', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsn exte', cursor: { start: 8, end: 8 } });
  test.is(        'VVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn exte', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsn exten', cursor: { start: 9, end: 9 } });
  test.is(        'VVVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn exten', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'tsn extend', cursor: { start: 10, end: 10 } });
  test.is(        'VVVVVVVVVV', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn extend', requ.commandAssignment.value.name);
  test.is('', assign1.arg.text);
  test.is(undefined, assign1.value);

  update({ typed: 'ts ', cursor: { start: 3, end: 3 } });
  test.is(        'EEV', statuses);
  test.is(Status.ERROR, status);
  test.is('ts', requ.commandAssignment.arg.text);
  test.is(undefined, assign1);
};


exports.testDeeplyNested = function() {
  update({ typed: 'tsn deep down nested cmd', cursor: { start: 24, end: 24 } });
  test.is(        'VVVVVVVVVVVVVVVVVVVVVVVV', statuses);
  test.is(Status.VALID, status);
  test.is('tsn deep down nested cmd', requ.commandAssignment.value.name);
  test.is(undefined, assign1);

  update({ typed: 'tsn deep down nested', cursor: { start: 20, end: 20 } });
  test.is(        'IIIVIIIIVIIIIVIIIIII', statuses);
  test.is(Status.ERROR, status);
  test.is('tsn deep down nested', requ.commandAssignment.value.name);
  test.is(undefined, assign1);
};


});
















define('gclitest/mockCommands', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/util', 'gcli/types/selection', 'gcli/types/basic', 'gcli/types'], function(require, exports, module) {


var canon = require('gcli/canon');
var util = require('gcli/util');

var SelectionType = require('gcli/types/selection').SelectionType;
var DeferredType = require('gcli/types/basic').DeferredType;
var types = require('gcli/types');




exports.setup = function() {
  
  
  
  exports.option1.type = types.getType('string');
  exports.option2.type = types.getType('number');

  types.registerType(exports.optionType);
  types.registerType(exports.optionValue);

  canon.addCommand(exports.tsv);
  canon.addCommand(exports.tsr);
  canon.addCommand(exports.tso);
  canon.addCommand(exports.tse);
  canon.addCommand(exports.tsj);
  canon.addCommand(exports.tsb);
  canon.addCommand(exports.tss);
  canon.addCommand(exports.tsu);
  canon.addCommand(exports.tsn);
  canon.addCommand(exports.tsnDif);
  canon.addCommand(exports.tsnExt);
  canon.addCommand(exports.tsnExte);
  canon.addCommand(exports.tsnExten);
  canon.addCommand(exports.tsnExtend);
  canon.addCommand(exports.tsnDeep);
  canon.addCommand(exports.tsnDeepDown);
  canon.addCommand(exports.tsnDeepDownNested);
  canon.addCommand(exports.tsnDeepDownNestedCmd);
  canon.addCommand(exports.tselarr);
  canon.addCommand(exports.tsm);
  canon.addCommand(exports.tsg);
  canon.addCommand(exports.tshidden);
  canon.addCommand(exports.tscook);
  canon.addCommand(exports.tslong);
};

exports.shutdown = function() {
  canon.removeCommand(exports.tsv);
  canon.removeCommand(exports.tsr);
  canon.removeCommand(exports.tso);
  canon.removeCommand(exports.tse);
  canon.removeCommand(exports.tsj);
  canon.removeCommand(exports.tsb);
  canon.removeCommand(exports.tss);
  canon.removeCommand(exports.tsu);
  canon.removeCommand(exports.tsn);
  canon.removeCommand(exports.tsnDif);
  canon.removeCommand(exports.tsnExt);
  canon.removeCommand(exports.tsnExte);
  canon.removeCommand(exports.tsnExten);
  canon.removeCommand(exports.tsnExtend);
  canon.removeCommand(exports.tsnDeep);
  canon.removeCommand(exports.tsnDeepDown);
  canon.removeCommand(exports.tsnDeepDownNested);
  canon.removeCommand(exports.tsnDeepDownNestedCmd);
  canon.removeCommand(exports.tselarr);
  canon.removeCommand(exports.tsm);
  canon.removeCommand(exports.tsg);
  canon.removeCommand(exports.tshidden);
  canon.removeCommand(exports.tscook);
  canon.removeCommand(exports.tslong);

  types.deregisterType(exports.optionType);
  types.deregisterType(exports.optionValue);
};


exports.option1 = { type: types.getType('string') };
exports.option2 = { type: types.getType('number') };

var lastOption = undefined;

exports.optionType = new SelectionType({
  name: 'optionType',
  lookup: [
    { name: 'option1', value: exports.option1 },
    { name: 'option2', value: exports.option2 }
  ],
  noMatch: function() {
    lastOption = undefined;
  },
  stringify: function(option) {
    lastOption = option;
    return SelectionType.prototype.stringify.call(this, option);
  },
  parse: function(arg) {
    var conversion = SelectionType.prototype.parse.call(this, arg);
    lastOption = conversion.value;
    return conversion;
  }
});

exports.optionValue = new DeferredType({
  name: 'optionValue',
  defer: function() {
    if (lastOption && lastOption.type) {
      return lastOption.type;
    }
    else {
      return types.getType('blank');
    }
  }
});

exports.onCommandExec = util.createEvent('commands.onCommandExec');

function createExec(name) {
  return function(args, context) {
    var data = {
      command: exports[name],
      args: args,
      context: context
    };
    exports.onCommandExec(data);
    return data;
  };
}

exports.tsv = {
  name: 'tsv',
  params: [
    { name: 'optionType', type: 'optionType' },
    { name: 'optionValue', type: 'optionValue' }
  ],
  exec: createExec('tsv')
};

exports.tsr = {
  name: 'tsr',
  params: [ { name: 'text', type: 'string' } ],
  exec: createExec('tsr')
};

exports.tso = {
  name: 'tso',
  params: [ { name: 'text', type: 'string', defaultValue: null } ],
  exec: createExec('tso')
};

exports.tse = {
  name: 'tse',
  params: [
    { name: 'node', type: 'node' },
    {
      group: 'options',
      params: [
        { name: 'nodes', type: { name: 'nodelist' } },
        { name: 'nodes2', type: { name: 'nodelist', allowEmpty: true } }
      ]
    }
  ],
  exec: createExec('tse')
};

exports.tsj = {
  name: 'tsj',
  params: [ { name: 'javascript', type: 'javascript' } ],
  exec: createExec('tsj')
};

exports.tsb = {
  name: 'tsb',
  params: [ { name: 'toggle', type: 'boolean' } ],
  exec: createExec('tsb')
};

exports.tss = {
  name: 'tss',
  exec: createExec('tss')
};

exports.tsu = {
  name: 'tsu',
  params: [ { name: 'num', type: { name: 'number', max: 10, min: -5, step: 3 } } ],
  exec: createExec('tsu')
};

exports.tsn = {
  name: 'tsn'
};

exports.tsnDif = {
  name: 'tsn dif',
  description: 'tsn dif',
  params: [ { name: 'text', type: 'string', description: 'tsn dif text' } ],
  exec: createExec('tsnDif')
};

exports.tsnExt = {
  name: 'tsn ext',
  params: [ { name: 'text', type: 'string' } ],
  exec: createExec('tsnExt')
};

exports.tsnExte = {
  name: 'tsn exte',
  params: [ { name: 'text', type: 'string' } ],
  exec: createExec('')
};

exports.tsnExten = {
  name: 'tsn exten',
  params: [ { name: 'text', type: 'string' } ],
  exec: createExec('tsnExte')
};

exports.tsnExtend = {
  name: 'tsn extend',
  params: [ { name: 'text', type: 'string' } ],
  exec: createExec('tsnExtend')
};

exports.tsnDeep = {
  name: 'tsn deep',
};

exports.tsnDeepDown = {
  name: 'tsn deep down',
};

exports.tsnDeepDownNested = {
  name: 'tsn deep down nested',
};

exports.tsnDeepDownNestedCmd = {
  name: 'tsn deep down nested cmd',
  exec: createExec('tsnDeepDownNestedCmd')
};

exports.tshidden = {
  name: 'tshidden',
  hidden: true,
  params: [
    {
      group: 'Options',
      params: [
        {
          name: 'visible',
          type: 'string',
          defaultValue: null,
          description: 'visible'
        },
        {
          name: 'invisiblestring',
          type: 'string',
          description: 'invisiblestring',
          defaultValue: null,
          hidden: true
        },
        {
          name: 'invisibleboolean',
          type: 'boolean',
          description: 'invisibleboolean',
          hidden: true
        }
      ]
    }
  ],
  exec: createExec('tshidden')
};

exports.tselarr = {
  name: 'tselarr',
  params: [
    { name: 'num', type: { name: 'selection', data: [ '1', '2', '3' ] } },
    { name: 'arr', type: { name: 'array', subtype: 'string' } },
  ],
  exec: createExec('tselarr')
};

exports.tsm = {
  name: 'tsm',
  description: 'a 3-param test selection|string|number',
  params: [
    { name: 'abc', type: { name: 'selection', data: [ 'a', 'b', 'c' ] } },
    { name: 'txt', type: 'string' },
    { name: 'num', type: { name: 'number', max: 42, min: 0 } },
  ],
  exec: createExec('tsm')
};

exports.tsg = {
  name: 'tsg',
  description: 'a param group test',
  params: [
    {
      name: 'solo',
      type: { name: 'selection', data: [ 'aaa', 'bbb', 'ccc' ] },
      description: 'solo param'
    },
    {
      group: 'First',
      params: [
        {
          name: 'txt1',
          type: 'string',
          defaultValue: null,
          description: 'txt1 param'
        },
        {
          name: 'bool',
          type: 'boolean',
          description: 'bool param'
        }
      ]
    },
    {
      group: 'Second',
      params: [
        {
          name: 'txt2',
          type: 'string',
          defaultValue: 'd',
          description: 'txt2 param'
        },
        {
          name: 'num',
          type: { name: 'number', min: 40 },
          defaultValue: 42,
          description: 'num param'
        }
      ]
    }
  ],
  exec: createExec('tsg')
};

exports.tscook = {
  name: 'tscook',
  description: 'param group test to catch problems with cookie command',
  params: [
    {
      name: 'key',
      type: 'string',
      description: 'tscookKeyDesc'
    },
    {
      name: 'value',
      type: 'string',
      description: 'tscookValueDesc'
    },
    {
      group: 'tscookOptionsDesc',
      params: [
        {
          name: 'path',
          type: 'string',
          defaultValue: '/',
          description: 'tscookPathDesc'
        },
        {
          name: 'domain',
          type: 'string',
          defaultValue: null,
          description: 'tscookDomainDesc'
        },
        {
          name: 'secure',
          type: 'boolean',
          description: 'tscookSecureDesc'
        }
      ]
    }
  ],
  exec: createExec('tscook')
};

exports.tslong = {
  name: 'tslong',
  description: 'long param tests to catch problems with the jsb command',
  returnValue:'string',
  params: [
    {
      name: 'msg',
      type: 'string',
      description: 'msg Desc'
    },
    {
      group: "Options Desc",
      params: [
        {
          name: 'num',
          type: 'number',
          description: 'num Desc',
          defaultValue: 2
        },
        {
          name: 'sel',
          type: {
            name: 'selection',
            lookup: [
              { name: "space", value: " " },
              { name: "tab", value: "\t" }
            ]
          },
          description: 'sel Desc',
          defaultValue: ' ',
        },
        {
          name: 'bool',
          type: 'boolean',
          description: 'bool Desc'
        },
        {
          name: 'num2',
          type: 'number',
          description: 'num2 Desc',
          defaultValue: -1
        },
        {
          name: 'bool2',
          type: 'boolean',
          description: 'bool2 Desc'
        },
        {
          name: 'sel2',
          type: {
            name: 'selection',
            data: ['collapse', 'expand', 'end-expand', 'expand-strict']
          },
          description: 'sel2 Desc',
          defaultValue: "collapse"
        }
      ]
    }
  ],
  exec: createExec('tslong')
};


});
















define('gclitest/testCompletion', ['require', 'exports', 'module' , 'test/assert', 'gclitest/helpers', 'gclitest/mockCommands'], function(require, exports, module) {


var test = require('test/assert');
var helpers = require('gclitest/helpers');
var mockCommands = require('gclitest/mockCommands');


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
    hints: ' [options]'
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


});
















define('gclitest/testExec', ['require', 'exports', 'module' , 'gcli/cli', 'gcli/canon', 'gclitest/mockCommands', 'gcli/types/node', 'test/assert'], function(require, exports, module) {


var Requisition = require('gcli/cli').Requisition;
var canon = require('gcli/canon');
var mockCommands = require('gclitest/mockCommands');
var nodetype = require('gcli/types/node');

var test = require('test/assert');

var actualExec;
var actualOutput;
var hideExec = false;
var skip = 'skip';

exports.setup = function() {
  mockCommands.setup();
  mockCommands.onCommandExec.add(commandExeced);
  canon.commandOutputManager.onOutput.add(commandOutputed);
};

exports.shutdown = function() {
  mockCommands.shutdown();
  mockCommands.onCommandExec.remove(commandExeced);
  canon.commandOutputManager.onOutput.remove(commandOutputed);
};

function commandExeced(ev) {
  actualExec = ev;
}

function commandOutputed(ev) {
  actualOutput = ev.output;
}

function exec(command, expectedArgs) {
  var environment = {};

  var requisition = new Requisition(environment);
  var outputObject = requisition.exec({ typed: command, hidden: hideExec });

  test.is(command.indexOf(actualExec.command.name), 0, 'Command name: ' + command);

  test.is(command, outputObject.typed, 'outputObject.command for: ' + command);
  test.ok(outputObject.completed, 'outputObject.completed false for: ' + command);

  if (expectedArgs == null) {
    test.ok(false, 'expectedArgs == null for ' + command);
    return;
  }
  if (actualExec.args == null) {
    test.ok(false, 'actualExec.args == null for ' + command);
    return;
  }

  test.is(Object.keys(expectedArgs).length, Object.keys(actualExec.args).length,
          'Arg count: ' + command);
  Object.keys(expectedArgs).forEach(function(arg) {
    var expectedArg = expectedArgs[arg];
    var actualArg = actualExec.args[arg];

    if (expectedArg === skip) {
      return;
    }

    if (Array.isArray(expectedArg)) {
      if (!Array.isArray(actualArg)) {
        test.ok(false, 'actual is not an array. ' + command + '/' + arg);
        return;
      }

      test.is(expectedArg.length, actualArg.length,
              'Array length: ' + command + '/' + arg);
      for (var i = 0; i < expectedArg.length; i++) {
        test.is(expectedArg[i], actualArg[i],
                'Member: "' + command + '/' + arg + '/' + i);
      }
    }
    else {
      test.is(expectedArg, actualArg, 'Command: "' + command + '" arg: ' + arg);
    }
  });

  test.is(environment, actualExec.context.environment, 'Environment');

  if (!hideExec) {
    test.is(false, actualOutput.error, 'output error is false');
    test.is(command, actualOutput.typed, 'command is typed');
    test.ok(typeof actualOutput.canonical === 'string', 'canonical exists');

    test.is(actualExec.args, actualOutput.args, 'actualExec.args is actualOutput.args');
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


});






define('gclitest/testFocus', ['require', 'exports', 'module' , 'gclitest/helpers', 'gclitest/mockCommands'], function(require, exports, module) {


var helpers = require('gclitest/helpers');
var mockCommands = require('gclitest/mockCommands');

exports.setup = function(options) {
  mockCommands.setup();
  helpers.setup(options);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  helpers.shutdown(options);
};

exports.testBasic = function(options) {
  helpers.focusInput();
  helpers.exec(options, 'help');

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


});
















define('gclitest/testHelp', ['require', 'exports', 'module' , 'gclitest/helpers'], function(require, exports, module) {

  var helpers = require('gclitest/helpers');

  exports.setup = function(options) {
    helpers.setup(options);
  };

  exports.shutdown = function(options) {
    helpers.shutdown(options);
  };

  exports.testHelpStatus = function(options) {
    helpers.setInput('help');
    helpers.check({
      typed:  'help',
      hints:      ' [search]',
      markup: 'VVVV',
      status: 'VALID'
    });

    helpers.setInput('help ');
    helpers.check({
      typed:  'help ',
      hints:       '[search]',
      markup: 'VVVVV',
      status: 'VALID'
    });

    
    helpers.setInput('help');
    helpers.pressTab();
    helpers.check({
      typed:  'help ',
      hints:       '[search]',
      markup: 'VVVVV',
      status: 'VALID'
    });

    helpers.setInput('help foo');
    helpers.check({
      typed:  'help foo',
      markup: 'VVVVVVVV',
      status: 'VALID',
      hints:  ''
    });

    helpers.setInput('help foo bar');
    helpers.check({
      typed:  'help foo bar',
      markup: 'VVVVVVVVVVVV',
      status: 'VALID',
      hints:  ''
    });
  };

  exports.testHelpExec = function(options) {
    if (options.isFirefox) {
      helpers.exec(options, {
        typed: 'help',
        args: { search: null },
        outputMatch: [
          /Available Commands/,
          /Get help/
        ]
      });
    }
    else {
      helpers.exec(options, {
        typed: 'help',
        args: { search: null },
        outputMatch: [
          /Welcome to GCLI/,
          /Source \(Apache-2.0\)/,
          /Get help/
        ]
      });
    }

    helpers.exec(options, {
      typed: 'help nomatch',
      args: { search: 'nomatch' },
      outputMatch: /No commands starting with 'nomatch'$/
    });

    helpers.exec(options, {
      typed: 'help help',
      args: { search: 'help' },
      outputMatch: [
        /Synopsis:/,
        /Provide help either/,
        /\(string, optional\)/
      ]
    });

    helpers.exec(options, {
      typed: 'help a b',
      args: { search: 'a b' },
      outputMatch: /No commands starting with 'a b'$/
    });

    helpers.exec(options, {
      typed: 'help hel',
      args: { search: 'hel' },
      outputMatch: [
        /Commands starting with 'hel':/,
        /Get help on the available commands/
      ]
    });
  };

});
















define('gclitest/testHistory', ['require', 'exports', 'module' , 'test/assert', 'gcli/history'], function(require, exports, module) {

var test = require('test/assert');
var History = require('gcli/history').History;

exports.setup = function() {
};

exports.shutdown = function() {
};

exports.testSimpleHistory = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  test.is('bar', history.backward());
  test.is('foo', history.backward());

  
  history.add('quux');
  test.is('quux', history.backward());
  test.is('bar', history.backward());
  test.is('foo', history.backward());
};

exports.testBackwardsPastIndex = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  test.is('bar', history.backward());
  test.is('foo', history.backward());

  
  
  test.is('foo', history.backward());
};

exports.testForwardsPastIndex = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  test.is('bar', history.backward());
  test.is('foo', history.backward());

  
  test.is('bar', history.forward());

  
  test.is('', history.forward());

  
  test.is('', history.forward());
};

});
















define('gclitest/testInputter', ['require', 'exports', 'module' , 'gclitest/mockCommands', 'gcli/util', 'test/assert'], function(require, exports, module) {


var mockCommands = require('gclitest/mockCommands');
var KeyEvent = require('gcli/util').KeyEvent;

var test = require('test/assert');

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

  test.is(inputter.element.value, 'tss', 'inputter should do nothing on RETURN keyDown');
  test.is(latestEvent, undefined, 'no events this test');
  test.is(latestData, undefined, 'no data this test');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_RETURN
  });

  test.ok(latestEvent != null, 'events this test');
  test.is(latestData.command.name, 'tss', 'last command is tss');

  test.is(inputter.element.value, '', 'inputter should exec on RETURN keyUp');

  test.ok(focusManager._recentOutput, 'recent output happened');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_F1
  });

  test.ok(!focusManager._recentOutput, 'no recent output happened post F1');
  test.ok(focusManager._helpRequested, 'F1 = help');

  inputter.onKeyUp({
    keyCode: KeyEvent.DOM_VK_ESCAPE
  });

  test.ok(!focusManager._helpRequested, 'ESCAPE = anti help');

  latestOutput.onClose();
};


});






define('gclitest/testIncomplete', ['require', 'exports', 'module' , 'test/assert', 'gclitest/helpers', 'gclitest/mockCommands'], function(require, exports, module) {


var test = require('test/assert');
var helpers = require('gclitest/helpers');
var mockCommands = require('gclitest/mockCommands');


exports.setup = function(options) {
  mockCommands.setup();
  helpers.setup(options);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  helpers.shutdown(options);
};

exports.testBasic = function(options) {
  var requisition = options.display.requisition;

  helpers.setInput('tsu 2 extra');
  helpers.check({
    args: {
      num: { value: 2, type: 'Argument' }
    }
  });
  test.is(requisition._unassigned.length, 1, 'single unassigned: tsu 2 extra');
  test.is(requisition._unassigned[0].param.type.isIncompleteName, false,
          'unassigned.isIncompleteName: tsu 2 extra');

  helpers.setInput('tsu');
  helpers.check({
    args: {
      num: { value: undefined, type: 'BlankArgument' }
    }
  });

  helpers.setInput('tsg');
  helpers.check({
    args: {
      solo: { type: 'BlankArgument' },
      txt1: { type: 'BlankArgument' },
      bool: { type: 'BlankArgument' },
      txt2: { type: 'BlankArgument' },
      num: { type: 'BlankArgument' }
    }
  });
};

exports.testCompleted = function(options) {
  helpers.setInput('tsela');
  helpers.pressTab();
  helpers.check({
    args: {
      command: { name: 'tselarr', type: 'Argument' },
      num: { type: 'BlankArgument' },
      arr: { type: 'ArrayArgument' },
    }
  });

  helpers.setInput('tsn dif ');
  helpers.check({
    input:  'tsn dif ',
    hints:          '<text>',
    markup: 'VVVVVVVV',
    cursor: 8,
    status: 'ERROR',
    args: {
      command: { name: 'tsn dif', type: 'MergedArgument' },
      text: { type: 'BlankArgument', status: 'INCOMPLETE' }
    }
  });

  helpers.setInput('tsn di');
  helpers.pressTab();
  helpers.check({
    input:  'tsn dif ',
    hints:          '<text>',
    markup: 'VVVVVVVV',
    cursor: 8,
    status: 'ERROR',
    args: {
      command: { name: 'tsn dif', type: 'Argument' },
      text: { type: 'BlankArgument', status: 'INCOMPLETE' }
    }
  });

  
  

  helpers.setInput('tsg -');
  helpers.check({
    input:  'tsg -',
    hints:       '-txt1 <solo> [options]',
    markup: 'VVVVI',
    cursor: 5,
    status: 'ERROR',
    args: {
      solo: { value: undefined, status: 'INCOMPLETE' },
      txt1: { value: undefined, status: 'VALID' },
      bool: { value: false, status: 'VALID' },
      txt2: { value: undefined, status: 'VALID' },
      num: { value: undefined, status: 'VALID' }
    }
  });

  helpers.pressTab();
  helpers.check({
    input:  'tsg --txt1 ',
    hints:             '<string> <solo> [options]',
    markup: 'VVVVIIIIIIV',
    cursor: 11,
    status: 'ERROR',
    args: {
      solo: { value: undefined, status: 'INCOMPLETE' },
      txt1: { value: undefined, status: 'INCOMPLETE' },
      bool: { value: false, status: 'VALID' },
      txt2: { value: undefined, status: 'VALID' },
      num: { value: undefined, status: 'VALID' }
    }
  });

  helpers.setInput('tsg --txt1 fred');
  helpers.check({
    input:  'tsg --txt1 fred',
    hints:                 ' <solo> [options]',
    markup: 'VVVVVVVVVVVVVVV',
    status: 'ERROR',
    args: {
      solo: { value: undefined, status: 'INCOMPLETE' },
      txt1: { value: 'fred', status: 'VALID' },
      bool: { value: false, status: 'VALID' },
      txt2: { value: undefined, status: 'VALID' },
      num: { value: undefined, status: 'VALID' }
    }
  });

  helpers.setInput('tscook key value --path path --');
  helpers.check({
    input:  'tscook key value --path path --',
    hints:                                 'domain [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVVVII',
    status: 'ERROR',
    args: {
      key: { value: 'key', status: 'VALID' },
      value: { value: 'value', status: 'VALID' },
      path: { value: 'path', status: 'VALID' },
      domain: { value: undefined, status: 'VALID' },
      secure: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tscook key value --path path --domain domain --');
  helpers.check({
    input:  'tscook key value --path path --domain domain --',
    hints:                                                 'secure [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVII',
    status: 'ERROR',
    args: {
      key: { value: 'key', status: 'VALID' },
      value: { value: 'value', status: 'VALID' },
      path: { value: 'path', status: 'VALID' },
      domain: { value: 'domain', status: 'VALID' },
      secure: { value: false, status: 'VALID' }
    }
  });
};

exports.testCase = function(options) {
  helpers.setInput('tsg AA');
  helpers.check({
    input:  'tsg AA',
    hints:        ' [options] -> aaa',
    markup: 'VVVVII',
    status: 'ERROR',
    args: {
      solo: { value: undefined, text: 'AA', status: 'INCOMPLETE' },
      txt1: { value: undefined, status: 'VALID' },
      bool: { value: false, status: 'VALID' },
      txt2: { value: undefined, status: 'VALID' },
      num: { value: undefined, status: 'VALID' }
    }
  });
};

exports.testIncomplete = function(options) {
  var requisition = options.display.requisition;

  helpers.setInput('tsm a a -');
  helpers.check({
    args: {
      abc: { value: 'a', type: 'Argument' },
      txt: { value: 'a', type: 'Argument' },
      num: { value: undefined, arg: ' -', type: 'Argument', status: 'INCOMPLETE' }
    }
  });

  helpers.setInput('tsg -');
  helpers.check({
    args: {
      solo: { type: 'BlankArgument' },
      txt1: { type: 'BlankArgument' },
      bool: { type: 'BlankArgument' },
      txt2: { type: 'BlankArgument' },
      num: { type: 'BlankArgument' }
    }
  });
  test.is(requisition._unassigned[0], requisition.getAssignmentAt(5),
          'unassigned -');
  test.is(requisition._unassigned.length, 1, 'single unassigned - tsg -');
  test.is(requisition._unassigned[0].param.type.isIncompleteName, true,
          'unassigned.isIncompleteName: tsg -');
};

exports.testHidden = function(options) {
  helpers.setInput('tshidde');
  helpers.check({
    input:  'tshidde',
    markup: 'EEEEEEE',
    status: 'ERROR',
    hints:  '',
  });

  helpers.setInput('tshidden');
  helpers.check({
    input:  'tshidden',
    hints:          ' [options]',
    markup: 'VVVVVVVV',
    status: 'VALID',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --vis');
  helpers.check({
    input:  'tshidden --vis',
    hints:                'ible [options]',
    markup: 'VVVVVVVVVIIIII',
    status: 'ERROR',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --invisiblestrin');
  helpers.check({
    input:  'tshidden --invisiblestrin',
    hints:                           ' [options]',
    markup: 'VVVVVVVVVEEEEEEEEEEEEEEEE',
    status: 'ERROR',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --invisiblestring');
  helpers.check({
    input:  'tshidden --invisiblestring',
    hints:                            ' <string> [options]',
    markup: 'VVVVVVVVVIIIIIIIIIIIIIIIII',
    status: 'ERROR',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'INCOMPLETE' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --invisiblestring x');
  helpers.check({
    input:  'tshidden --invisiblestring x',
    hints:                              ' [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV',
    status: 'VALID',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: 'x', status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --invisibleboolea');
  helpers.check({
    input:  'tshidden --invisibleboolea',
    hints:                            ' [options]',
    markup: 'VVVVVVVVVEEEEEEEEEEEEEEEEE',
    status: 'ERROR',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --invisibleboolean');
  helpers.check({
    input:  'tshidden --invisibleboolean',
    hints:                             ' [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVV',
    status: 'VALID',
    args: {
      visible: { value: undefined, status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: true, status: 'VALID' }
    }
  });

  helpers.setInput('tshidden --visible xxx');
  helpers.check({
    input:  'tshidden --visible xxx',
    markup: 'VVVVVVVVVVVVVVVVVVVVVV',
    status: 'VALID',
    hints:  '',
    args: {
      visible: { value: 'xxx', status: 'VALID' },
      invisiblestring: { value: undefined, status: 'VALID' },
      invisibleboolean: { value: false, status: 'VALID' }
    }
  });
};

});
















define('gclitest/testIntro', ['require', 'exports', 'module' , 'gclitest/helpers', 'test/assert'], function(require, exports, module) {

  var helpers = require('gclitest/helpers');
  var test = require('test/assert');

  exports.setup = function(options) {
    helpers.setup(options);
  };

  exports.shutdown = function(options) {
    helpers.shutdown(options);
  };

  exports.testIntroStatus = function(options) {
    if (options.isFirefox) {
      test.log('Skipping testIntroStatus in Firefox.');
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
    if (options.isFirefox) {
      test.log('Skipping testIntroExec in Firefox.');
      return;
    }

    helpers.exec(options, {
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

});
















define('gclitest/testJs', ['require', 'exports', 'module' , 'gcli/cli', 'gcli/types', 'gcli/types/javascript', 'gcli/canon', 'test/assert'], function(require, exports, module) {


var Requisition = require('gcli/cli').Requisition;
var Status = require('gcli/types').Status;
var javascript = require('gcli/types/javascript');
var canon = require('gcli/canon');

var test = require('test/assert');

var debug = false;
var requ;

var assign;
var status;
var statuses;
var tempWindow;


exports.setup = function(options) {
  tempWindow = javascript.getGlobalObject();
  javascript.setGlobalObject(options.window);

  Object.defineProperty(options.window, 'donteval', {
    get: function() {
      test.ok(false, 'donteval should not be used');
      return { cant: '', touch: '', 'this': '' };
    },
    enumerable: true,
    configurable : true
  });
};

exports.shutdown = function(options) {
  delete options.window.donteval;

  javascript.setGlobalObject(tempWindow);
  tempWindow = undefined;
};

function input(typed) {
  if (!requ) {
    requ = new Requisition();
  }
  var cursor = { start: typed.length, end: typed.length };
  requ.update(typed);

  if (debug) {
    console.log('####### TEST: typed="' + typed +
        '" cur=' + cursor.start +
        ' cli=', requ);
  }

  status = requ.getStatus();
  statuses = requ.getInputStatusMarkup(cursor.start).map(function(s) {
    return Array(s.string.length + 1).join(s.status.toString()[0]);
  }).join('');

  if (requ.commandAssignment.value) {
    assign = requ.getAssignment(0);
  }
  else {
    assign = undefined;
  }
}

function predictionsHas(name) {
  return assign.getPredictions().some(function(prediction) {
    return name === prediction.name;
  }, this);
}

function check(expStatuses, expStatus, expAssign, expPredict) {
  test.is('{', requ.commandAssignment.value.name, 'is exec');

  test.is(expStatuses, statuses, 'unexpected status markup');
  test.is(expStatus.toString(), status.toString(), 'unexpected status');
  test.is(expAssign, assign.value, 'unexpected assignment');

  if (expPredict != null) {
    var contains;
    if (Array.isArray(expPredict)) {
      expPredict.forEach(function(p) {
        contains = predictionsHas(p);
        test.ok(contains, 'missing prediction ' + p);
      });
    }
    else if (typeof expPredict === 'number') {
      contains = true;
      test.is(assign.getPredictions().length, expPredict, 'prediction count');
      if (assign.getPredictions().length !== expPredict) {
        assign.getPredictions().forEach(function(prediction) {
          test.log('actual prediction: ', prediction);
        });
      }
    }
    else {
      contains = predictionsHas(expPredict);
      test.ok(contains, 'missing prediction ' + expPredict);
    }

    if (!contains) {
      test.log('Predictions: ' + assign.getPredictions().map(function(p) {
        return p.name;
      }).join(', '));
    }
  }
}

exports.testBasic = function(options) {
  if (!canon.getCommand('{')) {
    test.log('Skipping exec tests because { is not registered');
    return;
  }

  input('{');
  check('V', Status.ERROR, undefined);

  input('{ ');
  check('VV', Status.ERROR, undefined);

  input('{ w');
  check('VVI', Status.ERROR, 'w', 'window');

  input('{ windo');
  check('VVIIIII', Status.ERROR, 'windo', 'window');

  input('{ window');
  check('VVVVVVVV', Status.VALID, 'window');

  input('{ window.d');
  check('VVIIIIIIII', Status.ERROR, 'window.d', 'window.document');

  input('{ window.document.title');
  check('VVVVVVVVVVVVVVVVVVVVVVV', Status.VALID, 'window.document.title', 0);

  input('{ d');
  check('VVI', Status.ERROR, 'd', 'document');

  input('{ document.title');
  check('VVVVVVVVVVVVVVVV', Status.VALID, 'document.title', 0);

  test.ok('donteval' in options.window, 'donteval exists');

  input('{ don');
  check('VVIII', Status.ERROR, 'don', 'donteval');

  input('{ donteval');
  check('VVVVVVVVVV', Status.VALID, 'donteval', 0);

  








  input('{ donteval.cant');
  check('VVVVVVVVVVVVVVV', Status.VALID, 'donteval.cant', 0);

  input('{ donteval.xxx');
  check('VVVVVVVVVVVVVV', Status.VALID, 'donteval.xxx', 0);
};


});
















define('gclitest/testKeyboard', ['require', 'exports', 'module' , 'gcli/cli', 'gcli/canon', 'gclitest/mockCommands', 'gcli/types/javascript', 'test/assert'], function(require, exports, module) {


var Requisition = require('gcli/cli').Requisition;
var canon = require('gcli/canon');
var mockCommands = require('gclitest/mockCommands');
var javascript = require('gcli/types/javascript');

var test = require('test/assert');

var tempWindow;
var inputter;

exports.setup = function(options) {
  tempWindow = javascript.getGlobalObject();
  javascript.setGlobalObject(options.window);

  if (options.display) {
    inputter = options.display.inputter;
  }

  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();

  inputter = undefined;
  javascript.setGlobalObject(tempWindow);
  tempWindow = undefined;
};

var COMPLETES_TO = 'complete';
var KEY_UPS_TO = 'keyup';
var KEY_DOWNS_TO = 'keydown';

function check(initial, action, after, choice, cursor, expectedCursor) {
  var requisition;
  if (inputter) {
    requisition = inputter.requisition;
    inputter.setInput(initial);
  }
  else {
    requisition = new Requisition();
    requisition.update(initial);
  }

  if (cursor == null) {
    cursor = initial.length;
  }
  var assignment = requisition.getAssignmentAt(cursor);
  switch (action) {
    case COMPLETES_TO:
      requisition.complete({ start: cursor, end: cursor }, choice);
      break;

    case KEY_UPS_TO:
      requisition.increment(assignment);
      break;

    case KEY_DOWNS_TO:
      requisition.decrement(assignment);
      break;
  }

  test.is(after, requisition.toString(),
          initial + ' + ' + action + ' -> ' + after);

  if (expectedCursor != null) {
    if (inputter) {
      test.is(expectedCursor, inputter.getInputState().cursor.start,
              'Ending cursor position for \'' + initial + '\'');
    }
  }
}

exports.testComplete = function(options) {
  if (!inputter) {
    test.log('Missing display, reduced checks');
  }

  check('tsela', COMPLETES_TO, 'tselarr ', 0);
  check('tsn di', COMPLETES_TO, 'tsn dif ', 0);
  check('tsg a', COMPLETES_TO, 'tsg aaa ', 0);

  check('tsn e', COMPLETES_TO, 'tsn extend ', -5);
  check('tsn e', COMPLETES_TO, 'tsn ext ', -4);
  check('tsn e', COMPLETES_TO, 'tsn exte ', -3);
  check('tsn e', COMPLETES_TO, 'tsn exten ', -2);
  check('tsn e', COMPLETES_TO, 'tsn extend ', -1);
  check('tsn e', COMPLETES_TO, 'tsn ext ', 0);
  check('tsn e', COMPLETES_TO, 'tsn exte ', 1);
  check('tsn e', COMPLETES_TO, 'tsn exten ', 2);
  check('tsn e', COMPLETES_TO, 'tsn extend ', 3);
  check('tsn e', COMPLETES_TO, 'tsn ext ', 4);
  check('tsn e', COMPLETES_TO, 'tsn exte ', 5);
  check('tsn e', COMPLETES_TO, 'tsn exten ', 6);
  check('tsn e', COMPLETES_TO, 'tsn extend ', 7);
  check('tsn e', COMPLETES_TO, 'tsn ext ', 8);

  if (!canon.getCommand('{')) {
    test.log('Skipping exec tests because { is not registered');
  }
  else {
    check('{ wind', COMPLETES_TO, '{ window', 0);
    check('{ window.docum', COMPLETES_TO, '{ window.document', 0);

    
    if (!options.isNode) {
      check('{ window.document.titl', COMPLETES_TO, '{ window.document.title ', 0);
    }
    else {
      test.log('Running under Node. Skipping tests due to bug 717228.');
    }
  }
};

exports.testInternalComplete = function(options) {
  
  
};

exports.testIncrDecr = function() {
  check('tsu -70', KEY_UPS_TO, 'tsu -5');
  check('tsu -7', KEY_UPS_TO, 'tsu -5');
  check('tsu -6', KEY_UPS_TO, 'tsu -5');
  check('tsu -5', KEY_UPS_TO, 'tsu -3');
  check('tsu -4', KEY_UPS_TO, 'tsu -3');
  check('tsu -3', KEY_UPS_TO, 'tsu 0');
  check('tsu -2', KEY_UPS_TO, 'tsu 0');
  check('tsu -1', KEY_UPS_TO, 'tsu 0');
  check('tsu 0', KEY_UPS_TO, 'tsu 3');
  check('tsu 1', KEY_UPS_TO, 'tsu 3');
  check('tsu 2', KEY_UPS_TO, 'tsu 3');
  check('tsu 3', KEY_UPS_TO, 'tsu 6');
  check('tsu 4', KEY_UPS_TO, 'tsu 6');
  check('tsu 5', KEY_UPS_TO, 'tsu 6');
  check('tsu 6', KEY_UPS_TO, 'tsu 9');
  check('tsu 7', KEY_UPS_TO, 'tsu 9');
  check('tsu 8', KEY_UPS_TO, 'tsu 9');
  check('tsu 9', KEY_UPS_TO, 'tsu 10');
  check('tsu 10', KEY_UPS_TO, 'tsu 10');
  check('tsu 100', KEY_UPS_TO, 'tsu -5');

  check('tsu -70', KEY_DOWNS_TO, 'tsu 10');
  check('tsu -7', KEY_DOWNS_TO, 'tsu 10');
  check('tsu -6', KEY_DOWNS_TO, 'tsu 10');
  check('tsu -5', KEY_DOWNS_TO, 'tsu -5');
  check('tsu -4', KEY_DOWNS_TO, 'tsu -5');
  check('tsu -3', KEY_DOWNS_TO, 'tsu -5');
  check('tsu -2', KEY_DOWNS_TO, 'tsu -3');
  check('tsu -1', KEY_DOWNS_TO, 'tsu -3');
  check('tsu 0', KEY_DOWNS_TO, 'tsu -3');
  check('tsu 1', KEY_DOWNS_TO, 'tsu 0');
  check('tsu 2', KEY_DOWNS_TO, 'tsu 0');
  check('tsu 3', KEY_DOWNS_TO, 'tsu 0');
  check('tsu 4', KEY_DOWNS_TO, 'tsu 3');
  check('tsu 5', KEY_DOWNS_TO, 'tsu 3');
  check('tsu 6', KEY_DOWNS_TO, 'tsu 3');
  check('tsu 7', KEY_DOWNS_TO, 'tsu 6');
  check('tsu 8', KEY_DOWNS_TO, 'tsu 6');
  check('tsu 9', KEY_DOWNS_TO, 'tsu 6');
  check('tsu 10', KEY_DOWNS_TO, 'tsu 9');
  check('tsu 100', KEY_DOWNS_TO, 'tsu 10');

  
  
  check('tselarr 1', KEY_DOWNS_TO, 'tselarr 2');
  check('tselarr 2', KEY_DOWNS_TO, 'tselarr 3');
  check('tselarr 3', KEY_DOWNS_TO, 'tselarr 1');

  check('tselarr 3', KEY_UPS_TO, 'tselarr 2');
};

});






define('gclitest/testMenu', ['require', 'exports', 'module' , 'gclitest/helpers', 'gclitest/mockCommands'], function(require, exports, module) {


var helpers = require('gclitest/helpers');
var mockCommands = require('gclitest/mockCommands');


exports.setup = function(options) {
  mockCommands.setup();
  helpers.setup(options);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  helpers.shutdown(options);
};

exports.testOptions = function(options) {
  helpers.setInput('tslong');
  helpers.check({
    input:  'tslong',
    markup: 'VVVVVV',
    status: 'ERROR',
    hints: ' <msg> [options]',
    args: {
      msg: { value: undefined, status: 'INCOMPLETE' },
      num: { value: undefined, status: 'VALID' },
      sel: { value: undefined, status: 'VALID' },
      bool: { value: false, status: 'VALID' },
      bool2: { value: false, status: 'VALID' },
      sel2: { value: undefined, status: 'VALID' },
      num2: { value: undefined, status: 'VALID' }
    }
  });
};


});







define('gclitest/testNode', ['require', 'exports', 'module' , 'test/assert', 'gclitest/helpers', 'gclitest/mockCommands'], function(require, exports, module) {


var test = require('test/assert');
var helpers = require('gclitest/helpers');
var mockCommands = require('gclitest/mockCommands');


exports.setup = function(options) {
  mockCommands.setup();
  helpers.setup(options);
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
  helpers.shutdown(options);
};

exports.testNode = function(options) {
  var requisition = options.display.requisition;

  helpers.setInput('tse ');
  helpers.check({
    input:  'tse ',
    hints:      '<node> [options]',
    markup: 'VVVV',
    cursor: 4,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: { status: 'INCOMPLETE', message: '' },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse :');
  helpers.check({
    input:  'tse :',
    hints:       ' [options]',
    markup: 'VVVVE',
    cursor: 5,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: {
        arg: ' :',
        status: 'ERROR',
        message: 'Syntax error in CSS query'
      },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse :root');
  helpers.check({
    input:  'tse :root',
    hints:           ' [options]',
    markup: 'VVVVVVVVV',
    cursor: 9,
    current: 'node',
    status: 'VALID',
    args: {
      command: { name: 'tse' },
      node: { arg: ' :root', status: 'VALID' },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse :root ');
  helpers.check({
    input:  'tse :root ',
    hints:            '[options]',
    markup: 'VVVVVVVVVV',
    cursor: 10,
    current: 'node',
    status: 'VALID',
    args: {
      command: { name: 'tse' },
      node: { arg: ' :root ', status: 'VALID' },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });
  test.is(requisition.getAssignment('node').value.tagName,
          'HTML',
          'root id');

  helpers.setInput('tse #gcli-nomatch');
  helpers.check({
    input:  'tse #gcli-nomatch',
    hints:                   ' [options]',
    markup: 'VVVVIIIIIIIIIIIII',
    cursor: 17,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: {
        value: undefined,
        arg: ' #gcli-nomatch',
        status: 'INCOMPLETE',
        message: 'No matches'
      },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse #');
  helpers.check({
    input:  'tse #',
    hints:       ' [options]',
    markup: 'VVVVE',
    cursor: 5,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: {
        value: undefined,
        arg: ' #',
        status: 'ERROR',
        message: 'Syntax error in CSS query'
      },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse .');
  helpers.check({
    input:  'tse .',
    hints:       ' [options]',
    markup: 'VVVVE',
    cursor: 5,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: {
        value: undefined,
        arg: ' .',
        status: 'ERROR',
        message: 'Syntax error in CSS query'
      },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });

  helpers.setInput('tse *');
  helpers.check({
    input:  'tse *',
    hints:       ' [options]',
    markup: 'VVVVE',
    cursor: 5,
    current: 'node',
    status: 'ERROR',
    args: {
      command: { name: 'tse' },
      node: {
        value: undefined,
        arg: ' *',
        status: 'ERROR',
        
      },
      nodes: { status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });
};

exports.testNodes = function(options) {
  var requisition = options.display.requisition;

  helpers.setInput('tse :root --nodes *');
  helpers.check({
    input:  'tse :root --nodes *',
    hints:                       ' [options]',
    markup: 'VVVVVVVVVVVVVVVVVVV',
    current: 'nodes',
    status: 'VALID',
    args: {
      command: { name: 'tse' },
      node: { arg: ' :root', status: 'VALID' },
      nodes: { arg: ' --nodes *', status: 'VALID' },
      nodes2: { status: 'VALID' }
    }
  });
  test.is(requisition.getAssignment('node').value.tagName,
          'HTML',
          '#gcli-input id');

  helpers.setInput('tse :root --nodes2 div');
  helpers.check({
    input:  'tse :root --nodes2 div',
    hints:                       ' [options]',
    markup: 'VVVVVVVVVVVVVVVVVVVVVV',
    cursor: 22,
    current: 'nodes2',
    status: 'VALID',
    args: {
      command: { name: 'tse' },
      node: { arg: ' :root', status: 'VALID' },
      nodes: { status: 'VALID' },
      nodes2: { arg: ' --nodes2 div', status: 'VALID' }
    }
  });
  test.is(requisition.getAssignment('node').value.tagName,
          'HTML',
          'root id');

  helpers.setInput('tse --nodes ffff');
  helpers.check({
    input:  'tse --nodes ffff',
    hints:                  ' <node> [options]',
    markup: 'VVVVIIIIIIIVIIII',
    cursor: 16,
    current: 'nodes',
    status: 'ERROR',
    outputState: 'false:default',
    tooltipState: 'true:isError',
    args: {
      command: { name: 'tse' },
      node: { value: undefined, arg: '', status: 'INCOMPLETE', message: '' },
      nodes: { value: undefined, arg: ' --nodes ffff', status: 'INCOMPLETE', message: 'No matches' },
      nodes2: { arg: '', status: 'VALID', message: '' },
    }
  });
  





  helpers.setInput('tse --nodes2 ffff');
  helpers.check({
    input:  'tse --nodes2 ffff',
    hints:                   ' <node> [options]',
    markup: 'VVVVVVVVVVVVVVVVV',
    cursor: 17,
    current: 'nodes2',
    status: 'ERROR',
    outputState: 'false:default',
    tooltipState: 'false:default',
    args: {
      command: { name: 'tse' },
      node: { value: undefined, arg: '', status: 'INCOMPLETE', message: '' },
      nodes: { arg: '', status: 'VALID', message: '' },
      nodes2: { arg: ' --nodes2 ffff', status: 'VALID', message: '' },
    }
  });
  







};


});
















define('gclitest/testPref', ['require', 'exports', 'module' , 'gcli/commands/pref', 'gclitest/helpers', 'gclitest/mockSettings', 'test/assert'], function(require, exports, module) {


var pref = require('gcli/commands/pref');
var helpers = require('gclitest/helpers');
var mockSettings = require('gclitest/mockSettings');
var test = require('test/assert');


exports.setup = function(options) {
  helpers.setup(options);

  if (!options.isFirefox) {
    mockSettings.setup();
  }
  else {
    test.log('Skipping testPref in Firefox.');
  }
};

exports.shutdown = function(options) {
  helpers.shutdown(options);

  if (!options.isFirefox) {
    mockSettings.shutdown();
  }
};

exports.testPrefShowStatus = function(options) {
  if (options.isFirefox) {
    test.log('Skipping testPrefShowStatus in Firefox.');
    return;
  }

  helpers.setInput('pref s');
  helpers.check({
    typed:  'pref s',
    hints:        'et',
    markup: 'IIIIVI',
    status: 'ERROR'
  });

  helpers.setInput('pref show');
  helpers.check({
    typed:  'pref show',
    hints:           ' <setting>',
    markup: 'VVVVVVVVV',
    status: 'ERROR'
  });

  helpers.setInput('pref show ');
  helpers.check({
    typed:  'pref show ',
    hints:            'allowSet',
    markup: 'VVVVVVVVVV',
    status: 'ERROR'
  });

  helpers.setInput('pref show tempTBo');
  helpers.check({
    typed:  'pref show tempTBo',
    hints:                   'ol',
    markup: 'VVVVVVVVVVIIIIIII',
    status: 'ERROR'
  });

  helpers.setInput('pref show tempTBool');
  helpers.check({
    typed:  'pref show tempTBool',
    markup: 'VVVVVVVVVVVVVVVVVVV',
    status: 'VALID',
    hints:  ''
  });

  helpers.setInput('pref show tempTBool 4');
  helpers.check({
    typed:  'pref show tempTBool 4',
    markup: 'VVVVVVVVVVVVVVVVVVVVE',
    status: 'ERROR',
    hints:  ''
  });

  helpers.setInput('pref show tempNumber 4');
  helpers.check({
    typed:  'pref show tempNumber 4',
    markup: 'VVVVVVVVVVVVVVVVVVVVVE',
    status: 'ERROR',
    hints:  ''
  });
};

exports.testPrefSetStatus = function(options) {
  if (options.isFirefox) {
    test.log('Skipping testPrefSetStatus in Firefox.');
    return;
  }

  helpers.setInput('pref s');
  helpers.check({
    typed:  'pref s',
    hints:        'et',
    markup: 'IIIIVI',
    status: 'ERROR',
  });

  helpers.setInput('pref set');
  helpers.check({
    typed:  'pref set',
    hints:          ' <setting> <value>',
    markup: 'VVVVVVVV',
    status: 'ERROR'
  });

  helpers.setInput('pref xxx');
  helpers.check({
    typed:  'pref xxx',
    markup: 'EEEEVEEE',
    status: 'ERROR'
  });

  helpers.setInput('pref set ');
  helpers.check({
    typed:  'pref set ',
    hints:           'allowSet <value>',
    markup: 'VVVVVVVVV',
    status: 'ERROR'
  });

  helpers.setInput('pref set tempTBo');
  helpers.check({
    typed:  'pref set tempTBo',
    hints:                  'ol <value>',
    markup: 'VVVVVVVVVIIIIIII',
    status: 'ERROR'
  });

  helpers.setInput('pref set tempTBool 4');
  helpers.check({
    typed:  'pref set tempTBool 4',
    markup: 'VVVVVVVVVVVVVVVVVVVE',
    status: 'ERROR',
    hints: ''
  });

  helpers.setInput('pref set tempNumber 4');
  helpers.check({
    typed:  'pref set tempNumber 4',
    markup: 'VVVVVVVVVVVVVVVVVVVVV',
    status: 'VALID',
    hints: ''
  });
};

exports.testPrefExec = function(options) {
  if (options.isFirefox) {
    test.log('Skipping testPrefExec in Firefox.');
    return;
  }

  var initialAllowSet = pref.allowSet.value;
  pref.allowSet.value = false;

  test.is(mockSettings.tempNumber.value, 42, 'set to 42');

  helpers.exec(options, {
    typed: 'pref set tempNumber 4',
    args: {
      setting: mockSettings.tempNumber,
      value: 4
    },
    outputMatch: [ /void your warranty/, /I promise/ ]
  });

  test.is(mockSettings.tempNumber.value, 42, 'still set to 42');
  pref.allowSet.value = true;

  helpers.exec(options, {
    typed: 'pref set tempNumber 4',
    args: {
      setting: mockSettings.tempNumber,
      value: 4
    },
    blankOutput: true
  });

  test.is(mockSettings.tempNumber.value, 4, 'set to 4');

  helpers.exec(options, {
    typed: 'pref reset tempNumber',
    args: {
      setting: mockSettings.tempNumber
    },
    blankOutput: true
  });

  test.is(mockSettings.tempNumber.value, 42, 'reset to 42');

  pref.allowSet.value = initialAllowSet;

  helpers.exec(options, {
    typed: 'pref list tempNum',
    args: {
      search: 'tempNum'
    },
    outputMatch: /Filter/
  });
};


});
















define('gclitest/mockSettings', ['require', 'exports', 'module' , 'gcli/settings'], function(require, exports, module) {


var settings = require('gcli/settings');


var tempTBoolSpec = {
  name: 'tempTBool',
  type: 'boolean',
  description: 'temporary default true boolean',
  defaultValue: true
};
exports.tempTBool = undefined;

var tempFBoolSpec = {
  name: 'tempFBool',
  type: 'boolean',
  description: 'temporary default false boolean',
  defaultValue: false
};
exports.tempFBool = undefined;

var tempUStringSpec = {
  name: 'tempUString',
  type: 'string',
  description: 'temporary default undefined string'
};
exports.tempUString = undefined;

var tempNStringSpec = {
  name: 'tempNString',
  type: 'string',
  description: 'temporary default undefined string',
  defaultValue: null
};
exports.tempNString = undefined;

var tempQStringSpec = {
  name: 'tempQString',
  type: 'string',
  description: 'temporary default "q" string',
  defaultValue: 'q'
};
exports.tempQString = undefined;

var tempNumberSpec = {
  name: 'tempNumber',
  type: 'number',
  description: 'temporary number',
  defaultValue: 42
};
exports.tempNumber = undefined;

var tempSelectionSpec = {
  name: 'tempSelection',
  type: { name: 'selection', data: [ 'a', 'b', 'c' ] },
  description: 'temporary selection',
  defaultValue: 'a'
};
exports.tempSelection = undefined;




exports.setup = function() {
  exports.tempTBool = settings.addSetting(tempTBoolSpec);
  exports.tempFBool = settings.addSetting(tempFBoolSpec);
  exports.tempUString = settings.addSetting(tempUStringSpec);
  exports.tempNString = settings.addSetting(tempNStringSpec);
  exports.tempQString = settings.addSetting(tempQStringSpec);
  exports.tempNumber = settings.addSetting(tempNumberSpec);
  exports.tempSelection = settings.addSetting(tempSelectionSpec);
};

exports.shutdown = function() {
  settings.removeSetting(tempTBoolSpec);
  settings.removeSetting(tempFBoolSpec);
  settings.removeSetting(tempUStringSpec);
  settings.removeSetting(tempNStringSpec);
  settings.removeSetting(tempQStringSpec);
  settings.removeSetting(tempNumberSpec);
  settings.removeSetting(tempSelectionSpec);

  exports.tempTBool = undefined;
  exports.tempFBool = undefined;
  exports.tempUString = undefined;
  exports.tempNString = undefined;
  exports.tempQString = undefined;
  exports.tempNumber = undefined;
  exports.tempSelection = undefined;
};


});
















define('gclitest/testRequire', ['require', 'exports', 'module' , 'test/assert', 'gclitest/requirable'], function(require, exports, module) {

var test = require('test/assert');


exports.testWorking = function() {
  
  
  
  var requireable = require('gclitest/requirable');
  test.is('thing1', requireable.thing1);
  test.is(2, requireable.thing2);
  test.ok(requireable.thing3 === undefined);
};

exports.testDomains = function(options) {
  var requireable = require('gclitest/requirable');
  test.ok(requireable.status === undefined);
  requireable.setStatus(null);
  test.is(null, requireable.getStatus());
  test.ok(requireable.status === undefined);
  requireable.setStatus('42');
  test.is('42', requireable.getStatus());
  test.ok(requireable.status === undefined);

  if (options.isUnamdized) {
    test.log('Running unamdized, Reduced tests');
    return;
  }

  if (define.Domain) {
    var domain = new define.Domain();
    var requireable2 = domain.require('gclitest/requirable');
    test.is(undefined, requireable2.status);
    test.is('initial', requireable2.getStatus());
    requireable2.setStatus(999);
    test.is(999, requireable2.getStatus());
    test.is(undefined, requireable2.status);

    test.is('42', requireable.getStatus());
    test.is(undefined, requireable.status);
  }
};

exports.testLeakage = function() {
  var requireable = require('gclitest/requirable');
  test.ok(requireable.setup === undefined);
  test.ok(requireable.shutdown === undefined);
  test.ok(requireable.testWorking === undefined);
};

exports.testMultiImport = function() {
  var r1 = require('gclitest/requirable');
  var r2 = require('gclitest/requirable');
  test.is(r1, r2);
};

exports.testUncompilable = function() {
  
  
  
  
  
  
  










};

exports.testRecursive = function() {
  
  


};


});
















define('gclitest/requirable', ['require', 'exports', 'module' ], function(require, exports, module) {

  exports.thing1 = 'thing1';
  exports.thing2 = 2;

  var status = 'initial';
  exports.setStatus = function(aStatus) { status = aStatus; };
  exports.getStatus = function() { return status; };

});
















define('gclitest/testResource', ['require', 'exports', 'module' , 'gcli/types/resource', 'gcli/types', 'test/assert'], function(require, exports, module) {


var resource = require('gcli/types/resource');
var types = require('gcli/types');
var Status = require('gcli/types').Status;

var test = require('test/assert');

var tempDocument;

exports.setup = function(options) {
  tempDocument = resource.getDocument();
  resource.setDocument(options.window.document);
};

exports.shutdown = function(options) {
  resource.setDocument(tempDocument);
  tempDocument = undefined;
};

exports.testPredictions = function(options) {
  if (options.window.isFake || options.isFirefox) {
    test.log('Skipping resource tests: window.isFake || isFirefox');
    return;
  }

  var resource1 = types.getType('resource');
  var options1 = resource1.getLookup();
  test.ok(options1.length > 1, 'have resources');
  options1.forEach(function(prediction) {
    checkPrediction(resource1, prediction);
  });

  var resource2 = types.getType({ name: 'resource', include: 'text/javascript' });
  var options2 = resource2.getLookup();
  test.ok(options2.length > 1, 'have resources');
  options2.forEach(function(prediction) {
    checkPrediction(resource2, prediction);
  });

  var resource3 = types.getType({ name: 'resource', include: 'text/css' });
  var options3 = resource3.getLookup();
  
  if (!options.isNode) {
    test.ok(options3.length >= 1, 'have resources');
  }
  else {
    test.log('Running under Node. ' +
             'Skipping checks due to jsdom document.stylsheets support.');
  }
  options3.forEach(function(prediction) {
    checkPrediction(resource3, prediction);
  });

  var resource4 = types.getType({ name: 'resource' });
  var options4 = resource4.getLookup();

  test.is(options1.length, options4.length, 'type spec');
  test.is(options2.length + options3.length, options4.length, 'split');
};

function checkPrediction(res, prediction) {
  var name = prediction.name;
  var value = prediction.value;

  var conversion = res.parseString(name);
  test.is(conversion.getStatus(), Status.VALID, 'status VALID for ' + name);
  test.is(conversion.value, value, 'value for ' + name);

  var strung = res.stringify(value);
  test.is(strung, name, 'stringify for ' + name);

  test.is(typeof value.loadContents, 'function', 'resource for ' + name);
  test.is(typeof value.element, 'object', 'resource for ' + name);
}

});
















define('gclitest/testScratchpad', ['require', 'exports', 'module' , 'test/assert'], function(require, exports, module) {


var test = require('test/assert');

var origScratchpad;

exports.setup = function(options) {
  if (options.display) {
    origScratchpad = options.display.inputter.scratchpad;
    options.display.inputter.scratchpad = stubScratchpad;
  }
};

exports.shutdown = function(options) {
  if (options.display) {
    options.display.inputter.scratchpad = origScratchpad;
  }
};

var stubScratchpad = {
  shouldActivate: function(ev) {
    return true;
  },
  activatedCount: 0,
  linkText: 'scratchpad.linkText'
};
stubScratchpad.activate = function(value) {
  stubScratchpad.activatedCount++;
  return true;
};


exports.testActivate = function(options) {
  if (!options.display) {
    test.log('No display. Skipping scratchpad tests');
    return;
  }

  var ev = {};
  stubScratchpad.activatedCount = 0;
  options.display.inputter.onKeyUp(ev);
  test.is(1, stubScratchpad.activatedCount, 'scratchpad is activated');
};


});
















define('gclitest/testSettings', ['require', 'exports', 'module' , 'gclitest/mockSettings', 'test/assert'], function(require, exports, module) {


var mockSettings = require('gclitest/mockSettings');
var test = require('test/assert');


exports.setup = function(options) {
  if (!options.isFirefox) {
    mockSettings.setup();
  }
  else {
    test.log('Skipping testSettings in Firefox.');
  }
};

exports.shutdown = function(options) {
  if (!options.isFirefox) {
    mockSettings.shutdown();
  }
};

exports.testChange = function(options) {
  if (options.isFirefox) {
    test.log('Skipping testPref in Firefox.');
    return;
  }

  mockSettings.tempTBool.setDefault();
  mockSettings.tempFBool.setDefault();
  mockSettings.tempUString.setDefault();
  mockSettings.tempNString.setDefault();
  mockSettings.tempQString.setDefault();
  mockSettings.tempNumber.setDefault();
  mockSettings.tempSelection.setDefault();

  test.is(mockSettings.tempTBool.value, true, 'tempTBool default');
  test.is(mockSettings.tempFBool.value, false, 'tempFBool default');
  test.is(mockSettings.tempUString.value, undefined, 'tempUString default');
  test.is(mockSettings.tempNString.value, null, 'tempNString default');
  test.is(mockSettings.tempQString.value, 'q', 'tempQString default');
  test.is(mockSettings.tempNumber.value, 42, 'tempNumber default');
  test.is(mockSettings.tempSelection.value, 'a', 'tempSelection default');

  function tempTBoolCheck(ev) {
    test.is(ev.setting, mockSettings.tempTBool, 'tempTBool event setting');
    test.is(ev.value, false, 'tempTBool event value');
    test.is(ev.setting.value, false, 'tempTBool event setting value');
  }
  mockSettings.tempTBool.onChange.add(tempTBoolCheck);
  mockSettings.tempTBool.value = false;
  test.is(mockSettings.tempTBool.value, false, 'tempTBool change');

  function tempFBoolCheck(ev) {
    test.is(ev.setting, mockSettings.tempFBool, 'tempFBool event setting');
    test.is(ev.value, true, 'tempFBool event value');
    test.is(ev.setting.value, true, 'tempFBool event setting value');
  }
  mockSettings.tempFBool.onChange.add(tempFBoolCheck);
  mockSettings.tempFBool.value = true;
  test.is(mockSettings.tempFBool.value, true, 'tempFBool change');

  function tempUStringCheck(ev) {
    test.is(ev.setting, mockSettings.tempUString, 'tempUString event setting');
    test.is(ev.value, 'x', 'tempUString event value');
    test.is(ev.setting.value, 'x', 'tempUString event setting value');
  }
  mockSettings.tempUString.onChange.add(tempUStringCheck);
  mockSettings.tempUString.value = 'x';
  test.is(mockSettings.tempUString.value, 'x', 'tempUString change');

  function tempNStringCheck(ev) {
    test.is(ev.setting, mockSettings.tempNString, 'tempNString event setting');
    test.is(ev.value, 'y', 'tempNString event value');
    test.is(ev.setting.value, 'y', 'tempNString event setting value');
  }
  mockSettings.tempNString.onChange.add(tempNStringCheck);
  mockSettings.tempNString.value = 'y';
  test.is(mockSettings.tempNString.value, 'y', 'tempNString change');

  function tempQStringCheck(ev) {
    test.is(ev.setting, mockSettings.tempQString, 'tempQString event setting');
    test.is(ev.value, 'qq', 'tempQString event value');
    test.is(ev.setting.value, 'qq', 'tempQString event setting value');
  }
  mockSettings.tempQString.onChange.add(tempQStringCheck);
  mockSettings.tempQString.value = 'qq';
  test.is(mockSettings.tempQString.value, 'qq', 'tempQString change');

  function tempNumberCheck(ev) {
    test.is(ev.setting, mockSettings.tempNumber, 'tempNumber event setting');
    test.is(ev.value, -1, 'tempNumber event value');
    test.is(ev.setting.value, -1, 'tempNumber event setting value');
  }
  mockSettings.tempNumber.onChange.add(tempNumberCheck);
  mockSettings.tempNumber.value = -1;
  test.is(mockSettings.tempNumber.value, -1, 'tempNumber change');

  function tempSelectionCheck(ev) {
    test.is(ev.setting, mockSettings.tempSelection, 'tempSelection event setting');
    test.is(ev.value, 'b', 'tempSelection event value');
    test.is(ev.setting.value, 'b', 'tempSelection event setting value');
  }
  mockSettings.tempSelection.onChange.add(tempSelectionCheck);
  mockSettings.tempSelection.value = 'b';
  test.is(mockSettings.tempSelection.value, 'b', 'tempSelection change');

  mockSettings.tempTBool.onChange.remove(tempTBoolCheck);
  mockSettings.tempFBool.onChange.remove(tempFBoolCheck);
  mockSettings.tempUString.onChange.remove(tempUStringCheck);
  mockSettings.tempNString.onChange.remove(tempNStringCheck);
  mockSettings.tempQString.onChange.remove(tempQStringCheck);
  mockSettings.tempNumber.onChange.remove(tempNumberCheck);
  mockSettings.tempSelection.onChange.remove(tempSelectionCheck);

  function tempNStringReCheck(ev) {
    test.is(ev.setting, mockSettings.tempNString, 'tempNString event reset');
    test.is(ev.value, null, 'tempNString event revalue');
    test.is(ev.setting.value, null, 'tempNString event setting revalue');
  }
  mockSettings.tempNString.onChange.add(tempNStringReCheck);

  mockSettings.tempTBool.setDefault();
  mockSettings.tempFBool.setDefault();
  mockSettings.tempUString.setDefault();
  mockSettings.tempNString.setDefault();
  mockSettings.tempQString.setDefault();
  mockSettings.tempNumber.setDefault();
  mockSettings.tempSelection.setDefault();

  mockSettings.tempNString.onChange.remove(tempNStringReCheck);

  test.is(mockSettings.tempTBool.value, true, 'tempTBool reset');
  test.is(mockSettings.tempFBool.value, false, 'tempFBool reset');
  test.is(mockSettings.tempUString.value, undefined, 'tempUString reset');
  test.is(mockSettings.tempNString.value, null, 'tempNString reset');
  test.is(mockSettings.tempQString.value, 'q', 'tempQString reset');
  test.is(mockSettings.tempNumber.value, 42, 'tempNumber reset');
  test.is(mockSettings.tempSelection.value, 'a', 'tempSelection reset');
};


});















define('gclitest/testSpell', ['require', 'exports', 'module' , 'test/assert', 'gcli/types/spell'], function(require, exports, module) {

var test = require('test/assert');
var Speller = require('gcli/types/spell').Speller;

exports.setup = function() {
};

exports.shutdown = function() {
};

exports.testSpellerSimple = function(options) {
  if (options.isFirefox) {
    test.log('Skipping testPref in Firefox.');
    return;
  }

  var speller = new Speller();
  speller.train(Object.keys(options.window));

  test.is(speller.correct('document'), 'document');
  test.is(speller.correct('documen'), 'document');
  test.is(speller.correct('ocument'), 'document');
  test.is(speller.correct('odcument'), 'document');

  test.is(speller.correct('========='), null);
};


});
















define('gclitest/testSplit', ['require', 'exports', 'module' , 'test/assert', 'gclitest/mockCommands', 'gcli/cli', 'gcli/canon'], function(require, exports, module) {

var test = require('test/assert');

var mockCommands = require('gclitest/mockCommands');
var Requisition = require('gcli/cli').Requisition;
var canon = require('gcli/canon');

exports.setup = function() {
  mockCommands.setup();
};

exports.shutdown = function() {
  mockCommands.shutdown();
};

exports.testSplitSimple = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('s');
  requ._split(args);
  test.is(0, args.length);
  test.is('s', requ.commandAssignment.arg.text);
};

exports.testFlatCommand = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('tsv');
  requ._split(args);
  test.is(0, args.length);
  test.is('tsv', requ.commandAssignment.value.name);

  args = requ._tokenize('tsv a b');
  requ._split(args);
  test.is('tsv', requ.commandAssignment.value.name);
  test.is(2, args.length);
  test.is('a', args[0].text);
  test.is('b', args[1].text);
};

exports.testJavascript = function() {
  if (!canon.getCommand('{')) {
    test.log('Skipping testJavascript because { is not registered');
    return;
  }

  var args;
  var requ = new Requisition();

  args = requ._tokenize('{');
  requ._split(args);
  test.is(1, args.length);
  test.is('', args[0].text);
  test.is('', requ.commandAssignment.arg.text);
  test.is('{', requ.commandAssignment.value.name);
};



});
















define('gclitest/testTokenize', ['require', 'exports', 'module' , 'test/assert', 'gcli/cli'], function(require, exports, module) {


var test = require('test/assert');
var Requisition = require('gcli/cli').Requisition;

exports.testBlanks = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('');
  test.is(1, args.length);
  test.is('', args[0].text);
  test.is('', args[0].prefix);
  test.is('', args[0].suffix);

  args = requ._tokenize(' ');
  test.is(1, args.length);
  test.is('', args[0].text);
  test.is(' ', args[0].prefix);
  test.is('', args[0].suffix);
};

exports.testTokSimple = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('s');
  test.is(1, args.length);
  test.is('s', args[0].text);
  test.is('', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('Argument', args[0].type);

  args = requ._tokenize('s s');
  test.is(2, args.length);
  test.is('s', args[0].text);
  test.is('', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('Argument', args[0].type);
  test.is('s', args[1].text);
  test.is(' ', args[1].prefix);
  test.is('', args[1].suffix);
  test.is('Argument', args[1].type);
};

exports.testJavascript = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('{x}');
  test.is(1, args.length);
  test.is('x', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{ x }');
  test.is(1, args.length);
  test.is('x', args[0].text);
  test.is('{ ', args[0].prefix);
  test.is(' }', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{x} {y}');
  test.is(2, args.length);
  test.is('x', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);
  test.is('y', args[1].text);
  test.is(' {', args[1].prefix);
  test.is('}', args[1].suffix);
  test.is('ScriptArgument', args[1].type);

  args = requ._tokenize('{x}{y}');
  test.is(2, args.length);
  test.is('x', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);
  test.is('y', args[1].text);
  test.is('{', args[1].prefix);
  test.is('}', args[1].suffix);
  test.is('ScriptArgument', args[1].type);

  args = requ._tokenize('{');
  test.is(1, args.length);
  test.is('', args[0].text);
  test.is('{', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{ ');
  test.is(1, args.length);
  test.is('', args[0].text);
  test.is('{ ', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{x');
  test.is(1, args.length);
  test.is('x', args[0].text);
  test.is('{', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('ScriptArgument', args[0].type);
};

exports.testRegularNesting = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('{"x"}');
  test.is(1, args.length);
  test.is('"x"', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{\'x\'}');
  test.is(1, args.length);
  test.is('\'x\'', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('"{x}"');
  test.is(1, args.length);
  test.is('{x}', args[0].text);
  test.is('"', args[0].prefix);
  test.is('"', args[0].suffix);
  test.is('Argument', args[0].type);

  args = requ._tokenize('\'{x}\'');
  test.is(1, args.length);
  test.is('{x}', args[0].text);
  test.is('\'', args[0].prefix);
  test.is('\'', args[0].suffix);
  test.is('Argument', args[0].type);
};

exports.testDeepNesting = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('{{}}');
  test.is(1, args.length);
  test.is('{}', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{{x} {y}}');
  test.is(1, args.length);
  test.is('{x} {y}', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  args = requ._tokenize('{{w} {{{x}}}} {y} {{{z}}}');

  test.is(3, args.length);

  test.is('{w} {{{x}}}', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  test.is('y', args[1].text);
  test.is(' {', args[1].prefix);
  test.is('}', args[1].suffix);
  test.is('ScriptArgument', args[1].type);

  test.is('{{z}}', args[2].text);
  test.is(' {', args[2].prefix);
  test.is('}', args[2].suffix);
  test.is('ScriptArgument', args[2].type);

  args = requ._tokenize('{{w} {{{x}}} {y} {{{z}}}');

  test.is(1, args.length);

  test.is('{w} {{{x}}} {y} {{{z}}}', args[0].text);
  test.is('{', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('ScriptArgument', args[0].type);
};

exports.testStrangeNesting = function() {
  var args;
  var requ = new Requisition();

  
  args = requ._tokenize('{"x}"}');

  test.is(2, args.length);

  test.is('"x', args[0].text);
  test.is('{', args[0].prefix);
  test.is('}', args[0].suffix);
  test.is('ScriptArgument', args[0].type);

  test.is('}', args[1].text);
  test.is('"', args[1].prefix);
  test.is('', args[1].suffix);
  test.is('Argument', args[1].type);
};

exports.testComplex = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize(' 1234  \'12 34\'');

  test.is(2, args.length);

  test.is('1234', args[0].text);
  test.is(' ', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('Argument', args[0].type);

  test.is('12 34', args[1].text);
  test.is('  \'', args[1].prefix);
  test.is('\'', args[1].suffix);
  test.is('Argument', args[1].type);

  args = requ._tokenize('12\'34 "12 34" \\'); 

  test.is(3, args.length);

  test.is('12\'34', args[0].text);
  test.is('', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('Argument', args[0].type);

  test.is('12 34', args[1].text);
  test.is(' "', args[1].prefix);
  test.is('"', args[1].suffix);
  test.is('Argument', args[1].type);

  test.is('\\', args[2].text);
  test.is(' ', args[2].prefix);
  test.is('', args[2].suffix);
  test.is('Argument', args[2].type);
};

exports.testPathological = function() {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('a\\ b \\t\\n\\r \\\'x\\\" \'d'); 

  test.is(4, args.length);

  test.is('a b', args[0].text);
  test.is('', args[0].prefix);
  test.is('', args[0].suffix);
  test.is('Argument', args[0].type);

  test.is('\t\n\r', args[1].text);
  test.is(' ', args[1].prefix);
  test.is('', args[1].suffix);
  test.is('Argument', args[1].type);

  test.is('\'x"', args[2].text);
  test.is(' ', args[2].prefix);
  test.is('', args[2].suffix);
  test.is('Argument', args[2].type);

  test.is('d', args[3].text);
  test.is(' \'', args[3].prefix);
  test.is('', args[3].suffix);
  test.is('Argument', args[3].type);
};


});
















define('gclitest/testTooltip', ['require', 'exports', 'module' , 'test/assert', 'gclitest/mockCommands'], function(require, exports, module) {


var test = require('test/assert');
var mockCommands = require('gclitest/mockCommands');


exports.setup = function() {
  mockCommands.setup();
};

exports.shutdown = function() {
  mockCommands.shutdown();
};


function type(typed, tests, options) {
  var inputter = options.display.inputter;
  var tooltip = options.display.tooltip;

  inputter.setInput(typed);
  if (tests.cursor) {
    inputter.setCursor({ start: tests.cursor, end: tests.cursor });
  }

  if (!options.isNode) {
    if (tests.important) {
      test.ok(tooltip.field.isImportant, 'Important for ' + typed);
    }
    else {
      test.ok(!tooltip.field.isImportant, 'Not important for ' + typed);
    }

    if (tests.options) {
      var names = tooltip.field.menu.items.map(function(item) {
        return item.name.textContent ? item.name.textContent : item.name;
      });
      test.is(tests.options.join('|'), names.join('|'), 'Options for ' + typed);
    }

    if (tests.error) {
      test.is(tests.error, tooltip.errorEle.textContent, 'Error for ' + typed);
    }
    else {
      test.is('', tooltip.errorEle.textContent, 'No error for ' + typed);
    }
  }
}

exports.testActivate = function(options) {
  if (!options.display) {
    test.log('No display. Skipping activate tests');
    return;
  }

  if (options.isNode) {
    test.log('Running under Node. Reduced checks due to JSDom.textContent');
  }

  type(' ', { }, options);

  type('tsb ', {
    important: true,
    options: [ 'false', 'true' ]
  }, options);

  type('tsb t', {
    important: true,
    options: [ 'true' ]
  }, options);

  type('tsb tt', {
    important: true,
    options: [ ],
    error: 'Can\'t use \'tt\'.'
  }, options);


  type('asdf', {
    important: false,
    options: [ ],
    error: 'Can\'t use \'asdf\'.'
  }, options);

  type('', { }, options);
};


});
















define('gclitest/testTypes', ['require', 'exports', 'module' , 'test/assert', 'gcli/types'], function(require, exports, module) {

var test = require('test/assert');
var types = require('gcli/types');

exports.setup = function() {
};

exports.shutdown = function() {
};

function forEachType(options, callback) {
  types.getTypeNames().forEach(function(name) {
    options.name = name;

    
    if (name === 'selection') {
      options.data = [ 'a', 'b' ];
    }
    else if (name === 'deferred') {
      options.defer = function() {
        return types.getType('string');
      };
    }
    else if (name === 'array') {
      options.subtype = 'string';
    }

    var type = types.getType(options);
    callback(type);
  });
}

exports.testDefault = function(options) {
  if (options.isNode) {
    test.log('Running under Node. ' +
             'Skipping tests due to issues with resource type.');
    return;
  }

  forEachType({}, function(type) {
    var blank = type.getBlank().value;

    
    if (type.name === 'boolean') {
      test.is(blank, false, 'blank boolean is false');
    }
    else if (type.name === 'array') {
      test.ok(Array.isArray(blank), 'blank array is array');
      test.is(blank.length, 0, 'blank array is empty');
    }
    else if (type.name === 'nodelist') {
      test.ok(typeof blank.item, 'function', 'blank.item is function');
      test.is(blank.length, 0, 'blank nodelist is empty');
    }
    else {
      test.is(blank, undefined, 'default defined for ' + type.name);
    }
  });
};

exports.testNullDefault = function(options) {
  forEachType({ defaultValue: null }, function(type) {
    test.is(type.stringify(null), '', 'stringify(null) for ' + type.name);
  });
};

});
















define('gclitest/testUtil', ['require', 'exports', 'module' , 'gcli/util', 'test/assert'], function(require, exports, module) {

var util = require('gcli/util');
var test = require('test/assert');

exports.testFindCssSelector = function(options) {
  if (options.window.isFake) {
    test.log('Skipping dom.findCssSelector tests due to window.isFake');
    return;
  }

  var nodes = options.window.document.querySelectorAll('*');
  for (var i = 0; i < nodes.length; i++) {
    var selector = util.findCssSelector(nodes[i]);
    var matches = options.window.document.querySelectorAll(selector);

    test.is(matches.length, 1, 'multiple matches for ' + selector);
    test.is(matches[0], nodes[i], 'non-matching selector: ' + selector);
  }
};


});
















define('gcli/ui/display', ['require', 'exports', 'module' , 'gcli/util', 'gcli/settings', 'gcli/ui/intro', 'gcli/ui/domtemplate', 'gcli/ui/tooltip', 'gcli/ui/output_terminal', 'gcli/ui/inputter', 'gcli/ui/completer', 'gcli/ui/focus', 'gcli/ui/prompt', 'gcli/cli', 'text!gcli/ui/display.css', 'text!gcli/ui/display.html'], function(require, exports, module) {


var util = require('gcli/util');
var settings = require('gcli/settings');
var intro = require('gcli/ui/intro');
var domtemplate = require('gcli/ui/domtemplate');

var Tooltip = require('gcli/ui/tooltip').Tooltip;
var OutputTerminal = require('gcli/ui/output_terminal').OutputTerminal;
var Inputter = require('gcli/ui/inputter').Inputter;
var Completer = require('gcli/ui/completer').Completer;
var FocusManager = require('gcli/ui/focus').FocusManager;
var Prompt = require('gcli/ui/prompt').Prompt;

var Requisition = require('gcli/cli').Requisition;

var displayCss = require('text!gcli/ui/display.css');
var displayHtml = require('text!gcli/ui/display.html');








exports.createDisplay = function(options) {
  if (options.settings != null) {
    settings.setDefaults(options.settings);
  }
  var display = new Display(options);
  var requisition = display.requisition;
  return {
    



    exec: requisition.exec.bind(requisition),
    update: requisition.update.bind(requisition),
    destroy: display.destroy.bind(display)
  };
};














function Display(options) {
  var doc = options.document || document;

  this.displayStyle = undefined;
  if (displayCss != null) {
    this.displayStyle = util.importCss(displayCss, doc, 'gcli-css-display');
  }

  
  
  
  
  
  
  
  this.requisition = new Requisition(options.enviroment || {}, doc);

  this.focusManager = new FocusManager(options, {
    document: doc
  });

  this.inputElement = find(doc, options.inputElement || null, 'gcli-input');
  this.inputter = new Inputter(options, {
    requisition: this.requisition,
    focusManager: this.focusManager,
    element: this.inputElement
  });

  
  
  
  this.completeElement = insert(this.inputElement,
                         options.completeElement || null, 'gcli-row-complete');
  this.completer = new Completer(options, {
    requisition: this.requisition,
    inputter: this.inputter,
    autoResize: this.completeElement.gcliCreated,
    element: this.completeElement
  });

  this.prompt = new Prompt(options, {
    inputter: this.inputter,
    element: insert(this.inputElement,
                    options.promptElement || null, 'gcli-prompt')
  });

  this.element = find(doc, options.displayElement || null, 'gcli-display');
  this.element.classList.add('gcli-display');

  this.template = util.toDom(doc, displayHtml);
  this.elements = {};
  domtemplate.template(this.template, this.elements, { stack: 'display.html' });
  this.element.appendChild(this.template);

  this.tooltip = new Tooltip(options, {
    requisition: this.requisition,
    inputter: this.inputter,
    focusManager: this.focusManager,
    element: this.elements.tooltip,
    panelElement: this.elements.panel
  });

  this.inputter.tooltip = this.tooltip;

  this.outputElement = util.createElement(doc, 'div');
  this.outputElement.classList.add('gcli-output');
  this.outputList = new OutputTerminal(options, {
    requisition: this.requisition,
    element: this.outputElement
  });

  this.element.appendChild(this.outputElement);

  intro.maybeShowIntro(this.outputList.commandOutputManager, this.requisition);
}




Display.prototype.destroy = function() {
  delete this.element;
  delete this.template;

  this.outputList.destroy();
  delete this.outputList;
  delete this.outputElement;

  this.tooltip.destroy();
  delete this.tooltip;

  this.prompt.destroy();
  delete this.prompt;

  this.completer.destroy();
  delete this.completer;
  delete this.completeElement;

  this.inputter.destroy();
  delete this.inputter;
  delete this.inputElement;

  this.focusManager.destroy();
  delete this.focusManager;

  this.requisition.destroy();
  delete this.requisition;

  if (this.displayStyle) {
    this.displayStyle.parentNode.removeChild(this.displayStyle);
  }
  delete this.displayStyle;
};

exports.Display = Display;




function find(doc, element, id) {
  if (!element) {
    element = doc.getElementById(id);
    if (!element) {
      throw new Error('Missing element, id=' + id);
    }
  }
  return element;
}




function insert(sibling, element, id) {
  var doc = sibling.ownerDocument;
  if (!element) {
    element = doc.getElementById('gcli-row-complete');
    if (!element) {
      element = util.createElement(doc, 'div');
      sibling.parentNode.insertBefore(element, sibling.nextSibling);
      element.gcliCreated = true;
    }
  }
  return element;
}


});
















define('gcli/ui/output_terminal', ['require', 'exports', 'module' , 'gcli/util', 'gcli/canon', 'gcli/ui/domtemplate', 'text!gcli/ui/output_view.css', 'text!gcli/ui/output_terminal.html'], function(require, exports, module) {

var util = require('gcli/util');

var canon = require('gcli/canon');
var domtemplate = require('gcli/ui/domtemplate');

var outputViewCss = require('text!gcli/ui/output_view.css');
var outputViewHtml = require('text!gcli/ui/output_terminal.html');













function OutputTerminal(options, components) {
  this.element = components.element;
  this.requisition = components.requisition;

  this.commandOutputManager = options.commandOutputManager ||
          canon.commandOutputManager;
  this.commandOutputManager.onOutput.add(this.outputted, this);

  var document = components.element.ownerDocument;
  if (outputViewCss != null) {
    this.style = util.importCss(outputViewCss, document, 'gcli-output-view');
  }

  this.template = util.toDom(document, outputViewHtml);
  this.templateOptions = { allowEval: true, stack: 'output_terminal.html' };
}




OutputTerminal.prototype.destroy = function() {
  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  this.commandOutputManager.onOutput.remove(this.outputted, this);

  delete this.commandOutputManager;
  delete this.requisition;
  delete this.element;
  delete this.template;
};




OutputTerminal.prototype.outputted = function(ev) {
  if (ev.output.hidden) {
    return;
  }

  ev.output.view = new OutputView(ev.output, this);
};




OutputTerminal.prototype.setHeight = function(height) {
  this.element.style.height = height + 'px';
};

exports.OutputTerminal = OutputTerminal;





function OutputView(outputData, outputTerminal) {
  this.outputData = outputData;
  this.outputTerminal = outputTerminal;

  this.url = util.createUrlLookup(module);

  
  this.elems = {
    rowin: null,
    rowout: null,
    hide: null,
    show: null,
    duration: null,
    throb: null,
    prompt: null
  };

  var template = this.outputTerminal.template.cloneNode(true);
  domtemplate.template(template, this, this.outputTerminal.templateOptions);

  this.outputTerminal.element.appendChild(this.elems.rowin);
  this.outputTerminal.element.appendChild(this.elems.rowout);

  this.outputData.onClose.add(this.closed, this);
  this.outputData.onChange.add(this.changed, this);
}

OutputView.prototype.destroy = function() {
  this.outputData.onChange.remove(this.changed, this);
  this.outputData.onClose.remove(this.closed, this);

  this.outputTerminal.element.removeChild(this.elems.rowin);
  this.outputTerminal.element.removeChild(this.elems.rowout);

  delete this.outputData;
  delete this.outputTerminal;
  delete this.url;
  delete this.elems;
};




Object.defineProperty(OutputView.prototype, 'prompt', {
  get: function() {
    return this.outputData.canonical ? '\u00bb' : '';
  },
  enumerable: true
});





OutputView.prototype.copyToInput = function() {
  if (this.outputTerminal.requisition) {
    this.outputTerminal.requisition.update(this.outputData.typed);
  }
};




OutputView.prototype.execute = function(ev) {
  if (this.outputTerminal.requisition) {
    this.outputTerminal.requisition.exec({ typed: this.outputData.typed });
  }
};

OutputView.prototype.hideOutput = function(ev) {
  this.elems.rowout.style.display = 'none';
  this.elems.hide.classList.add('cmd_hidden');
  this.elems.show.classList.remove('cmd_hidden');

  ev.stopPropagation();
};

OutputView.prototype.showOutput = function(ev) {
  this.elems.rowout.style.display = 'block';
  this.elems.hide.classList.remove('cmd_hidden');
  this.elems.show.classList.add('cmd_hidden');

  ev.stopPropagation();
};

OutputView.prototype.closed = function(ev) {
  this.destroy();
};

OutputView.prototype.changed = function(ev) {
  var document = this.elems.rowout.ownerDocument;
  var duration = this.outputData.duration != null ?
          'completed in ' + (this.outputData.duration / 1000) + ' sec ' :
          '';
  duration = document.createTextNode(duration);
  this.elems.duration.appendChild(duration);

  if (this.outputData.completed) {
    this.elems.prompt.classList.add('gcli-row-complete');
  }
  if (this.outputData.error) {
    this.elems.prompt.classList.add('gcli-row-error');
  }

  this.outputData.toDom(this.elems.rowout);

  
  
  
  var scrollHeight = Math.max(this.outputTerminal.element.scrollHeight,
      this.outputTerminal.element.clientHeight);
  this.outputTerminal.element.scrollTop =
      scrollHeight - this.outputTerminal.element.clientHeight;

  this.elems.throb.style.display = this.outputData.completed ? 'none' : 'block';
};

exports.OutputView = OutputView;


});
define("text!gcli/ui/output_view.css", [], "\n" +
  ".gcli-row-in {\n" +
  "  padding: 0 4px;\n" +
  "  box-shadow: 0 -6px 10px -6px #ddd;\n" +
  "  border-top: 1px solid #bbb;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-in > img {\n" +
  "  cursor: pointer;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-hover {\n" +
  "  display: none;\n" +
  "  float: right;\n" +
  "  padding: 2px 2px 0;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-in:hover > .gcli-row-hover {\n" +
  "  display: inline;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-in:hover > .gcli-row-hover.gcli-row-hidden {\n" +
  "  display: none;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-duration {\n" +
  "  color: #666;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-prompt {\n" +
  "  color: #00F;\n" +
  "  font-weight: bold;\n" +
  "  font-size: 120%;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-prompt.gcli-row-complete {\n" +
  "  color: #060;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-prompt.gcli-row-error {\n" +
  "  color: #F00;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-duration {\n" +
  "  font-size: 80%;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out {\n" +
  "  margin: 0 10px 15px;\n" +
  "  padding: 0 10px;\n" +
  "  line-height: 1.2em;\n" +
  "  font-size: 95%;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out strong,\n" +
  ".gcli-row-out b,\n" +
  ".gcli-row-out th,\n" +
  ".gcli-row-out h1,\n" +
  ".gcli-row-out h2,\n" +
  ".gcli-row-out h3 {\n" +
  "  color: #000;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out p {\n" +
  "  margin: 5px 0;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out a {\n" +
  "  color: hsl(200,40%,40%);\n" +
  "  text-decoration: none;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out a:hover {\n" +
  "  cursor: pointer;\n" +
  "  border-bottom: 1px dotted hsl(200,40%,60%);\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out input[type=password],\n" +
  ".gcli-row-out input[type=text],\n" +
  ".gcli-row-out textarea {\n" +
  "  font-size: 120%;\n" +
  "  background: transparent;\n" +
  "  padding: 3px;\n" +
  "  border-radius: 3px;\n" +
  "  border: 1px solid #bbb;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-out table,\n" +
  ".gcli-row-out td,\n" +
  ".gcli-row-out th {\n" +
  "  border: 0;\n" +
  "  padding: 0 2px;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-terminal,\n" +
  ".gcli-row-subterminal {\n" +
  "  border-radius: 3px;\n" +
  "  border: 1px solid #ddd;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-terminal {\n" +
  "  height: 200px;\n" +
  "  width: 620px;\n" +
  "  font-size: 80%;\n" +
  "}\n" +
  "\n" +
  ".gcli-row-subterminal {\n" +
  "  height: 150px;\n" +
  "  width: 300px;\n" +
  "  font-size: 75%;\n" +
  "}\n" +
  "\n" +
  ".gcli-out-shortcut {\n" +
  "  font-weight: normal;\n" +
  "  border: 1px solid #999;\n" +
  "  border-radius: 3px;\n" +
  "  color: #666;\n" +
  "  cursor: pointer;\n" +
  "  padding: 0 3px 1px;\n" +
  "  margin: 1px 4px;\n" +
  "  display: inline-block;\n" +
  "}\n" +
  "\n" +
  ".gcli-out-shortcut:before {\n" +
  "  content: '\\bb';\n" +
  "  padding-right: 2px;\n" +
  "  color: hsl(25,78%,50%);\n" +
  "  font-weight: bold;\n" +
  "  font-size: 110%;\n" +
  "}\n" +
  "");

define("text!gcli/ui/output_terminal.html", [], "\n" +
  "<div class=\"gcli-row\">\n" +
  "  <!-- The div for the input (i.e. what was typed) -->\n" +
  "  <div class=\"gcli-row-in\" save=\"${elems.rowin}\" aria-live=\"assertive\"\n" +
  "      onclick=\"${copyToInput}\" ondblclick=\"${execute}\">\n" +
  "\n" +
  "    <!-- What the user actually typed -->\n" +
  "    <span save=\"${elems.prompt}\" class=\"gcli-row-prompt ${elems.error ? 'gcli-row-error' : ''} ${elems.completed ? 'gcli-row-complete' : ''}\">${prompt}</span>\n" +
  "    <span class=\"gcli-row-in-typed\">${outputData.canonical}</span>\n" +
  "\n" +
  "    <!-- The extra details that appear on hover -->\n" +
  "    <span class=\"gcli-row-duration gcli-row-hover\" save=\"${elems.duration}\"></span>\n" +
  "    <!--\n" +
  "    <img class=\"gcli-row-hover\" onclick=\"${hideOutput}\" save=\"${elems.hide}\"\n" +
  "        alt=\"Hide command output\" _src=\"${url('images/minus.png')}\"/>\n" +
  "    <img class=\"gcli-row-hover gcli-row-hidden\" onclick=\"${showOutput}\" save=\"${elems.show}\"\n" +
  "        alt=\"Show command output\" _src=\"${url('images/plus.png')}\"/>\n" +
  "    <img class=\"gcli-row-hover\" onclick=\"${remove}\"\n" +
  "        alt=\"Remove this command from the history\"\n" +
  "        _src=\"${url('images/closer.png')}\"/>\n" +
  "    -->\n" +
  "    <img style=\"float:right;\" _src=\"${url('images/throbber.gif')}\" save=\"${elems.throb}\"/>\n" +
  "  </div>\n" +
  "\n" +
  "  <!-- The div for the command output -->\n" +
  "  <div class=\"gcli-row-out\" aria-live=\"assertive\" save=\"${elems.rowout}\">\n" +
  "  </div>\n" +
  "</div>\n" +
  "");

















define('gcli/ui/prompt', ['require', 'exports', 'module' ], function(require, exports, module) {













function Prompt(options, components) {
  this.element = components.element;
  this.element.classList.add('gcli-prompt');

  var prompt = options.promptChar || '\u00bb';
  var text = this.element.ownerDocument.createTextNode(prompt);
  this.element.appendChild(text);

  this.inputter = components.inputter;
  if (this.inputter) {
    this.inputter.onResize.add(this.resized, this);

    var dimensions = this.inputter.getDimensions();
    if (dimensions) {
      this.resized(dimensions);
    }
  }
}




Prompt.prototype.destroy = function() {
  if (this.inputter) {
    this.inputter.onResize.remove(this.resized, this);
  }

  delete this.element;
};




Prompt.prototype.resized = function(ev) {
  this.element.style.top = ev.top + 'px';
  this.element.style.height = ev.height + 'px';
  this.element.style.left = ev.left + 'px';
  this.element.style.width = ev.width + 'px';
};

exports.Prompt = Prompt;


});
define("text!gcli/ui/display.css", [], "\n" +
  ".gcli-output {\n" +
  "  height: 100%;\n" +
  "  overflow-x: hidden;\n" +
  "  overflow-y: auto;\n" +
  "  font-family: Segoe UI, Helvetica Neue, Verdana, Arial, sans-serif;\n" +
  "}\n" +
  "");

define("text!gcli/ui/display.html", [], "\n" +
  "<div class=\"gcli-panel\" save=\"${panel}\">\n" +
  "  <div save=\"${tooltip}\"></div>\n" +
  "  <div class=\"gcli-panel-connector\"></div>\n" +
  "</div>\n" +
  "");


let testModuleNames = [
  'gclitest/index',
  'gclitest/suite',
  'test/examiner',
  'test/assert',
  'test/status',
  'gclitest/testCanon',
  'gclitest/helpers',
  'gclitest/testCli',
  'gclitest/mockCommands',
  'gclitest/testCompletion',
  'gclitest/testExec',
  'gclitest/testFocus',
  'gclitest/testHelp',
  'gclitest/testHistory',
  'gclitest/testInputter',
  'gclitest/testIncomplete',
  'gclitest/testIntro',
  'gclitest/testJs',
  'gclitest/testKeyboard',
  'gclitest/testMenu',
  'gclitest/testNode',
  'gclitest/testPref',
  'gclitest/mockSettings',
  'gclitest/testRequire',
  'gclitest/requirable',
  'gclitest/testResource',
  'gclitest/testScratchpad',
  'gclitest/testSettings',
  'gclitest/testSpell',
  'gclitest/testSplit',
  'gclitest/testTokenize',
  'gclitest/testTooltip',
  'gclitest/testTypes',
  'gclitest/testUtil',
  'gcli/ui/display',
  'gcli/ui/output_terminal',
  'text!gcli/ui/output_view.css',
  'text!gcli/ui/output_terminal.html',
  'gcli/ui/prompt',
  'text!gcli/ui/display.css',
  'text!gcli/ui/display.html',
];


let localDefine;

const TEST_URI = "data:text/html;charset=utf-8,gcli-web";

function test() {
  localDefine = define;

  DeveloperToolbarTest.test(TEST_URI, function(browser, tab) {
    var examiner = define.globalDomain.require('gclitest/suite').examiner;
    examiner.runAsync({
      display: DeveloperToolbar.display,
      isFirefox: true,
      window: browser.contentDocument.defaultView
    }, finish);
  });
}

registerCleanupFunction(function() {
  testModuleNames.forEach(function(moduleName) {
    delete localDefine.modules[moduleName];
    delete localDefine.globalDomain.modules[moduleName];
  });

  localDefine = undefined;
});
