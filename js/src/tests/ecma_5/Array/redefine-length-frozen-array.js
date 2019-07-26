







var BUGNUMBER = 866580;
var summary = "Assertion redefining length property of a frozen array";

print(BUGNUMBER + ": " + summary);





var arr = Object.freeze([]);
Object.defineProperty(arr, "length", {});



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
