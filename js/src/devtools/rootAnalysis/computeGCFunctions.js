

"use strict";

loadRelativeToScript('utility.js');
loadRelativeToScript('annotations.js');
loadRelativeToScript('loadCallgraph.js');

if (typeof scriptArgs[0] != 'string')
    throw "Usage: computeGCFunctions.js <callgraph.txt> <out:gcFunctions.txt> <out:gcFunctions.lst> <out:gcEdges.txt> <out:suppressedFunctions.lst>";

var start = "Time: " + new Date;

var callgraph_filename = scriptArgs[0];
var gcFunctions_filename = scriptArgs[1] || "gcFunctions.txt";
var gcFunctionsList_filename = scriptArgs[2] || "gcFunctions.lst";
var gcEdges_filename = scriptArgs[3] || "gcEdges.txt";
var suppressedFunctionsList_filename = scriptArgs[4] || "suppressedFunctions.lst";

loadCallgraph(callgraph_filename);

printErr("Writing " + gcFunctions_filename);
redirect(gcFunctions_filename);
for (var name in gcFunctions) {
    print("");
    print("GC Function: " + name);
    do {
        name = gcFunctions[name];
        print("    " + name);
    } while (name in gcFunctions);
}

printErr("Writing " + gcFunctionsList_filename);
redirect(gcFunctionsList_filename);
for (var name in gcFunctions) {
    print(name);
}










printErr("Writing " + gcEdges_filename);
redirect(gcEdges_filename);
for (var block in gcEdges) {
  for (var edge in gcEdges[block]) {
      var func = gcEdges[block][edge];
    print([ block, edge, func ].join(" || "));
  }
}

printErr("Writing " + suppressedFunctionsList_filename);
redirect(suppressedFunctionsList_filename);
for (var name in suppressedFunctions) {
    print(name);
}
