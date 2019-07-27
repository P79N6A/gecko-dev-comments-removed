

















var SimpleTest = { };
var parentRunner = null;




var isSingleTestRun = (parent == window && !opener)
try {
  var isPrimaryTestWindow = !!parent.TestRunner || isSingleTestRun;
} catch(e) {
  dump("TEST-UNEXPECTED-FAIL, Exception caught: " + e.message +
                ", at: " + e.fileName + " (" + e.lineNumber +
                "), location: " + window.location.href + "\n");
}







(function() {
    function ancestor(w) {
        return w.parent != w ? w.parent : w.opener;
    }

    var w = ancestor(window);
    while (w && (!parentRunner || !window.SpecialPowers)) {
        if (!parentRunner) {
            parentRunner = w.TestRunner;
            if (!parentRunner && w.wrappedJSObject) {
                parentRunner = w.wrappedJSObject.TestRunner;
            }
        }
        if (!window.SpecialPowers) {
            window.SpecialPowers = w.SpecialPowers;
        }
        w = ancestor(w);
    }

    if (parentRunner) {
        SimpleTest.harnessParameters = parentRunner.getParameterInfo();
    }
})();


if (typeof(repr) == 'undefined') {
    this.repr = function(o) {
        if (typeof(o) == "undefined") {
            return "undefined";
        } else if (o === null) {
            return "null";
        }
        try {
            if (typeof(o.__repr__) == 'function') {
                return o.__repr__();
            } else if (typeof(o.repr) == 'function' && o.repr != arguments.callee) {
                return o.repr();
            }
       } catch (e) {
       }
       try {
            if (typeof(o.NAME) == 'string' && (
                    o.toString == Function.prototype.toString ||
                    o.toString == Object.prototype.toString
                )) {
                return o.NAME;
            }
        } catch (e) {
        }
        try {
            var ostring = (o + "");
        } catch (e) {
            return "[" + typeof(o) + "]";
        }
        if (typeof(o) == "function") {
            o = ostring.replace(/^\s+/, "");
            var idx = o.indexOf("{");
            if (idx != -1) {
                o = o.substr(0, idx) + "{...}";
            }
        }
        return ostring;
    };
}




if (typeof(partial) == 'undefined') {
    this.partial = function(func) {
        var args = [];
        for (var i = 1; i < arguments.length; i++) {
            args.push(arguments[i]);
        }
        return function() {
            if (arguments.length > 0) {
                for (var i = 1; i < arguments.length; i++) {
                    args.push(arguments[i]);
                }
            }
            func(args);
        };
    };
}

if (typeof(getElement) == 'undefined') {
    this.getElement = function(id) {
        return ((typeof(id) == "string") ?
            document.getElementById(id) : id);
    };
    this.$ = this.getElement;
}

SimpleTest._newCallStack = function(path) {
    var rval = function () {
        var callStack = arguments.callee.callStack;
        for (var i = 0; i < callStack.length; i++) {
            if (callStack[i].apply(this, arguments) === false) {
                break;
            }
        }
        try {
            this[path] = null;
        } catch (e) {
            
        }
    };
    rval.callStack = [];
    return rval;
};

if (typeof(addLoadEvent) == 'undefined') {
    this.addLoadEvent = function(func) {
        var existing = window["onload"];
        var regfunc = existing;
        if (!(typeof(existing) == 'function'
                && typeof(existing.callStack) == "object"
                && existing.callStack !== null)) {
            regfunc = SimpleTest._newCallStack("onload");
            if (typeof(existing) == 'function') {
                regfunc.callStack.push(existing);
            }
            window["onload"] = regfunc;
        }
        regfunc.callStack.push(func);
    };
}

function createEl(type, attrs, html) {
    
    var el;
    if (!document.body) {
        el = document.createElementNS("http://www.w3.org/1999/xhtml", type);
    }
    else {
        el = document.createElement(type);
    }
    if (attrs !== null && attrs !== undefined) {
        for (var k in attrs) {
            el.setAttribute(k, attrs[k]);
        }
    }
    if (html !== null && html !== undefined) {
        el.appendChild(document.createTextNode(html));
    }
    return el;
}


if (typeof(computedStyle) == 'undefined') {
    this.computedStyle = function(elem, cssProperty) {
        elem = getElement(elem);
        if (elem.currentStyle) {
            return elem.currentStyle[cssProperty];
        }
        if (typeof(document.defaultView) == 'undefined' || document === null) {
            return undefined;
        }
        var style = document.defaultView.getComputedStyle(elem, null);
        if (typeof(style) == 'undefined' || style === null) {
            return undefined;
        }

        var selectorCase = cssProperty.replace(/([A-Z])/g, '-$1'
            ).toLowerCase();

        return style.getPropertyValue(selectorCase);
    };
}




