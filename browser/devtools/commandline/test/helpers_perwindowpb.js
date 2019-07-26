























































this.EXPORTED_SYMBOLS = [ 'helpers_perwindowpb' ];

var test = { };







let DeveloperToolbarTestPW = { };




DeveloperToolbarTestPW.show = function DTTPW_show(aWindow, aCallback) {
  if (aWindow.DeveloperToolbar.visible) {
    ok(false, "DeveloperToolbar.visible at start of openDeveloperToolbar");
  }
  else {
    aWindow.DeveloperToolbar.show(true, aCallback);
  }
};




DeveloperToolbarTestPW.hide = function DTTPW_hide(aWindow) {
  if (!aWindow.DeveloperToolbar.visible) {
    ok(false, "!DeveloperToolbar.visible at start of closeDeveloperToolbar");
  }
  else {
    aWindow.DeveloperToolbar.display.inputter.setInput("");
    aWindow.DeveloperToolbar.hide();
  }
};
























DeveloperToolbarTestPW.checkInputStatus = function DTTPW_checkInputStatus(aWindow, checks) {
  if (!checks.emptyParameters) {
    checks.emptyParameters = [];
  }
  if (!checks.directTabText) {
    checks.directTabText = '';
  }
  if (!checks.arrowTabText) {
    checks.arrowTabText = '';
  }

  var display = aWindow.DeveloperToolbar.display;

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















DeveloperToolbarTestPW.exec = function DTTPW_exec(aWindow, tests) {
  tests = tests || {};

  if (tests.typed) {
    aWindow.DeveloperToolbar.display.inputter.setInput(tests.typed);
  }

  let typed = aWindow.DeveloperToolbar.display.inputter.getInputState().typed;
  let output = aWindow.DeveloperToolbar.display.requisition.exec();

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

  let displayed = aWindow.DeveloperToolbar.outputPanel._div.textContent;

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
















DeveloperToolbarTestPW.test = function DTTPW_test(aWindow, uri, target, isGcli, aCallback) {
  let menuItem = aWindow.document.getElementById("menu_devToolbar");
  let command = aWindow.document.getElementById("Tools:DevToolbar");
  let appMenuItem = aWindow.document.getElementById("appmenu_devToolbar");

  function cleanup() {
    DeveloperToolbarTestPW.hide(aWindow);

    
    if (menuItem) {
      menuItem.hidden = true;
    }
    if (command) {
      command.setAttribute("disabled", "true");
    }
    if (appMenuItem) {
      appMenuItem.hidden = true;
    }
    
  };

  
  if (menuItem) {
    menuItem.hidden = false;
  }
  if (command) {
    command.removeAttribute("disabled");
  }
  if (appMenuItem) {
    appMenuItem.hidden = false;
  }

  let browser = aWindow.gBrowser.selectedBrowser;
  browser.addEventListener("load", function onTabLoad() {
    if (aWindow.content.location.href != uri) {
      browser.loadURI(uri);
      return;
    }
    browser.removeEventListener("load", onTabLoad, true);

    DeveloperToolbarTestPW.show(aWindow, function() {
      var options = {
        window: aWindow.content,
        display: aWindow.DeveloperToolbar.display,
        isFirefox: true
      };

      if (helpers_perwindowpb && !isGcli) {
        helpers_perwindowpb.setup(options);
      }

      if (Array.isArray(target)) {
        try {
          var args = isGcli ? options : aWindow;
          var targetCount = 0;
          function callTarget() {
            if (targetCount < target.length) {
              info("Running target test: " + targetCount);
              target[targetCount++](args, callTarget);
            } else {
              info("Clean up and continue test");
              cleanup();
              aCallback();
            }
          }
          callTarget();
        } catch (ex) {
          ok(false, "" + ex);
          DeveloperToolbarTestPW._finish(aWindow);
          throw ex;
        }
      } else {
        try {
          target(aWindow, function() {
            info("Clean up and continue test");
            cleanup();
            aCallback();
          });
        } catch (ex) {
          ok(false, "" + ex);
          DeveloperToolbarTestPW._finish(aWindow);
          throw ex;
        }
      }
    });
  }, true);

  browser.loadURI(uri);
};

DeveloperToolbarTestPW._outstanding = [];

DeveloperToolbarTestPW._checkFinish = function(aWindow) {
  info('_checkFinish. ' + DeveloperToolbarTestPW._outstanding.length + ' outstanding');
  if (DeveloperToolbarTestPW._outstanding.length == 0) {
    DeveloperToolbarTestPW._finish(aWindow);
  }
}

DeveloperToolbarTestPW._finish = function(aWindow) {
  info('Finish');
  DeveloperToolbarTestPW.closeAllTabs(aWindow);
  finish();
}

DeveloperToolbarTestPW.checkCalled = function(aWindow, aFunc, aScope) {
  var todo = function() {
    var reply = aFunc.apply(aScope, arguments);
    DeveloperToolbarTestPW._outstanding = DeveloperToolbarTestPW._outstanding.filter(function(aJob) {
      return aJob != todo;
    });
    DeveloperToolbarTestPW._checkFinish(aWindow);
    return reply;
  }
  DeveloperToolbarTestPW._outstanding.push(todo);
  return todo;
};

DeveloperToolbarTestPW.checkNotCalled = function(aMsg, aFunc, aScope) {
  return function() {
    ok(false, aMsg);
    return aFunc.apply(aScope, arguments);
  }
};




DeveloperToolbarTestPW.closeAllTabs = function(aWindow) {
  while (aWindow.gBrowser.tabs.length > 1) {
    aWindow.gBrowser.removeCurrentTab();
  }
};



this.helpers_perwindowpb = {};

var assert = { ok: ok, is: is, log: info };

helpers_perwindowpb._display = undefined;
helpers_perwindowpb._options = undefined;

helpers_perwindowpb.setup = function(options) {
  helpers_perwindowpb._options = options;
  helpers_perwindowpb._display = options.display;
};

helpers_perwindowpb.shutdown = function(options) {
  helpers_perwindowpb._options = undefined;
  helpers_perwindowpb._display = undefined;
};




helpers_perwindowpb._actual = {
  input: function() {
    return helpers_perwindowpb._display.inputter.element.value;
  },

  hints: function() {
    var templateData = helpers_perwindowpb._display.completer._getCompleterTemplateData();
    var actualHints = templateData.directTabText +
                      templateData.emptyParameters.join('') +
                      templateData.arrowTabText;
    return actualHints.replace(/\u00a0/g, ' ')
                      .replace(/\u21E5/, '->')
                      .replace(/ $/, '');
  },

  markup: function() {
    var cursor = helpers_perwindowpb._display.inputter.element.selectionStart;
    var statusMarkup = helpers_perwindowpb._display.requisition.getInputStatusMarkup(cursor);
    return statusMarkup.map(function(s) {
      return Array(s.string.length + 1).join(s.status.toString()[0]);
    }).join('');
  },

  cursor: function() {
    return helpers_perwindowpb._display.inputter.element.selectionStart;
  },

  current: function() {
    return helpers_perwindowpb._display.requisition.getAssignmentAt(helpers_perwindowpb._actual.cursor()).param.name;
  },

  status: function() {
    return helpers_perwindowpb._display.requisition.getStatus().toString();
  },

  outputState: function() {
    var outputData = helpers_perwindowpb._display.focusManager._shouldShowOutput();
    return outputData.visible + ':' + outputData.reason;
  },

  tooltipState: function() {
    var tooltipData = helpers_perwindowpb._display.focusManager._shouldShowTooltip();
    return tooltipData.visible + ':' + tooltipData.reason;
  }
};

helpers_perwindowpb._directToString = [ 'boolean', 'undefined', 'number' ];

helpers_perwindowpb._createDebugCheck = function() {
  var requisition = helpers_perwindowpb._display.requisition;
  var command = requisition.commandAssignment.value;
  var input = helpers_perwindowpb._actual.input();
  var padding = Array(input.length + 1).join(' ');

  var output = '';
  output += 'helpers_perwindowpb.setInput(\'' + input + '\');\n';
  output += 'helpers_perwindowpb.check({\n';
  output += '  input:  \'' + input + '\',\n';
  output += '  hints:  ' + padding + '\'' + helpers_perwindowpb._actual.hints() + '\',\n';
  output += '  markup: \'' + helpers_perwindowpb._actual.markup() + '\',\n';
  output += '  cursor: ' + helpers_perwindowpb._actual.cursor() + ',\n';
  output += '  current: \'' + helpers_perwindowpb._actual.current() + '\',\n';
  output += '  status: \'' + helpers_perwindowpb._actual.status() + '\',\n';
  output += '  outputState: \'' + helpers_perwindowpb._actual.outputState() + '\',\n';

  if (command) {
    output += '  tooltipState: \'' + helpers_perwindowpb._actual.tooltipState() + '\',\n';
    output += '  args: {\n';
    output += '    command: { name: \'' + command.name + '\' },\n';

    requisition.getAssignments().forEach(function(assignment) {
      output += '    ' + assignment.param.name + ': { ';

      if (typeof assignment.value === 'string') {
        output += 'value: \'' + assignment.value + '\', ';
      }
      else if (helpers_perwindowpb._directToString.indexOf(typeof assignment.value) !== -1) {
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
    output += '  tooltipState: \'' + helpers_perwindowpb._actual.tooltipState() + '\'\n';
  }
  output += '});';

  return output;
};





helpers_perwindowpb.setInput = function(typed, cursor) {
  helpers_perwindowpb._display.inputter.setInput(typed);

  if (cursor) {
    helpers_perwindowpb._display.inputter.setCursor({ start: cursor, end: cursor });
  }
  else {
    
    
    if (helpers_perwindowpb._options.isJsdom) {
      helpers_perwindowpb._display.inputter.setCursor({
        start: typed.length,
        end: typed.length
      });
    }
  }

  helpers_perwindowpb._display.focusManager.onInputChange();

  
  if (helpers_perwindowpb._options.isFirefox) {
    assert.log('setInput("' + typed + '"' + (cursor == null ? '' : ', ' + cursor) + ')');
  }
};




helpers_perwindowpb.focusInput = function() {
  helpers_perwindowpb._display.inputter.focus();
};




helpers_perwindowpb.pressTab = function() {
  helpers_perwindowpb.pressKey(9 );
};




helpers_perwindowpb.pressReturn = function() {
  helpers_perwindowpb.pressKey(13 );
};




helpers_perwindowpb.pressKey = function(keyCode) {
  var fakeEvent = {
    keyCode: keyCode,
    preventDefault: function() { },
    timeStamp: new Date().getTime()
  };
  helpers_perwindowpb._display.inputter.onKeyDown(fakeEvent);
  helpers_perwindowpb._display.inputter.onKeyUp(fakeEvent);
};























helpers_perwindowpb.check = function(checks) {
  if ('input' in checks) {
    assert.is(helpers_perwindowpb._actual.input(), checks.input, 'input');
  }

  if ('cursor' in checks) {
    assert.is(helpers_perwindowpb._actual.cursor(), checks.cursor, 'cursor');
  }

  if ('current' in checks) {
    assert.is(helpers_perwindowpb._actual.current(), checks.current, 'current');
  }

  if ('status' in checks) {
    assert.is(helpers_perwindowpb._actual.status(), checks.status, 'status');
  }

  if ('markup' in checks) {
    assert.is(helpers_perwindowpb._actual.markup(), checks.markup, 'markup');
  }

  if ('hints' in checks) {
    assert.is(helpers_perwindowpb._actual.hints(), checks.hints, 'hints');
  }

  if ('tooltipState' in checks) {
    assert.is(helpers_perwindowpb._actual.tooltipState(), checks.tooltipState, 'tooltipState');
  }

  if ('outputState' in checks) {
    assert.is(helpers_perwindowpb._actual.outputState(), checks.outputState, 'outputState');
  }

  if (checks.args != null) {
    var requisition = helpers_perwindowpb._display.requisition;
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
        assert.ok(false, 'Unknown arg: ' + paramName);
        return;
      }

      if ('value' in check) {
        assert.is(assignment.value,
                check.value,
                'arg.' + paramName + '.value');
      }

      if ('name' in check) {
        assert.is(assignment.value.name,
                check.name,
                'arg.' + paramName + '.name');
      }

      if ('type' in check) {
        assert.is(assignment.arg.type,
                check.type,
                'arg.' + paramName + '.type');
      }

      if ('arg' in check) {
        assert.is(assignment.arg.toString(),
                check.arg,
                'arg.' + paramName + '.arg');
      }

      if ('status' in check) {
        assert.is(assignment.getStatus().toString(),
                check.status,
                'arg.' + paramName + '.status');
      }

      if ('message' in check) {
        assert.is(assignment.getMessage(),
                check.message,
                'arg.' + paramName + '.message');
      }
    });
  }
};














helpers_perwindowpb.exec = function(tests) {
  var requisition = helpers_perwindowpb._display.requisition;
  var inputter = helpers_perwindowpb._display.inputter;

  tests = tests || {};

  if (tests.typed) {
    inputter.setInput(tests.typed);
  }

  var typed = inputter.getInputState().typed;
  var output = requisition.exec({ hidden: true });

  assert.is(typed, output.typed, 'output.command for: ' + typed);

  if (tests.completed !== false) {
    assert.ok(output.completed, 'output.completed false for: ' + typed);
  }
  else {
    
    
    
  }

  if (tests.args != null) {
    assert.is(Object.keys(tests.args).length, Object.keys(output.args).length,
            'arg count for ' + typed);

    Object.keys(output.args).forEach(function(arg) {
      var expectedArg = tests.args[arg];
      var actualArg = output.args[arg];

      if (Array.isArray(expectedArg)) {
        if (!Array.isArray(actualArg)) {
          assert.ok(false, 'actual is not an array. ' + typed + '/' + arg);
          return;
        }

        assert.is(expectedArg.length, actualArg.length,
                'array length: ' + typed + '/' + arg);
        for (var i = 0; i < expectedArg.length; i++) {
          assert.is(expectedArg[i], actualArg[i],
                  'member: "' + typed + '/' + arg + '/' + i);
        }
      }
      else {
        assert.is(expectedArg, actualArg, 'typed: "' + typed + '" arg: ' + arg);
      }
    });
  }

  if (!helpers_perwindowpb._options.window.document.createElement) {
    assert.log('skipping output tests (missing doc.createElement) for ' + typed);
    return;
  }

  var div = helpers_perwindowpb._options.window.document.createElement('div');
  output.toDom(div);
  var displayed = div.textContent.trim();

  if (tests.outputMatch) {
    var doTest = function(match, against) {
      if (!match.test(against)) {
        assert.ok(false, "html output for " + typed + " against " + match.source);
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
      assert.ok(false, "html for " + typed + " (textContent sent to info)");
      console.log("Actual textContent");
      console.log(displayed);
    }
  }
};
