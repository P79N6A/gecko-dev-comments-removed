



var BUGNUMBER = 843004;
var summary =
  "Don't emit a strict warning for the undefined-property detection pattern in self-hosted code";

print(BUGNUMBER + ": " + summary);





options("strict", "werror");




new Date().toLocaleString("en-US", {});



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