SimpleTest.testPluginIsOOP = function () {
    var testPluginIsOOP = false;
    if (navigator.platform.indexOf("Mac") == 0) {
        if (SpecialPowers.XPCOMABI.match(/x86-/)) {
            try {
                testPluginIsOOP = SpecialPowers.getBoolPref("dom.ipc.plugins.enabled.i386.test.plugin");
            } catch (e) {
                testPluginIsOOP = SpecialPowers.getBoolPref("dom.ipc.plugins.enabled.i386");
            }
        }
        else if (SpecialPowers.XPCOMABI.match(/x86_64-/)) {
            try {
                testPluginIsOOP = SpecialPowers.getBoolPref("dom.ipc.plugins.enabled.x86_64.test.plugin");
            } catch (e) {
                testPluginIsOOP = SpecialPowers.getBoolPref("dom.ipc.plugins.enabled.x86_64");
            }
        }
    }
    else {
        testPluginIsOOP = SpecialPowers.getBoolPref("dom.ipc.plugins.enabled");
    }

    return testPluginIsOOP;
};

SimpleTest._tests = [];
SimpleTest._stopOnLoad = true;
SimpleTest._cleanupFunctions = [];
SimpleTest.expected = 'pass';
SimpleTest.num_failed = 0;

SimpleTest.setExpected = function () {
  if (parent.TestRunner) {
    SimpleTest.expected = parent.TestRunner.expected;
  }
}
SimpleTest.setExpected();




SimpleTest.ok = function (condition, name, diag) {

    var test = {'result': !!condition, 'name': name, 'diag': diag};
    if (SimpleTest.expected == 'fail') {
      if (!test.result) {
        SimpleTest.num_failed++;
        test.result = !test.result;
      }
      var successInfo = {status:"PASS", expected:"PASS", message:"TEST-PASS"};
      var failureInfo = {status:"FAIL", expected:"FAIL", message:"TEST-KNOWN-FAIL"};
    } else {
      var successInfo = {status:"PASS", expected:"PASS", message:"TEST-PASS"};
      var failureInfo = {status:"FAIL", expected:"PASS", message:"TEST-UNEXPECTED-FAIL"};
    }
    SimpleTest._logResult(test, successInfo, failureInfo);
    SimpleTest._tests.push(test);
};




SimpleTest.is = function (a, b, name) {
    var pass = (a == b);
    var diag = pass ? "" : "got " + repr(a) + ", expected " + repr(b)
    SimpleTest.ok(pass, name, diag);
};

SimpleTest.isfuzzy = function (a, b, epsilon, name) {
  var pass = (a >= b - epsilon) && (a <= b + epsilon);
  var diag = pass ? "" : "got " + repr(a) + ", expected " + repr(b) + " epsilon: +/- " + repr(epsilon)
  SimpleTest.ok(pass, name, diag);
};

SimpleTest.isnot = function (a, b, name) {
    var pass = (a != b);
    var diag = pass ? "" : "didn't expect " + repr(a) + ", but got it";
    SimpleTest.ok(pass, name, diag);
};




SimpleTest.ise = function (a, b, name) {
    var pass = (a === b);
    var diag = pass ? "" : "got " + repr(a) + ", strictly expected " + repr(b)
    SimpleTest.ok(pass, name, diag);
};




SimpleTest.doesThrow = function(fn, name) {
    var gotException = false;
    try {
      fn();
    } catch (ex) { gotException = true; }
    ok(gotException, name);
};



SimpleTest.todo = function(condition, name, diag) {
    var test = {'result': !!condition, 'name': name, 'diag': diag, todo: true};
    var successInfo = {status:"PASS", expected:"FAIL", message:"TEST-UNEXPECTED-PASS"};
    var failureInfo = {status:"FAIL", expected:"FAIL", message:"TEST-KNOWN-FAIL"};
    SimpleTest._logResult(test, successInfo, failureInfo);
    SimpleTest._tests.push(test);
};









SimpleTest.getTestFileURL = function(path) {
  var lastSlashIdx = path.lastIndexOf("/") + 1;
  var filename = path.substr(lastSlashIdx);
  var location = window.location;
  
  var remotePath = location.pathname.replace(/\/[^\/]+?$/,"");
  var url = location.origin +
            remotePath + "/" + path;
  return url;
};

