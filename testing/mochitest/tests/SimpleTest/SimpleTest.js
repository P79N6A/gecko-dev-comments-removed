















var SimpleTest = { };

var parentRunner = null;
if (parent) {
    parentRunner = parent.TestRunner;
    if (!parentRunner && parent.wrappedJSObject) {
        parentRunner = parent.wrappedJSObject.TestRunner;
    }
}


var ipcMode = false;
if (parentRunner) {
    ipcMode = parentRunner.ipcMode;
} else if (typeof SpecialPowers != 'undefined') {
    ipcMode = SpecialPowers.hasContentProcesses();
}




SimpleTest.testPluginIsOOP = function () {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var prefservice = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefBranch);

    var testPluginIsOOP = false;
    if (navigator.platform.indexOf("Mac") == 0) {
        var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"]
                                   .getService(Components.interfaces.nsIXULAppInfo)
                                   .QueryInterface(Components.interfaces.nsIXULRuntime);
        if (xulRuntime.XPCOMABI.match(/x86-/)) {
            try {
                testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.i386.test.plugin");
            } catch (e) {
                testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.i386");
            }
        }
        else if (xulRuntime.XPCOMABI.match(/x86_64-/)) {
            try {
                testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.x86_64.test.plugin");
            } catch (e) {
                testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.x86_64");
            }
        }
    }
    else {
        testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled");
    }

    return testPluginIsOOP;
};

SimpleTest._tests = [];
SimpleTest._stopOnLoad = true;




SimpleTest.ok = function (condition, name, diag) {
    var test = {'result': !!condition, 'name': name, 'diag': diag};
    SimpleTest._logResult(test, "TEST-PASS", "TEST-UNEXPECTED-FAIL");
    SimpleTest._tests.push(test);
};




SimpleTest.is = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    var pass = (a == b);
    var diag = pass ? repr(a) + " should equal " + repr(b)
                    : "got " + repr(a) + ", expected " + repr(b)
    SimpleTest.ok(pass, name, diag);
};

SimpleTest.isnot = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    var pass = (a != b);
    var diag = pass ? repr(a) + " should not equal " + repr(b)
                    : "didn't expect " + repr(a) + ", but got it";
    SimpleTest.ok(pass, name, diag);
};



SimpleTest.todo = function(condition, name, diag) {
    var test = {'result': !!condition, 'name': name, 'diag': diag, todo: true};
    SimpleTest._logResult(test, "TEST-UNEXPECTED-PASS", "TEST-KNOWN-FAIL");
    SimpleTest._tests.push(test);
};

SimpleTest._getCurrentTestURL = function() {
    return parentRunner && parentRunner.currentTestURL ||
           typeof gTestPath == "string" && gTestPath ||
           "";
};

SimpleTest._logResult = function(test, passString, failString) {
    var isError = !test.result == !test.todo;
    var resultString = test.result ? passString : failString;
    var url = SimpleTest._getCurrentTestURL();
    var diagnostic = test.name + (test.diag ? " - " + test.diag : "");
    var msg = [resultString, url, diagnostic].join(" | ");
    if (parentRunner) {
        if (isError) {
            parentRunner.error(msg);
        } else {
            parentRunner.log(msg);
        }
    } else {
        dump(msg + "\n");
    }
};





SimpleTest.todo_is = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    var pass = (a == b);
    var diag = pass ? repr(a) + " should equal " + repr(b)
                    : "got " + repr(a) + ", expected " + repr(b);
    SimpleTest.todo(pass, name, diag);
};

SimpleTest.todo_isnot = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    var pass = (a != b);
    var diag = pass ? repr(a) + " should not equal " + repr(b)
                    : "didn't expect " + repr(a) + ", but got it";
    SimpleTest.todo(pass, name, diag);
};





SimpleTest.report = function () {
    var DIV = MochiKit.DOM.DIV;
    var passed = 0;
    var failed = 0;
    var todo = 0;

    
    if (SimpleTest._tests.length == 0)
      
      SimpleTest.todo(false, "[SimpleTest.report()] No checks actually run.");

    var results = MochiKit.Base.map(
        function (test) {
            var cls, msg;
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
            return DIV({"class": cls}, msg);
        },
        SimpleTest._tests
    );

    var summary_class = failed != 0 ? 'some_fail' :
                          passed == 0 ? 'todo_only' : 'all_pass';

    return DIV({'class': 'tests_report'},
        DIV({'class': 'tests_summary ' + summary_class},
            DIV({'class': 'tests_passed'}, "Passed: " + passed),
            DIV({'class': 'tests_failed'}, "Failed: " + failed),
            DIV({'class': 'tests_todo'}, "Todo: " + todo)),
        results
    );
};




