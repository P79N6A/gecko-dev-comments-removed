





































let gcli;
let console;
let require;
let define;

(function() {
  let tempScope = {};
  Components.utils.import("resource:///modules/gcli.jsm", tempScope);

  gcli = tempScope.gcli;
  console = gcli._internal.console;
  define = gcli._internal.define;
  require = gcli._internal.require;
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
    aCallback(browser, tab);
  }

  browser.addEventListener("load", onTabLoad, true);
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  console = undefined;
  define = undefined;
  require = undefined;
  gcli = undefined;
});




let DeveloperToolbarTest = {
  


  show: function DTT_show() {
    if (DeveloperToolbar.visible) {
      ok(false, "DeveloperToolbar.visible at start of openDeveloperToolbar");
    }
    else {
      DeveloperToolbar.show();
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

  















  checkInputStatus: function DTT_checkInputStatus(test) {
    if (test.typed) {
      DeveloperToolbar.display.inputter.setInput(test.typed);
    }
    else {
     ok(false, "Missing typed for " + JSON.stringify(test));
     return;
    }

    if (test.cursor) {
      DeveloperToolbar.display.inputter.setCursor(test.cursor)
    }

    if (test.status) {
      is(DeveloperToolbar.display.requisition.getStatus().toString(),
         test.status,
         "status for " + test.typed);
    }

    if (test.emptyParameters == null) {
      test.emptyParameters = [];
    }

    let completer = DeveloperToolbar.display.completer;
    let realParams = completer.emptyParameters;
    is(realParams.length, test.emptyParameters.length,
       'emptyParameters.length for \'' + test.typed + '\'');

    if (realParams.length === test.emptyParameters.length) {
      for (let i = 0; i < realParams.length; i++) {
        is(realParams[i].replace(/\u00a0/g, ' '), test.emptyParameters[i],
           'emptyParameters[' + i + '] for \'' + test.typed + '\'');
      }
    }

    if (test.directTabText) {
      is(completer.directTabText, test.directTabText,
         'directTabText for \'' + test.typed + '\'');
    }
    else {
      is(completer.directTabText, '', 'directTabText for \'' + test.typed + '\'');
    }

    if (test.arrowTabText) {
      is(completer.arrowTabText, ' \u00a0\u21E5 ' + test.arrowTabText,
         'arrowTabText for \'' + test.typed + '\'');
    }
    else {
      is(completer.arrowTabText, '', 'arrowTabText for \'' + test.typed + '\'');
    }
  },

  












  exec: function DTT_exec(test) {
    test = test || {};

    if (test.typed) {
      DeveloperToolbar.display.inputter.setInput(test.typed);
    }

    let typed = DeveloperToolbar.display.inputter.getInputState().typed;
    let output = DeveloperToolbar.display.requisition.exec();

    is(typed, output.typed, 'output.command for: ' + typed);

    if (test.completed !== false) {
      ok(output.completed, 'output.completed false for: ' + typed);
    }
    else {
      
      
      
    }

    if (test.args != null) {
      is(Object.keys(test.args).length, Object.keys(output.args).length,
         'arg count for ' + typed);

      Object.keys(output.args).forEach(function(arg) {
        let expectedArg = test.args[arg];
        let actualArg = output.args[arg];

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
      });
    }

    let displayed = DeveloperToolbar.outputPanel._div.textContent;

    if (test.outputMatch) {
      if (!test.outputMatch.test(displayed)) {
        ok(false, "html output for " + typed + " (textContent sent to info)");
        info("Actual textContent");
        info(displayed);
      }
    }

    if (test.blankOutput != null) {
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
      DeveloperToolbarTest.show();
      testFunc(browser, tab);
    });
  },
};
