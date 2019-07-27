
var BUGNUMBER = 922115;
var summary = 'cannot embed unsized array in a struct';






var T = TypedObject;

function runTests() {
  print(BUGNUMBER + ": " + summary);

  var Uints = T.uint32.array();
  assertThrowsInstanceOf(() => { new T.StructType({f: Uints}) }, TypeError);
  assertThrowsInstanceOf(() => { Uints.array() }, TypeError);

  reportCompare(true, true);
  print("Tests complete");
}

runTests();
