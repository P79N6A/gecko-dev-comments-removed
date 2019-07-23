




































var bug = 361346;
var summary = 'Crash with setter, watch, GC';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);
  
expect = actual = 'No Crash';

this.x setter= new Function; 
this.watch('x', function(){}); 
gc(); 
x = {};

reportCompare(expect, actual, summary);
