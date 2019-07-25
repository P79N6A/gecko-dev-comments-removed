























































var EXPORTED_SYMBOLS = [ 'helpers' ];

var test = { };







let DeveloperToolbarTest = { };




DeveloperToolbarTest.show = function DTT_show(aCallback) {
  if (DeveloperToolbar.visible) {
    ok(false, "DeveloperToolbar.visible at start of openDeveloperToolbar");
  }
  else {
    DeveloperToolbar.show(true, aCallback);
  }
};




DeveloperToolbarTest.hide = function DTT_hide() {
  if (!DeveloperToolbar.visible) {
    ok(false, "!DeveloperToolbar.visible at start of closeDeveloperToolbar");
  }
  else {
    DeveloperToolbar.display.inputter.setInput("");
    DeveloperToolbar.hide();
  }
};
























DeveloperToolbarTest.checkInputStatus = function DTT_checkInputStatus(checks) {
  if (!checks.emptyParameters) {
    checks.emptyParameters = [];
  }
  if (!checks.directTabText) {
    checks.directTabText = '';
  }
  if (!checks.arrowTabText) {
    checks.arrowTabText = '';
  }

  var display = DeveloperToolbar.display;

  if (checks.typed) {
    info('Starting tests for ' + checks.typed);
    display.inputter.setInput(checks.typed);
  }
  else {
    ok(false, "Missing typed for " + JSON.stringify(checks));
    return;
  }

  if (checks.cursor) {
    display.inputter.setCursor(checks.cursor)
  }

  var cursor = checks.cursor ? checks.cursor.start : checks.typed.length;

  var requisition = display.requisition;
  var completer = display.completer;
  var actual = completer._getCompleterTemplateData();

  













  if (checks.status) {
    is(requisition.getStatus().toString(),
            checks.status,
            'status');
  }

  if (checks.markup) {
    var statusMarkup = requisition.getInputStatusMarkup(cursor);
    var actualMarkup = statusMarkup.map(function(s) {
      return Array(s.string.length + 1).join(s.status.toString()[0]);
    }).join('');

    is(checks.markup,
            actualMarkup,
            'markup');
  }

  if (checks.emptyParameters) {
    var actualParams = actual.emptyParameters;
    is(actualParams.length,
            checks.emptyParameters.length,
            'emptyParameters.length');

    if (actualParams.length === checks.emptyParameters.length) {
      for (var i = 0; i < actualParams.length; i++) {
        is(actualParams[i].replace(/\u00a0/g, ' '),
                checks.emptyParameters[i],
                'emptyParameters[' + i + ']');
      }
    }
    else {
      info('Expected: [ \"' + actualParams.join('", "') + '" ]');
    }
  }

  if (checks.directTabText) {
    is(actual.directTabText,
            checks.directTabText,
            'directTabText');
  }

  if (checks.arrowTabText) {
    is(actual.arrowTabText,
            ' \u00a0\u21E5 ' + checks.arrowTabText,
            'arrowTabText');
  }

  if (checks.args) {
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
        ok(false, 'Unknown parameter: ' + paramName);
        return;
      }

      if (check.value) {
        is(assignment.value,
                check.value,
                'checkStatus value for ' + paramName);
      }

      if (check.name) {
        is(assignment.value.name,
                check.name,
                'checkStatus name for ' + paramName);
      }

      if (check.type) {
        is(assignment.arg.type,
                check.type,
                'checkStatus type for ' + paramName);
      }

      if (check.arg) {
        is(assignment.arg.toString(),
                check.arg,
                'checkStatus arg for ' + paramName);
      }

      if (check.status) {
        is(assignment.getStatus().toString(),
                check.status,
                'checkStatus status for ' + paramName);
      }

      if (check.message) {
        is(assignment.getMessage(),
                check.message,
                'checkStatus message for ' + paramName);
      }
    });
  }
};















