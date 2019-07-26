


var gTestfile = 'stringify-large-replacer-array.js';

var BUGNUMBER = 816033;
var summary = "JSON.stringify with a large replacer array";

print(BUGNUMBER + ": " + summary);





var replacer = [];
for (var i = 0; i < 4096; i++)
  replacer.push(i);

assertEq(JSON.stringify({ "foopy": "FAIL", "4093": 17 }, replacer), '{"4093":17}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
