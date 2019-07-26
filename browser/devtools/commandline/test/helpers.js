















'use strict';



this.EXPORTED_SYMBOLS = [ 'helpers' ];
var helpers = {};
this.helpers = helpers;

var TargetFactory = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.TargetFactory;
var require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;

var assert = { ok: ok, is: is, log: info };
var util = require('gcli/util/util');
var promise = require('gcli/util/promise');
var cli = require('gcli/cli');
var KeyEvent = require('gcli/util/util').KeyEvent;
var gcli = require('gcli/index');




var createFFDisplayAutomator = function(display) {
  var automator = {
    setInput: function(typed) {
      return display.inputter.setInput(typed);
    },

    setCursor: function(cursor) {
      return display.inputter.setCursor(cursor);
    },

    focus: function() {
      return display.inputter.focus();
    },

    fakeKey: function(keyCode) {
      var fakeEvent = {
        keyCode: keyCode,
        preventDefault: function() { },
        timeStamp: new Date().getTime()
      };

      display.inputter.onKeyDown(fakeEvent);

      if (keyCode === KeyEvent.DOM_VK_BACK_SPACE) {
        var input = display.inputter.element;
        input.value = input.value.slice(0, -1);
      }

      return display.inputter.handleKeyUp(fakeEvent);
    },

    getInputState: function() {
      return display.inputter.getInputState();
    },

    getCompleterTemplateData: function() {
      return display.completer._getCompleterTemplateData();
    },

    getErrorMessage: function() {
      return display.tooltip.errorEle.textContent;
    }
  };

  Object.defineProperty(automator, 'focusManager', {
    get: function() { return display.focusManager; },
    enumerable: true
  });

  Object.defineProperty(automator, 'field', {
    get: function() { return display.tooltip.field; },
    enumerable: true
  });

  return automator;
};



























helpers.addTab = function(url, callback, options) {
  waitForExplicitFinish();

  options = options || {};
  options.chromeWindow = options.chromeWindow || window;
  options.isFirefox = true;

  var tabbrowser = options.chromeWindow.gBrowser;
  options.tab = tabbrowser.addTab();
  tabbrowser.selectedTab = options.tab;
  options.browser = tabbrowser.getBrowserForTab(options.tab);
  options.target = TargetFactory.forTab(options.tab);

  var loaded = helpers.listenOnce(options.browser, "load", true).then(function(ev) {
    options.document = options.browser.contentDocument;
    options.window = options.document.defaultView;

    var reply = callback.call(null, options);

    return promise.resolve(reply).then(null, function(error) {
      ok(false, error);
    }).then(function() {
      tabbrowser.removeTab(options.tab);

      delete options.window;
      delete options.document;

      delete options.target;
      delete options.browser;
      delete options.tab;

      delete options.chromeWindow;
      delete options.isFirefox;
    });
  });

  options.browser.contentWindow.location = url;
  return loaded;
};















helpers.openTab = function(url, options) {
  waitForExplicitFinish();

  options = options || {};
  options.chromeWindow = options.chromeWindow || window;
  options.isFirefox = true;

  var tabbrowser = options.chromeWindow.gBrowser;
  options.tab = tabbrowser.addTab();
  tabbrowser.selectedTab = options.tab;
  options.browser = tabbrowser.getBrowserForTab(options.tab);
  options.target = TargetFactory.forTab(options.tab);

  return helpers.navigate(url, options);
};






helpers.closeTab = function(options) {
  options.chromeWindow.gBrowser.removeTab(options.tab);

  delete options.window;
  delete options.document;

  delete options.target;
  delete options.browser;
  delete options.tab;

  delete options.chromeWindow;
  delete options.isFirefox;

  return promise.resolve(undefined);
};










helpers.openToolbar = function(options) {
  options = options || {};
  options.chromeWindow = options.chromeWindow || window;

  return options.chromeWindow.DeveloperToolbar.show(true).then(function() {
    var display = options.chromeWindow.DeveloperToolbar.display;
    options.automator = createFFDisplayAutomator(display);
    options.requisition = display.requisition;
    return options;
  });
};




