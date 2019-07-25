




var gTestfile = 'function-bind.js';
var BUGNUMBER = 429507;
var summary = "ES5: Function.prototype.bind";

print(BUGNUMBER + ": " + summary);







assertEq(Function.prototype.hasOwnProperty("bind"), true);

var bind = Function.prototype.bind;
assertEq(bind.length, 1);


var strictReturnThis = function() { "use strict"; return this; };

assertEq(strictReturnThis.bind(undefined)(), undefined);
assertEq(strictReturnThis.bind(null)(), null);

var obj = {};
assertEq(strictReturnThis.bind(obj)(), obj);

assertEq(strictReturnThis.bind(NaN)(), NaN);

assertEq(strictReturnThis.bind(true)(), true);
assertEq(strictReturnThis.bind(false)(), false);

assertEq(strictReturnThis.bind("foopy")(), "foopy");




function expectThrowTypeError(fun)
{
  try
  {
    var r = fun();
    throw new Error("didn't throw TypeError, returned " + r);
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "didn't throw TypeError, got: " + e);
  }
}





expectThrowTypeError(function() { bind.call(null); });
expectThrowTypeError(function() { bind.call(undefined); });
expectThrowTypeError(function() { bind.call(NaN); });
expectThrowTypeError(function() { bind.call(0); });
expectThrowTypeError(function() { bind.call(-0); });
expectThrowTypeError(function() { bind.call(17); });
expectThrowTypeError(function() { bind.call(42); });
expectThrowTypeError(function() { bind.call("foobar"); });
expectThrowTypeError(function() { bind.call(true); });
expectThrowTypeError(function() { bind.call(false); });
expectThrowTypeError(function() { bind.call([]); });
expectThrowTypeError(function() { bind.call({}); });

















var toString = Object.prototype.toString;
assertEq(toString.call(function(){}), "[object Function]");
assertEq(toString.call(function a(){}), "[object Function]");
assertEq(toString.call(function(a){}), "[object Function]");
assertEq(toString.call(function a(b){}), "[object Function]");
assertEq(toString.call(function(){}.bind()), "[object Function]");
assertEq(toString.call(function a(){}.bind()), "[object Function]");
assertEq(toString.call(function(a){}.bind()), "[object Function]");
assertEq(toString.call(function a(b){}.bind()), "[object Function]");






assertEq(Object.getPrototypeOf(bind.call(function(){})), Function.prototype);
assertEq(Object.getPrototypeOf(bind.call(function a(){})), Function.prototype);
assertEq(Object.getPrototypeOf(bind.call(function(a){})), Function.prototype);
assertEq(Object.getPrototypeOf(bind.call(function a(b){})), Function.prototype);





var a = Array.bind(1, 2);
assertEq(a().length, 2);
assertEq(a(4).length, 2);
assertEq(a(4, 8).length, 3);

function t() { return this; }
var bt = t.bind(t);
assertEq(bt(), t);

function callee() { return arguments.callee; }
var call = callee.bind();
assertEq(call(), callee);
assertEq(new call(), callee);





function Point(x, y)
{
  this.x = x;
  this.y = y;
}
var YAxisPoint = Point.bind(null, 0)

assertEq(YAxisPoint.hasOwnProperty("prototype"), false);
var p = new YAxisPoint(5);
assertEq(p.x, 0);
assertEq(p.y, 5);
assertEq(p instanceof Point, true);
assertEq(p instanceof YAxisPoint, true);
assertEq(Object.prototype.toString.call(YAxisPoint), "[object Function]");
assertEq(YAxisPoint.length, 1);






function JoinArguments()
{
  this.args = Array.prototype.join.call(arguments, ", ");
}

var Join1 = JoinArguments.bind(null, 1);
var Join2 = Join1.bind(null, 2);
var Join3 = Join2.bind(null, 3);
var Join4 = Join3.bind(null, 4);
var Join5 = Join4.bind(null, 5);
var Join6 = Join5.bind(null, 6);

var r = new Join6(7);
assertEq(r instanceof Join6, true);
assertEq(r instanceof Join5, true);
assertEq(r instanceof Join4, true);
assertEq(r instanceof Join3, true);
assertEq(r instanceof Join2, true);
assertEq(r instanceof Join1, true);
assertEq(r instanceof JoinArguments, true);
assertEq(r.args, "1, 2, 3, 4, 5, 6, 7");








function none() { return arguments.length; }
assertEq(none.bind(1, 2)(3, 4), 3);
assertEq(none.bind(1, 2)(), 1);
assertEq(none.bind(1)(2, 3), 2);
assertEq(none.bind().length, 0);
assertEq(none.bind(null).length, 0);
assertEq(none.bind(null, 1).length, 0);
assertEq(none.bind(null, 1, 2).length, 0);

function one(a) { }
assertEq(one.bind().length, 1);
assertEq(one.bind(null).length, 1);
assertEq(one.bind(null, 1).length, 0);
assertEq(one.bind(null, 1, 2).length, 0);


var br = Object.create(null, { length: { value: 0 } });
try
{
  br = bind.call(/a/g, /a/g, "aaaa");
}
catch (e) {  }
assertEq(br.length, 0);






var len1Desc =
  Object.getOwnPropertyDescriptor(function(a, b, c){}.bind(), "length");
assertEq(len1Desc.value, 3);
assertEq(len1Desc.writable, false);
assertEq(len1Desc.enumerable, false);
assertEq(len1Desc.configurable, false);

var len2Desc =
  Object.getOwnPropertyDescriptor(function(a, b, c){}.bind(null, 2), "length");
assertEq(len2Desc.value, 2);
assertEq(len2Desc.writable, false);
assertEq(len2Desc.enumerable, false);
assertEq(len2Desc.configurable, false);





var bound = (function() { }).bind();

var isExtensible = Object.isExtensible || function() { return true; };
assertEq(isExtensible(bound), true);

bound.foo = 17;
var fooDesc = Object.getOwnPropertyDescriptor(bound, "foo");
assertEq(fooDesc.value, 17);
assertEq(fooDesc.writable, true);
assertEq(fooDesc.enumerable, true);
assertEq(fooDesc.configurable, true);











function f() { "use strict"; }
var canonicalTTE = Object.getOwnPropertyDescriptor(f, "caller").get;

var tte;

var boundf = f.bind();

var boundfCaller = Object.getOwnPropertyDescriptor(boundf, "caller");
assertEq("get" in boundfCaller, true);
assertEq("set" in boundfCaller, true);
tte = boundfCaller.get;
assertEq(tte, canonicalTTE);
assertEq(tte, boundfCaller.set);

var boundfArguments = Object.getOwnPropertyDescriptor(boundf, "arguments");
assertEq("get" in boundfArguments, true);
assertEq("set" in boundfArguments, true);
tte = boundfArguments.get;
assertEq(tte, canonicalTTE);
assertEq(tte, boundfArguments.set);



var passim = function p(){}.bind(1);
assertEq(typeof passim, "function");




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
