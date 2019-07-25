




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
test(foo, 1);


test(function(f) { return f.bar; }, 19);
test(function(f) { return f(); }, 19);

test(function(){
        
        
    eval("function baz() { for (var i = 0; i < 10; i += a.b); assertEq(i !== i, true); }");
    baz();
}, 41);



test(function() { var tmp = null; tmp(); }, 34)
test(function() { var tmp = null;  tmp.foo; }, 35)


test(function() {

    foo({}); throw new Error('a');
}, 13);


test(function() {
    function f() { return true; }
    function g() { return false; }

    f(); g(); f(); if (f()) a += e;
}, 28);


test(function() { e++; }, 18);
test(function() {print += e; }, 17);
test(function(){e += 1 }, 16);
test(function() {  print[e]; }, 19);
test(function() { e[1]; }, 18);
test(function() { e(); }, 18);
test(function() { 1(); }, 18);
test(function() { Object.defineProperty() }, 18);

test(function() {

    function foo() { asdf; } foo()
}, 21);

reportCompare(0, 0, "ok");

printStatus("All tests passed!");
