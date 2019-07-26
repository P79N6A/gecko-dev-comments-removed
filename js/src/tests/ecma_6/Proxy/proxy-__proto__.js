




var gTestfile = 'proxy-__proto__.js';
var BUGNUMBER = 950407;
var summary = "Behavior of __proto__ on ES6 proxies";

print(BUGNUMBER + ": " + summary);





var protoDesc = Object.getOwnPropertyDescriptor(Object.prototype, "__proto__");
var protoGetter = protoDesc.get;
var protoSetter = protoDesc.set;

function testProxy(target, initialProto)
{
  print("Now testing behavior for new Proxy(" + ("" + target) + ", {})");

  var pobj = new Proxy(target, {});

  
  assertEq(Object.getPrototypeOf(pobj), initialProto);
  assertEq(protoGetter.call(pobj), initialProto);

  
  protoSetter.call(pobj, null);

  
  assertEq(Object.getPrototypeOf(pobj), null);
  assertEq(protoGetter.call(pobj), null);
  assertEq(Object.getPrototypeOf(target), null);
}


var nonNullProto = { toString: function() { return "non-null prototype"; } };
var target = Object.create(nonNullProto);
testProxy(target, nonNullProto);


target = Object.create(null);
target.toString = function() { return "null prototype" };
testProxy(target, null);


var callForCallOnly = function () { };
callForCallOnly.toString = function() { return "callable target"; };
testProxy(callForCallOnly, Function.prototype);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
