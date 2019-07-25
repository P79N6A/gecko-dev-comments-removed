






var gTestfile = '15.2.3.4-01.js';

var BUGNUMBER = 518663;
var summary =
  'Object.getOwnPropertyNames should play nicely with enumerator caching';

print(BUGNUMBER + ": " + summary);





for (var p in JSON);
var names = Object.getOwnPropertyNames(JSON);
assertEq(names.length >= 2, true,
         "wrong number of property names?  [" + names + "]");
assertEq(names.indexOf("parse") >= 0, true);
assertEq(names.indexOf("stringify") >= 0, true);



reportCompare(true, true);

print("All tests passed!");
