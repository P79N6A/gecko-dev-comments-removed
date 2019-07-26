
var BUGNUMBER = 938728;
var summary = 'float32x4 reify';






var ArrayType = TypedObject.ArrayType;
var float32x4 = TypedObject.float32x4;

function test() {
  print(BUGNUMBER + ": " + summary);

  var Array = new ArrayType(float32x4, 3);
  var array = new Array([float32x4(1, 2, 3, 4),
                         float32x4(5, 6, 7, 8),
                         float32x4(9, 10, 11, 12)]);

  
  

  var f = array[1];
  assertEq(f.w, 8);
  assertEq(array[1].w, 8);
  array[1] = float32x4(15, 16, 17, 18);
  assertEq(f.w, 8);
  assertEq(array[1].w, 18);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();
