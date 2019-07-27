




var gTestfile = "clear-dictionary-accessor-getset.js";
var BUGNUMBER = 1082662;
var summary =
  "Properly handle GC of a dictionary accessor property whose [[Get]] or " +
  "[[Set]] has been changed to |undefined|";

print(BUGNUMBER + ": " + summary);





function test(field)
{
  var prop = "[[" + field[0].toUpperCase() + field.substring(1) + "]]";
  print("Testing for GC crashes after setting " + prop + " to undefined...");

  function inner()
  {
     
     var obj = { x: 42, get y() {}, set y(v) {} };

     
     
     
     delete obj.x;

     var desc = {};
     desc[field] = undefined;

     
     
     
     Object.defineProperty(obj, "y", desc);

  }

  inner();
  gc(); 
}

test("get");
test("set");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
