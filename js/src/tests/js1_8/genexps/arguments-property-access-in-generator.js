







var BUGNUMBER = 721322;
var summary = 'Allow f.arguments in generator expressions';

print(BUGNUMBER + ": " + summary);





eval("(function() { return (f.arguments for (x in [1])); })()");
eval("(function() { var f = { arguments: 12 }; return [f.arguments for (x in [1])]; })()");




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
