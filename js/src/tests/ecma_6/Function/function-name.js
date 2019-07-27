function testFunctionName(f) {
    var name = f.name;
    f.name = 'g';
    assertEq(f.name, name);
    assertEq(delete f.name, true);
    assertEq(f.name, '');
    assertEq(f.hasOwnProperty('name'), false);
    f.name = 'g';
    assertEq(f.name, '');
    Object.defineProperty(f, 'name', {value: 'g'});
    assertEq(f.name, 'g');
}
function testFunctionNameStrict(f) {
    "use strict";
    var name = f.name;
    var error;
    try {
        f.name = 'g';
    } catch (e) {
        error = e;
    }
    assertEq(f.name, name);
    assertEq(error instanceof TypeError, true);
    assertEq(delete f.name, true);
    assertEq(f.name, '');
    assertEq(f.hasOwnProperty('name'), false);
    error = null;
    try {
        f.name = 'g';
    } catch (e) {
        error = e;
    }
    assertEq(f.name, '');
    assertEq(error instanceof TypeError, true);
    Object.defineProperty(f, 'name', {value: 'g'});
    assertEq(f.name, 'g');
}

assertEq(Object.getOwnPropertyDescriptor(Object, "name").writable, false);
assertEq(Object.getOwnPropertyDescriptor(Object, "name").enumerable, false);
assertEq(Object.getOwnPropertyDescriptor(Object, "name").configurable, true);
assertEq(Object.getOwnPropertyDescriptor(Object, "name").value, 'Object');
assertEq(Object.getOwnPropertyDescriptor(function f(){}, "name").writable, false);
assertEq(Object.getOwnPropertyDescriptor(function f(){}, "name").enumerable, false);
assertEq(Object.getOwnPropertyDescriptor(function f(){}, "name").configurable, true);
assertEq(Object.getOwnPropertyDescriptor(function f(){}, "name").value, 'f');


function f() {};
Object.defineProperty(f, 'name', {value: 'g'});
assertEq(f.name, 'g');


testFunctionName(function f(){});
testFunctionNameStrict(function f(){});

testFunctionName(Function.prototype.apply);
testFunctionNameStrict(Function.prototype.call);

testFunctionName(Array.prototype.forEach);
testFunctionNameStrict(Array.prototype.some);

if (typeof reportCompare === "function")
    reportCompare(0, 0);
