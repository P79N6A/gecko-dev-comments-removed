



var BUGNUMBER = 886949;
var summary = "ES6 (draft May 2013) 15.7.3.9 Number.parseInt(string, radix)." +
			  " Verify that Number.parseInt defaults to decimal.";

print(BUGNUMBER + ": " + summary);





assertEq(Number.parseInt("08"), 8);
assertEq(Number.parseInt("09"), 9);
assertEq(Number.parseInt("014"), 14);

function strictParseInt(s) { "use strict"; return Number.parseInt(s); }

assertEq(strictParseInt("08"), 8);
assertEq(strictParseInt("09"), 9);
assertEq(strictParseInt("014"), 14);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
