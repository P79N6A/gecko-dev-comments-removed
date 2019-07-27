



if (!this.hasOwnProperty("TypedObject"))
  quit();

load(libdir + "asserts.js");

var {StructType, uint32, Object, Any, storage, objectType} = TypedObject;

function main() { 
  var Uints = new StructType({f: uint32, g: uint32});
  var p = Uints.prototype;
  Uints.prototype = {}; 
  assertEq(p, Uints.prototype);

  var uints = new Uints();
  assertEq(uints.__proto__, p);
  assertThrowsInstanceOf(() => uints.__proto__ = {},
                         TypeError);
  assertThrowsInstanceOf(() => Object.setPrototypeOf(uints, {}),
                         TypeError);
  assertEq(uints.__proto__, p);

  var Uintss = Uints.array(2);
  var p = Uintss.prototype;
  Uintss.prototype = {}; 
  assertEq(p, Uintss.prototype);

  print("Tests complete");
}

main();
