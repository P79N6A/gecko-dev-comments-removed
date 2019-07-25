


var gTestfile = 'stringify-toJSON-arguments.js';

var BUGNUMBER = 584909;
var summary = "Arguments when an object's toJSON method is called";

print(BUGNUMBER + ": " + summary);





var obj =
  {
    p: {
         toJSON: function(key)
         {
           assertEq(arguments.length, 1);
           assertEq(key, "p");
           return 17;
         }
       }
  };

assertEq(JSON.stringify(obj), '{"p":17}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