helpers.navigate = function(url, options) {
  options = options || {};
  options.chromeWindow = options.chromeWindow || window;
  options.tab = options.tab || options.chromeWindow.gBrowser.selectedTab;

  var tabbrowser = options.chromeWindow.gBrowser;
  options.browser = tabbrowser.getBrowserForTab(options.tab);

  var promise = helpers.listenOnce(options.browser, "load", true).then(function() {
    options.document = options.browser.contentDocument;
    options.window = options.document.defaultView;
    return options;
  });

  options.browser.contentWindow.location = url;

  return promise;
};






helpers.closeToolbar = function(options) {
  return options.chromeWindow.DeveloperToolbar.hide().then(function() {
    delete options.automator;
    delete options.requisition;
  });
};





helpers.handleError = function(ex) {
  console.error(ex);
  ok(false, ex);
  finish();
};









helpers.listenOnce = function(element, event, useCapture) {
  var deferred = promise.defer();
  var onEvent = function(ev) {
    element.removeEventListener(event, onEvent, useCapture);
    deferred.resolve(ev);
  };
  element.addEventListener(event, onEvent, useCapture);
  return deferred.promise;
};










helpers.observeOnce = function(topic, ownsWeak=false) {
  let deferred = promise.defer();
  let resolver = function(subject) {
    Services.obs.removeObserver(resolver, topic);
    deferred.resolve(subject);
  };
  Services.obs.addObserver(resolver, topic, ownsWeak);
  return deferred.promise;
};





helpers.promiseify = function(functionWithLastParamCallback, scope) {
  return function() {
    let deferred = promise.defer();

    let args = [].slice.call(arguments);
    args.push(function(callbackParam) {
      deferred.resolve(callbackParam);
    });

    try {
      functionWithLastParamCallback.apply(scope, args);
    }
    catch (ex) {
      deferred.resolve(ex);
    }

    return deferred.promise;
  }
};








helpers.addTabWithToolbar = function(url, callback, options) {
  return helpers.addTab(url, function(innerOptions) {
    var win = innerOptions.chromeWindow;

    return win.DeveloperToolbar.show(true).then(function() {
      var display = win.DeveloperToolbar.display;
      innerOptions.automator = createFFDisplayAutomator(display);
      innerOptions.requisition = display.requisition;

      var reply = callback.call(null, innerOptions);

      return promise.resolve(reply).then(null, function(error) {
        ok(false, error);
        console.error(error);
      }).then(function() {
        win.DeveloperToolbar.hide().then(function() {
          delete innerOptions.automator;
        });
      });
    });
  }, options);
};













helpers.runTests = function(options, tests) {
  var testNames = Object.keys(tests).filter(function(test) {
    return test != "setup" && test != "shutdown";
  });

  var recover = function(error) {
    ok(false, error);
    console.error(error);
  };

  info("SETUP");
  var setupDone = (tests.setup != null) ?
      promise.resolve(tests.setup(options)) :
      promise.resolve();

  var testDone = setupDone.then(function() {
    return util.promiseEach(testNames, function(testName) {
      info(testName);
      var action = tests[testName];

      if (typeof action === "function") {
        var reply = action.call(tests, options);
        return promise.resolve(reply);
      }
      else if (Array.isArray(action)) {
        return helpers.audit(options, action);
      }

      return promise.reject("test action '" + testName +
                            "' is not a function or helpers.audit() object");
    });
  }, recover);

  return testDone.then(function() {
    info("SHUTDOWN");
    return (tests.shutdown != null) ?
        promise.resolve(tests.shutdown(options)) :
        promise.resolve();
  }, recover);
};


















function checkOptions(options) {
  if (options == null) {
    console.trace();
    throw new Error('Missing options object');
  }
  if (options.requisition == null) {
    console.trace();
    throw new Error('options.requisition == null');
  }
}




