



print("Test for correct implementation of |Date == boolean| and vice versa");





Date.prototype.toString = function() { return 1; };
Date.prototype.valueOf = function() { return 0; };














assertEq(new Date == true, true);
assertEq(new Date == false, false);


assertEq(true == new Date, true);
assertEq(false == new Date, false);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
