
var BUGNUMBER = 898356;

var {StructType, uint32, Object, Any, storage, objectType} = TypedObject;

function main() { 
  print(BUGNUMBER + ": " + summary);

  var Uints = new StructType({f: uint32, g: uint32});

  var anArray = new Uint32Array(4);
  anArray[1] = 22;
  anArray[2] = 44;

  var uints = new Uints(anArray.buffer, 4);
  assertEq(storage(uints).buffer, anArray.buffer);
  assertEq(uints.f, 22);
  assertEq(uints.g, 44);
  uints.f++;
  assertEq(anArray[1], 23);

  
  assertThrows(() => new Uints(anArray.buffer, -4)); 
  assertThrows(() => new Uints(anArray.buffer, -3)); 
  assertThrows(() => new Uints(anArray.buffer, -2)); 
  assertThrows(() => new Uints(anArray.buffer, -1)); 
  new Uints(anArray.buffer, 0);                      
  assertThrows(() => new Uints(anArray.buffer, 1));  
  assertThrows(() => new Uints(anArray.buffer, 2));  
  assertThrows(() => new Uints(anArray.buffer, 3));  
  new Uints(anArray.buffer, 4);                      
  assertThrows(() => new Uints(anArray.buffer, 5));  
  assertThrows(() => new Uints(anArray.buffer, 6));  
  assertThrows(() => new Uints(anArray.buffer, 7));  
  new Uints(anArray.buffer, 8);                      
  assertThrows(() => new Uints(anArray.buffer, 9));  
  assertThrows(() => new Uints(anArray.buffer, 10)); 
  assertThrows(() => new Uints(anArray.buffer, 11)); 
  assertThrows(() => new Uints(anArray.buffer, 12)); 
  assertThrows(() => new Uints(anArray.buffer, 13)); 
  assertThrows(() => new Uints(anArray.buffer, 14)); 
  assertThrows(() => new Uints(anArray.buffer, 15)); 
  assertThrows(() => new Uints(anArray.buffer, 16)); 
  assertThrows(() => new Uints(anArray.buffer, 17)); 
  assertThrows(() => new Uints(anArray.buffer, 4294967292)); 

  reportCompare(true, true);
  print("Tests complete");
}

main();