SimpleTest._getCurrentTestURL = function() {
    return parentRunner && parentRunner.currentTestURL ||
           typeof gTestPath == "string" && gTestPath ||
           "unknown test url";
};

SimpleTest._forceLogMessageOutput = false;




SimpleTest.requestCompleteLog = function() {
    if (!parentRunner || SimpleTest._forceLogMessageOutput) {
        return;
    }

    parentRunner.structuredLogger.deactivateBuffering();
    SimpleTest._forceLogMessageOutput = true;

    SimpleTest.registerCleanupFunction(function() {
        parentRunner.structuredLogger.activateBuffering();
        SimpleTest._forceLogMessageOutput = false;
    });
};

SimpleTest._logResult = function (test, passInfo, failInfo) {
    var url = SimpleTest._getCurrentTestURL();
    var result = test.result ? passInfo : failInfo;
    var diagnostic = test.diag || null;
    
    var subtest = test.name ? String(test.name) : null;
    var isError = !test.result == !test.todo;

    if (parentRunner) {
        if (!result.status || !result.expected) {
            if (diagnostic) {
                parentRunner.structuredLogger.info(diagnostic);
            }
            return;
        }

        if (isError) {
            parentRunner.addFailedTest(url);
        }

        parentRunner.structuredLogger.testStatus(url,
                                                 subtest,
                                                 result.status,
                                                 result.expected,
                                                 diagnostic);
    } else if (typeof dump === "function") {
        var diagMessage = test.name + (test.diag ? " - " + test.diag : "");
        var debugMsg = [result.message, url, diagMessage].join(' | ');
        dump(debugMsg + "\n");
    } else {
        
    }
};

SimpleTest.info = function(name, message) {
    var log = message ? name + ' | ' + message : name;
    if (parentRunner) {
        parentRunner.structuredLogger.info(log);
    } else {
        dump(log + '\n');
    }
};





SimpleTest.todo_is = function (a, b, name) {
    var pass = (a == b);
    var diag = pass ? repr(a) + " should equal " + repr(b)
                    : "got " + repr(a) + ", expected " + repr(b);
    SimpleTest.todo(pass, name, diag);
};

SimpleTest.todo_isnot = function (a, b, name) {
    var pass = (a != b);
    var diag = pass ? repr(a) + " should not equal " + repr(b)
                    : "didn't expect " + repr(a) + ", but got it";
    SimpleTest.todo(pass, name, diag);
};





SimpleTest.report = function () {
    var passed = 0;
    var failed = 0;
    var todo = 0;

    var tallyAndCreateDiv = function (test) {
            var cls, msg, div;
            var diag = test.diag ? " - " + test.diag : "";
            if (test.todo && !test.result) {
                todo++;
                cls = "test_todo";
                msg = "todo | " + test.name + diag;
            } else if (test.result && !test.todo) {
                passed++;
                cls = "test_ok";
                msg = "passed | " + test.name + diag;
            } else {
                failed++;
                cls = "test_not_ok";
                msg = "failed | " + test.name + diag;
            }
          div = createEl('div', {'class': cls}, msg);
          return div;
        };
    var results = [];
    for (var d=0; d<SimpleTest._tests.length; d++) {
        results.push(tallyAndCreateDiv(SimpleTest._tests[d]));
    }

    var summary_class = failed != 0 ? 'some_fail' :
                          passed == 0 ? 'todo_only' : 'all_pass';

    var div1 = createEl('div', {'class': 'tests_report'});
    var div2 = createEl('div', {'class': 'tests_summary ' + summary_class});
    var div3 = createEl('div', {'class': 'tests_passed'}, 'Passed: ' + passed);
    var div4 = createEl('div', {'class': 'tests_failed'}, 'Failed: ' + failed);
    var div5 = createEl('div', {'class': 'tests_todo'}, 'Todo: ' + todo);
    div2.appendChild(div3);
    div2.appendChild(div4);
    div2.appendChild(div5);
    div1.appendChild(div2);
    for (var t=0; t<results.length; t++) {
        
        div1.appendChild(results[t]);
    }
    return div1;
};




SimpleTest.toggle = function(el) {
    if (computedStyle(el, 'display') == 'block') {
        el.style.display = 'none';
    } else {
        el.style.display = 'block';
    }
};




