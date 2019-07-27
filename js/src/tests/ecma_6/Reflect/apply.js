



assertEq(Reflect.apply(Math.floor, undefined, [1.75]), 1);


class clsX { constructor() {} }  
var nonCallable = [{}, [], clsX];
for (var value of nonCallable) {
    assertThrowsInstanceOf(() => Reflect.apply(nonCallable), TypeError);
}


var hits = 0;
var bogusArgumentList = {get length() { hit++; throw "FAIL";}};
assertThrowsInstanceOf(() => Reflect.apply({callable: false}, null, bogusArgumentList),
                       TypeError);
assertEq(hits, 0);



assertEq(Reflect.apply(String.fromCharCode,
                       undefined,
                       [104, 101, 108, 108, 111]),
         "hello");


assertEq(Reflect.apply(RegExp.prototype.exec,
                       /ab/,
                       ["confabulation"]).index,
         4);


assertEq(Reflect.apply("".charAt,
                       "ponies",
                       [3]),
         "i");


assertEq(Reflect.apply(function () { return this; }.bind(Math),
                       Function,
                       []),
         Math);
assertEq(Reflect.apply(Array.prototype.concat.bind([1, 2], [3]),
                       [4, 5],
                       [[6, 7, 8]]).join(),
         "1,2,3,6,7,8");


function* g(arg) { yield "pass" + arg; }
assertEq(Reflect.apply(g,
                       undefined,
                       ["word"]).next().value,
         "password");


function f() { return 13; }
assertEq(Reflect.apply(new Proxy(f, {}),
                       undefined,
                       []),
         13);


var gw = newGlobal();
assertEq(Reflect.apply(gw.parseInt,
                       undefined,
                       ["45"]),
         45);
assertEq(Reflect.apply(gw.Symbol.for,
                       undefined,
                       ["moon"]),
         Symbol.for("moon"));

gw.eval("function q() { return q; }");
assertEq(Reflect.apply(gw.q,
                       undefined,
                       []),
         gw.q);



var nope = new Error("nope");
function fail() {
    throw nope;
}
assertThrowsValue(() => Reflect.apply(fail, undefined, []),
                  nope);



var gxw = gw.eval("var x = new Error('x'); x");
gw.eval("function fail() { throw x; }");
assertThrowsValue(() => Reflect.apply(gw.fail, undefined, []),
                  gxw);


var obj = {};
hits = 0;
assertEq(Reflect.apply(function () { hits++; assertEq(this, obj); },
                       obj,
                       []),
         undefined);
assertEq(hits, 1);


function strictThis() { "use strict"; return this; }
for (var value of [null, undefined, 0, -0, NaN, Symbol("moon")]) {
    assertEq(Reflect.apply(strictThis, value, []),
             value);
}




var testValues = [true, 1e9, "ok", Symbol("ok")];
function nonStrictThis(expected) {
    assertEq(typeof this, "object");
    assertEq(Reflect.apply(Object.prototype.toString, this, []).toLowerCase(), expected);
    return "ok";
}
for (var value of testValues) {
    assertEq(Reflect.apply(nonStrictThis,
                           value,
                           ["[object " + typeof value + "]"]),
             "ok");
}



reportCompare(0, 0);
