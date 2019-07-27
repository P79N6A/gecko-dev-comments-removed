

var gTimeoutSeconds = 45;
var gConfig;

if (Cc === undefined) {
  var Cc = Components.classes;
}
if (Ci === undefined) {
  var Ci = Components.interfaces;
}
if (Cu === undefined) {
  var Cu = Components.utils;
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizationTabPreloader",
  "resource:///modules/CustomizationTabPreloader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ContentSearch",
  "resource:///modules/ContentSearch.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SelfSupportBackend",
  "resource:///modules/SelfSupportBackend.jsm");

const SIMPLETEST_OVERRIDES =
  ["ok", "is", "isnot", "ise", "todo", "todo_is", "todo_isnot", "info", "expectAssertions", "requestCompleteLog"];

window.addEventListener("load", function testOnLoad() {
  window.removeEventListener("load", testOnLoad);
  window.addEventListener("MozAfterPaint", function testOnMozAfterPaint() {
    window.removeEventListener("MozAfterPaint", testOnMozAfterPaint);
    setTimeout(testInit, 0);
  });
});

function b2gStart() {
  let homescreen = document.getElementById('systemapp');
  var webNav = homescreen.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIWebNavigation);
  var url = "chrome://mochikit/content/harness.xul?manifestFile=tests.json";

  webNav.loadURI(url, null, null, null, null);
}

let TabDestroyObserver = {
  outstanding: new Set(),
  promiseResolver: null,

  init: function() {
    Services.obs.addObserver(this, "message-manager-close", false);
    Services.obs.addObserver(this, "message-manager-disconnect", false);
  },

  destroy: function() {
    Services.obs.removeObserver(this, "message-manager-close");
    Services.obs.removeObserver(this, "message-manager-disconnect");
  },

  observe: function(subject, topic, data) {
    if (topic == "message-manager-close") {
      this.outstanding.add(subject);
    } else if (topic == "message-manager-disconnect") {
      this.outstanding.delete(subject);
      if (!this.outstanding.size && this.promiseResolver) {
        this.promiseResolver();
      }
    }
  },

  wait: function() {
    if (!this.outstanding.size) {
      return Promise.resolve();
    }

    return new Promise((resolve) => {
      this.promiseResolver = resolve;
    });
  },
};

function testInit() {
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
    
    let messageHandler = function(m) {
      messageManager.removeMessageListener("chromeEvent", messageHandler);
      var url = m.json.data;

      
      
      var webNav = content.window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                         .getInterface(Components.interfaces.nsIWebNavigation);
      webNav.loadURI(url, null, null, null, null);
    };

    var listener = 'data:,function doLoad(e) { var data=e.detail&&e.detail.data;removeEventListener("contentEvent", function (e) { doLoad(e); }, false, true);sendAsyncMessage("chromeEvent", {"data":data}); };addEventListener("contentEvent", function (e) { doLoad(e); }, false, true);';
    messageManager.loadFrameScript(listener, true);
    messageManager.addMessageListener("chromeEvent", messageHandler);
  }
  if (gConfig.e10s) {
    e10s_init();
    let globalMM = Cc["@mozilla.org/globalmessagemanager;1"]
                     .getService(Ci.nsIMessageListenerManager);
    globalMM.loadFrameScript("chrome://mochikit/content/shutdown-leaks-collector.js", true);
  } else {
    
    Components.utils.import("chrome://mochikit/content/ShutdownLeaksCollector.jsm");
  }
}