helpers._actual = {
  input: function(options) {
    return options.automator.getInputState().typed;
  },

  hints: function(options) {
    return options.automator.getCompleterTemplateData().then(function(data) {
      var emptyParams = data.emptyParameters.join('');
      return (data.directTabText + emptyParams + data.arrowTabText)
                .replace(/\u00a0/g, ' ')
                .replace(/\u21E5/, '->')
                .replace(/ $/, '');
    });
  },

  markup: function(options) {
    var cursor = helpers._actual.cursor(options);
    var statusMarkup = options.requisition.getInputStatusMarkup(cursor);
    return statusMarkup.map(function(s) {
      return new Array(s.string.length + 1).join(s.status.toString()[0]);
    }).join('');
  },

  cursor: function(options) {
    return options.automator.getInputState().cursor.start;
  },

  current: function(options) {
    var cursor = helpers._actual.cursor(options);
    return options.requisition.getAssignmentAt(cursor).param.name;
  },

  status: function(options) {
    return options.requisition.status.toString();
  },

  predictions: function(options) {
    var cursor = helpers._actual.cursor(options);
    var assignment = options.requisition.getAssignmentAt(cursor);
    var context = options.requisition.executionContext;
    return assignment.getPredictions(context).then(function(predictions) {
      return predictions.map(function(prediction) {
        return prediction.name;
      });
    });
  },

  unassigned: function(options) {
    return options.requisition._unassigned.map(function(assignment) {
      return assignment.arg.toString();
    }.bind(this));
  },

  outputState: function(options) {
    var outputData = options.automator.focusManager._shouldShowOutput();
    return outputData.visible + ':' + outputData.reason;
  },

  tooltipState: function(options) {
    var tooltipData = options.automator.focusManager._shouldShowTooltip();
    return tooltipData.visible + ':' + tooltipData.reason;
  },

  options: function(options) {
    if (options.automator.field.menu == null) {
      return [];
    }
    return options.automator.field.menu.items.map(function(item) {
      return item.name.textContent ? item.name.textContent : item.name;
    });
  },

  message: function(options) {
    return options.automator.getErrorMessage();
  }
};

function shouldOutputUnquoted(value) {
  var type = typeof value;
  return value == null || type === 'boolean' || type === 'number';
}

function outputArray(array) {
  return (array.length === 0) ?
      '[ ]' :
      '[ \'' + array.join('\', \'') + '\' ]';
}

helpers._createDebugCheck = function(options) {
  checkOptions(options);
  var requisition = options.requisition;
  var command = requisition.commandAssignment.value;
  var cursor = helpers._actual.cursor(options);
  var input = helpers._actual.input(options);
  var padding = new Array(input.length + 1).join(' ');

  var hintsPromise = helpers._actual.hints(options);
  var predictionsPromise = helpers._actual.predictions(options);

  return promise.all(hintsPromise, predictionsPromise).then(function(values) {
    var hints = values[0];
    var predictions = values[1];
    var output = '';

    output += 'return helpers.audit(options, [\n';
    output += '  {\n';

    if (cursor === input.length) {
      output += '    setup:    \'' + input + '\',\n';
    }
    else {
      output += '    name: \'' + input + ' (cursor=' + cursor + ')\',\n';
      output += '    setup: function() {\n';
      output += '      return helpers.setInput(options, \'' + input + '\', ' + cursor + ');\n';
      output += '    },\n';
    }

    output += '    check: {\n';

    output += '      input:  \'' + input + '\',\n';
    output += '      hints:  ' + padding + '\'' + hints + '\',\n';
    output += '      markup: \'' + helpers._actual.markup(options) + '\',\n';
    output += '      cursor: ' + cursor + ',\n';
    output += '      current: \'' + helpers._actual.current(options) + '\',\n';
    output += '      status: \'' + helpers._actual.status(options) + '\',\n';
    output += '      options: ' + outputArray(helpers._actual.options(options)) + ',\n';
    output += '      message: \'' + helpers._actual.message(options) + '\',\n';
    output += '      predictions: ' + outputArray(predictions) + ',\n';
    output += '      unassigned: ' + outputArray(requisition._unassigned) + ',\n';
    output += '      outputState: \'' + helpers._actual.outputState(options) + '\',\n';
    output += '      tooltipState: \'' + helpers._actual.tooltipState(options) + '\'' +
              (command ? ',' : '') +'\n';

    if (command) {
      output += '      args: {\n';
      output += '        command: { name: \'' + command.name + '\' },\n';

      requisition.getAssignments().forEach(function(assignment) {
        output += '        ' + assignment.param.name + ': { ';

        if (typeof assignment.value === 'string') {
          output += 'value: \'' + assignment.value + '\', ';
        }
        else if (shouldOutputUnquoted(assignment.value)) {
          output += 'value: ' + assignment.value + ', ';
        }
        else {
          output += '/*value:' + assignment.value + ',*/ ';
        }

        output += 'arg: \'' + assignment.arg + '\', ';
        output += 'status: \'' + assignment.getStatus().toString() + '\', ';
        output += 'message: \'' + assignment.message + '\'';
        output += ' },\n';
      });

      output += '      }\n';
    }

    output += '    },\n';
    output += '    exec: {\n';
    output += '      output: \'\',\n';
    output += '      type: \'string\',\n';
    output += '      error: false\n';
    output += '    }\n';
    output += '  }\n';
    output += ']);';

    return output;
  }.bind(this), util.errorHandler);
};




