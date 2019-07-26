
var BUGNUMBER = 926401;
var summary = 'TypedObjects ArrayType implementation';



function runTest() {
  var T = TypedObject;
  var Color = new T.StructType({'r': T.uint8, 'g': T.uint8, 'b': T.uint8});
  var Rainbow = new T.ArrayType(Color, 0);
  var theOneISawWasJustBlack = new Rainbow([]);
  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

runTest();
