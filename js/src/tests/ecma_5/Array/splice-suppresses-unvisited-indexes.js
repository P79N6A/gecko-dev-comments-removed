





var BUGNUMBER = 668024;
var summary =
  'Array.prototype.splice, when it deletes elements, should make sure any ' +
  'deleted but not visited elements are suppressed from subsequent enumeration';

print(BUGNUMBER + ": " + summary);





var arr = [0, 1, 2, 3, 4, 5, , 7];

var seen = [];
var sawOneBeforeThree = true;
for (var p in arr)
{
  if (p === "1")
  {
    
    
    
    
    
    
    if (seen.indexOf("3") >= 0)
    {
      sawOneBeforeThree = false;
      break;
    }

    arr.splice(2, 3);
  }

  seen.push(p);
}

if (sawOneBeforeThree)
{
  
  
  
  
  
  
  
  assertEq(seen.indexOf("3"), -1);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
