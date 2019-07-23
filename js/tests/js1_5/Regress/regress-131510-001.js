













































var gTestfile = 'regress-131510-001.js';
var BUGNUMBER = 131510;
var summary = "Shouldn't crash if define |var arguments| inside a function";
printBugNumber(BUGNUMBER);
printStatus(summary);


function f() {var arguments;}
f();





function g() { function f() {var arguments;}; f();};
g();





var s = 'function f() {var arguments;}; f();';
eval(s);

s = 'function g() { function f() {var arguments;}; f();}; g();';
eval(s);

reportCompare('No Crash', 'No Crash', '');