SimpleTest.toggleByClass = function (cls, evt) {
    var children = document.getElementsByTagName('div');
    var elements = [];
    for (var i=0; i<children.length; i++) {
        var child = children[i];
        var clsName = child.className;
        if (!clsName) {
            continue;
        }
        var classNames = clsName.split(' ');
        for (var j = 0; j < classNames.length; j++) {
            if (classNames[j] == cls) {
                elements.push(child);
                break;
            }
        }
    }
    for (var t=0; t<elements.length; t++) {
        
        SimpleTest.toggle(elements[t]);
    }
    if (evt)
        evt.preventDefault();
};




SimpleTest.showReport = function() {
    var togglePassed = createEl('a', {'href': '#'}, "Toggle passed checks");
    var toggleFailed = createEl('a', {'href': '#'}, "Toggle failed checks");
    var toggleTodo = createEl('a',{'href': '#'}, "Toggle todo checks");
    togglePassed.onclick = partial(SimpleTest.toggleByClass, 'test_ok');
    toggleFailed.onclick = partial(SimpleTest.toggleByClass, 'test_not_ok');
    toggleTodo.onclick = partial(SimpleTest.toggleByClass, 'test_todo');
    var body = document.body;  
    if (!body) {
        
        body = document.getElementsByTagNameNS("http://www.w3.org/1999/xhtml",
                                               "body")[0];
    }
    var firstChild = body.childNodes[0];
    var addNode;
    if (firstChild) {
        addNode = function (el) {
            body.insertBefore(el, firstChild);
        };
    } else {
        addNode = function (el) {
            body.appendChild(el)
        };
    }
    addNode(togglePassed);
    addNode(createEl('span', null, " "));
    addNode(toggleFailed);
    addNode(createEl('span', null, " "));
    addNode(toggleTodo);
    addNode(SimpleTest.report());
    
    addNode(createEl('hr'));
};








SimpleTest.waitForExplicitFinish = function () {
    SimpleTest._stopOnLoad = false;
};









SimpleTest.requestLongerTimeout = function (factor) {
    if (parentRunner) {
        parentRunner.requestLongerTimeout(factor);
    }
}

































SimpleTest.expectAssertions = function(min, max) {
    if (parentRunner) {
        parentRunner.expectAssertions(min, max);
    }
}

SimpleTest._flakyTimeoutIsOK = false;
SimpleTest._originalSetTimeout = window.setTimeout;
window.setTimeout = function SimpleTest_setTimeoutShim() {
    var testSuiteSupported = false;
    
    switch (SimpleTest.harnessParameters.testRoot) {
    case "browser":
    case "chrome":
    case "a11y":
    case "webapprtContent":
        break;
    default:
        if (!SimpleTest._alreadyFinished && arguments.length > 1 && arguments[1] > 0) {
            if (SimpleTest._flakyTimeoutIsOK) {
                SimpleTest.todo(false, "The author of the test has indicated that flaky timeouts are expected.  Reason: " + SimpleTest._flakyTimeoutReason);
            } else {
                SimpleTest.ok(false, "Test attempted to use a flaky timeout value " + arguments[1]);
            }
        }
    }
    return SimpleTest._originalSetTimeout.apply(window, arguments);
}















SimpleTest.requestFlakyTimeout = function (reason) {
    SimpleTest.is(typeof(reason), "string", "A valid string reason is expected");
    SimpleTest.isnot(reason, "", "Reason cannot be empty");
    SimpleTest._flakyTimeoutIsOK = true;
    SimpleTest._flakyTimeoutReason = reason;
}

SimpleTest.waitForFocus_started = false;
SimpleTest.waitForFocus_loaded = false;
SimpleTest.waitForFocus_focused = false;
SimpleTest._pendingWaitForFocusCount = 0;



















