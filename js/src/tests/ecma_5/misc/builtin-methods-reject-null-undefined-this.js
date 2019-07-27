





var BUGNUMBER = 619283;
var summary =
  "ECMAScript built-in methods that immediately throw when |this| is " +
  "|undefined| or |null| (due to CheckObjectCoercible, ToObject, or ToString)";

print(BUGNUMBER + ": " + summary);











var ClassToMethodMap =
  {
    Object:  [
              "toLocaleString", "valueOf", "hasOwnProperty",
              



              


],
    
    
    
    Array:   ["toString", "toLocaleString", "concat", "join", "pop", "push",
              "reverse", "shift", "slice", "sort", "splice", "unshift",
              "indexOf", "lastIndexOf", "every", "some", "forEach", "map",
              "filter", "reduce", "reduceRight"],
    String:  ["toString", "valueOf", "charAt", "charCodeAt", "concat",
              "indexOf", "lastIndexOf", "localeCompare", "match", "replace",
              "search", "slice", "split", "substring", "toLowerCase",
              "toLocaleLowerCase", "toUpperCase", "toLocaleUpperCase", "trim",
              



              ],
    Boolean: ["toString", "valueOf"],
    Number:  ["toString", "toLocaleString", "valueOf",
              





              "toFixed",
              "toExponential", "toPrecision"],
    Date:    ["toString", "toDateString", "toTimeString", "toLocaleString",
              "toLocaleDateString", "toLocaleTimeString", "valueOf", "getTime",
              "getFullYear", "getUTCFullYear", "getMonth", "getUTCMonth",
              "getDate", "getUTCDate", "getDay", "getUTCDay", "getHours",
              "getUTCHours", "getMinutes", "getUTCMinutes", "getSeconds",
              "getUTCSeconds", "getMilliseconds", "getUTCMilliseconds",
              





              "setTime",
              "getTimezoneOffset", "setMilliseconds", "setUTCMilliseconds",
              "setSeconds", "setUTCSeconds", "setMinutes", "setUTCMinutes",
              "setHours", "setUTCHours", "setDate", "setUTCDate",  "setMonth",
              "setUTCMonth", "setFullYear", "setUTCFullYear", "toUTCString",
              "toISOString", "toJSON"],
    RegExp:  ["exec", "test", "toString"],
    Error:   ["toString"],
  };

var badThisValues = [null, undefined];

function testMethod(Class, className, method)
{
  var expr;

  
  for (var i = 0, sz = badThisValues.length; i < sz; i++)
  {
    var badThis = badThisValues[i];

    expr = className + ".prototype." + method + ".call(" + badThis + ")";
    try
    {
      Class.prototype[method].call(badThis);
      throw new Error(expr + " didn't throw a TypeError");
    }
    catch (e)
    {
      assertEq(e instanceof TypeError, true,
               "wrong error for " + expr + ", instead threw " + e);
    }

    expr = className + ".prototype." + method + ".apply(" + badThis + ")";
    try
    {
      Class.prototype[method].apply(badThis);
      throw new Error(expr + " didn't throw a TypeError");
    }
    catch (e)
    {
      assertEq(e instanceof TypeError, true,
               "wrong error for " + expr + ", instead threw " + e);
    }
  }

  

  expr = "(0, " + className + ".prototype." + method + ")()"
  try
  {
    
    (0, Class.prototype[method])();
    throw new Error(expr + " didn't throw a TypeError");
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "wrong error for " + expr + ", instead threw " + e);
  }
}

for (var className in ClassToMethodMap)
{
  var Class = this[className];

  var methodNames = ClassToMethodMap[className];
  for (var i = 0, sz = methodNames.length; i < sz; i++)
  {
    var method = methodNames[i];
    testMethod(Class, className, method);
  }
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
