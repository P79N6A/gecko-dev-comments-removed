


var gTestfile = 'toJSON-01.js';

var BUGNUMBER = 584811;
var summary = "Date.prototype.toJSON isn't to spec";

print(BUGNUMBER + ": " + summary);





var called;

var dateToJSON = Date.prototype.toJSON;
assertEq(Date.prototype.hasOwnProperty("toJSON"), true);
assertEq(typeof dateToJSON, "function");


var invalidDate = new Date();
invalidDate.setTime(NaN);
assertEq(JSON.stringify({ p: invalidDate }), '{"p":null}');



assertEq(dateToJSON.length, 1);





function strictThis() { "use strict"; return this; }
if (strictThis.call(null) === null)
{
  try
  {
    dateToJSON.call(null);
    throw new Error("should have thrown a TypeError");
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "ToObject throws TypeError for null/undefined");
  }

  try
  {
    dateToJSON.call(undefined);
    throw new Error("should have thrown a TypeError");
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "ToObject throws TypeError for null/undefined");
  }
}

















try
{
  var r = dateToJSON.call({ get valueOf() { throw 17; } });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e, 17, "bad exception: " + e);
}

called = false;
assertEq(dateToJSON.call({ valueOf: null,
                           toString: function() { called = true; return 12; },
                           toISOString: function() { return "ohai"; } }),
         "ohai");
assertEq(called, true);

called = false;
assertEq(dateToJSON.call({ valueOf: function() { called = true; return 42; },
                           toISOString: function() { return null; } }),
         null);
assertEq(called, true);

try
{
  called = false;
  dateToJSON.call({ valueOf: function() { called = true; return {}; },
                    get toString() { throw 42; } });
}
catch (e)
{
  assertEq(called, true);
  assertEq(e, 42, "bad exception: " + e);
}

called = false;
assertEq(dateToJSON.call({ valueOf: function() { called = true; return {}; },
                           get toString() { return function() { return 8675309; }; },
                           toISOString: function() { return true; } }),
         true);
assertEq(called, true);

var asserted = false;
called = false;
assertEq(dateToJSON.call({ valueOf: function() { called = true; return {}; },
                           get toString()
                           {
                             assertEq(called, true);
                             asserted = true;
                             return function() { return 8675309; };
                           },
                           toISOString: function() { return NaN; } }),
         NaN);
assertEq(asserted, true);

try
{
  var r = dateToJSON.call({ valueOf: null, toString: null,
                            get toISOString()
                            {
                              throw new Error("shouldn't have been gotten");
                            } });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e instanceof TypeError, true, "bad exception: " + e);
}



assertEq(dateToJSON.call({ valueOf: function() { return Infinity; } }), null);
assertEq(dateToJSON.call({ valueOf: function() { return -Infinity; } }), null);
assertEq(dateToJSON.call({ valueOf: function() { return NaN; } }), null);

assertEq(dateToJSON.call({ valueOf: function() { return Infinity; },
                           toISOString: function() { return {}; } }), null);
assertEq(dateToJSON.call({ valueOf: function() { return -Infinity; },
                           toISOString: function() { return []; } }), null);
assertEq(dateToJSON.call({ valueOf: function() { return NaN; },
                           toISOString: function() { return undefined; } }), null);






try
{
  var r = dateToJSON.call({ get toISOString() { throw 42; } });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e, 42, "bad exception: " + e);
}



try
{
  var r = dateToJSON.call({ toISOString: null });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e instanceof TypeError, true, "bad exception: " + e);
}

try
{
  var r = dateToJSON.call({ toISOString: undefined });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e instanceof TypeError, true, "bad exception: " + e);
}

try
{
  var r = dateToJSON.call({ toISOString: "oogabooga" });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e instanceof TypeError, true, "bad exception: " + e);
}

try
{
  var r = dateToJSON.call({ toISOString: Math.PI });
  throw new Error("didn't throw, returned: " + r);
}
catch (e)
{
  assertEq(e instanceof TypeError, true, "bad exception: " + e);
}






var o =
  {
    toISOString: function(a)
    {
      called = true;
      assertEq(this, o);
      assertEq(a, undefined);
      assertEq(arguments.length, 0);
      return obj;
    }
  };
var obj = {};
called = false;
assertEq(dateToJSON.call(o), obj, "should have gotten obj back");
assertEq(called, true);




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