helpers.focusInput = function(options) {
  checkOptions(options);
  options.automator.focus();
};




helpers.pressTab = function(options) {
  checkOptions(options);
  return helpers.pressKey(options, KeyEvent.DOM_VK_TAB);
};




helpers.pressReturn = function(options) {
  checkOptions(options);
  return helpers.pressKey(options, KeyEvent.DOM_VK_RETURN);
};




helpers.pressKey = function(options, keyCode) {
  checkOptions(options);
  return options.automator.fakeKey(keyCode);
};





var ACTIONS = {
  '<TAB>': function(options) {
    return helpers.pressTab(options);
  },
  '<RETURN>': function(options) {
    return helpers.pressReturn(options);
  },
  '<UP>': function(options) {
    return helpers.pressKey(options, KeyEvent.DOM_VK_UP);
  },
  '<DOWN>': function(options) {
    return helpers.pressKey(options, KeyEvent.DOM_VK_DOWN);
  },
  '<BACKSPACE>': function(options) {
    return helpers.pressKey(options, KeyEvent.DOM_VK_BACK_SPACE);
  }
};






var CHUNKER = /([^<]*)(<[A-Z]+>)/;






helpers.setInput = function(options, typed, cursor) {
  checkOptions(options);
  var inputPromise;
  var automator = options.automator;
  
  
  var chunkLen = 1;

  
  if (typed.indexOf('<') === -1) {
    inputPromise = automator.setInput(typed);
  }
  else {
    
    
    var chunks = typed.split(CHUNKER).filter(function(s) {
      return s !== '';
    });
    chunkLen = chunks.length + 1;

    
    inputPromise = automator.setInput('').then(function() {
      return util.promiseEach(chunks, function(chunk) {
        if (chunk.charAt(0) === '<') {
          var action = ACTIONS[chunk];
          if (typeof action !== 'function') {
            console.error('Known actions: ' + Object.keys(ACTIONS).join());
            throw new Error('Key action not found "' + chunk + '"');
          }
          return action(options);
        }
        else {
          return automator.setInput(automator.getInputState().typed + chunk);
        }
      });
    });
  }

  return inputPromise.then(function() {
    if (cursor != null) {
      automator.setCursor({ start: cursor, end: cursor });
    }

    if (automator.focusManager) {
      automator.focusManager.onInputChange();
    }

    
    if (options.isFirefox) {
      var cursorStr = (cursor == null ? '' : ', ' + cursor);
      log('setInput("' + typed + '"' + cursorStr + ')');
    }

    return chunkLen;
  });
};








