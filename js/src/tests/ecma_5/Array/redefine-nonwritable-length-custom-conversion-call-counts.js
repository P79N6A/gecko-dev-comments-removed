







var BUGNUMBER = 866700;
var summary = "Assertion redefining non-writable length to a non-numeric value";

print(BUGNUMBER + ": " + summary);





var count = 0;

var convertible =
  {
    valueOf: function()
    {
      count++;
      return 0;
    }
  };

var arr = [];
Object.defineProperty(arr, "length", { value: 0, writable: false });

Object.defineProperty(arr, "length", { value: convertible });
assertEq(count, 2);

Object.defineProperty(arr, "length", { value: convertible });
assertEq(count, 4);

assertEq(arr.length, 0);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
