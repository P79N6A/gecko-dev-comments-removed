






var BUGNUMBER = 898342;
var summary = 'Unattached handles';

var T = TypedObject;

function runTests() {
  var Line = new T.StructType({from: T.uint8, to: T.uint8});
  var Lines = new T.ArrayType(Line, 3);

  
  var handle = Lines.handle();
  var handle0 = Line.handle();

  
  assertThrowsInstanceOf(function() handle[0], TypeError,
                         "Unattached handle did not yield error");
  assertThrowsInstanceOf(function() handle0.from, TypeError,
                         "Unattached handle did not yield error");

  
  assertThrowsInstanceOf(function() T.Handle.get(handle), TypeError,
                         "Unattached handle did not yield error");
  assertThrowsInstanceOf(function() T.Handle.get(handle0), TypeError,
                         "Unattached handle did not yield error");

  
  assertThrowsInstanceOf(function() T.Handle.set(handle, [{},{},{}]), TypeError,
                         "Unattached handle did not yield error");
  assertThrowsInstanceOf(function() T.Handle.set(handle0, {}), TypeError,
                         "Unattached handle did not yield error");

  reportCompare(true, true);
  print("Tests complete");
}

runTests();


