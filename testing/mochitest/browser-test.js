
var gTimeoutSeconds = 30;
var gConfig;

if (Cc === undefined) {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  var Cu = Components.utils;
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

window.addEventListener("load", testOnLoad, false);

function testOnLoad() {
  window.removeEventListener("load", testOnLoad, false);

  gConfig = readConfig();
  if (gConfig.testRoot == "browser" ||
      gConfig.testRoot == "metro" ||
      gConfig.testRoot == "webapprtChrome") {
    
    var prefs = Services.prefs;
    if (prefs.prefHasUserValue("testing.browserTestHarness.running"))
      return;

    prefs.setBoolPref("testing.browserTestHarness.running", true);

    if (prefs.prefHasUserValue("testing.browserTestHarness.timeout"))
      gTimeoutSeconds = prefs.getIntPref("testing.browserTestHarness.timeout");

    var sstring = Cc["@mozilla.org/supports-string;1"].
                  createInstance(Ci.nsISupportsString);
    sstring.data = location.search;

    Services.ww.openWindow(window, "chrome://mochikit/content/browser-harness.xul", "browserTest",
                           "chrome,centerscreen,dialog=no,resizable,titlebar,toolbar=no,width=800,height=600", sstring);
  } else {
    
    function messageHandler(m) {
      messageManager.removeMessageListener("chromeEvent", messageHandler);
      var url = m.json.data;

      
      
      var webNav = content.window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                         .getInterface(Components.interfaces.nsIWebNavigation);
      webNav.loadURI(url, null, null, null, null);
    }

    var listener = 'data:,function doLoad(e) { var data=e.getData("data");removeEventListener("contentEvent", function (e) { doLoad(e); }, false, true);sendAsyncMessage("chromeEvent", {"data":data}); };addEventListener("contentEvent", function (e) { doLoad(e); }, false, true);';
    messageManager.loadFrameScript(listener, true);
    messageManager.addMessageListener("chromeEvent", messageHandler);
  }
}

function Tester(aTests, aDumper, aCallback) {
  this.dumper = aDumper;
  this.tests = aTests;
  this.callback = aCallback;
  this.openedWindows = {};
  this.openedURLs = {};

  this._scriptLoader = Services.scriptloader;
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/EventUtils.js", this.EventUtils);
  var simpleTestScope = {};
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/specialpowersAPI.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/SpecialPowersObserverAPI.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromePowers.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/SimpleTest.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/chrome-harness.js", simpleTestScope);
  this.SimpleTest = simpleTestScope.SimpleTest;
}
Tester.prototype = {
  EventUtils: {},
  SimpleTest: {},

  repeat: 0,
  checker: null,
  currentTestIndex: -1,
  lastStartTime: null,
  openedWindows: null,

  get currentTest() {
    return this.tests[this.currentTestIndex];
  },
  get done() {
    return this.currentTestIndex == this.tests.length - 1;
  },

  start: function Tester_start() {
    
    if (window.BrowserChromeTest) {
      BrowserChromeTest.runWhenReady(this.actuallyStart.bind(this));
      return;
    }
    this.actuallyStart();
  },

  actuallyStart: function Tester_actuallyStart() {
    
    if (!gConfig)
      gConfig = readConfig();
    this.repeat = gConfig.repeat;
    this.dumper.dump("*** Start BrowserChrome Test Results ***\n");
    Services.console.registerListener(this);
    Services.obs.addObserver(this, "chrome-document-global-created", false);
    Services.obs.addObserver(this, "content-document-global-created", false);
    this._globalProperties = Object.keys(window);
    this._globalPropertyWhitelist = [
      "navigator", "constructor", "top",
      "Application",
      "__SS_tabsToRestore", "__SSi",
      "webConsoleCommandController",
    ];

    if (this.tests.length)
      this.nextTest();
    else
      this.finish();
  },

  waitForWindowsState: function Tester_waitForWindowsState(aCallback) {
    let timedOut = this.currentTest && this.currentTest.timedOut;
    let baseMsg = timedOut ? "Found a {elt} after previous test timed out"
                           : this.currentTest ? "Found an unexpected {elt} at the end of test run"
                                              : "Found an unexpected {elt}";

    if (this.currentTest && window.gBrowser && gBrowser.tabs.length > 1) {
      while (gBrowser.tabs.length > 1) {
        let lastTab = gBrowser.tabContainer.lastChild;
        let msg = baseMsg.replace("{elt}", "tab") +
                  ": " + lastTab.linkedBrowser.currentURI.spec;
        this.currentTest.addResult(new testResult(false, msg, "", false));
        gBrowser.removeTab(lastTab);
      }
    }

    this.dumper.dump("TEST-INFO | checking window state\n");
    let windowsEnum = Services.wm.getEnumerator(null);
    while (windowsEnum.hasMoreElements()) {
      let win = windowsEnum.getNext();
      if (win != window && !win.closed &&
          win.document.documentElement.getAttribute("id") != "browserTestHarness") {
        let type = win.document.documentElement.getAttribute("windowtype");
        switch (type) {
        case "navigator:browser":
          type = "browser window";
          break;
        case null:
          type = "unknown window";
          break;
        }
        let msg = baseMsg.replace("{elt}", type);
        if (this.currentTest)
          this.currentTest.addResult(new testResult(false, msg, "", false));
        else
          this.dumper.dump("TEST-UNEXPECTED-FAIL | (browser-test.js) | " + msg + "\n");

        win.close();
      }
    }

    
    this.SimpleTest.waitForFocus(aCallback);
  },

  finish: function Tester_finish(aSkipSummary) {
    if (this.repeat > 0) {
      --this.repeat;
      this.currentTestIndex = -1;
      this.nextTest();
    }
    else{
      Services.console.unregisterListener(this);
      Services.obs.removeObserver(this, "chrome-document-global-created");
      Services.obs.removeObserver(this, "content-document-global-created");
  
      this.dumper.dump("\nINFO TEST-START | Shutdown\n");
      if (this.tests.length) {
        this.dumper.dump("Browser Chrome Test Summary\n");
  
        function sum(a,b) a+b;
        var passCount = this.tests.map(function (f) f.passCount).reduce(sum);
        var failCount = this.tests.map(function (f) f.failCount).reduce(sum);
        var todoCount = this.tests.map(function (f) f.todoCount).reduce(sum);
  
        this.dumper.dump("\tPassed: " + passCount + "\n" +
                         "\tFailed: " + failCount + "\n" +
                         "\tTodo: " + todoCount + "\n");
      } else {
        this.dumper.dump("TEST-UNEXPECTED-FAIL | (browser-test.js) | " +
                         "No tests to run. Did you pass an invalid --test-path?\n");
      }
  
      this.dumper.dump("\n*** End BrowserChrome Test Results ***\n");
  
      this.dumper.done();
  
      
      this.callback(this.tests);
      this.callback = null;
      this.tests = null;
      this.openedWindows = null;
    }
  },

  observe: function Tester_observe(aSubject, aTopic, aData) {
    if (!aTopic) {
      this.onConsoleMessage(aSubject);
    } else if (this.currentTest) {
      this.onDocumentCreated(aSubject);
    }
  },

  onDocumentCreated: function Tester_onDocumentCreated(aWindow) {
    let utils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    let outerID = utils.outerWindowID;
    let innerID = utils.currentInnerWindowID;

    if (!(outerID in this.openedWindows)) {
      this.openedWindows[outerID] = this.currentTest;
    }
    this.openedWindows[innerID] = this.currentTest;

    let url = aWindow.location.href || "about:blank";
    this.openedURLs[outerID] = this.openedURLs[innerID] = url;
  },

  onConsoleMessage: function Tester_onConsoleMessage(aConsoleMessage) {
    
    if (!aConsoleMessage.message)
      return;

    try {
      var msg = "Console message: " + aConsoleMessage.message;
      if (this.currentTest)
        this.currentTest.addResult(new testMessage(msg));
      else
        this.dumper.dump("TEST-INFO | (browser-test.js) | " + msg.replace(/\n$/, "") + "\n");
    } catch (ex) {
      
      
    }
  },

  nextTest: function Tester_nextTest() {
    if (this.currentTest) {
      
      
      let testScope = this.currentTest.scope;
      while (testScope.__cleanupFunctions.length > 0) {
        let func = testScope.__cleanupFunctions.shift();
        try {
          func.apply(testScope);
        }
        catch (ex) {
          this.currentTest.addResult(new testResult(false, "Cleanup function threw an exception", ex, false));
        }
      };

      let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      if (winUtils.isTestControllingRefreshes) {
        this.currentTest.addResult(new testResult(false, "test left refresh driver under test control", "", false));
        winUtils.restoreNormalRefresh();
      }

      if (this.SimpleTest.isExpectingUncaughtException()) {
        this.currentTest.addResult(new testResult(false, "expectUncaughtException was called but no uncaught exception was detected!", "", false));
      }

      Object.keys(window).forEach(function (prop) {
        if (parseInt(prop) == prop) {
          
          
          
          
          return;
        }
        if (this._globalProperties.indexOf(prop) == -1) {
          this._globalProperties.push(prop);
          if (this._globalPropertyWhitelist.indexOf(prop) == -1)
            this.currentTest.addResult(new testResult(false, "leaked window property: " + prop, "", false));
        }
      }, this);

      
      
      
      document.popupNode = null;

      
      let time = Date.now() - this.lastStartTime;
      this.dumper.dump("INFO TEST-END | " + this.currentTest.path + " | finished in " + time + "ms\n");
      this.currentTest.setDuration(time);

      testScope.destroy();
      this.currentTest.scope = null;
    }

    
    
    
    this.waitForWindowsState((function () {
      if (this.done) {
        
        
        
        
        
        if (window.gBrowser) {
          gBrowser.addTab();
          gBrowser.removeCurrentTab();
        }

        
        
        Cu.schedulePreciseGC((function () {
          let analyzer = new CCAnalyzer();
          analyzer.run(function () {
            for (let obj of analyzer.find("nsGlobalWindow ")) {
              let m = obj.name.match(/^nsGlobalWindow #(\d+)/);
              if (m && m[1] in this.openedWindows) {
                let test = this.openedWindows[m[1]];
                let msg = "leaked until shutdown [" + obj.name +
                          " " + (this.openedURLs[m[1]] || "NULL") + "]";
                test.addResult(new testResult(false, msg, "", false));
              }
            }

            this.finish();
          }.bind(this));
        }).bind(this));
        return;
      }

      this.currentTestIndex++;
      this.execTest();
    }).bind(this));
  },

  execTest: function Tester_execTest() {
    this.dumper.dump("TEST-START | " + this.currentTest.path + "\n");

    this.SimpleTest.reset();

    
    this.currentTest.scope = new testScope(this, this.currentTest);

    
    this.currentTest.scope.EventUtils = this.EventUtils;
    this.currentTest.scope.SimpleTest = this.SimpleTest;
    this.currentTest.scope.gTestPath = this.currentTest.path;

    
    ["ok", "is", "isnot", "ise", "todo", "todo_is", "todo_isnot", "info"].forEach(function(m) {
      this.SimpleTest[m] = this[m];
    }, this.currentTest.scope);

    
    try {
      this._scriptLoader.loadSubScript("chrome://mochikit/content/chrome-harness.js", this.currentTest.scope);
    } catch (ex) {  }

    
    var currentTestDirPath =
      this.currentTest.path.substr(0, this.currentTest.path.lastIndexOf("/"));
    var headPath = currentTestDirPath + "/head.js";
    try {
      this._scriptLoader.loadSubScript(headPath, this.currentTest.scope);
    } catch (ex) {
      
      
      
      if (ex.toString() != 'Error opening input stream (invalid filename?)') {
       this.currentTest.addResult(new testResult(false, "head.js import threw an exception", ex, false));
      }
    }

    
    try {
      this._scriptLoader.loadSubScript(this.currentTest.path,
                                       this.currentTest.scope);

      
      this.lastStartTime = Date.now();
      if ("generatorTest" in this.currentTest.scope) {
        if ("test" in this.currentTest.scope)
          throw "Cannot run both a generator test and a normal test at the same time.";

        
        this.currentTest.scope.waitForExplicitFinish();
        var result = this.currentTest.scope.generatorTest();
        this.currentTest.scope.__generator = result;
        result.next();
      } else {
        this.currentTest.scope.test();
      }
    } catch (ex) {
      var isExpected = !!this.SimpleTest.isExpectingUncaughtException();
      if (!this.SimpleTest.isIgnoringAllUncaughtExceptions()) {
        this.currentTest.addResult(new testResult(isExpected, "Exception thrown", ex, false));
        this.SimpleTest.expectUncaughtException(false);
      } else {
        this.currentTest.addResult(new testMessage("Exception thrown: " + ex));
      }
      this.currentTest.scope.finish();
    }

    
    
    if (this.currentTest.scope.__done) {
      this.nextTest();
    }
    else {
      var self = this;
      this.currentTest.scope.__waitTimer = setTimeout(function() {
        if (--self.currentTest.scope.__timeoutFactor > 0) {
          
          self.currentTest.scope.info(
            "Longer timeout required, waiting longer...  Remaining timeouts: " +
            self.currentTest.scope.__timeoutFactor);
          self.currentTest.scope.__waitTimer =
            setTimeout(arguments.callee, gTimeoutSeconds * 1000);
          return;
        }
        self.currentTest.addResult(new testResult(false, "Test timed out", "", false));
        self.currentTest.timedOut = true;
        self.currentTest.scope.__waitTimer = null;
        self.nextTest();
      }, gTimeoutSeconds * 1000);
    }
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIConsoleListener) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

function testResult(aCondition, aName, aDiag, aIsTodo, aStack) {
  this.msg = aName || "";

  this.info = false;
  this.pass = !!aCondition;
  this.todo = aIsTodo;

  if (this.pass) {
    if (aIsTodo)
      this.result = "TEST-KNOWN-FAIL";
    else
      this.result = "TEST-PASS";
  } else {
    if (aDiag) {
      if (typeof aDiag == "object" && "fileName" in aDiag) {
        
        this.msg += " at " + aDiag.fileName + ":" + aDiag.lineNumber;
      }
      this.msg += " - " + aDiag;
    }
    if (aStack) {
      this.msg += "\nStack trace:\n";
      var frame = aStack;
      while (frame) {
        this.msg += "    " + frame + "\n";
        frame = frame.caller;
      }
    }
    if (aIsTodo)
      this.result = "TEST-UNEXPECTED-PASS";
    else
      this.result = "TEST-UNEXPECTED-FAIL";
  }
}

function testMessage(aName) {
  this.msg = aName || "";
  this.info = true;
  this.result = "TEST-INFO";
}



function testScope(aTester, aTest) {
  this.__tester = aTester;

  var self = this;
  this.ok = function test_ok(condition, name, diag, stack) {
    aTest.addResult(new testResult(condition, name, diag, false,
                                   stack ? stack : Components.stack.caller));
  };
  this.is = function test_is(a, b, name) {
    self.ok(a == b, name, "Got " + a + ", expected " + b, false,
            Components.stack.caller);
  };
  this.isnot = function test_isnot(a, b, name) {
    self.ok(a != b, name, "Didn't expect " + a + ", but got it", false,
            Components.stack.caller);
  };
  this.ise = function test_ise(a, b, name) {
    self.ok(a === b, name, "Got " + a + ", strictly expected " + b, false,
            Components.stack.caller);
  };
  this.todo = function test_todo(condition, name, diag, stack) {
    aTest.addResult(new testResult(!condition, name, diag, true,
                                   stack ? stack : Components.stack.caller));
  };
  this.todo_is = function test_todo_is(a, b, name) {
    self.todo(a == b, name, "Got " + a + ", expected " + b,
              Components.stack.caller);
  };
  this.todo_isnot = function test_todo_isnot(a, b, name) {
    self.todo(a != b, name, "Didn't expect " + a + ", but got it",
              Components.stack.caller);
  };
  this.info = function test_info(name) {
    aTest.addResult(new testMessage(name));
  };

  this.executeSoon = function test_executeSoon(func) {
    Services.tm.mainThread.dispatch({
      run: function() {
        func();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  };

  this.nextStep = function test_nextStep(arg) {
    if (self.__done) {
      aTest.addResult(new testResult(false, "nextStep was called too many times", "", false));
      return;
    }

    if (!self.__generator) {
      aTest.addResult(new testResult(false, "nextStep called with no generator", "", false));
      self.finish();
      return;
    }

    try {
      self.__generator.send(arg);
    } catch (ex if ex instanceof StopIteration) {
      
      self.finish();
    } catch (ex) {
      var isExpected = !!self.SimpleTest.isExpectingUncaughtException();
      if (!self.SimpleTest.isIgnoringAllUncaughtExceptions()) {
        aTest.addResult(new testResult(isExpected, "Exception thrown", ex, false));
        self.SimpleTest.expectUncaughtException(false);
      } else {
        aTest.addResult(new testMessage("Exception thrown: " + ex));
      }
      self.finish();
    }
  };

  this.waitForExplicitFinish = function test_waitForExplicitFinish() {
    self.__done = false;
  };

  this.waitForFocus = function test_waitForFocus(callback, targetWindow, expectBlankPage) {
    self.SimpleTest.waitForFocus(callback, targetWindow, expectBlankPage);
  };

  this.waitForClipboard = function test_waitForClipboard(expected, setup, success, failure, flavor) {
    self.SimpleTest.waitForClipboard(expected, setup, success, failure, flavor);
  };

  this.registerCleanupFunction = function test_registerCleanupFunction(aFunction) {
    self.__cleanupFunctions.push(aFunction);
  };

  this.requestLongerTimeout = function test_requestLongerTimeout(aFactor) {
    self.__timeoutFactor = aFactor;
  };

  this.copyToProfile = function test_copyToProfile(filename) {
    self.SimpleTest.copyToProfile(filename);
  };

  this.expectUncaughtException = function test_expectUncaughtException(aExpecting) {
    self.SimpleTest.expectUncaughtException(aExpecting);
  };

  this.ignoreAllUncaughtExceptions = function test_ignoreAllUncaughtExceptions(aIgnoring) {
    self.SimpleTest.ignoreAllUncaughtExceptions(aIgnoring);
  };

  this.finish = function test_finish() {
    self.__done = true;
    if (self.__waitTimer) {
      self.executeSoon(function() {
        if (self.__done && self.__waitTimer) {
          clearTimeout(self.__waitTimer);
          self.__waitTimer = null;
          self.__tester.nextTest();
        }
      });
    }
  };
}
testScope.prototype = {
  __done: true,
  __generator: null,
  __waitTimer: null,
  __cleanupFunctions: [],
  __timeoutFactor: 1,

  EventUtils: {},
  SimpleTest: {},

  destroy: function test_destroy() {
    for (let prop in this)
      delete this[prop];
  }
};