SimpleTest.waitForFocus = function (callback, targetWindow, expectBlankPage) {
    SimpleTest._pendingWaitForFocusCount++;
    if (!targetWindow)
      targetWindow = window;

    SimpleTest.waitForFocus_started = false;
    expectBlankPage = !!expectBlankPage;

    var childTargetWindow = SpecialPowers.getFocusedElementForWindow(targetWindow, true);

    function info(msg) {
        SimpleTest.info(msg);
    }
    function getHref(aWindow) {
      return SpecialPowers.getPrivilegedProps(aWindow, 'location.href');
    }

    function maybeRunTests() {
        if (SimpleTest.waitForFocus_loaded &&
            SimpleTest.waitForFocus_focused &&
            !SimpleTest.waitForFocus_started) {
            SimpleTest._pendingWaitForFocusCount--;
            SimpleTest.waitForFocus_started = true;
            SimpleTest.executeSoon(function() { callback(targetWindow) });
        }
    }

    function waitForEvent(event) {
        try {
            
            
            if (event.type == "load" && (expectBlankPage != (event.target.location == "about:blank")))
                return;

            SimpleTest["waitForFocus_" + event.type + "ed"] = true;
            var win = (event.type == "load") ? targetWindow : childTargetWindow;
            win.removeEventListener(event.type, waitForEvent, true);
            maybeRunTests();
        } catch (e) {
            SimpleTest.ok(false, "Exception caught in waitForEvent: " + e.message +
                ", at: " + e.fileName + " (" + e.lineNumber + ")");
        }
    }

    
    
    
    
    
    SimpleTest.waitForFocus_loaded =
        expectBlankPage ?
            getHref(targetWindow) == "about:blank" :
            getHref(targetWindow) != "about:blank" && targetWindow.document.readyState == "complete";
    if (!SimpleTest.waitForFocus_loaded) {
        info("must wait for load");
        targetWindow.addEventListener("load", waitForEvent, true);
    }

    
    var focusedChildWindow = null;
    if (SpecialPowers.activeWindow()) {
        focusedChildWindow = SpecialPowers.getFocusedElementForWindow(SpecialPowers.activeWindow(), true);
    }

    
    SimpleTest.waitForFocus_focused = (focusedChildWindow == childTargetWindow);
    if (SimpleTest.waitForFocus_focused) {
        
        maybeRunTests();
    }
    else {
        info("must wait for focus");
        childTargetWindow.addEventListener("focus", waitForEvent, true);
        SpecialPowers.focus(childTargetWindow);
    }
};

SimpleTest.waitForClipboard_polls = 0;





















SimpleTest.__waitForClipboardMonotonicCounter = 0;
SimpleTest.__defineGetter__("_waitForClipboardMonotonicCounter", function () {
  return SimpleTest.__waitForClipboardMonotonicCounter++;
});
SimpleTest.waitForClipboard = function(aExpectedStringOrValidatorFn, aSetupFn,
                                       aSuccessFn, aFailureFn, aFlavor) {
    var requestedFlavor = aFlavor || "text/unicode";

    
    var inputValidatorFn = typeof(aExpectedStringOrValidatorFn) == "string"
        ? function(aData) { return aData == aExpectedStringOrValidatorFn; }
        : aExpectedStringOrValidatorFn;

    
    function reset() {
        SimpleTest.waitForClipboard_polls = 0;
    }

    function wait(validatorFn, successFn, failureFn, flavor) {
        if (++SimpleTest.waitForClipboard_polls > 50) {
            
            SimpleTest.ok(false, "Timed out while polling clipboard for pasted data.");
            reset();
            failureFn();
            return;
        }

        var data = SpecialPowers.getClipboardData(flavor);

        if (validatorFn(data)) {
            
            if (preExpectedVal)
                preExpectedVal = null;
            else
                SimpleTest.ok(true, "Clipboard has the correct value");
            reset();
            successFn();
        } else {
            setTimeout(function() { return wait(validatorFn, successFn, failureFn, flavor); }, 100);
        }
    }

    
    var preExpectedVal = SimpleTest._waitForClipboardMonotonicCounter +
                         "-waitForClipboard-known-value";
    SpecialPowers.clipboardCopyString(preExpectedVal);
    wait(function(aData) { return aData  == preExpectedVal; },
         function() {
           
           aSetupFn();
           wait(inputValidatorFn, aSuccessFn, aFailureFn, requestedFlavor);
         }, aFailureFn, "text/unicode");
}





SimpleTest.executeSoon = function(aFunc) {
    if ("SpecialPowers" in window) {
        return SpecialPowers.executeSoon(aFunc, window);
    }
    setTimeout(aFunc, 0);
    return null;		
};

SimpleTest.registerCleanupFunction = function(aFunc) {
    SimpleTest._cleanupFunctions.push(aFunc);
};