helpers._check = function(options, name, checks) {
  
  var requisition = options.requisition;
  requisition._args.forEach(function(arg) {
    if (arg.assignment == null) {
      assert.ok(false, 'No assignment for ' + arg);
    }
  });

  if (checks == null) {
    return promise.resolve();
  }

  var outstanding = [];
  var suffix = name ? ' (for \'' + name + '\')' : '';

  if (!options.isNoDom && 'input' in checks) {
    assert.is(helpers._actual.input(options), checks.input, 'input' + suffix);
  }

  if (!options.isNoDom && 'cursor' in checks) {
    assert.is(helpers._actual.cursor(options), checks.cursor, 'cursor' + suffix);
  }

  if (!options.isNoDom && 'current' in checks) {
    assert.is(helpers._actual.current(options), checks.current, 'current' + suffix);
  }

  if ('status' in checks) {
    assert.is(helpers._actual.status(options), checks.status, 'status' + suffix);
  }

  if (!options.isNoDom && 'markup' in checks) {
    assert.is(helpers._actual.markup(options), checks.markup, 'markup' + suffix);
  }

  if (!options.isNoDom && 'hints' in checks) {
    var hintCheck = function(actualHints) {
      assert.is(actualHints, checks.hints, 'hints' + suffix);
    };
    outstanding.push(helpers._actual.hints(options).then(hintCheck));
  }

  if (!options.isNoDom && 'predictions' in checks) {
    var predictionsCheck = function(actualPredictions) {
      helpers.arrayIs(actualPredictions,
                       checks.predictions,
                       'predictions' + suffix);
    };
    outstanding.push(helpers._actual.predictions(options).then(predictionsCheck));
  }

  if (!options.isNoDom && 'predictionsContains' in checks) {
    var containsCheck = function(actualPredictions) {
      checks.predictionsContains.forEach(function(prediction) {
        var index = actualPredictions.indexOf(prediction);
        assert.ok(index !== -1,
                  'predictionsContains:' + prediction + suffix);
      });
    };
    outstanding.push(helpers._actual.predictions(options).then(containsCheck));
  }

  if ('unassigned' in checks) {
    helpers.arrayIs(helpers._actual.unassigned(options),
                     checks.unassigned,
                     'unassigned' + suffix);
  }

  







  if (!options.isNoDom && 'outputState' in checks) {
    assert.is(helpers._actual.outputState(options),
              checks.outputState,
              'outputState' + suffix);
  }

  if (!options.isNoDom && 'options' in checks) {
    helpers.arrayIs(helpers._actual.options(options),
                     checks.options,
                     'options' + suffix);
  }

  if (!options.isNoDom && 'error' in checks) {
    assert.is(helpers._actual.message(options), checks.error, 'error' + suffix);
  }

  if (checks.args != null) {
    Object.keys(checks.args).forEach(function(paramName) {
      var check = checks.args[paramName];

      
      
      
      
      var assignment = requisition.getAssignment(paramName);
      if (assignment == null && paramName === 'command') {
        assignment = requisition.commandAssignment;
      }

      if (assignment == null) {
        assert.ok(false, 'Unknown arg: ' + paramName + suffix);
        return;
      }

      if ('value' in check) {
        if (typeof check.value === 'function') {
          try {
            check.value(assignment.value);
          }
          catch (ex) {
            assert.ok(false, '' + ex);
          }
        }
        else {
          assert.is(assignment.value,
                    check.value,
                    'arg.' + paramName + '.value' + suffix);
        }
      }

      if ('name' in check) {
        assert.is(assignment.value.name,
                  check.name,
                  'arg.' + paramName + '.name' + suffix);
      }

      if ('type' in check) {
        assert.is(assignment.arg.type,
                  check.type,
                  'arg.' + paramName + '.type' + suffix);
      }

      if ('arg' in check) {
        assert.is(assignment.arg.toString(),
                  check.arg,
                  'arg.' + paramName + '.arg' + suffix);
      }

      if ('status' in check) {
        assert.is(assignment.getStatus().toString(),
                  check.status,
                  'arg.' + paramName + '.status' + suffix);
      }

      if (!options.isNoDom && 'message' in check) {
        if (typeof check.message.test === 'function') {
          assert.ok(check.message.test(assignment.message),
                    'arg.' + paramName + '.message' + suffix);
        }
        else {
          assert.is(assignment.message,
                    check.message,
                    'arg.' + paramName + '.message' + suffix);
        }
      }
    });
  }

  return promise.all(outstanding).then(function() {
    
    return undefined;
  });
};








