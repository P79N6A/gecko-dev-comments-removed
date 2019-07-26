







var BUGNUMBER = 866700;
var summary = "Assertion redefining non-writable length to a non-numeric value";

print(BUGNUMBER + ": " + summary);





var count = 0;

var convertible =
  {
    valueOf: function()
    {
      count++;
      if (count > 2)
        return 0;
      throw new SyntaxError("fnord");
    }
  };

var arr = [];
Object.defineProperty(arr, "length", { value: 0, writable: false });

try
{
  Object.defineProperty(arr, "length",
                        {
                          value: convertible,
                          writable: true,
                          configurable: true,
                          enumerable: true
                        });
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e instanceof SyntaxError, true, "expected SyntaxError, got " + e);
}

assertEq(count, 2);
assertEq(arr.length, 0);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
