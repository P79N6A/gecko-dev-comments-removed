



const TEST_BASE_HTTP = "http://example.com/browser/browser/devtools/commandline/test/";
const TEST_BASE_HTTPS = "https://example.com/browser/browser/devtools/commandline/test/";

let console = (function() {
  let tempScope = {};
  Components.utils.import("resource:///modules/devtools/Console.jsm", tempScope);
  return tempScope.console;
})();




function addTab(aURL, aCallback)
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;

  let tab = gBrowser.selectedTab;
  let browser = gBrowser.getBrowserForTab(tab);

  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);
    aCallback(browser, tab, browser.contentDocument);
  }

  browser.addEventListener("load", onTabLoad, true);
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  console = undefined;
});







let DeveloperToolbarTest = {
  


  show: function DTT_show(aCallback) {
    if (DeveloperToolbar.visible) {
      ok(false, "DeveloperToolbar.visible at start of openDeveloperToolbar");
    }
    else {
      DeveloperToolbar.show(aCallback);
    }
  },

  


  hide: function DTT_hide() {
    if (!DeveloperToolbar.visible) {
      ok(false, "!DeveloperToolbar.visible at start of closeDeveloperToolbar");
    }
    else {
      DeveloperToolbar.display.inputter.setInput("");
      DeveloperToolbar.hide();
    }
  },

  
















  checkInputStatus: function DTT_checkInputStatus(tests) {
    let display = DeveloperToolbar.display;

    if (tests.typed) {
      display.inputter.setInput(tests.typed);
    }
    else {
      ok(false, "Missing typed for " + JSON.stringify(tests));
      return;
    }

    if (tests.cursor) {
      display.inputter.setCursor(tests.cursor)
    }

    if (tests.status) {
      is(display.requisition.getStatus().toString(),
              tests.status, "status for " + tests.typed);
    }

    if (tests.emptyParameters == null) {
      tests.emptyParameters = [];
    }

    let realParams = display.completer.emptyParameters;
    is(realParams.length, tests.emptyParameters.length,
            'emptyParameters.length for \'' + tests.typed + '\'');

    if (realParams.length === tests.emptyParameters.length) {
      for (let i = 0; i < realParams.length; i++) {
        is(realParams[i].replace(/\u00a0/g, ' '), tests.emptyParameters[i],
                'emptyParameters[' + i + '] for \'' + tests.typed + '\'');
      }
    }

    if (tests.directTabText) {
      is(display.completer.directTabText, tests.directTabText,
              'directTabText for \'' + tests.typed + '\'');
    }
    else {
      is(display.completer.directTabText, '',
              'directTabText for \'' + tests.typed + '\'');
    }

    if (tests.arrowTabText) {
      is(display.completer.arrowTabText, ' \u00a0\u21E5 ' + tests.arrowTabText,
              'arrowTabText for \'' + tests.typed + '\'');
    }
    else {
      is(display.completer.arrowTabText, '',
              'arrowTabText for \'' + tests.typed + '\'');
    }

    if (tests.markup) {
      let cursor = tests.cursor ? tests.cursor.start : tests.typed.length;
      let statusMarkup = display.requisition.getInputStatusMarkup(cursor);
      let actualMarkup = statusMarkup.map(function(s) {
        return Array(s.string.length + 1).join(s.status.toString()[0]);
      }).join('');
      is(tests.markup, actualMarkup, 'markup for ' + tests.typed);
    }
  },

  













  exec: function DTT_exec(tests) {
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
      function doTest(match, against) {
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
  },

  












  test: function DTT_test(uri, testFunc) {
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

    addTab(uri, function(browser, tab) {
      DeveloperToolbarTest.show(function() {

        try {
          testFunc(browser, tab);
        }
        catch (ex) {
          ok(false, "" + ex);
          console.error(ex);
          finish();
          throw ex;
        }
      });
    });
  },
};











var noRecurse = [
  /^string$/, /^number$/, /^boolean$/, /^null/, /^undefined/,
  /^Window$/, /^Document$/,
  /^XULDocument$/, /^XULElement$/,
  /^DOMWindow$/, /^HTMLDocument$/, /^HTML.*Element$/
];

var hide = [ /^string$/, /^number$/, /^boolean$/, /^null/, /^undefined/ ];

function leakHunt(root, path, seen) {
  path = path || [];
  seen = seen || [];

  try {
    var output = leakHuntInner(root, path, seen);
    output.forEach(function(line) {
      dump(line + '\n');
    });
  }
  catch (ex) {
    dump(ex + '\n');
  }
}

function leakHuntInner(root, path, seen) {
  var prefix = new Array(path.length).join('  ');

  var reply = [];
  function log(msg) {
    reply.push(msg);
  }

  var direct
  try {
    direct = Object.keys(root);
  }
  catch (ex) {
    log(prefix + '  Error enumerating: ' + ex);
    return reply;
  }

  for (var prop in root) {
    var newPath = path.slice();
    newPath.push(prop);
    prefix = new Array(newPath.length).join('  ');

    var data;
    try {
      data = root[prop];
    }
    catch (ex) {
      log(prefix + prop + '  Error reading: ' + ex);
      continue;
    }

    var recurse = true;
    var message = getType(data);

    if (matchesAnyPattern(message, hide)) {
      continue;
    }

    if (message === 'function' && direct.indexOf(prop) == -1) {
      continue;
    }

    if (message === 'string') {
      var extra = data.length > 10 ? data.substring(0, 9) + '_' : data;
      message += ' "' + extra.replace(/\n/g, "|") + '"';
      recurse = false;
    }
    else if (matchesAnyPattern(message, noRecurse)) {
      message += ' (no recurse)'
      recurse = false;
    }
    else if (seen.indexOf(data) !== -1) {
      message += ' (already seen)';
      recurse = false;
    }

    if (recurse) {
      seen.push(data);
      var lines = leakHuntInner(data, newPath, seen);
      if (lines.length == 0) {
        if (message !== 'function') {
          log(prefix + prop + ' = ' + message + ' { }');
        }
      }
      else {
        log(prefix + prop + ' = ' + message + ' {');
        lines.forEach(function(line) {
          reply.push(line);
        });
        log(prefix + '}');
      }
    }
    else {
      log(prefix + prop + ' = ' + message);
    }
  }

  return reply;
}

function matchesAnyPattern(str, patterns) {
  var match = false;
  patterns.forEach(function(pattern) {
    if (str.match(pattern)) {
      match = true;
    }
  });
  return match;
}

function getType(data) {
  if (data === null) {
    return 'null';
  }
  if (data === undefined) {
    return 'undefined';
  }

  var type = typeof data;
  if (type === 'object' || type === 'Object') {
    type = getCtorName(data);
  }

  return type;
}

function getCtorName(aObj) {
  try {
    if (aObj.constructor && aObj.constructor.name) {
      return aObj.constructor.name;
    }
  }
  catch (ex) {
    return 'UnknownObject';
  }

  
  
  return Object.prototype.toString.call(aObj).slice(8, -1);
}