SimpleTest.finish = function() {
    if (SimpleTest._alreadyFinished) {
        var err = "[SimpleTest.finish()] this test already called finish!";
        if (parentRunner) {
            parentRunner.structuredLogger.error(err);
        } else {
            dump(err + '\n');
        }
    }

    if (SimpleTest.expected == 'fail' && SimpleTest.num_failed <= 0) {
        msg = 'We expected at least one failure';
        var test = {'result': false, 'name': 'fail-if condition in manifest', 'diag': msg};
        var successInfo = {status:"PASS", expected:"PASS", message:"TEST-PASS"};
        var failureInfo = {status:"FAIL", expected:"FAIL", message:"TEST-KNOWN-FAIL"};

        SimpleTest._logResult(test, successInfo, failureInfo);
        SimpleTest._tests.push(test);
    }

    SimpleTest.testsLength = SimpleTest._tests.length;

    SimpleTest._alreadyFinished = true;

    var afterCleanup = function() {
        if (SpecialPowers.DOMWindowUtils.isTestControllingRefreshes) {
            SimpleTest.ok(false, "test left refresh driver under test control");
            SpecialPowers.DOMWindowUtils.restoreNormalRefresh();
        }
        if (SimpleTest._expectingUncaughtException) {
            SimpleTest.ok(false, "expectUncaughtException was called but no uncaught exception was detected!");
        }
        if (SimpleTest._pendingWaitForFocusCount != 0) {
            SimpleTest.is(SimpleTest._pendingWaitForFocusCount, 0,
                          "[SimpleTest.finish()] waitForFocus() was called a "
                          + "different number of times from the number of "
                          + "callbacks run.  Maybe the test terminated "
                          + "prematurely -- be sure to use "
                          + "SimpleTest.waitForExplicitFinish().");
        }
        if (SimpleTest._tests.length == 0) {
            SimpleTest.ok(false, "[SimpleTest.finish()] No checks actually run. "
                               + "(You need to call ok(), is(), or similar "
                               + "functions at least once.  Make sure you use "
                               + "SimpleTest.waitForExplicitFinish() if you need "
                               + "it.)");
        }

        if (parentRunner) {
            
            parentRunner.testFinished(SimpleTest._tests);
        }

        if (!parentRunner || parentRunner.showTestReport) {
            SpecialPowers.flushAllAppsLaunchable();
            SpecialPowers.flushPermissions(function () {
              SpecialPowers.flushPrefEnv(function() {
                SimpleTest.showReport();
              });
            });
        }
    }

    var executeCleanupFunction = function() {
        var func = SimpleTest._cleanupFunctions.pop();

        if (!func) {
            afterCleanup();
            return;
        }

        var ret;
        try {
            ret = func();
        } catch (ex) {
            SimpleTest.ok(false, "Cleanup function threw exception: " + ex);
        }

        if (ret && ret.constructor.name == "Promise") {
            ret.then(executeCleanupFunction,
                     (ex) => SimpleTest.ok(false, "Cleanup promise rejected: " + ex));
        } else {
            executeCleanupFunction();
        }
    };

    executeCleanupFunction();
};































SimpleTest.monitorConsole = function (continuation, msgs, forbidUnexpectedMsgs) {
  if (SimpleTest._stopOnLoad) {
    ok(false, "Console monitoring requires use of waitForExplicitFinish.");
  }

  function msgMatches(msg, pat) {
    for (k in pat) {
      if (!(k in msg)) {
        return false;
      }
      if (pat[k] instanceof RegExp && typeof(msg[k]) === 'string') {
        if (!pat[k].test(msg[k])) {
          return false;
        }
      } else if (msg[k] !== pat[k]) {
        return false;
      }
    }
    return true;
  }

  var forbiddenMsgs = [];
  var i = 0;
  while (i < msgs.length) {
    var pat = msgs[i];
    if ("forbid" in pat) {
      var forbid = pat.forbid;
      delete pat.forbid;
      if (forbid) {
        forbiddenMsgs.push(pat);
        msgs.splice(i, 1);
        continue;
      }
    }
    i++;
  }

  var counter = 0;
  function listener(msg) {
    if (msg.message === "SENTINEL" && !msg.isScriptError) {
      is(counter, msgs.length, "monitorConsole | number of messages");
      SimpleTest.executeSoon(continuation);
      return;
    }
    for (var pat of forbiddenMsgs) {
      if (msgMatches(msg, pat)) {
        ok(false, "monitorConsole | observed forbidden message " +
                  JSON.stringify(msg));
        return;
      }
    }
    if (counter >= msgs.length) {
      var str = "monitorConsole | extra message | " + JSON.stringify(msg);
      if (forbidUnexpectedMsgs) {
        ok(false, str);
      } else {
        info(str);
      }
      return;
    }
    var matches = msgMatches(msg, msgs[counter]);
    if (forbidUnexpectedMsgs) {
      ok(matches, "monitorConsole | [" + counter + "] must match " +
                  JSON.stringify(msg));
    } else {
      info("monitorConsole | [" + counter + "] " +
           (matches ? "matched " : "did not match ") + JSON.stringify(msg));
    }
    if (matches)
      counter++;
  }
  SpecialPowers.registerConsoleListener(listener);
};