SimpleTest.toggle = function(el) {
    if (MochiKit.Style.computedStyle(el, 'display') == 'block') {
        el.style.display = 'none';
    } else {
        el.style.display = 'block';
    }
};




SimpleTest.toggleByClass = function (cls, evt) {
    var elems = getElementsByTagAndClassName('div', cls);
    MochiKit.Base.map(SimpleTest.toggle, elems);
    if (evt)
        evt.preventDefault();
};




SimpleTest.showReport = function() {
    var togglePassed = A({'href': '#'}, "Toggle passed checks");
    var toggleFailed = A({'href': '#'}, "Toggle failed checks");
    var toggleTodo = A({'href': '#'}, "Toggle todo checks");
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
    addNode(SPAN(null, " "));
    addNode(toggleFailed);
    addNode(SPAN(null, " "));
    addNode(toggleTodo);
    addNode(SimpleTest.report());
    
    addNode(HR());
};








SimpleTest.waitForExplicitFinish = function () {
    SimpleTest._stopOnLoad = false;
};









SimpleTest.requestLongerTimeout = function (factor) {
    if (parentRunner) {
        parentRunner.requestLongerTimeout(factor);
    }
}

SimpleTest.waitForFocus_started = false;
SimpleTest.waitForFocus_loaded = false;
SimpleTest.waitForFocus_focused = false;



