helpers._exec = function(options, name, expected) {
  var requisition = options.requisition;
  if (expected == null) {
    return promise.resolve({});
  }

  var origLogErrors = cli.logErrors;
  if (expected.error) {
    cli.logErrors = false;
  }

  try {
    return requisition.exec({ hidden: true }).then(function(output) {
      if ('type' in expected) {
        assert.is(output.type,
                  expected.type,
                  'output.type for: ' + name);
      }

      if ('error' in expected) {
        assert.is(output.error,
                  expected.error,
                  'output.error for: ' + name);
      }

      if (!('output' in expected)) {
        return { output: output };
      }

      var context = requisition.conversionContext;
      var convertPromise;
      if (options.isNoDom) {
        convertPromise = output.convert('string', context);
      }
      else {
        convertPromise = output.convert('dom', context).then(function(node) {
          return node.textContent.trim();
        });
      }

      return convertPromise.then(function(textOutput) {
        var doTest = function(match, against) {
          
          if (against.match(match) != null) {
            assert.ok(true, 'html output for \'' + name + '\' ' +
                            'should match /' + (match.source || match) + '/');
          } else {
            assert.ok(false, 'html output for \'' + name + '\' ' +
                             'should match /' + (match.source || match) + '/. ' +
                             'Actual textContent: "' + against + '"');
          }
        };

        if (typeof expected.output === 'string') {
          assert.is(textOutput,
                    expected.output,
                    'html output for ' + name);
        }
        else if (Array.isArray(expected.output)) {
          expected.output.forEach(function(match) {
            doTest(match, textOutput);
          });
        }
        else {
          doTest(expected.output, textOutput);
        }

        if (expected.error) {
          cli.logErrors = origLogErrors;
        }
        return { output: output, text: textOutput };
      });
    }.bind(this)).then(function(data) {
      if (expected.error) {
        cli.logErrors = origLogErrors;
      }

      return data;
    });
  }
  catch (ex) {
    assert.ok(false, 'Failure executing \'' + name + '\': ' + ex);
    util.errorHandler(ex);

    if (expected.error) {
      cli.logErrors = origLogErrors;
    }
    return promise.resolve({});
  }
};




helpers._setup = function(options, name, audit) {
  if (typeof audit.setup === 'string') {
    return helpers.setInput(options, audit.setup);
  }

  if (typeof audit.setup === 'function') {
    return promise.resolve(audit.setup.call(audit));
  }

  return promise.reject('\'setup\' property must be a string or a function. Is ' + audit.setup);
};




helpers._post = function(name, audit, data) {
  if (typeof audit.post === 'function') {
    return promise.resolve(audit.post.call(audit, data.output, data.text));
  }
  return promise.resolve(audit.post);
};




var totalResponseTime = 0;
var averageOver = 0;
var maxResponseTime = 0;
var maxResponseCulprit;
var start;




helpers.resetResponseTimes = function() {
  start = new Date().getTime();
  totalResponseTime = 0;
  averageOver = 0;
  maxResponseTime = 0;
  maxResponseCulprit = undefined;
};




Object.defineProperty(helpers, 'averageResponseTime', {
  get: function() {
    return averageOver === 0 ?
        undefined :
        Math.round(100 * totalResponseTime / averageOver) / 100;
  },
  enumerable: true
});




