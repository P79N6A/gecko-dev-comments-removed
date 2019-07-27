
var BUGNUMBER = 1031203;
var float64x2 = SIMD.float64x2;

var summary = 'float64x2 reify';






var ArrayType = TypedObject.ArrayType;

function test() {
  print(BUGNUMBER + ": " + summary);

  var Array = float64x2.array(3);
  var array = new Array([float64x2(1, 2),
                         float64x2(3, 4),
                         float64x2(5, 6)]);

  
  

  var f = array[1];
  assertEq(f.y, 4);
  assertEq(array[1].y, 4);
  array[1] = float64x2(7, 8);
  assertEq(f.y, 4);
  assertEq(array[1].y, 8);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
