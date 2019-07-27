




var gTestfile = 'builtin-function-arguments-caller.js';
var BUGNUMBER = 929642;
var summary =
  "Built-in functions defined in ECMAScript pick up arguments/caller " +
  "properties from Function.prototype";

print(BUGNUMBER + ": " + summary);





function expectNoProperty(obj, prop)
{
  var desc = Object.getOwnPropertyDescriptor(obj, prop);
  assertEq(desc, undefined,
           "should be no '" + prop + "' property on " + obj);
}


expectNoProperty(Object, "arguments");
expectNoProperty(Object, "caller");


expectNoProperty(Array.prototype.indexOf, "arguments");
expectNoProperty(Array.prototype.indexOf, "caller");



expectNoProperty(Function, "arguments");
expectNoProperty(Function, "caller");

var argsDesc = Object.getOwnPropertyDescriptor(Function.prototype, "arguments");
var callerDesc = Object.getOwnPropertyDescriptor(Function.prototype, "caller");

var argsGet = argsDesc.get, argsSet = argsDesc.set;

expectNoProperty(argsGet, "arguments");
expectNoProperty(argsGet, "caller");
expectNoProperty(argsSet, "arguments");
expectNoProperty(argsSet, "caller");

var callerGet = callerDesc.get, callerSet = callerDesc.set;

expectNoProperty(callerGet, "arguments");
expectNoProperty(callerGet, "caller");
expectNoProperty(callerSet, "arguments");
expectNoProperty(callerSet, "caller");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
