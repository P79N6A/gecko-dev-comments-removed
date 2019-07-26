


if (typeof TypedObject === "undefined")
  quit();

load(libdir + "asserts.js")

var {StructType, uint32, Object, Any, storage, objectType} = TypedObject;

function main(variant) { 
  var Uints = uint32.array();
  var Unit = new StructType({});   
  var buffer = new ArrayBuffer(0); 
  var p = new Unit(buffer);        
  neuter(buffer, variant);
  assertThrowsInstanceOf(() => new Unit(buffer), TypeError,
                         "Able to instantiate atop neutered buffer");
  assertThrowsInstanceOf(() => new Uints(buffer, 0), TypeError,
                         "Able to instantiate atop neutered buffer");
}

main("same-data");
main("change-data");
