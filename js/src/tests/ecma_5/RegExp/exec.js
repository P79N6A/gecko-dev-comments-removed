




var BUGNUMBER = 646490;
var summary =
  "RegExp.prototype.exec doesn't get the lastIndex and ToInteger() it for " +
  "non-global regular expressions when it should";

print(BUGNUMBER + ": " + summary);





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

function checkExec(description, regex, args, obj)
{
  var lastIndex = obj.lastIndex;
  var index = obj.index;
  var input = obj.input;
  var indexArray = obj.indexArray;

  var res = regex.exec.apply(regex, args);

  assertEq(Array.isArray(res), true, description + ": not an array");
  assertEq(regex.lastIndex, lastIndex, description + ": wrong lastIndex");
  assertEq(res.index, index, description + ": wrong index");
  assertEq(res.input, input, description + ": wrong input");
  assertEq(res.length, indexArray.length, description + ": wrong length");
  for (var i = 0, sz = indexArray.length; i < sz; i++)
    assertEq(res[i], indexArray[i], description + " " + i + ": wrong index value");
}

var exec = RegExp.prototype.exec;
var r, res, called, obj;


expectThrowTypeError(function() { exec.call(null); });
expectThrowTypeError(function() { exec.call(""); });
expectThrowTypeError(function() { exec.call(5); });
expectThrowTypeError(function() { exec.call({}); });
expectThrowTypeError(function() { exec.call([]); });
expectThrowTypeError(function() { exec.call(); });
expectThrowTypeError(function() { exec.call(true); });
expectThrowTypeError(function() { exec.call(Object.create(RegExp.prototype)); });
expectThrowTypeError(function() { exec.call(Object.create(/a/)); });



called = false;
r = /a/;
assertEq(r.lastIndex, 0);

checkExec("/a/", r, [{ toString: function() { called = true; return 'ba'; } }],
          { lastIndex: 0,
            index: 1,
            input: "ba",
            indexArray: ["a"] });
assertEq(called, true);

called = false;
try
{
  res = r.exec({ toString: null, valueOf: function() { called = true; throw 17; } });
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e, 17);
}

assertEq(called, true);

called = false;
obj = r.lastIndex = { valueOf: function() { assertEq(true, false, "shouldn't have been called"); } };
try
{
  res = r.exec({ toString: null, valueOf: function() { assertEq(called, false); called = true; throw 17; } });
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e, 17);
}

assertEq(called, true);
assertEq(r.lastIndex, obj);










r = /b/;
r.lastIndex = { valueOf: {}, toString: {} };
expectThrowTypeError(function() { r.exec("foopy"); });
r.lastIndex = { valueOf: function() { throw new TypeError(); } };
expectThrowTypeError(function() { r.exec("foopy"); });






obj = { valueOf: function() { return 5; } };
r = /abc/;
r.lastIndex = obj;

checkExec("/abc/ take one", r, ["abc-------abc"],
          { lastIndex: obj,
            index: 0,
            input: "abc-------abc",
            indexArray: ["abc"] });

checkExec("/abc/ take two", r, ["abc-------abc"],
          { lastIndex: obj,
            index: 0,
            input: "abc-------abc",
            indexArray: ["abc"] });
















r = /abc()?/;
r.lastIndex = -5;
checkExec("/abc()?/ with lastIndex -5", r, ["abc-------abc"],
          { lastIndex: -5,
            index: 0,
            input: "abc-------abc",
            indexArray: ["abc", undefined] });


r = /abc/;
r.lastIndex = -17;
res = r.exec("cdefg");
assertEq(res, null);
assertEq(r.lastIndex, -17);

r = /abc/g;
r.lastIndex = -42;
res = r.exec("cdefg");
assertEq(res, null);
assertEq(r.lastIndex, 0);







r = /abc/g;
r.lastIndex = 17;
assertEq(r.exec("sdfs"), null);
assertEq(r.lastIndex, 0);

r = /abc/g;
r.lastIndex = 2;
checkExec("/abc/g", r, ["00abc"],
          { lastIndex: 5,
            index: 2,
            input: "00abc",
            indexArray: ["abc"] });



r = /a(b)c/g;
r.lastIndex = 2;
checkExec("/a(b)c/g take two", r, ["00abcd"],
          { lastIndex: 5,
            index: 2,
            input: "00abcd",
            indexArray: ["abc", "b"] });

































if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