SimpleTest.endMonitorConsole = function () {
  SpecialPowers.postConsoleSentinel();
};









SimpleTest.expectConsoleMessages = function (testfn, msgs, continuation) {
  SimpleTest.monitorConsole(continuation, msgs);
  testfn();
  SimpleTest.executeSoon(SimpleTest.endMonitorConsole);
};





SimpleTest.runTestExpectingConsoleMessages = function(testfn, msgs) {
  SimpleTest.waitForExplicitFinish();
  SimpleTest.expectConsoleMessages(testfn, msgs, SimpleTest.finish);
};






SimpleTest.expectChildProcessCrash = function () {
    if (parentRunner) {
        parentRunner.expectChildProcessCrash();
    }
};





SimpleTest.expectUncaughtException = function (aExpecting) {
    SimpleTest._expectingUncaughtException = aExpecting === void 0 || !!aExpecting;
};





SimpleTest.isExpectingUncaughtException = function () {
    return SimpleTest._expectingUncaughtException;
};






SimpleTest.ignoreAllUncaughtExceptions = function (aIgnoring) {
    SimpleTest._ignoringAllUncaughtExceptions = aIgnoring === void 0 || !!aIgnoring;
};





SimpleTest.isIgnoringAllUncaughtExceptions = function () {
    return SimpleTest._ignoringAllUncaughtExceptions;
};






SimpleTest.reset = function () {
    SimpleTest._ignoringAllUncaughtExceptions = false;
    SimpleTest._expectingUncaughtException = false;
    SimpleTest._bufferedMessages = [];
};

if (isPrimaryTestWindow) {
    addLoadEvent(function() {
        if (SimpleTest._stopOnLoad) {
            SimpleTest.finish();
        }
    });
}




SimpleTest.DNE = {dne: 'Does not exist'};
SimpleTest.LF = "\r\n";
SimpleTest._isRef = function (object) {
    var type = typeof(object);
    return type == 'object' || type == 'function';
};


SimpleTest._deepCheck = function (e1, e2, stack, seen) {
    var ok = false;
    
    var sameRef = !(!SimpleTest._isRef(e1) ^ !SimpleTest._isRef(e2));
    if (e1 == null && e2 == null) {
        ok = true;
    } else if (e1 != null ^ e2 != null) {
        ok = false;
    } else if (e1 == SimpleTest.DNE ^ e2 == SimpleTest.DNE) {
        ok = false;
    } else if (sameRef && e1 == e2) {
        
        
        ok = true;
    } else if (SimpleTest.isa(e1, 'Array') && SimpleTest.isa(e2, 'Array')) {
        ok = SimpleTest._eqArray(e1, e2, stack, seen);
    } else if (typeof e1 == "object" && typeof e2 == "object") {
        ok = SimpleTest._eqAssoc(e1, e2, stack, seen);
    } else if (typeof e1 == "number" && typeof e2 == "number"
               && isNaN(e1) && isNaN(e2)) {
        ok = true;
    } else {
        
        
        stack.push({ vals: [e1, e2] });
        ok = false;
    }
    return ok;
};

SimpleTest._eqArray = function (a1, a2, stack, seen) {
    
    if (a1 == a2) return true;

    
    
    
    
    
    
    for (var j = 0; j < seen.length; j++) {
        if (seen[j][0] == a1) {
            return seen[j][1] == a2;
        }
    }

    
    
    seen.push([ a1, a2 ]);

    var ok = true;
    
    
    var max = a1.length > a2.length ? a1.length : a2.length;
    if (max == 0) return SimpleTest._eqAssoc(a1, a2, stack, seen);
    for (var i = 0; i < max; i++) {
        var e1 = i > a1.length - 1 ? SimpleTest.DNE : a1[i];
        var e2 = i > a2.length - 1 ? SimpleTest.DNE : a2[i];
        stack.push({ type: 'Array', idx: i, vals: [e1, e2] });
        ok = SimpleTest._deepCheck(e1, e2, stack, seen);
        if (ok) {
            stack.pop();
        } else {
            break;
        }
    }
    return ok;
};