Object.defineProperty(helpers, 'maxResponseTime', {
  get: function() { return Math.round(maxResponseTime * 100) / 100; },
  enumerable: true
});




Object.defineProperty(helpers, 'maxResponseCulprit', {
  get: function() { return maxResponseCulprit; },
  enumerable: true
});




Object.defineProperty(helpers, 'timingSummary', {
  get: function() {
    var elapsed = (new Date().getTime() - start) / 1000;
    return 'Total ' + elapsed + 's, ' +
           'ave response ' + helpers.averageResponseTime + 'ms, ' +
           'max response ' + helpers.maxResponseTime + 'ms ' +
           'from \'' + helpers.maxResponseCulprit + '\'';
  },
  enumerable: true
});


















































helpers.audit = function(options, audits) {
  checkOptions(options);
  var skipReason = null;
  return util.promiseEach(audits, function(audit) {
    var name = audit.name;
    if (name == null && typeof audit.setup === 'string') {
      name = audit.setup;
    }

    if (assert.testLogging) {
      log('- START \'' + name + '\' in ' + assert.currentTest);
    }

    if (audit.skipRemainingIf) {
      var skipRemainingIf = (typeof audit.skipRemainingIf === 'function') ?
          audit.skipRemainingIf(options) :
          !!audit.skipRemainingIf;
      if (skipRemainingIf) {
        skipReason = audit.skipRemainingIf.name ?
            'due to ' + audit.skipRemainingIf.name :
            '';
        assert.log('Skipped ' + name + ' ' + skipReason);
        return promise.resolve(undefined);
      }
    }

    if (audit.skipIf) {
      var skip = (typeof audit.skipIf === 'function') ?
          audit.skipIf(options) :
          !!audit.skipIf;
      if (skip) {
        var reason = audit.skipIf.name ? 'due to ' + audit.skipIf.name : '';
        assert.log('Skipped ' + name + ' ' + reason);
        return promise.resolve(undefined);
      }
    }

    if (skipReason != null) {
      assert.log('Skipped ' + name + ' ' + skipReason);
      return promise.resolve(undefined);
    }

    var start = new Date().getTime();

    var setupDone = helpers._setup(options, name, audit);
    return setupDone.then(function(chunkLen) {
      if (typeof chunkLen !== 'number') {
        chunkLen = 1;
      }

      
      
      if (chunkLen === -1) {
        assert.log('Skipped ' + name + ' ' + skipReason);
        return promise.resolve(undefined);
      }

      if (assert.currentTest) {
        var responseTime = (new Date().getTime() - start) / chunkLen;
        totalResponseTime += responseTime;
        if (responseTime > maxResponseTime) {
          maxResponseTime = responseTime;
          maxResponseCulprit = assert.currentTest + '/' + name;
        }
        averageOver++;
      }

      var checkDone = helpers._check(options, name, audit.check);
      return checkDone.then(function() {
        var execDone = helpers._exec(options, name, audit.exec);
        return execDone.then(function(data) {
          return helpers._post(name, audit, data).then(function() {
            if (assert.testLogging) {
              log('- END \'' + name + '\' in ' + assert.currentTest);
            }
          });
        });
      });
    });
  }).then(function() {
    return options.automator.setInput('');
  });
};




helpers.arrayIs = function(actual, expected, message) {
  assert.ok(Array.isArray(actual), 'actual is not an array: ' + message);
  assert.ok(Array.isArray(expected), 'expected is not an array: ' + message);

  if (!Array.isArray(actual) || !Array.isArray(expected)) {
    return;
  }

  assert.is(actual.length, expected.length, 'array length: ' + message);

  for (var i = 0; i < actual.length && i < expected.length; i++) {
    assert.is(actual[i], expected[i], 'member[' + i + ']: ' + message);
  }
};




function log(message) {
  if (typeof info === 'function') {
    info(message);
  }
  else {
    console.log(message);
  }
}
