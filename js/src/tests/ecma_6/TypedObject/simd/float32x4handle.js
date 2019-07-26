
var BUGNUMBER = 938728;
var summary = 'float32x4 handles';






var ArrayType = TypedObject.ArrayType;
var float32x4 = TypedObject.float32x4;
var float32 = TypedObject.float32;
var Handle = TypedObject.Handle;

function test() {
  print(BUGNUMBER + ": " + summary);

  var Array = float32x4.array(3);
  var array = new Array([float32x4(1, 2, 3, 4),
                         float32x4(5, 6, 7, 8),
                         float32x4(9, 10, 11, 12)]);

  
  

  assertThrowsInstanceOf(function() {
    var h = float32.handle(array, 1, "w");
  }, TypeError, "Creating a float32 handle to prop via ctor");

  assertThrowsInstanceOf(function() {
    var h = float32.handle();
    Handle.move(h, array, 1, "w");
  }, TypeError, "Creating a float32 handle to prop via move");

  assertThrowsInstanceOf(function() {
    var h = float32.handle(array, 1, 0);
  }, TypeError, "Creating a float32 handle to elem via ctor");

  assertThrowsInstanceOf(function() {
    var h = float32.handle();
    Handle.move(h, array, 1, 0);
  }, TypeError, "Creating a float32 handle to elem via move");

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();
