


if (!this.hasOwnProperty("TypedObject"))
  quit();

var {StructType, uint32, Object, Any, storage, objectType} = TypedObject;

function main() { 
  var Uints = new StructType({f: uint32, g: uint32});
  var p = Uints.prototype;
  Uints.prototype = {}; 
  assertEq(p, Uints.prototype);

  var Uintss = Uints.array(2);
  var p = Uintss.prototype;
  Uintss.prototype = {}; 
  assertEq(p, Uintss.prototype);

  print("Tests complete");
}

main();
