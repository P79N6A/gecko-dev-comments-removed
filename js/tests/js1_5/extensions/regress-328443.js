




































var bug = 328443;
var summary = 'Uncatchable exception with |new (G.call) (F);| when F proto is null';
var actual = '';
var expect = 'Exception caught';

printBugNumber (bug);
printStatus (summary);

var F = (function(){});
F.__proto__ = null;

var G = (function(){});

var z;

z = "uncatchable exception!!!";
try { 
  new (G.call) (F);

  actual = "No exception";
} catch (er) {
  actual = "Exception caught";
  printStatus("Exception was caught: " + er);
}
  
reportCompare(expect, actual, summary);
