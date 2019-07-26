

"use strict";

load('utility.js');
load('annotations.js');
load('loadCallgraph.js');

if (typeof arguments[0] != 'string')
    throw "Usage: computeGCFunctions.js <callgraph.txt>";

print("Time: " + new Date);

loadCallgraph(arguments[0]);

for (var name in gcFunctions) {
    print("");
    print("GC Function: " + name);
    do {
        name = gcFunctions[name];
        print("    " + name);
    } while (name in gcFunctions);
}

for (var name in suppressedFunctions) {
    print("");
    print("Suppressed Function: " + name);
}
