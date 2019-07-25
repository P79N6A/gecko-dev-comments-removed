





var BUGNUMBER = 722260;
var summary = 'All NaNs must be treated as identical keys for Map';

print(BUGNUMBER + ": " + summary);






var key = -/a/g.missingProperty;

var m = new Map();
m.set(key, 17);
assertEq(m.get(key), 17);
assertEq(m.get(-key), 17);
assertEq(m.get(NaN), 17);

m.delete(-key);
assertEq(m.has(key), false);
assertEq(m.has(-key), false);
assertEq(m.has(NaN), false);

m.set(-key, 17);
assertEq(m.get(key), 17);
assertEq(m.get(-key), 17);
assertEq(m.get(NaN), 17);

m.delete(NaN);
assertEq(m.has(key), false);
assertEq(m.has(-key), false);
assertEq(m.has(NaN), false);

m.set(NaN, 17);
assertEq(m.get(key), 17);
assertEq(m.get(-key), 17);
assertEq(m.get(NaN), 17);

m.delete(key);
assertEq(m.has(key), false);
assertEq(m.has(-key), false);
assertEq(m.has(NaN), false);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
