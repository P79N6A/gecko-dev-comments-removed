



var BUGNUMBER = 730810;
var summary = "Don't assert on compound assignment to a const";

print(BUGNUMBER + ": " + summary);





const x = 3;
x += 2;
assertEq(x, 2);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete!");
