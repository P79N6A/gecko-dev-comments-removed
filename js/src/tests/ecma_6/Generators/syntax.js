




function assertSyntaxError(str) {
    var msg;
    var evil = eval;
    try {
        
        evil(str);
    } catch (exc) {
        if (exc instanceof SyntaxError)
            return;
        msg = "Assertion failed: expected SyntaxError, got " + exc;
    }
    if (msg === undefined)
        msg = "Assertion failed: expected SyntaxError, but no exception thrown";
    throw new Error(msg + " - " + str);
}


function* g() { yield 3; yield 4; }


function* g() { (yield 3) + (yield 4); }


function* g() { yield; }
function* g() { yield }
function* g() {
    yield
}
function* g() { (yield) }
function* g() { [yield] }
function* g() { {yield} }
function* g() { yield, yield }
function* g() { yield; yield }
function* g() { (yield) ? yield : yield }
function* g() {
    (yield)
    ? yield
    : yield
}



function* g() {
    yield *
    foo
}
assertThrows("function* g() { yield\n* foo }", SyntaxError);
assertIteratorNext(function*(){
                       yield
                       3
                   }(), undefined)


assertThrows("function* g() { yield ? yield : yield }", SyntaxError);


function* g() { "use strict"; yield 3; yield 4; }



function* g() { yield 1; return; }
function* g() { yield 1; return 2; }
function* g() { yield 1; return 2; yield "dead"; }


(function* () { yield 3; });


(function* g() { yield 3; });


function* g() { }


function* g() { yield yield 1; }
function* g() { yield 3 + (yield 4); }




function* yield() { (yield 3) + (yield 4); }
assertSyntaxError("function* yield() { 'use strict'; (yield 3) + (yield 4); }");


function yield(yield) { yield: yield (yield + yield (0)); }


({ yield: 1 });
function* g() { yield ({ yield: 1 }) }
function* g() { yield ({ get yield() { return 1; }}) }


function* g(obj) { yield obj.yield; }



function f() { yield: 1 }
assertSyntaxError("function f() { 'use strict'; yield: 1 }")
assertSyntaxError("function* g() { yield: 1 }")



function* g() { function f(yield) { yield (yield + yield (0)); } }


assertSyntaxError("function* g() { yield = 10; }");



assertSyntaxError("function* g() { yield 3 + yield 4; }");


assertSyntaxError("function f() { 'use strict'; var yield = 13; }");


assertSyntaxError("function* g() { yield (function yield() {}); }");


assertSyntaxError("function* g(yield) { yield (10); }");

if (typeof reportCompare == "function")
    reportCompare(true, true);