DeveloperToolbarTest.exec = function DTT_exec(tests) {
  tests = tests || {};

  if (tests.typed) {
    DeveloperToolbar.display.inputter.setInput(tests.typed);
  }

  let typed = DeveloperToolbar.display.inputter.getInputState().typed;
  let output = DeveloperToolbar.display.requisition.exec();

  is(typed, output.typed, 'output.command for: ' + typed);

  if (tests.completed !== false) {
    ok(output.completed, 'output.completed false for: ' + typed);
  }
  else {
    
    
    
  }

  if (tests.args != null) {
    is(Object.keys(tests.args).length, Object.keys(output.args).length,
       'arg count for ' + typed);

    Object.keys(output.args).forEach(function(arg) {
      let expectedArg = tests.args[arg];
      let actualArg = output.args[arg];

      if (typeof expectedArg === 'function') {
        ok(expectedArg(actualArg), 'failed test func. ' + typed + '/' + arg);
      }
      else {
        if (Array.isArray(expectedArg)) {
          if (!Array.isArray(actualArg)) {
            ok(false, 'actual is not an array. ' + typed + '/' + arg);
            return;
          }

          is(expectedArg.length, actualArg.length,
                  'array length: ' + typed + '/' + arg);
          for (let i = 0; i < expectedArg.length; i++) {
            is(expectedArg[i], actualArg[i],
                    'member: "' + typed + '/' + arg + '/' + i);
          }
        }
        else {
          is(expectedArg, actualArg, 'typed: "' + typed + '" arg: ' + arg);
        }
      }
    });
  }

  let displayed = DeveloperToolbar.outputPanel._div.textContent;

  if (tests.outputMatch) {
    var doTest = function(match, against) {
      if (!match.test(against)) {
        ok(false, "html output for " + typed + " against " + match.source +
                " (textContent sent to info)");
        info("Actual textContent");
        info(against);
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
      ok(false, "html output for " + typed + " (textContent sent to info)");
      info("Actual textContent");
      info(displayed);
    }
  }
};
















DeveloperToolbarTest.test = function DTT_test(uri, target) {
  let menuItem = document.getElementById("menu_devToolbar");
  let command = document.getElementById("Tools:DevToolbar");
  let appMenuItem = document.getElementById("appmenu_devToolbar");

  registerCleanupFunction(function() {
    DeveloperToolbarTest.hide();

    
    if (menuItem) {
      menuItem.hidden = true;
    }
    if (command) {
      command.setAttribute("disabled", "true");
    }
    if (appMenuItem) {
      appMenuItem.hidden = true;
    }

    
  });

  
  if (menuItem) {
    menuItem.hidden = false;
  }
  if (command) {
    command.removeAttribute("disabled");
  }
  if (appMenuItem) {
    appMenuItem.hidden = false;
  }

  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = uri;

  let tab = gBrowser.selectedTab;
  let browser = gBrowser.getBrowserForTab(tab);

  var onTabLoad = function() {
    browser.removeEventListener("load", onTabLoad, true);

    DeveloperToolbarTest.show(function() {
      if (helpers) {
        helpers.setup({ display: DeveloperToolbar.display });
      }

      if (Array.isArray(target)) {
        try {
          target.forEach(function(func) {
            func(browser, tab);
          })
        }
        finally {
          DeveloperToolbarTest._checkFinish();
        }
      }
      else {
        try {
          target(browser, tab);
        }
        catch (ex) {
          ok(false, "" + ex);
          DeveloperToolbarTest._finish();
          throw ex;
        }
      }
    });
  }

  browser.addEventListener("load", onTabLoad, true);
};

DeveloperToolbarTest._outstanding = [];

DeveloperToolbarTest._checkFinish = function() {
  info('_checkFinish. ' + DeveloperToolbarTest._outstanding.length + ' outstanding');
  if (DeveloperToolbarTest._outstanding.length == 0) {
    DeveloperToolbarTest._finish();
  }
}

DeveloperToolbarTest._finish = function() {
  info('Finish');
  DeveloperToolbarTest.closeAllTabs();
  finish();
}

DeveloperToolbarTest.checkCalled = function(aFunc, aScope) {
  var todo = function() {
    var reply = aFunc.apply(aScope, arguments);
    DeveloperToolbarTest._outstanding = DeveloperToolbarTest._outstanding.filter(function(aJob) {
      return aJob != todo;
    });
    DeveloperToolbarTest._checkFinish();
    return reply;
  }
  DeveloperToolbarTest._outstanding.push(todo);
  return todo;
};

DeveloperToolbarTest.checkNotCalled = function(aMsg, aFunc, aScope) {
  return function() {
    ok(false, aMsg);
    return aFunc.apply(aScope, arguments);
  }
};




DeveloperToolbarTest.closeAllTabs = function() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
};



var helpers = {};

helpers._display = undefined;

helpers.setup = function(options) {
  helpers._display = options.display;
  if (typeof ok !== 'undefined') {
    test.ok = ok;
    test.is = is;
    test.log = info;
  }
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

  test.log('setInput("' + typed + '"' + (cursor == null ? '' : ', ' + cursor) + ')');
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














helpers.exec = function(tests) {
  var requisition = helpers._display.requisition;
  var inputter = helpers._display.inputter;

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
