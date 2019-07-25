


var gTestfile = 'stringify-call-toJSON-once.js';

var BUGNUMBER = 584909;
var summary = "Stringification of Boolean/String/Number objects";

print(BUGNUMBER + ": " + summary);





var obj =
  {
    p: {
         toJSON: function()
         {
           return { toJSON: function() { return 17; } };
         }
       }
  };

assertEq(JSON.stringify(obj), '{"p":{}}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