SimpleTest.waitForFocus = function (callback, targetWindow, expectBlankPage) {
    if (!targetWindow)
      targetWindow = window;

    if (ipcMode) {
      var domutils = targetWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                     getInterface(Components.interfaces.nsIDOMWindowUtils);

      
      if (parent && parent.ipcWaitForFocus) {
          parent.contentAsyncEvent("waitForFocus", {"callback":callback, "targetWindow":domutils.outerWindowID});
      }
      return;
    }

    SimpleTest.waitForFocus_started = false;
    expectBlankPage = !!expectBlankPage;

    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var fm = Components.classes["@mozilla.org/focus-manager;1"].
                        getService(Components.interfaces.nsIFocusManager);

    var childTargetWindow = { };
    fm.getFocusedElementForWindow(targetWindow, true, childTargetWindow);
    childTargetWindow = childTargetWindow.value;

    function info(msg) {
        SimpleTest._logResult({result: true, name: msg}, "TEST-INFO");
    }

    function debugFocusLog(prefix) {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        var baseWindow = targetWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                     .getInterface(Components.interfaces.nsIWebNavigation)
                                     .QueryInterface(Components.interfaces.nsIBaseWindow);
        info(prefix + " -- loaded: " + targetWindow.document.readyState +
           " active window: " +
               (fm.activeWindow ? "(" + fm.activeWindow + ") " + fm.activeWindow.location : "<no window active>") +
           " focused window: " +
               (fm.focusedWindow ? "(" + fm.focusedWindow + ") " + fm.focusedWindow.location : "<no window focused>") +
           " desired window: (" + targetWindow + ") " + targetWindow.location +
           " child window: (" + childTargetWindow + ") " + childTargetWindow.location +
           " docshell visible: " + baseWindow.visibility);
    }

    debugFocusLog("before wait for focus");

    function maybeRunTests() {
        debugFocusLog("maybe run tests <load:" +
                      SimpleTest.waitForFocus_loaded + ", focus:" + SimpleTest.waitForFocus_focused + ">");
        if (SimpleTest.waitForFocus_loaded &&
            SimpleTest.waitForFocus_focused &&
            !SimpleTest.waitForFocus_started) {
            SimpleTest.waitForFocus_started = true;
            setTimeout(callback, 0, targetWindow);
        }
    }

    function waitForEvent(event) {
        try {
            debugFocusLog("waitForEvent called <type:" + event.type + ", target" + event.target + ">");

            
            
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
        (expectBlankPage == (targetWindow.location == "about:blank")) &&
        targetWindow.document.readyState == "complete";
    if (!SimpleTest.waitForFocus_loaded) {
        info("must wait for load");
        targetWindow.addEventListener("load", waitForEvent, true);
    }

    
    var focusedChildWindow = { };
    if (fm.activeWindow) {
        fm.getFocusedElementForWindow(fm.activeWindow, true, focusedChildWindow);
        focusedChildWindow = focusedChildWindow.value;
    }

    
    SimpleTest.waitForFocus_focused = (focusedChildWindow == childTargetWindow);
    if (SimpleTest.waitForFocus_focused) {
        info("already focused");
        
        maybeRunTests();
    }
    else {
        info("must wait for focus");
        childTargetWindow.addEventListener("focus", waitForEvent, true);
        childTargetWindow.focus();
    }
};

SimpleTest.waitForClipboard_polls = 0;





















SimpleTest.__waitForClipboardMonotonicCounter = 0;
SimpleTest.__defineGetter__("_waitForClipboardMonotonicCounter", function () {
  return SimpleTest.__waitForClipboardMonotonicCounter++;
});
SimpleTest.waitForClipboard = function(aExpectedStringOrValidatorFn, aSetupFn,
                                       aSuccessFn, aFailureFn, aFlavor) {
    if (ipcMode) {
      
      dump("E10S_TODO: bug 573735 addresses adding support for this");
      return;
    }

    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

    var cbSvc = Components.classes["@mozilla.org/widget/clipboard;1"].
                getService(Components.interfaces.nsIClipboard);

    var requestedFlavor = aFlavor || "text/unicode";

    function dataToString(aData)
      aData.QueryInterface(Components.interfaces.nsISupportsString).data;

    
    var inputValidatorFn = typeof(aExpectedStringOrValidatorFn) == "string"
        ? function(aData) aData && dataToString(aData) == aExpectedStringOrValidatorFn
        : aExpectedStringOrValidatorFn;

    
    function reset() {
        SimpleTest.waitForClipboard_polls = 0;
    }

    function wait(validatorFn, successFn, failureFn, flavor) {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        if (++SimpleTest.waitForClipboard_polls > 50) {
            
            SimpleTest.ok(false, "Timed out while polling clipboard for pasted data.");
            reset();
            failureFn();
            return;
        }

        var xferable = Components.classes["@mozilla.org/widget/transferable;1"].
                       createInstance(Components.interfaces.nsITransferable);
        xferable.addDataFlavor(flavor);
        cbSvc.getData(xferable, cbSvc.kGlobalClipboard);
        var data = {};
        try {
            xferable.getTransferData(flavor, data, {});
        } catch (e) {}
        data = data.value || null;

        if (validatorFn(data)) {
            
            if (preExpectedVal)
                preExpectedVal = null;
            else
                SimpleTest.ok(true, "Clipboard has the correct value");
            reset();
            successFn();
        } else {
            setTimeout(function() wait(validatorFn, successFn, failureFn, flavor), 100);
        }
    }

    
    var preExpectedVal = SimpleTest._waitForClipboardMonotonicCounter +
                         "-waitForClipboard-known-value";
    var cbHelperSvc = Components.classes["@mozilla.org/widget/clipboardhelper;1"].
                      getService(Components.interfaces.nsIClipboardHelper);
    cbHelperSvc.copyString(preExpectedVal);
    wait(function(aData) aData && dataToString(aData) == preExpectedVal,
         function() {
           
           aSetupFn();
           wait(inputValidatorFn, aSuccessFn, aFailureFn, requestedFlavor);
         }, aFailureFn, "text/unicode");
}





SimpleTest.executeSoon = function(aFunc) {
    if ("Components" in window && "classes" in window.Components) {
        try {
            netscape.security.PrivilegeManager
              .enablePrivilege("UniversalXPConnect");
            var tm = Components.classes["@mozilla.org/thread-manager;1"]
                       .getService(Components.interfaces.nsIThreadManager);

            tm.mainThread.dispatch({
                run: function() {
                    aFunc();
                }
            }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
            return;
        } catch (ex) {
            
            
        }
    }
    setTimeout(aFunc, 0);
}





SimpleTest.finish = function () {
    if (parentRunner) {
        
        parentRunner.testFinished(SimpleTest._tests);
    } else {
        SimpleTest.showReport();
    }
};


addLoadEvent(function() {
    if (SimpleTest._stopOnLoad) {
        SimpleTest.finish();
    }
});




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
var isnot = SimpleTest.isnot;
var todo = SimpleTest.todo;
var todo_is = SimpleTest.todo_is;
var todo_isnot = SimpleTest.todo_isnot;
var isDeeply = SimpleTest.isDeeply;

var gOldOnError = window.onerror;
window.onerror = function simpletestOnerror(errorMsg, url, lineNumber) {
    var funcIdentifier = "[SimpleTest/SimpleTest.js, window.onerror]";

    
    
    
    
    
    
    function logInfo(message) {
        if (parentRunner) {
            SimpleTest._logInfo(funcIdentifier, message);
        } else {
            dump(funcIdentifier + " " + message);
        }
    }
    logInfo("An error occurred: " + errorMsg + " at " + url + ":" + lineNumber);
    

    
    if (gOldOnError) {
        try {
            
            gOldOnError(errorMsg, url, lineNumber);
        } catch (e) {
            
            logInfo("Exception thrown by gOldOnError(): " + e);
            
            if (e.stack) {
                logInfo("JavaScript error stack:\n" + e.stack);
            }
        }
    }

    if (!SimpleTest._stopOnLoad) {
        
        SimpleTest.executeSoon(SimpleTest.finish);
    }
};
