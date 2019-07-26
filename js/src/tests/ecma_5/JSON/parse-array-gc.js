


var gTestfile = "parse-array-gc.js";

var BUGNUMBER = 852563;
var summary =
  "IdValuePair::value should be initialized to avoid GC sequence-point issues";

print(BUGNUMBER + ": " + summary);

print("Note: You must run this test under valgrind to be certain it passes");





var x;

gczeal(2, 1);
x = JSON.parse('{"foo":[]}');
Object.getPrototypeOf(x.foo) == Array.prototype;
x = JSON.parse('{"foo":[], "bar":[]}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
