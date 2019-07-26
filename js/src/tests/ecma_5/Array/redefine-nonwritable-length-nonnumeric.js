







var BUGNUMBER = 866700;
var summary = "Assertion redefining non-writable length to a non-numeric value";

print(BUGNUMBER + ": " + summary);





var arr = [];
Object.defineProperty(arr, "length", { value: 0, writable: false });



Object.defineProperty(arr, "length", { value: '' });

assertEq(arr.length, 0);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
