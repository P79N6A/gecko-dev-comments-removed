





var gTestfile = "indirect-proxy-preventExtensions-error-realm.js";
var BUGNUMBER = 1085566;
var summary =
  "The preventExtensions trap should return success/failure (with the " +
  "outermost preventExtension caller deciding what to do in response), " +
  "rather than throwing a TypeError itself";

print(BUGNUMBER + ": " + summary);





var g = newGlobal();

















var p = g.Proxy.create({}, {});

try
{
  
  
  
  
  
  
  
  
  
  
  
  
  Object.preventExtensions(p);

  throw new Error("didn't throw at all");
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "expected a TypeError from this global, instead got " + e +
           ", from " +
           (e.constructor === TypeError
            ? "this global"
            : e.constructor === g.TypeError
            ? "the proxy's global"
            : "somewhere else (!!!)"));
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