SimpleTest._eqAssoc = function (o1, o2, stack, seen) {
    
    if (o1 == o2) return true;

    
    
    
    
    
    
    seen = seen.slice(0);
    for (var j = 0; j < seen.length; j++) {
        if (seen[j][0] == o1) {
            return seen[j][1] == o2;
        }
    }

    
    
    seen.push([ o1, o2 ]);

    

    var ok = true;
    
    var o1Size = 0; for (var i in o1) o1Size++;
    var o2Size = 0; for (var i in o2) o2Size++;
    var bigger = o1Size > o2Size ? o1 : o2;
    for (var i in bigger) {
        var e1 = o1[i] == undefined ? SimpleTest.DNE : o1[i];
        var e2 = o2[i] == undefined ? SimpleTest.DNE : o2[i];
        stack.push({ type: 'Object', idx: i, vals: [e1, e2] });
        ok = SimpleTest._deepCheck(e1, e2, stack, seen)
        if (ok) {
            stack.pop();
        } else {
            break;
        }
    }
    return ok;
};

SimpleTest._formatStack = function (stack) {
    var variable = '$Foo';
    for (var i = 0; i < stack.length; i++) {
        var entry = stack[i];
        var type = entry['type'];
        var idx = entry['idx'];
        if (idx != null) {
            if (/^\d+$/.test(idx)) {
                
                variable += '[' + idx + ']';
            } else {
                
                idx = idx.replace("'", "\\'");
                variable += "['" + idx + "']";
            }
        }
    }

    var vals = stack[stack.length-1]['vals'].slice(0, 2);
    var vars = [
        variable.replace('$Foo',     'got'),
        variable.replace('$Foo',     'expected')
    ];

    var out = "Structures begin differing at:" + SimpleTest.LF;
    for (var i = 0; i < vals.length; i++) {
        var val = vals[i];
        if (val == null) {
            val = 'undefined';
        } else {
            val == SimpleTest.DNE ? "Does not exist" : "'" + val + "'";
        }
    }

    out += vars[0] + ' = ' + vals[0] + SimpleTest.LF;
    out += vars[1] + ' = ' + vals[1] + SimpleTest.LF;

    return '    ' + out;
};


SimpleTest.isDeeply = function (it, as, name) {
    var ok;
    
    if (SimpleTest._isRef(it) ^ SimpleTest._isRef(as)) {
        
        ok = false;
    } else if (!SimpleTest._isRef(it) && !SimpleTest._isRef(as)) {
        
        ok = SimpleTest.is(it, as, name);
    } else {
        
        var stack = [], seen = [];
        if ( SimpleTest._deepCheck(it, as, stack, seen)) {
            ok = SimpleTest.ok(true, name);
        } else {
            ok = SimpleTest.ok(false, name, SimpleTest._formatStack(stack));
        }
    }
    return ok;
};

SimpleTest.typeOf = function (object) {
    var c = Object.prototype.toString.apply(object);
    var name = c.substring(8, c.length - 1);
    if (name != 'Object') return name;
    
    
    if (/function ([^(\s]+)/.test(Function.toString.call(object.constructor))) {
        return RegExp.$1;
    }
    
    return name;
};

SimpleTest.isa = function (object, clas) {
    return SimpleTest.typeOf(object) == clas;
};


var ok = SimpleTest.ok;
var is = SimpleTest.is;
var isfuzzy = SimpleTest.isfuzzy;
var isnot = SimpleTest.isnot;
var ise = SimpleTest.ise;
var todo = SimpleTest.todo;
var todo_is = SimpleTest.todo_is;
var todo_isnot = SimpleTest.todo_isnot;
var isDeeply = SimpleTest.isDeeply;
var info = SimpleTest.info;

var gOldOnError = window.onerror;
window.onerror = function simpletestOnerror(errorMsg, url, lineNumber) {
    
    
    
    
    
    
    var isExpected = !!SimpleTest._expectingUncaughtException;
    var message = (isExpected ? "expected " : "") + "uncaught exception";
    var error = errorMsg + " at " + url + ":" + lineNumber;
    if (!SimpleTest._ignoringAllUncaughtExceptions) {
        
        if (!SimpleTest._alreadyFinished)
          SimpleTest.ok(isExpected, message, error);
        SimpleTest._expectingUncaughtException = false;
    } else {
        SimpleTest.todo(false, message + ": " + error);
    }
    

    
    if (gOldOnError) {
        try {
            
            gOldOnError(errorMsg, url, lineNumber);
        } catch (e) {
            
            SimpleTest.info("Exception thrown by gOldOnError(): " + e);
            
            if (e.stack) {
                SimpleTest.info("JavaScript error stack:\n" + e.stack);
            }
        }
    }

    if (!SimpleTest._stopOnLoad && !isExpected) {
        
        SimpleTest.executeSoon(SimpleTest.finish);
    }
};
