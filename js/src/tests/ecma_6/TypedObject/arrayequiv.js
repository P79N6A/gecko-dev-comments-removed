
var BUGNUMBER = 922216;
var summary = 'TypedObjects Equivalent ArrayTypes';

var ArrayType = TypedObject.ArrayType;
var StructType = TypedObject.StructType;
var uint8 = TypedObject.uint8;
var uint16 = TypedObject.uint16;
var uint32 = TypedObject.uint32;
var uint8Clamped = TypedObject.uint8Clamped;
var int8 = TypedObject.int8;
var int16 = TypedObject.int16;
var int32 = TypedObject.int32;
var float32 = TypedObject.float32;
var float64 = TypedObject.float64;

function assertEquivalent(t1, t2) {
  assertEq(true, t1.equivalent(t2));
  assertEq(true, t2.equivalent(t1));
}

function assertNotEquivalent(t1, t2) {
  assertEq(false, t1.equivalent(t2));
  assertEq(false, t2.equivalent(t1));
}

function runTests() {
  print(BUGNUMBER + ": " + summary);

  
  var PixelType1 = new StructType({x: uint8, y: uint8});
  var PixelsType1 = new ArrayType(PixelType1, 22);

  
  assertEquivalent(PixelType1, PixelType1);
  assertEquivalent(PixelsType1, PixelsType1);
  assertNotEquivalent(PixelType1, PixelsType1);

  
  var PixelType2 = new StructType({x: uint8, y: uint8});
  var PixelsType2 = new ArrayType(PixelType2, 22);
  assertEquivalent(PixelType1, PixelType2);
  assertEquivalent(PixelsType1, PixelsType2);

  
  var PixelType3 = new StructType({y: uint8, x: uint8});
  var PixelsType3 = new ArrayType(PixelType3, 22);
  assertNotEquivalent(PixelType1, PixelType3);
  assertNotEquivalent(PixelsType1, PixelsType3);

  
  var PixelsType3 = new ArrayType(PixelType1, 23);
  assertNotEquivalent(PixelsType1, PixelsType3);

  reportCompare(true, true);
  print("Tests complete");
}

runTests();
