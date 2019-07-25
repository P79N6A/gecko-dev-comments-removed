







var BUGNUMBER = 623301;
var summary = "Properly root argument names during Function()";
print(BUGNUMBER + ": " + summary);





if (typeof gczeal === "function")
  gczeal(2);

function crashMe(n)
{
  var nasty = [];
  while (n--)
    nasty.push("a" + n);
  return Function.apply(null, nasty);
}

var count = 64; 
assertEq(crashMe(count + 1).length, count);

gczeal(0); 



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
