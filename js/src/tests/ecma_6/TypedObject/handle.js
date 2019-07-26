






var BUGNUMBER = 898342;
var summary = 'Handles';

var T = TypedObject;

function runTests() {
  var Point = new T.ArrayType(T.float32, 3);
  var Line = new T.StructType({from: Point, to: Point});
  var Lines = new T.ArrayType(Line, 3);

  var lines = new Lines([
    {from: [1, 2, 3], to: [4, 5, 6]},
    {from: [7, 8, 9], to: [10, 11, 12]},
    {from: [13, 14, 15], to: [16, 17, 18]}
  ]);

  var handle = Lines.handle(lines);
  var handle0 = Line.handle(lines, 0);
  var handle2 = Line.handle(lines, 2);

  
  assertEq(handle[0].from[0], 1);
  assertEq(handle0.from[0], 1);
  assertEq(handle2.from[0], 13);

  
  handle2.from[0] = 22;
  assertEq(lines[2].from[0], 22);

  
  assertEq(handle[0].from[0], 1);
  assertEq(handle0.from[0], 1);
  assertEq(handle2.from[0], 22);

  
  assertEq(T.Handle.isHandle(22), false);
  assertEq(T.Handle.isHandle({}), false);

  
  assertEq(T.Handle.isHandle(lines), false);
  assertEq(T.Handle.isHandle(lines[0]), false);
  assertEq(T.Handle.isHandle(lines[0].from), false);
  assertEq(T.Handle.isHandle(handle), true);
  assertEq(T.Handle.isHandle(handle[0]), true);
  assertEq(T.Handle.isHandle(handle[0].from), true);

  reportCompare(true, true);
  print("Tests complete");
}

runTests();


