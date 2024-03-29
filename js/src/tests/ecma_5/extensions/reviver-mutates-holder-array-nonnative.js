







var BUGNUMBER = 901380;
var summary = "Behavior when JSON.parse walks over a non-native object";

print(BUGNUMBER + ": " + summary);





var typedArray = null;

var observedTypedArrayElementCount = 0;

var arr = JSON.parse('[0, 1]', function(prop, v) {
  if (prop === "0" && Array.isArray(this)) 
  {
    typedArray = new Int8Array(1);
    this[1] = typedArray;
  }
  if (this instanceof Int8Array)
  {
    assertEq(prop, "0");
    observedTypedArrayElementCount++;
  }
  return v;
});

assertEq(arr[0], 0);
assertEq(arr[1], typedArray);

assertEq(observedTypedArrayElementCount, 1);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
