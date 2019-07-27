




var f = function (a1, a2, a3, a4) {};
assertEq(delete f.length, true);
assertEq(f.hasOwnProperty("length"), false);
assertEq(f.length, 0);  
assertEq(delete Function.prototype.length, true);
assertEq(f.length, undefined);










assertEq("length" in Function.prototype, false);
Function.prototype.length = 7;
assertEq(Function.prototype.length, 7);
delete Function.prototype.length;
assertEq(Function.prototype.length, undefined);



Object.defineProperty(Function.prototype, "length", {value: 0, configurable: true});


var g = function f(a1, a2, a3, a4, a5) {};
var desc = Object.getOwnPropertyDescriptor(g, "length");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, false);
assertEq(desc.value, 5);



delete g.length;
g.length = 12;
assertEq(g.hasOwnProperty("length"), false);
assertEq(g.length, 0);



delete Function.prototype.length;
g.length = 13;
var desc = Object.getOwnPropertyDescriptor(g, "length");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, true);
assertEq(desc.writable, true);
assertEq(desc.value, 13);



function mkfun() {
    function fun(a1, a2, a3, a4, a5) {}
    return fun;
}
g = mkfun();
var h = mkfun();
delete h.length;
assertEq(g.length, 5);
assertEq(mkfun().length, 5);



g = mkfun();
Object.defineProperty(g, "length", {value: 0});
assertEq(delete g.length, true);
assertEq(g.hasOwnProperty("length"), false);



g = mkfun();
Object.defineProperty(g, "length", { value: 42 });
desc = Object.getOwnPropertyDescriptor(g, "length");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, false);
assertEq(desc.value, 42);

reportCompare(0, 0, 'ok');