function Tester(aTests, aDumper, aCallback) {
  this.dumper = aDumper;
  this.tests = aTests;
  this.callback = aCallback;
  this.openedWindows = {};
  this.openedURLs = {};

  this._scriptLoader = Services.scriptloader;
  this.EventUtils = {};
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/EventUtils.js", this.EventUtils);
  var simpleTestScope = {};
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/specialpowersAPI.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/SpecialPowersObserverAPI.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromePowers.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/SimpleTest.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/MemoryStats.js", simpleTestScope);
  this._scriptLoader.loadSubScript("chrome://mochikit/content/chrome-harness.js", simpleTestScope);
  this.SimpleTest = simpleTestScope.SimpleTest;

  this.SimpleTest.harnessParameters = gConfig;

  this.MemoryStats = simpleTestScope.MemoryStats;
  this.Task = Task;
  this.ContentTask = Components.utils.import("resource://testing-common/ContentTask.jsm", null).ContentTask;
  this.BrowserTestUtils = Components.utils.import("resource://testing-common/BrowserTestUtils.jsm", null).BrowserTestUtils;
  this.TestUtils = Components.utils.import("resource://testing-common/TestUtils.jsm", null).TestUtils;
  this.Task.Debugging.maintainStack = true;
  this.Promise = Components.utils.import("resource://gre/modules/Promise.jsm", null).Promise;
  this.Assert = Components.utils.import("resource://testing-common/Assert.jsm", null).Assert;

  this.SimpleTestOriginal = {};
  SIMPLETEST_OVERRIDES.forEach(m => {
    this.SimpleTestOriginal[m] = this.SimpleTest[m];
  });

  this._toleratedUncaughtRejections = null;
  this._uncaughtErrorObserver = function({message, date, fileName, stack, lineNumber}) {
    let error = message;
    if (fileName || lineNumber) {
      error = {
        fileName: fileName,
        lineNumber: lineNumber,
        message: message,
        toString: function() {
          return message;
        }
      };
    }

    
    let tolerate = this._toleratedUncaughtRejections &&
      this._toleratedUncaughtRejections.indexOf(message) != -1;
    let name = "A promise chain failed to handle a rejection: ";
    if (tolerate) {
      name = "WARNING: (PLEASE FIX THIS AS PART OF BUG 1077403) " + name;
    }

    this.currentTest.addResult(
      new testResult(
	      tolerate,
        name,
        error,
        tolerate,
        stack));
    }.bind(this);
}
Tester.prototype = {
  EventUtils: {},
  SimpleTest: {},
  Task: null,
  ContentTask: null,
  Assert: null,

  repeat: 0,
  runUntilFailure: false,
  checker: null,
  currentTestIndex: -1,
  lastStartTime: null,
  openedWindows: null,
  lastAssertionCount: 0,

  get currentTest() {
    return this.tests[this.currentTestIndex];
  },
  get done() {
    return this.currentTestIndex == this.tests.length - 1;
  },

  start: function Tester_start() {
    TabDestroyObserver.init();

    
    if (!gConfig)
      gConfig = readConfig();

    if (gConfig.runUntilFailure)
      this.runUntilFailure = true;

    if (gConfig.repeat)
      this.repeat = gConfig.repeat;

    this.dumper.structuredLogger.info("*** Start BrowserChrome Test Results ***");
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

    this.Promise.Debugging.clearUncaughtErrorObservers();
    this.Promise.Debugging.addUncaughtErrorObserver(this._uncaughtErrorObserver);

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

    
    if (window.gBrowser) {
      gBrowser.addTab("about:blank", { skipAnimation: true });
      gBrowser.removeCurrentTab();
      gBrowser.stop();
    }

    
    this.dumper.structuredLogger.info("checking window state");
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
          this.dumper.structuredLogger.testEnd("browser-test.js",
                                               "FAIL",
                                               "PASS",
                                               msg);

        win.close();
      }
    }

    
    this.SimpleTest.waitForFocus(aCallback);
  },

  finish: function Tester_finish(aSkipSummary) {
    this.Promise.Debugging.flushUncaughtErrors();

    var passCount = this.tests.reduce(function(a, f) a + f.passCount, 0);
    var failCount = this.tests.reduce(function(a, f) a + f.failCount, 0);
    var todoCount = this.tests.reduce(function(a, f) a + f.todoCount, 0);

    if (this.repeat > 0) {
      --this.repeat;
      this.currentTestIndex = -1;
      this.nextTest();
    }
    else{
      TabDestroyObserver.destroy();
      Services.console.unregisterListener(this);
      Services.obs.removeObserver(this, "chrome-document-global-created");
      Services.obs.removeObserver(this, "content-document-global-created");
      this.Promise.Debugging.clearUncaughtErrorObservers();
      this._treatUncaughtRejectionsAsFailures = false;

      
      let pid = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).processID;
      dump("Completed ShutdownLeaks collections in process " + pid + "\n");

      this.dumper.structuredLogger.info("TEST-START | Shutdown");

      if (this.tests.length) {
        this.dumper.structuredLogger.info("Browser Chrome Test Summary");
        this.dumper.structuredLogger.info("Passed:  " + passCount);
        this.dumper.structuredLogger.info("Failed:  " + failCount);
        this.dumper.structuredLogger.info("Todo:    " + todoCount);
      } else {
        this.dumper.structuredLogger.testEnd("browser-test.js",
                                             "FAIL",
                                             "PASS",
                                             "No tests to run. Did you pass an invalid --test-path?");
      }
      this.dumper.structuredLogger.info("*** End BrowserChrome Test Results ***");

      this.dumper.done();

      
      this.callback(this.tests);
      this.callback = null;
      this.tests = null;
      this.openedWindows = null;
    }
  },

  haltTests: function Tester_haltTests() {
    
    this.currentTestIndex = this.tests.length - 1;
    this.repeat = 0;
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

  nextTest: Task.async(function*() {
    if (this.currentTest) {
      this.Promise.Debugging.flushUncaughtErrors();

      
      
      let testScope = this.currentTest.scope;
      while (testScope.__cleanupFunctions.length > 0) {
        let func = testScope.__cleanupFunctions.shift();
        try {
          yield func.apply(testScope);
        }
        catch (ex) {
          this.currentTest.addResult(new testResult(false, "Cleanup function threw an exception", ex, false));
        }
      }

      if (this.currentTest.passCount === 0 &&
          this.currentTest.failCount === 0 &&
          this.currentTest.todoCount === 0) {
        this.currentTest.addResult(new testResult(false, "This test contains no passes, no fails and no todos. Maybe it threw a silent exception? Make sure you use waitForExplicitFinish() if you need it.", "", false));
      }

      if (testScope.__expected == 'fail' && testScope.__num_failed <= 0) {
        this.currentTest.addResult(new testResult(false, "We expected at least one assertion to fail because this test file was marked as fail-if in the manifest!", "", true));
      }

      this.Promise.Debugging.flushUncaughtErrors();

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

      
      if (this.currentTest.unexpectedTimeouts && !this.currentTest.timedOut) {
        let msg = "This test exceeded the timeout threshold. It should be " +
                  "rewritten or split up. If that's not possible, use " +
                  "requestLongerTimeout(N), but only as a last resort.";
        this.currentTest.addResult(new testResult(false, msg, "", false));
      }

      
      
      
      let debugsvc = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
      if (debugsvc.isDebugBuild) {
        let newAssertionCount = debugsvc.assertionCount;
        let numAsserts = newAssertionCount - this.lastAssertionCount;
        this.lastAssertionCount = newAssertionCount;

        let max = testScope.__expectedMaxAsserts;
        let min = testScope.__expectedMinAsserts;
        if (numAsserts > max) {
          let msg = "Assertion count " + numAsserts +
                    " is greater than expected range " +
                    min + "-" + max + " assertions.";
          
          
          this.currentTest.addResult(new testResult(true, msg, "", true));
        } else if (numAsserts < min) {
          let msg = "Assertion count " + numAsserts +
                    " is less than expected range " +
                    min + "-" + max + " assertions.";
          
          this.currentTest.addResult(new testResult(false, msg, "", true));
        } else if (numAsserts > 0) {
          let msg = "Assertion count " + numAsserts +
                    " is within expected range " +
                    min + "-" + max + " assertions.";
          
          this.currentTest.addResult(new testResult(true, msg, "", true));
        }
      }

      
      if (Cc["@mozilla.org/xre/runtime;1"]
          .getService(Ci.nsIXULRuntime)
          .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT)
      {
        this.MemoryStats.dump(this.currentTestIndex,
                              this.currentTest.path,
                              gConfig.dumpOutputDirectory,
                              gConfig.dumpAboutMemoryAfterTest,
                              gConfig.dumpDMDAfterTest);
      }

      
      let time = Date.now() - this.lastStartTime;
      this.dumper.structuredLogger.testEnd(this.currentTest.path,
                                           "OK",
                                           undefined,
                                           "finished in " + time + "ms");
      this.currentTest.setDuration(time);

      if (this.runUntilFailure && this.currentTest.failCount > 0) {
        this.haltTests();
      }

      
      SIMPLETEST_OVERRIDES.forEach(m => {
        this.SimpleTest[m] = this.SimpleTestOriginal[m];
      });

      testScope.destroy();
      this.currentTest.scope = null;
    }

    
    
    
    this.waitForWindowsState((function () {
      if (this.done) {

        
        
        
        if (gConfig.testRoot == "browser") {
          
          
          
          
          let sidebar = document.getElementById("sidebar");
          sidebar.setAttribute("src", "data:text/html;charset=utf-8,");
          sidebar.docShell.createAboutBlankContentViewer(null);
          sidebar.setAttribute("src", "about:blank");

          
          let socialSidebar = document.getElementById("social-sidebar-browser");
          socialSidebar.setAttribute("src", "data:text/html;charset=utf-8,");
          socialSidebar.docShell.createAboutBlankContentViewer(null);
          socialSidebar.setAttribute("src", "about:blank");

          
          let {BackgroundPageThumbs} =
            Cu.import("resource://gre/modules/BackgroundPageThumbs.jsm", {});
          BackgroundPageThumbs._destroy();

          
          if (gBrowser._preloadedBrowser) {
            let browser = gBrowser._preloadedBrowser;
            gBrowser._preloadedBrowser = null;
            gBrowser.getNotificationBox(browser).remove();
          }

          SelfSupportBackend.uninit();
          CustomizationTabPreloader.uninit();
          SocialFlyout.unload();
          SocialShare.uninit();
          TabView.uninit();
        }

        
        
        
        

        let checkForLeakedGlobalWindows = aCallback => {
          Cu.schedulePreciseShrinkingGC(() => {
            let analyzer = new CCAnalyzer();
            analyzer.run(() => {
              let results = [];
              for (let obj of analyzer.find("nsGlobalWindow ")) {
                let m = obj.name.match(/^nsGlobalWindow #(\d+)/);
                if (m && m[1] in this.openedWindows)
                  results.push({ name: obj.name, url: m[1] });
              }
              aCallback(results);
            });
          });
        };

        let reportLeaks = aResults => {
          for (let result of aResults) {
            let test = this.openedWindows[result.url];
            let msg = "leaked until shutdown [" + result.name +
                      " " + (this.openedURLs[result.url] || "NULL") + "]";
            test.addResult(new testResult(false, msg, "", false));
          }
        };

        let {AsyncShutdown} =
          Cu.import("resource://gre/modules/AsyncShutdown.jsm", {});

        let barrier = new AsyncShutdown.Barrier(
          "ShutdownLeaks: Wait for cleanup to be finished before checking for leaks");
        Services.obs.notifyObservers({wrappedJSObject: barrier},
          "shutdown-leaks-before-check", null);

        barrier.client.addBlocker("ShutdownLeaks: Wait for tabs to finish closing",
                                  TabDestroyObserver.wait());

        barrier.wait().then(() => {
          
          
          Services.obs.notifyObservers(null, "memory-pressure", "heap-minimize");

          let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                       .getService(Ci.nsIMessageBroadcaster);
          ppmm.broadcastAsyncMessage("browser-test:collect-request");

          checkForLeakedGlobalWindows(aResults => {
            if (aResults.length == 0) {
              this.finish();
              return;
            }
            
            
            
            setTimeout(() => {
              checkForLeakedGlobalWindows(aResults => {
                reportLeaks(aResults);
                this.finish();
              });
            }, 1000);
          });
        });

        return;
      }

      this.currentTestIndex++;
      this.execTest();
    }).bind(this));
  }),

  execTest: function Tester_execTest() {
    this.dumper.structuredLogger.testStart(this.currentTest.path);

    this.SimpleTest.reset();

    
    let currentScope = this.currentTest.scope = new testScope(this, this.currentTest, this.currentTest.expected);
    let currentTest = this.currentTest;

    
    this.currentTest.scope.EventUtils = this.EventUtils;
    this.currentTest.scope.SimpleTest = this.SimpleTest;
    this.currentTest.scope.gTestPath = this.currentTest.path;
    this.currentTest.scope.Task = this.Task;
    this.currentTest.scope.ContentTask = this.ContentTask;
    this.currentTest.scope.BrowserTestUtils = this.BrowserTestUtils;
    this.currentTest.scope.TestUtils = this.TestUtils;
    
    this.currentTest.scope.Assert = new this.Assert(function(err, message, stack) {
      let res;
      if (err) {
        res = new testResult(false, err.message, err.stack, false, err.stack);
      } else {
        res = new testResult(true, message, "", false, stack);
      }
      currentTest.addResult(res);
    });

    
    this.currentTest.scope.export_assertions = function() {
      for (let func in this.Assert) {
        this[func] = this.Assert[func].bind(this.Assert);
      }
    };

    
    SIMPLETEST_OVERRIDES.forEach(function(m) {
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
      
      
      
      if (!/^Error opening input stream/.test(ex.toString())) {
       this.currentTest.addResult(new testResult(false, "head.js import threw an exception", ex, false));
      }
    }

    
    try {
      this._scriptLoader.loadSubScript(this.currentTest.path,
                                       this.currentTest.scope);
      this.Promise.Debugging.flushUncaughtErrors();
      
      this.lastStartTime = Date.now();
      if (this.currentTest.scope.__tasks) {
        
        if ("test" in this.currentTest.scope) {
          throw "Cannot run both a add_task test and a normal test at the same time.";
        }
        let Promise = this.Promise;
        this.Task.spawn(function*() {
          let task;
          while ((task = this.__tasks.shift())) {
            this.SimpleTest.info("Entering test " + task.name);
            try {
              yield task();
            } catch (ex) {
              let isExpected = !!this.SimpleTest.isExpectingUncaughtException();
              let stack = (typeof ex == "object" && "stack" in ex)?ex.stack:null;
              let name = "Uncaught exception";
              let result = new testResult(isExpected, name, ex, false, stack);
              currentTest.addResult(result);
            }
            Promise.Debugging.flushUncaughtErrors();
            this.SimpleTest.info("Leaving test " + task.name);
          }
          this.finish();
        }.bind(currentScope));
      } else if ("generatorTest" in this.currentTest.scope) {
        if ("test" in this.currentTest.scope) {
          throw "Cannot run both a generator test and a normal test at the same time.";
        }

        
        this.currentTest.scope.waitForExplicitFinish();
        var result = this.currentTest.scope.generatorTest();
        this.currentTest.scope.__generator = result;
        result.next();
      } else {
        this.currentTest.scope.test();
      }
    } catch (ex) {
      let isExpected = !!this.SimpleTest.isExpectingUncaughtException();
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
      var timeoutExpires = Date.now() + gTimeoutSeconds * 1000;
      var waitUntilAtLeast = timeoutExpires - 1000;
      this.currentTest.scope.__waitTimer =
        this.SimpleTest._originalSetTimeout.apply(window, [function timeoutFn() {
        
        
        
        if (Date.now() < waitUntilAtLeast) {
          self.currentTest.scope.__waitTimer =
            setTimeout(timeoutFn, timeoutExpires - Date.now());
          return;
        }

        if (--self.currentTest.scope.__timeoutFactor > 0) {
          
          self.currentTest.scope.info(
            "Longer timeout required, waiting longer...  Remaining timeouts: " +
            self.currentTest.scope.__timeoutFactor);
          self.currentTest.scope.__waitTimer =
            setTimeout(timeoutFn, gTimeoutSeconds * 1000);
          return;
        }

        
        
        
        
        
        
        
        const MAX_UNEXPECTED_TIMEOUTS = 10;
        if (Date.now() - self.currentTest.lastOutputTime < (gTimeoutSeconds / 2) * 1000 &&
            ++self.currentTest.unexpectedTimeouts <= MAX_UNEXPECTED_TIMEOUTS) {
            self.currentTest.scope.__waitTimer =
              setTimeout(timeoutFn, gTimeoutSeconds * 1000);
          return;
        }

        self.currentTest.addResult(new testResult(false, "Test timed out", null, false));
        self.currentTest.timedOut = true;
        self.currentTest.scope.__waitTimer = null;
        self.nextTest();
      }, gTimeoutSeconds * 1000]);
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
  this.name = aName;
  this.msg = "";

  this.info = false;
  this.pass = !!aCondition;
  this.todo = aIsTodo;

  if (this.pass) {
    if (aIsTodo) {
      this.status = "FAIL";
      this.expected = "FAIL";
    } else {
      this.status = "PASS";
      this.expected = "PASS";
    }

  } else {
    if (aDiag) {
      if (typeof aDiag == "object" && "fileName" in aDiag) {
        
        this.msg += "at " + aDiag.fileName + ":" + aDiag.lineNumber + " - ";
      }
      this.msg += String(aDiag);
    }
    if (aStack) {
      this.msg += "\nStack trace:\n";
      let normalized;
      if (aStack instanceof Components.interfaces.nsIStackFrame) {
        let frames = [];
        for (let frame = aStack; frame; frame = frame.caller) {
          frames.push(frame.filename + ":" + frame.name + ":" + frame.lineNumber);
        }
        normalized = frames.join("\n");
      } else {
        normalized = "" + aStack;
      }
      this.msg += Task.Debugging.generateReadableStack(normalized, "    ");
    }
    if (aIsTodo) {
      this.status = "PASS";
      this.expected = "FAIL";
    } else {
      this.status = "FAIL";
      this.expected = "PASS";
    }

    if (gConfig.debugOnFailure) {
      
      
      debugger;
    }
  }
}

function testMessage(aName) {
  this.msg = aName || "";
  this.info = true;
}



function testScope(aTester, aTest, expected) {
  this.__tester = aTester;
  this.__expected = expected;
  this.__num_failed = 0;

  var self = this;
  this.ok = function test_ok(condition, name, diag, stack) {
    if (this.__expected == 'fail') {
        if (!condition) {
          this.__num_failed++;
          condition = true;
        }
    }

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

  this.thisTestLeaksUncaughtRejectionsAndShouldBeFixed = function(...rejections) {
    if (!aTester._toleratedUncaughtRejections) {
      aTester._toleratedUncaughtRejections = [];
    }
    aTester._toleratedUncaughtRejections.push(...rejections);
  };

  this.expectAssertions = function test_expectAssertions(aMin, aMax) {
    let min = aMin;
    let max = aMax;
    if (typeof(max) == "undefined") {
      max = min;
    }
    if (typeof(min) != "number" || typeof(max) != "number" ||
        min < 0 || max < min) {
      throw "bad parameter to expectAssertions";
    }
    self.__expectedMinAsserts = min;
    self.__expectedMaxAsserts = max;
  };

  this.setExpected = function test_setExpected() {
    self.__expected = 'fail';
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

  this.requestCompleteLog = function test_requestCompleteLog() {
    self.__tester.dumper.structuredLogger.deactivateBuffering();
    self.registerCleanupFunction(function() {
      self.__tester.dumper.structuredLogger.activateBuffering();
    })
  };
}
testScope.prototype = {
  __done: true,
  __generator: null,
  __tasks: null,
  __waitTimer: null,
  __cleanupFunctions: [],
  __timeoutFactor: 1,
  __expectedMinAsserts: 0,
  __expectedMaxAsserts: 0,
  __expected: 'pass',

  EventUtils: {},
  SimpleTest: {},
  Task: null,
  ContentTask: null,
  BrowserTestUtils: null,
  TestUtils: null,
  Assert: null,

  


































  add_task: function(aFunction) {
    if (!this.__tasks) {
      this.waitForExplicitFinish();
      this.__tasks = [];
    }
    this.__tasks.push(aFunction.bind(this));
  },

  destroy: function test_destroy() {
    for (let prop in this)
      delete this[prop];
  }
};
