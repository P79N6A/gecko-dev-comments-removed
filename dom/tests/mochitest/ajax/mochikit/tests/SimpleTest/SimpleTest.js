











if (typeof(SimpleTest) == "undefined") {
    var SimpleTest = {};
}

var parentRunner = null;
if (typeof(parent) != "undefined" && parent.TestRunner) {
    parentRunner = parent.TestRunner;
} else if (parent && parent.wrappedJSObject &&
	   parent.wrappedJSObject.TestRunner) {
    parentRunner = parent.wrappedJSObject.TestRunner;
}


if (parentRunner) {
    SimpleTest._logEnabled = parentRunner.logEnabled;
}

SimpleTest._tests = [];
SimpleTest._stopOnLoad = true;




SimpleTest.ok = function (condition, name, diag) {
    var test = {'result': !!condition, 'name': name, 'diag': diag || ""};
    if (SimpleTest._logEnabled)
        SimpleTest._logResult(test, "TEST-PASS", "TEST-UNEXPECTED-FAIL");
    SimpleTest._tests.push(test);
};




SimpleTest.is = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    SimpleTest.ok(a == b, name, "got " + repr(a) + ", expected " + repr(b));
};

SimpleTest.isnot = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    SimpleTest.ok(a != b, name, "Didn't expect " + repr(a) + ", but got it.");
};



SimpleTest.todo = function(condition, name, diag) {
  var test = {'result': !!condition, 'name': name, 'diag': diag || "", todo: true};
  if (SimpleTest._logEnabled)
      SimpleTest._logResult(test, "TEST-UNEXPECTED-PASS", "TEST-KNOWN-FAIL");
  SimpleTest._tests.push(test);
};

SimpleTest._logResult = function(test, passString, failString) {
  var msg = test.result ? passString : failString;
  msg += " | ";
  if (parentRunner.currentTestURL)
    msg += parentRunner.currentTestURL;
  msg += " | " + test.name;
  var diag = "";
  if (test.diag)
    diag = " - " + test.diag;
  if (test.result) {
      if (test.todo)
          parentRunner.logger.error(msg + diag);
      else
          parentRunner.logger.log(msg);
  } else {
      if (test.todo)
          parentRunner.logger.log(msg);
      else
          parentRunner.logger.error(msg + diag);
  }
};





SimpleTest.todo_is = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    SimpleTest.todo(a == b, name, "got " + repr(a) + ", expected " + repr(b));
};

SimpleTest.todo_isnot = function (a, b, name) {
    var repr = MochiKit.Base.repr;
    SimpleTest.todo(a != b, name, "Didn't expect " + repr(a) + ", but got it.");
};





SimpleTest.report = function () {
    var DIV = MochiKit.DOM.DIV;
    var passed = 0;
    var failed = 0;
    var todo = 0;
    var results = MochiKit.Base.map(
        function (test) {
            var cls, msg;
            if (test.todo && !test.result) {
                todo++;
                cls = "test_todo";
                msg = "todo - " + test.name + " " + test.diag;
            } else if (test.result &&!test.todo) {
                passed++;
                cls = "test_ok";
                msg = "ok - " + test.name;
            } else {
                failed++;
                cls = "test_not_ok";
                msg = "not ok - " + test.name + " " + test.diag;
            }
            return DIV({"class": cls}, msg);
        },
        SimpleTest._tests
    );
    var summary_class = ((failed == 0) ? 'all_pass' : 'some_fail');
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
    var togglePassed = A({'href': '#'}, "Toggle passed tests");
    var toggleFailed = A({'href': '#'}, "Toggle failed tests");
    togglePassed.onclick = partial(SimpleTest.toggleByClass, 'test_ok');
    toggleFailed.onclick = partial(SimpleTest.toggleByClass, 'test_not_ok');
    var body = document.body;  
    if (!body) {
	
	body = document.getElementsByTagNameNS("http://www.w3.org/1999/xhtml",
					       "body")[0]
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
    addNode(SimpleTest.report());
};








SimpleTest.waitForExplicitFinish = function () {
    SimpleTest._stopOnLoad = false;
};





SimpleTest.talkToRunner = function () {
    if (parentRunner) {
        parentRunner.testFinished(document);
    }
};





SimpleTest.finish = function () {
    SimpleTest.showReport();
    SimpleTest.talkToRunner();
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
        if (ok = SimpleTest._deepCheck(e1, e2, stack, seen)) {
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
        if (ok = SimpleTest._deepCheck(e1, e2, stack, seen)) {
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

if ( parent.SimpleTest && parent.runAJAXTest ) (function(){
    var oldOK = SimpleTest.ok;

    SimpleTest.ok = function(condition, name, diag) {
        parent.SimpleTest.ok( condition, name, diag );
        return oldOK( condition, name, diag );
    };

    var oldFinish = SimpleTest.finish;

    SimpleTest.finish = function() {
	oldFinish();
	parent.runAJAXTest();
    };
})();


var ok = SimpleTest.ok;
var is = SimpleTest.is;
var isnot = SimpleTest.isnot;
var todo = SimpleTest.todo;
var todo_is = SimpleTest.todo_is;
var todo_isnot = SimpleTest.todo_isnot;
var isDeeply = SimpleTest.isDeeply;
var oldOnError = window.onerror;
window.onerror = function (ev) {
    is(0, 1, "Error thrown during test: " + ev);
    if (oldOnError) {
	try {
	  oldOnError(ev);
	} catch (e) {
	}
    }
    if (SimpleTest._stopOnLoad == false) {
      
      SimpleTest.finish();
    }
}
