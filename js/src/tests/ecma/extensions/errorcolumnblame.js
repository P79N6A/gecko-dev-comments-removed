




var BUGNUMBER = 568142;
var summary = 'error reporting blames column as well as line';

function test(f, col) {
    var caught = false;
    try {
        f();
    } catch (e) {
        caught = true;
        assertEq(e.columnNumber, col);
    }
    assertEq(caught, true);
}


function foo(o) {
	return o.p;
}
test(foo, 2);


test(function(f) { return f.bar; }, 20);


test(function(f) { return f(); }, 27);

test(function(){
        
        
    eval("function baz() { for (var i = 0; i < 10; i += a.b); assertEq(i !== i, true); }");
    baz();
}, 42);



test(function() { var tmp = null; tmp(); }, 35)
test(function() { var tmp = null;  tmp.foo; }, 36)


test(function() {


    foo({}); throw new Error('a');
}, 20);


test(function() {
    function f() { return true; }
    function g() { return false; }


    f(); g(); f(); if (f()) a += e;
}, 29);



test(function() { e++; }, 19);
test(function() {print += e; }, 18);
test(function(){e += 1 }, 17);
test(function() {  print[e]; }, 20);
test(function() { e[1]; }, 19);
test(function() { e(); }, 19);
test(function() { 1(); }, 19);
test(function() { Object.defineProperty() }, 19);

test(function() {


    function foo() { asdf; } foo()
}, 22);

reportCompare(0, 0, "ok");

printStatus("All tests passed!");
